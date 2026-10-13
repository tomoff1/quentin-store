#pragma once
#include "json.hpp"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
namespace qs {
using Json = nlohmann::json;
constexpr const char* Version = "0.1.0";
constexpr uint64_t MaxAsset = 2ULL * 1024 * 1024 * 1024;
struct Item {
    std::string id, title, version, category, description, url, sha256, license, source;
    uint64_t size = 0;
};
inline bool safe_name(const std::string& s) {
    return !s.empty() && s.size() <= 80 && s != "." && s != ".." &&
      std::all_of(s.begin(), s.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
      });
}
inline bool https(const std::string& s) {
    if (s.size() > 2048 || s.rfind("https://", 0) != 0) return false;
    auto end = s.find_first_of("/?#", 8);
    auto host = s.substr(8, end == std::string::npos ? end : end - 8);
    if (host.empty() || host.find('@') != std::string::npos || host.find('\\') != std::string::npos) return false;
    return std::none_of(s.begin(), s.end(), [](unsigned char c) { return c <= 32 || c == 127 || c == '\\'; });
}
inline bool hash_valid(const std::string& s) {
    return s.size() == 64 && std::all_of(s.begin(), s.end(), [](unsigned char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}
inline std::string field(const Json& j, const char* name, size_t max) {
    auto s = j.at(name).get<std::string>();
    if (s.empty() || s.size() > max || s.find('\0') != std::string::npos) throw std::runtime_error(std::string("Champ invalide: ") + name);
    return s;
}
inline Item parse_item(const Json& j) {
    Item a;
    a.id=field(j,"id",80); a.title=field(j,"title",80); a.version=field(j,"version",40);
    a.category=field(j,"category",40); a.description=field(j,"description",1500);
    a.url=field(j,"url",2048); a.sha256=field(j,"sha256",64);
    a.license=field(j,"license",100); a.source=field(j,"source",2048);
    if (!j.at("size").is_number_unsigned() && !j.at("size").is_number_integer()) throw std::runtime_error("Taille invalide");
    if (j.at("size").get<int64_t>() <= 0) throw std::runtime_error("Taille invalide");
    a.size=j.at("size").get<uint64_t>();
    if (!safe_name(a.id) || !https(a.url) || !https(a.source) || !hash_valid(a.sha256) || a.size > MaxAsset || j.value("redistribution",false) != true || j.value("format","") != "elf")
        throw std::runtime_error("Entree refusee: nom, HTTPS, SHA256, taille, ELF ou droits");
    return a;
}
inline std::vector<Item> parse_catalog(const std::string& text) {
    if (text.size() > 1024*1024) throw std::runtime_error("Catalogue trop volumineux");
    auto j=Json::parse(text);
    if(j.at("schema") != 1 || !j.at("items").is_array() || j.at("items").size()>200) throw std::runtime_error("Schema de catalogue invalide");
    std::vector<Item> out; std::set<std::string> ids;
    for(const auto& e:j.at("items")) { auto a=parse_item(e); if(!ids.insert(a.id).second) throw std::runtime_error("Identifiant duplique"); out.push_back(a); }
    return out;
}
inline std::string lower(std::string s) {
    std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return char(std::tolower(c));}); return s;
}
inline bool matches(const Item& a,const std::string& query,const std::string& category) {
    return (category=="Tous" || a.category==category) && lower(a.title+" "+a.description).find(lower(query))!=std::string::npos;
}
inline std::vector<int> semver(const std::string& s) {
    std::vector<int> v; std::string part;
    for(char c:s+".") { if(c=='.') { if(part.empty() || part.size()>5) throw std::runtime_error("Version invalide"); v.push_back(std::stoi(part)); part.clear(); } else if(c>='0'&&c<='9') part+=c; else throw std::runtime_error("Version invalide"); }
    if(v.size()!=3) throw std::runtime_error("Version invalide");
    return v;
}
inline bool newer(const std::string& a,const std::string& b) { return semver(a)>semver(b); }
}
