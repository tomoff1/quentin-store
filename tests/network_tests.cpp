#include "network.hpp"
#include <curl/curl.h>
#include <iostream>
#include <cstdio>
using namespace qs;
int tests=0;
void check(bool b){tests++;if(!b)throw std::runtime_error("Echec controle reseau "+std::to_string(tests));}
template<class F>void reject(F f){bool caught=false;try{f();}catch(const std::exception& e){caught=true;std::cout<<"Refus attendu: "<<e.what()<<"\n";}check(caught);}
int main(int argc,char** argv){
    if(argc!=4)return 2;std::string base=argv[1],ca=argv[2],root=argv[3];directory(root);Progress p;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    std::string payload;for(int i=0;i<10000;i++)payload+="QUENTIN42\n";
    write_text(root+"/reference",payload);auto digest=sha256_file(root+"/reference");
    Item a{"test-game","Test","0.1.0","Jeux","Test",base+"/asset",digest,"GPL-3.0-or-later","https://example.org",payload.size()};
    check(fetch_json(base+"/json",ca,p)=="{\"schema\":1,\"items\":[]}");
    reject([&]{fetch_json(base+"/404",ca,p);});reject([&]{fetch_json(base+"/large-json",ca,p);});reject([&]{fetch_json(base+"/json",root+"/missing.crt",p);});
    auto wrongHost=base;wrongHost.replace(8,9,"127.0.0.1");reject([&]{fetch_json(wrongHost+"/json",ca,p);});
    download(a,root,ca,p);check(sha256_file(root+"/test-game.elf")==digest);check(Json::parse(read_text(root+"/history.json")).size()==1);
    auto resume=[&](const std::string& route){a.url=base+route;std::remove((root+"/test-game.elf").c_str());write_text(root+"/test-game.elf.part",payload.substr(0,1234));write_text(root+"/test-game.elf.part.json",Json{{"url",a.url},{"sha256",a.sha256},{"size",a.size}}.dump());};
    resume("/asset");download(a,root,ca,p);check(sha256_file(root+"/test-game.elf")==digest);
    resume("/ignore");download(a,root,ca,p);check(sha256_file(root+"/test-game.elf")==digest);
    resume("/wrong-range");reject([&]{download(a,root,ca,p);});check(read_text(root+"/test-game.elf.part")==payload.substr(0,1234));
    a.url=base+"/corrupt";reject([&]{download(a,root,ca,p);});reject([&]{read_text(root+"/test-game.elf.part");});
    a.url=base+"/short";reject([&]{download(a,root,ca,p);});
    a.url=base+"/oversize";reject([&]{download(a,root,ca,p);});check(read_text(root+"/test-game.elf.part").size()<=a.size);
    a.url=base+"/drop";reject([&]{download(a,root,ca,p);});download(a,root,ca,p);check(sha256_file(root+"/test-game.elf")==digest);
    a.url=base+"/asset";p.cancel=true;reject([&]{download(a,root,ca,p);});p.cancel=false;
    a.id="../escape";reject([&]{download(a,root,ca,p);});
    curl_global_cleanup();std::cout<<tests<<" controles reseau reussis\n";
}
