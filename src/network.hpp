#pragma once
#include "core.hpp"
#include <atomic>
#include <mutex>
namespace qs {
struct Progress { std::mutex mutex; std::string message="Pret", error; std::atomic<uint64_t> done{0}, total{0}; std::atomic<bool> cancel{false}, busy{false}; };
std::string read_text(const std::string& path,size_t limit=1024*1024);
void write_text(const std::string& path,const std::string& text);
void directory(const std::string& path);
std::string sha256_file(const std::string& path);
std::string fetch_json(const std::string& url,const std::string& ca,Progress& p);
void download(const Item& a,const std::string& root,const std::string& ca,Progress& p);
void status(Progress& p,const std::string& msg,const std::string& error="");
}
