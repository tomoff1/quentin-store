#include "network.hpp"
#include <curl/curl.h>
#include <iostream>
int main(int argc,char** argv){
    if(argc!=4)return 2;
    try {
        curl_global_init(CURL_GLOBAL_DEFAULT);qs::Progress p;
        auto items=qs::parse_catalog(qs::fetch_json(argv[1],argv[2],p));
        auto it=std::find_if(items.begin(),items.end(),[](const qs::Item& a){return a.id=="quentin-blocks";});
        if(it==items.end())throw std::runtime_error("Jeu original absent du catalogue public");
        qs::download(*it,argv[3],argv[2],p);
        if(qs::sha256_file(std::string(argv[3])+"/quentin-blocks.elf")!=it->sha256)throw std::runtime_error("Hash public incorrect");
        std::cout<<"Catalogue public HTTPS + release GitHub + SHA256 verifies par le moteur C++ reel\n";
        curl_global_cleanup();return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<"\n";curl_global_cleanup();return 1;}
}
