#include "network.hpp"
#include <curl/curl.h>
#include <openssl/evp.h>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#include <direct.h>
#define QS_OPEN _open
#define QS_CLOSE _close
#define QS_FDOPEN _fdopen
#else
#include <unistd.h>
#define QS_OPEN open
#define QS_CLOSE close
#define QS_FDOPEN fdopen
#endif
namespace qs {
static FILE* safe_open(const std::string& path,bool append=false,bool reading=false) {
    int flags=reading?O_RDONLY:(O_WRONLY|O_CREAT|(append?O_APPEND:0));
#ifdef O_NOFOLLOW
    flags|=O_NOFOLLOW;
#endif
#ifdef O_BINARY
    flags|=O_BINARY;
#endif
    int fd=QS_OPEN(path.c_str(),flags,0600);
    if(fd<0) throw std::runtime_error("Ouverture refusee: "+path);
    struct stat s{};
    int ok=fstat(fd,&s);
    if(ok || (s.st_mode&S_IFMT)!=S_IFREG || (!reading && s.st_nlink!=1)) { QS_CLOSE(fd); throw std::runtime_error("Fichier non regulier ou lien multiple"); }
    if(!reading && !append) {
#ifdef _WIN32
        int rc=_chsize_s(fd,0);
#else
        int rc=ftruncate(fd,0);
#endif
        if(rc) { QS_CLOSE(fd); throw std::runtime_error("Troncature impossible"); }
    }
    FILE* f=QS_FDOPEN(fd,reading?"rb":(append?"ab":"wb"));
    if(!f) { QS_CLOSE(fd); throw std::runtime_error("fdopen"); } return f;
}
struct File { FILE* f; explicit File(FILE* v):f(v){} ~File(){if(f)std::fclose(f);} };
void directory(const std::string& path) {
#ifdef _WIN32
    int rc=_mkdir(path.c_str());
#else
    int rc=mkdir(path.c_str(),0700);
#endif
    if(rc && errno!=EEXIST) throw std::runtime_error("Creation de dossier impossible: "+path);
    struct stat s{};
#ifdef _WIN32
    int err=stat(path.c_str(),&s);
#else
    int err=lstat(path.c_str(),&s);
#endif
    if(err || (s.st_mode&S_IFMT)!=S_IFDIR) throw std::runtime_error("Dossier dangereux: "+path);
}
std::string read_text(const std::string& path,size_t limit) {
    File f(safe_open(path,false,true)); std::string text; char b[4096]; size_t n;
    while((n=fread(b,1,sizeof b,f.f))) { if(text.size()+n>limit)throw std::runtime_error("Fichier trop grand"); text.append(b,n); }
    if(ferror(f.f))throw std::runtime_error("Lecture impossible"); return text;
}
void write_text(const std::string& path,const std::string& text) {
    std::string tmp=path+".tmp"; File f(safe_open(tmp));
    if(fwrite(text.data(),1,text.size(),f.f)!=text.size() || fflush(f.f))throw std::runtime_error("Ecriture impossible");
    if(fclose(f.f)) { f.f=nullptr; throw std::runtime_error("Fermeture impossible"); } f.f=nullptr;
#ifdef _WIN32
    std::remove(path.c_str());
#endif
    if(std::rename(tmp.c_str(),path.c_str())) throw std::runtime_error("Validation de fichier impossible");
}
std::string sha256_file(const std::string& path) {
    File f(safe_open(path,false,true)); EVP_MD_CTX* ctx=EVP_MD_CTX_new(); if(!ctx)throw std::runtime_error("SHA256 init");
    EVP_DigestInit_ex(ctx,EVP_sha256(),nullptr); char b[65536]; size_t n;
    while((n=fread(b,1,sizeof b,f.f))) EVP_DigestUpdate(ctx,b,n);
    if(ferror(f.f)) { EVP_MD_CTX_free(ctx); throw std::runtime_error("SHA256 lecture"); }
    unsigned char digest[32]; unsigned len=0; EVP_DigestFinal_ex(ctx,digest,&len); EVP_MD_CTX_free(ctx);
    std::ostringstream out; for(unsigned char c:digest)out<<std::hex<<std::setw(2)<<std::setfill('0')<<int(c); return out.str();
}
void status(Progress& p,const std::string& msg,const std::string& error) { std::lock_guard<std::mutex> l(p.mutex); p.message=msg; p.error=error; }
struct Curl { CURL* c=curl_easy_init(); ~Curl(){if(c)curl_easy_cleanup(c);} };
static int progress(void* opaque,curl_off_t,curl_off_t,curl_off_t,curl_off_t) { return static_cast<Progress*>(opaque)->cancel?1:0; }
static void common(CURL* c,const std::string& url,const std::string& ca,Progress& p) {
    if(!c || !https(url))throw std::runtime_error("URL HTTPS invalide");
    curl_easy_setopt(c,CURLOPT_URL,url.c_str()); curl_easy_setopt(c,CURLOPT_CAINFO,ca.c_str());
    curl_easy_setopt(c,CURLOPT_SSL_VERIFYPEER,1L); curl_easy_setopt(c,CURLOPT_SSL_VERIFYHOST,2L);
    curl_easy_setopt(c,CURLOPT_PROTOCOLS_STR,"https"); curl_easy_setopt(c,CURLOPT_REDIR_PROTOCOLS_STR,"https");
    curl_easy_setopt(c,CURLOPT_FOLLOWLOCATION,1L); curl_easy_setopt(c,CURLOPT_MAXREDIRS,5L);
    curl_easy_setopt(c,CURLOPT_CONNECTTIMEOUT,15L); curl_easy_setopt(c,CURLOPT_LOW_SPEED_LIMIT,128L); curl_easy_setopt(c,CURLOPT_LOW_SPEED_TIME,30L);
    curl_easy_setopt(c,CURLOPT_NOSIGNAL,1L); curl_easy_setopt(c,CURLOPT_USERAGENT,"QUENTIN-STORE/0.1.2");
    curl_easy_setopt(c,CURLOPT_NOPROGRESS,0L); curl_easy_setopt(c,CURLOPT_XFERINFOFUNCTION,progress); curl_easy_setopt(c,CURLOPT_XFERINFODATA,&p);
}
struct Memory { std::string data; };
static size_t memory_write(char* b,size_t s,size_t n,void* opaque) { auto& m=*static_cast<Memory*>(opaque); if(s && n>SIZE_MAX/s)return 0; size_t bytes=s*n; if(m.data.size()+bytes>1024*1024)return 0; m.data.append(b,bytes); return bytes; }
std::string fetch_json(const std::string& url,const std::string& ca,Progress& p) {
    Curl h; common(h.c,url,ca,p); curl_easy_setopt(h.c,CURLOPT_TIMEOUT,60L); Memory m;
    curl_easy_setopt(h.c,CURLOPT_WRITEFUNCTION,memory_write); curl_easy_setopt(h.c,CURLOPT_WRITEDATA,&m);
    auto rc=curl_easy_perform(h.c); long code=0; curl_easy_getinfo(h.c,CURLINFO_RESPONSE_CODE,&code);
    if(rc!=CURLE_OK)throw std::runtime_error(curl_easy_strerror(rc)); if(code!=200)throw std::runtime_error("HTTP "+std::to_string(code)); return m.data;
}
struct Transfer { FILE* f; Progress* p; uint64_t start,size,written=0; bool range=false; long code=0; };
static size_t header(char* b,size_t s,size_t n,void* opaque) {
    auto& t=*static_cast<Transfer*>(opaque); std::string line(b,s*n);
    if(line.rfind("HTTP/",0)==0) { t.range=false; auto pos=line.find(' '); t.code=pos==std::string::npos?0:std::strtol(line.c_str()+pos+1,nullptr,10); }
    if(lower(line).rfind("content-range:",0)==0) { unsigned long long from=0,to=0,total=0;
        if(std::sscanf(line.c_str()+14," bytes %llu-%llu/%llu",&from,&to,&total)==3) t.range=from==t.start && total==t.size && to==total-1;
    } return s*n;
}
static size_t file_write(char* b,size_t s,size_t n,void* opaque) {
    auto& t=*static_cast<Transfer*>(opaque); if(s && n>SIZE_MAX/s)return 0; size_t bytes=s*n;
    if(t.p->cancel || (t.start && (t.code!=206 || !t.range)) || (!t.start && t.code!=200) || bytes>t.size-t.start-t.written) return 0;
    size_t done=fwrite(b,1,bytes,t.f); t.written+=done; t.p->done=t.start+t.written; return done;
}
void download(const Item& a,const std::string& root,const std::string& ca,Progress& p) {
    if(!safe_name(a.id)||!https(a.url)||!hash_valid(a.sha256)||a.size==0||a.size>MaxAsset)throw std::runtime_error("Telechargement refuse");
    directory(root); const auto final=root+"/"+a.id+".elf", part=final+".part", meta=part+".json";
    Json identity={{"url",a.url},{"sha256",a.sha256},{"size",a.size}}; uint64_t offset=0; struct stat st{};
    try { if(Json::parse(read_text(meta))==identity && !stat(part.c_str(),&st) && st.st_size>=0 && uint64_t(st.st_size)<=a.size) offset=uint64_t(st.st_size); }catch(...){}
    write_text(meta,identity.dump()); p.total=a.size; p.done=offset; status(p,"Telechargement: "+a.title);
    if(offset!=a.size) {
        for(int attempt=0;attempt<2;attempt++) {
            File file(safe_open(part,offset>0)); Transfer t{file.f,&p,offset,a.size}; Curl h; common(h.c,a.url,ca,p);
            curl_easy_setopt(h.c,CURLOPT_WRITEFUNCTION,file_write); curl_easy_setopt(h.c,CURLOPT_WRITEDATA,&t);
            curl_easy_setopt(h.c,CURLOPT_HEADERFUNCTION,header); curl_easy_setopt(h.c,CURLOPT_HEADERDATA,&t);
            if(offset)curl_easy_setopt(h.c,CURLOPT_RESUME_FROM_LARGE,curl_off_t(offset));
            auto rc=curl_easy_perform(h.c);
            if(fflush(file.f))throw std::runtime_error("Disque plein ou erreur d'ecriture");
            if(offset && !p.cancel && (t.code==200 || t.code==416)) { offset=0; p.done=0; continue; }
            if(rc!=CURLE_OK)throw std::runtime_error(p.cancel?"Annule, reprise possible":curl_easy_strerror(rc));
            if(t.code!=(offset?206:200)||t.written+offset!=a.size)throw std::runtime_error("Reponse HTTP ou taille incorrecte"); break;
        }
    }
    if(p.cancel)throw std::runtime_error("Annule, reprise possible");
    status(p,"Verification SHA-256");
    if(sha256_file(part)!=a.sha256) { std::remove(part.c_str()); std::remove(meta.c_str()); throw std::runtime_error("SHA-256 incorrect: fichier rejete"); }
#ifdef _WIN32
    std::remove(final.c_str());
#endif
    if(std::rename(part.c_str(),final.c_str()))throw std::runtime_error("Impossible de finaliser le fichier");
    std::remove(meta.c_str());
    Json hist=Json::array(); try { hist=Json::parse(read_text(root+"/history.json")); if(!hist.is_array())hist=Json::array(); }catch(...){}
    hist.push_back({{"title",a.title},{"version",a.version},{"sha256",a.sha256},{"path",final}});
    if(hist.size()>100)hist.erase(hist.begin()); write_text(root+"/history.json",hist.dump(2)); status(p,"Verifie: "+a.title+" | lancement manuel via ELF Loader");
}
}
