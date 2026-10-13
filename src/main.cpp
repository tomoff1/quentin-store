#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL2_gfxPrimitives.h>
#include <SDL_ttf.h>
#include "drawing.hpp"
#include "network.hpp"
#include "embedded_assets.hpp"
#include "startup.hpp"
#include <curl/curl.h>
#include <thread>
#include <cmath>
#include <cstdlib>
#include <map>
using namespace qs;
namespace {
SDL_Renderer* renderer=nullptr;
TTF_Font* fonts[3]={nullptr,nullptr,nullptr};
struct Label { SDL_Texture* texture; int width,height; };
std::map<std::string,Label> labels;
void clearLabels(){for(auto& t:labels)SDL_DestroyTexture(t.second.texture);labels.clear();}
void rect(int x,int y,int w,int h,Uint32 color) { boxColor(renderer,x,y,x+w,y+h,color); }
void text(int x,int y,const std::string& s,Uint32 color=0xe8eefaff,int scale=2) {
    if(s.empty())return;
    if(fonts[scale-1]) {
        auto key=std::to_string(scale)+":"+std::to_string(color)+":"+s;
        if(labels.size()>256)clearLabels();
        auto it=labels.find(key);
        if(it==labels.end()) {
            SDL_Color c={Uint8(color>>24),Uint8(color>>16),Uint8(color>>8),Uint8(color)};
            auto* surface=TTF_RenderUTF8_Blended(fonts[scale-1],s.c_str(),c);
            if(surface){auto* t=SDL_CreateTextureFromSurface(renderer,surface);if(t)it=labels.emplace(key,Label{t,surface->w,surface->h}).first;SDL_FreeSurface(surface);}
        }
        if(it!=labels.end()){auto l=it->second;SDL_Rect dest={x,y,l.width,l.height};SDL_RenderCopy(renderer,l.texture,nullptr,&dest);return;}
    }
    SDL_RenderSetScale(renderer,float(scale),float(scale));
    stringColor(renderer,x/scale,y/scale,s.c_str(),color); SDL_RenderSetScale(renderer,1,1);
}
void wrap(int x,int y,const std::string& s,int width=48) {
    size_t start=0; int line=0;
    while(start<s.size() && line<12) { size_t end=std::min(start+size_t(width),s.size()); if(end<s.size()){auto sp=s.rfind(' ',end); if(sp!=std::string::npos&&sp>start)end=sp;}
        text(x,y+line*24,s.substr(start,end-start),0xb7c4dfff); start=end; if(start<s.size()&&s[start]==' ')start++; line++; }
}
void artwork(int x,int y,int w,int h,int seed) {
    Uint32 colors[]={0x383a85ff,0x104f65ff,0x4b2869ff,0x175444ff}; rect(x,y,w,h,colors[seed%4]);
    for(int k=0;k<5;k++) { int a=x+30+k*42, b=y+25+(k%2)*23; roundedBoxColor(renderer,a,b,a+30,b+32,5,0x90f5e044); }
    circleColor(renderer,x+w-50,y+h-40,48,0x98b9ff88); text(x+22,y+h-35,"HOMEBREW",0xffffffff,2);
}
}
int main(int argc,char** argv) {
#ifdef __PROSPERO__
    std::string root="/data/quentin-store";
#else
    std::string root="quentin-store-data";
    if(argc>1)root=argv[1];
#endif
    StartupLog boot(root);
    startup_notice("Demarrage du store 0.1.2");
    Progress progress; std::thread worker; std::mutex resultMutex; std::string pendingCatalog,pendingUpdate;
    std::string catalogUrl="https://raw.githubusercontent.com/tomoff1/quentin-store/main/site/catalog.json",updateUrl="https://raw.githubusercontent.com/tomoff1/quentin-store/main/site/update.json",ca=root+"/ca-bundle.crt",last="Catalogue local",query,category="Tous";
    std::vector<Item> items; Json history=Json::array();
    try {
        directory(root); directory(root+"/downloads");
        boot.record("Dossier de donnees accessible");
        write_text(ca,std::string(reinterpret_cast<const char*>(embedded::ca),sizeof embedded::ca));
        items=parse_catalog(std::string(reinterpret_cast<const char*>(embedded::catalog),sizeof embedded::catalog));
        try { auto config=Json::parse(read_text(root+"/config.json")); catalogUrl=config.value("catalog_url",""); updateUrl=config.value("update_url",""); }catch(...){write_text(root+"/config.json",Json{{"catalog_url",catalogUrl},{"update_url",updateUrl}}.dump(2));}
        try { items=parse_catalog(read_text(root+"/catalog.json")); }catch(...){write_text(root+"/catalog.json",std::string(reinterpret_cast<const char*>(embedded::catalog),sizeof embedded::catalog));}
        try { history=Json::parse(read_text(root+"/downloads/history.json")); if(!history.is_array())history=Json::array(); }catch(...){}
    }catch(const std::exception& e) { boot.record(std::string("Echec fichiers: ")+e.what(),true); return 1; }
    boot.record("Ressources embarquees et configuration chargees");
    if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK){boot.record("Echec initialisation HTTPS",true);return 2;}
    boot.record("Initialisation video et manette");
    SDL_SetMainReady(); if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK|SDL_INIT_EVENTS)) { boot.record(std::string("Echec SDL: ")+SDL_GetError(),true); curl_global_cleanup(); return 3; }
    boot.record("SDL initialise");
    SDL_Window* window=SDL_CreateWindow("QUENTIN STORE",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1280,720,SDL_WINDOW_SHOWN);
    if(window)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
    if(!renderer){boot.record(std::string("Echec affichage: ")+SDL_GetError(),true); SDL_Quit(); curl_global_cleanup(); return 4;}
    boot.record("Fenetre et moteur de rendu crees");
    SDL_RenderSetLogicalSize(renderer,1280,720);
    if(TTF_Init()==0){int sizes[]={13,20,32};for(int i=0;i<3;i++){fonts[i]=TTF_OpenFontRW(SDL_RWFromConstMem(embedded::font,int(sizeof embedded::font)),1,sizes[i]);if(fonts[i])TTF_SetFontStyle(fonts[i],TTF_STYLE_BOLD);}}
    std::map<int,SDL_Joystick*> pads;
    auto openPad=[&](int index){auto* p=SDL_JoystickOpen(index); if(p)pads[SDL_JoystickInstanceID(p)]=p;};
    for(int i=0;i<SDL_NumJoysticks();i++)openPad(i);
    auto launch=[&](auto task) {
        if(progress.busy)return;
        if(worker.joinable())worker.join(); progress.cancel=false; progress.busy=true; progress.done=0; progress.total=0;
        status(progress,"Connexion...");
        worker=std::thread([&,task]{try{task();}catch(const std::exception& e){status(progress,"Operation interrompue",e.what()); try{write_text(root+"/diagnostic.log",e.what());}catch(...){} } progress.busy=false;});
    };
    auto refresh=[&]{ if(catalogUrl.empty()){status(progress,"Configurer catalog_url dans config.json");return;} auto url=catalogUrl; launch([&,url]{auto data=fetch_json(url,ca,progress); parse_catalog(data); write_text(root+"/catalog.json",data); {std::lock_guard<std::mutex> l(resultMutex);pendingCatalog=data;} status(progress,"Catalogue actualise");}); };
    int tab=0,selection=0,detail=-1,keyIndex=0,historyScroll=0; bool running=true,keyboard=false; std::string editTarget="search";
    const std::string alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -.:/?_";
    float focusY=160; Uint32 prev=SDL_GetTicks(),lastAxis=0; int frames=0;
    const char* tabs[]={"Accueil","Catalogue","Recherche","Historique","Parametres"};
    std::vector<int> visible;
    auto rebuild=[&]{visible.clear(); for(size_t i=0;i<items.size();i++)if(matches(items[i],query,category))visible.push_back(int(i)); selection=visible.empty()?0:std::max(0,std::min(selection,int(visible.size())-1));};
    rebuild(); if(!catalogUrl.empty())refresh();
    auto action=[&](int key){
        if(key==SDLK_ESCAPE){ if(keyboard){keyboard=false;return;} if(detail>=0){detail=-1;return;}running=false;return; }
        if(key==SDLK_BACKSPACE && !keyboard){if(detail>=0)detail=-1;else if(progress.busy)progress.cancel=true;return;}
        if(keyboard){std::string& dest=editTarget=="catalog"?catalogUrl:editTarget=="update"?updateUrl:query;
            if(key==SDLK_LEFT)keyIndex=(keyIndex+int(alphabet.size())-1)%int(alphabet.size());
            if(key==SDLK_RIGHT)keyIndex=(keyIndex+1)%int(alphabet.size());
            if(key==SDLK_UP)keyIndex=(keyIndex+int(alphabet.size())-10)%int(alphabet.size());
            if(key==SDLK_DOWN)keyIndex=(keyIndex+10)%int(alphabet.size());
            if(key==SDLK_RETURN && dest.size()<2048)dest+=editTarget=="search"?alphabet[keyIndex]:char(std::tolower(alphabet[keyIndex]));
            if(key==SDLK_BACKSPACE && !dest.empty())dest.pop_back();
            if(key==SDLK_TAB || key==SDLK_r){keyboard=false;rebuild(); if(editTarget!="search")try{write_text(root+"/config.json",Json{{"catalog_url",catalogUrl},{"update_url",updateUrl}}.dump(2));}catch(const std::exception& e){status(progress,"Erreur",e.what());}} return;
        }
        if(key==SDLK_TAB){tab=(tab+1)%5;detail=-1;return;}
        if(key==SDLK_q){tab=(tab+4)%5;detail=-1;return;}
        if(key==SDLK_r){refresh();return;}
        if(detail>=0){if(key==SDLK_RETURN){auto a=items[detail];launch([&,a]{download(a,root+"/downloads",ca,progress);});}return;}
        if(tab==4){
            if(key==SDLK_UP)selection=(selection+3)%4;
            if(key==SDLK_DOWN)selection=(selection+1)%4;
            if(key==SDLK_RETURN){
                if(selection<2){editTarget=selection==0?"catalog":"update";keyboard=true;keyIndex=0;}
                if(selection==2){if(updateUrl.empty())status(progress,"Configurer update_url");else {auto url=updateUrl;launch([&,url]{auto data=fetch_json(url,ca,progress); auto j=Json::parse(data); if(j.at("schema")!=1)throw std::runtime_error("Manifeste invalide"); auto a=parse_item(j.at("asset"));if(a.id!="quentin-store" || a.version!=j.at("version").get<std::string>())throw std::runtime_error("Mise a jour incoherente"); auto notes=field(j,"notes",1500); {std::lock_guard<std::mutex> l(resultMutex);pendingUpdate=data;} status(progress,newer(a.version,Version)?"Mise a jour disponible: "+a.version:"Version a jour");});}}
                if(selection==3)running=false;
            } return;
        }
        if(tab==3){if(key==SDLK_DOWN)historyScroll++;if(key==SDLK_UP)historyScroll=std::max(0,historyScroll-1);return;}
        if(tab==2 && key==SDLK_c){editTarget="search";keyboard=true;keyIndex=0;return;}
        if(key==SDLK_UP)selection=std::max(0,selection-3);
        if(key==SDLK_DOWN)selection=std::min(std::max(0,int(visible.size())-1),selection+3);
        if(key==SDLK_LEFT)selection=std::max(0,selection-1);
        if(key==SDLK_RIGHT)selection=std::min(std::max(0,int(visible.size())-1),selection+1);
        if(key==SDLK_c){std::vector<std::string> cats={"Tous"};for(auto& a:items)if(std::find(cats.begin(),cats.end(),a.category)==cats.end())cats.push_back(a.category);auto it=std::find(cats.begin(),cats.end(),category);category=cats[(size_t(it-cats.begin())+1)%cats.size()];selection=0;rebuild();}
        if(key==SDLK_RETURN && !visible.empty())detail=visible[selection];
    };
    std::string updateData; bool wasBusy=false;
    while(running){
        Uint32 now=SDL_GetTicks(); float dt=std::min(0.05f,float(now-prev)/1000);prev=now;
        {std::lock_guard<std::mutex> l(resultMutex); if(!pendingCatalog.empty()){items=parse_catalog(pendingCatalog);pendingCatalog.clear();rebuild();detail=-1;}if(!pendingUpdate.empty()){updateData=pendingUpdate;pendingUpdate.clear();}}
        if(wasBusy&&!progress.busy)try{history=Json::parse(read_text(root+"/downloads/history.json"));}catch(...){} wasBusy=progress.busy;
        SDL_Event ev;
        while(SDL_PollEvent(&ev)){
            if(ev.type==SDL_QUIT)running=false;
            if(ev.type==SDL_KEYDOWN){if(ev.key.keysym.sym==SDLK_u&&!updateData.empty()){auto j=Json::parse(updateData); auto a=parse_item(j.at("asset")); if(newer(a.version,Version))launch([&,a]{download(a,root+"/downloads",ca,progress);});}else action(ev.key.keysym.sym);}
            if(ev.type==SDL_JOYDEVICEADDED)openPad(ev.jdevice.which);
            if(ev.type==SDL_JOYDEVICEREMOVED){auto it=pads.find(ev.jdevice.which);if(it!=pads.end()){SDL_JoystickClose(it->second);pads.erase(it);}}
            if(ev.type==SDL_JOYBUTTONDOWN){int keys[]={SDLK_RETURN,SDLK_BACKSPACE,SDLK_c,SDLK_r,SDLK_r,0,SDLK_ESCAPE,0,0,SDLK_q,SDLK_TAB,SDLK_UP,SDLK_DOWN,SDLK_LEFT,SDLK_RIGHT}; if(ev.jbutton.button<15)action(keys[ev.jbutton.button]);if(ev.jbutton.button==15 && !updateData.empty()){auto j=Json::parse(updateData);auto a=parse_item(j.at("asset"));if(newer(a.version,Version))launch([&,a]{download(a,root+"/downloads",ca,progress);});}}
            if(ev.type==SDL_JOYHATMOTION){if(ev.jhat.value&SDL_HAT_UP)action(SDLK_UP);if(ev.jhat.value&SDL_HAT_DOWN)action(SDLK_DOWN);if(ev.jhat.value&SDL_HAT_LEFT)action(SDLK_LEFT);if(ev.jhat.value&SDL_HAT_RIGHT)action(SDLK_RIGHT);}
            if(ev.type==SDL_JOYAXISMOTION && now-lastAxis>160 && std::abs(int(ev.jaxis.value))>16000 && ev.jaxis.axis<2){action(ev.jaxis.axis==0?(ev.jaxis.value>0?SDLK_RIGHT:SDLK_LEFT):(ev.jaxis.value>0?SDLK_DOWN:SDLK_UP));lastAxis=now;}
        }
        SDL_SetRenderDrawColor(renderer,10,15,27,255);SDL_RenderClear(renderer);
        rect(0,0,245,720,0x111a2bff); text(26,45,"QUENTIN",0xffffffff,3);text(26,78,"STORE",0x75ebd5ff,3);text(26,120,"HOME OF HOMEBREW",0x91a0baff,1);
        focusY+=(160+tab*65-focusY)*std::min(1.f,dt*14);roundedBoxColor(renderer,16,int(focusY)-12,229,int(focusY)+35,8,0x244a58ff);
        for(int i=0;i<5;i++)text(28,160+i*65,tabs[i],i==tab?0x94ffe4ff:0xa5b4ceff,2);
        text(28,615,std::string("v")+Version+" | ELF",0x8797b4ff,1);text(28,640,"13.60: test requis",0xffc783ff,1);
        text(280,36,tabs[tab],0xffffffff,3);text(280,78,"Libre. Gratuit. Verifie.",0x94a8c5ff,2);
        if(tab<3){text(280,117,"Categorie: "+category+"  |  Recherche: "+query.substr(0,25),0x75ebd5ff,1);
            if(visible.empty()){artwork(280,160,900,210,0);text(310,390,"Catalogue vide",0xffffffff,3);wrap(310,440,"Configurer le catalogue HTTPS dans Parametres, puis actualiser avec Triangle. Le catalogue local est aussi accepte.",52);}
            int first=(selection/6)*6;
            for(int slot=0;slot<6 && first+slot<int(visible.size());slot++){auto& a=items[visible[first+slot]];int x=280+(slot%3)*310,y=155+(slot/3)*215;
                bool selected=first+slot==selection;roundedBoxColor(renderer,x-5,y-5,x+285,y+195,10,selected?0x69dfcaff:0x202c43ff);artwork(x,y,280,135,visible[first+slot]);rect(x,y+135,280,55,0x192338ff);text(x+12,y+148,a.title.substr(0,16),0xffffffff,2);text(x+12,y+175,a.category+" | v"+a.version,0xa8b9d5ff,1);}
            if(tab==2)text(280,604,"Carre / C: clavier de recherche | X: ouvrir",0x9eb5d4ff,1);
        }
        if(tab==3){text(280,140,"Fichiers verifies SHA-256",0x75ebd5ff,2);historyScroll=std::min(historyScroll,std::max(0,int(history.size())-1));for(int k=0;k<8 && k+historyScroll<int(history.size());k++){const auto& h=history[history.size()-1-k-historyScroll];text(280,190+k*45,h.value("title","")+"  v"+h.value("version",""),0xdce8ffff,2);}}
        if(tab==4){const std::string settings[]={"URL du catalogue","URL des mises a jour","Verifier les mises a jour","Quitter"};selection=std::min(selection,3);for(int i=0;i<4;i++){rect(280,150+i*70,900,55,selection==i?0x24535bff:0x1a263bff);text(298,166+i*70,settings[i],0xe6f2ffff,2);}text(280,447,"HTTPS + certificats | aucun suivi | installation manuelle",0x92a5c5ff,1);
            if(!updateData.empty()){auto j=Json::parse(updateData);wrap(280,475,j.value("notes",""),70);text(280,560,"L2 / U: telecharger la mise a jour verifiee",0x75ebd5ff,1);}}
        if(detail>=0 && detail<int(items.size())){auto& a=items[detail];rect(265,120,975,490,0x152137ff);artwork(285,140,320,220,detail);text(640,145,a.title,0xffffffff,2);text(640,180,"v"+a.version+"  |  "+a.license,0x75ebd5ff,1);wrap(640,215,a.description,34);text(290,390,"Taille: "+std::to_string(a.size)+" octets",0xbacbe5ff,2);text(290,425,"SHA256: "+a.sha256.substr(0,32),0x9cafd0ff,1);text(290,447,a.sha256.substr(32),0x9cafd0ff,1);rect(290,500,560,60,0x23665fff);text(310,521,"X / Entree: telecharger",0xffffffff,2);text(290,583,"Cercle: retour | lancement manuel via ELF Loader",0xaec0deff,1);}
        if(keyboard){rect(265,120,975,490,0x18243cff);text(290,145,"Clavier: "+editTarget,0x75ebd5ff,2);std::string dest=editTarget=="catalog"?catalogUrl:editTarget=="update"?updateUrl:query;wrap(290,185,dest,70);for(size_t k=0;k<alphabet.size();k++){int x=300+int(k%10)*80,y=300+int(k/10)*45;rect(x,y,65,35,int(k)==keyIndex?0x398478ff:0x273852ff);text(x+25,y+8,std::string(1,alphabet[k]),0xffffffff,2);}text(290,570,"X: ajouter | Cercle: effacer | Triangle/R1: valider",0xbfd0edff,1);}
        std::string msg,error;{std::lock_guard<std::mutex> l(progress.mutex);msg=progress.message;error=progress.error;}
        uint64_t total=progress.total,done=progress.done;rect(280,630,910,7,0x25344cff);if(total)rect(280,630,int(910*std::min(1.0,double(done)/double(total))),7,0x75ebd5ff);
        text(280,650,(error.empty()?msg:"Erreur: "+error).substr(0,108),error.empty()?0xc7d6efff:0xff938aff,1);
        text(280,683,"L1/R1: onglets | X: ouvrir | Carre: categorie | Triangle: actualiser | Options: quitter",0x91a5c2ff,1);
        SDL_RenderPresent(renderer);SDL_Delay(16);frames++;
        if(frames==1)boot.record("Premiere image envoyee au moteur video",true);
        if(std::getenv("QS_CAPTURE") && frames==30){SDL_Surface* s=SDL_CreateRGBSurfaceWithFormat(0,1280,720,32,SDL_PIXELFORMAT_ARGB8888);if(s){SDL_RenderReadPixels(renderer,nullptr,s->format->format,s->pixels,s->pitch);SDL_SaveBMP(s,std::getenv("QS_CAPTURE"));SDL_FreeSurface(s);}running=false;}
    }
    progress.cancel=true; if(worker.joinable())worker.join();for(auto& p:pads)SDL_JoystickClose(p.second);
    clearLabels();for(auto* f:fonts)if(f)TTF_CloseFont(f);TTF_Quit();SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();curl_global_cleanup(); return 0;
}
