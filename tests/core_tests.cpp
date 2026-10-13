#include "core.hpp"
#include <iostream>
using namespace qs;
int checks=0;
void check(bool b) { checks++; if(!b)throw std::runtime_error("Echec test "+std::to_string(checks)); }
template<class F> void reject(F f){bool caught=false;try{f();}catch(...){caught=true;}check(caught);}
int main(){
    check(safe_name("quentin-blocks"));for(auto s:{"../x","..","/x","a\\b","CON","a%2fb","a.b",""})check(!safe_name(s));
    check(https("https://github.com/a/b"));for(auto s:{"http://x","https://","https://u:p@x","https://x\\y","https://x\n"})check(!https(s));
    check(hash_valid(std::string(64,'a')));check(!hash_valid(std::string(64,'g')));
    Json item={{"id","quentin-blocks"},{"title","Quentin Blocks"},{"version","0.1.0"},{"category","Jeux"},{"description","Jeu original"},{"url","https://example.org/game.elf"},{"sha256",std::string(64,'a')},{"size",123},{"license","GPL-3.0-or-later"},{"source","https://example.org/source"},{"redistribution",true},{"format","elf"}};
    auto a=parse_item(item);check(matches(a,"BLOCKS","Jeux"));check(!matches(a,"Blocks","Outils"));
    check(parse_catalog(Json{{"schema",1},{"items",Json::array({item})}}.dump()).size()==1);
    reject([&]{parse_catalog(Json{{"schema",1},{"items",Json::array({item,item})}}.dump());});
    for(auto mutation:{"id","url","sha256","license"}){auto bad=item;bad[mutation]="";reject([&]{parse_item(bad);});}
    for(int64_t size:{int64_t(-1),int64_t(0),int64_t(MaxAsset+1)}){auto bad=item;bad["size"]=size;reject([&]{parse_item(bad);});}
    auto bad=item;bad["redistribution"]=false;reject([&]{parse_item(bad);});bad=item;bad["format"]="pkg";reject([&]{parse_item(bad);});
    reject([&]{parse_catalog("{");});reject([&]{parse_catalog(std::string(1024*1024+1,'x'));});
    check(newer("0.2.0","0.1.9"));check(!newer("0.1.0","0.1.0"));check(newer("1.0.0","0.99.0"));reject([]{semver("1.2-beta");});
    std::cout<<checks<<" controles reussis\n";
}
