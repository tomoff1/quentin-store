#pragma once
#include "network.hpp"
#include <cstdio>
#include <cstring>
#ifdef __PROSPERO__
extern "C" {
struct QSNotification { char reserved[45]; char message[3075]; };
int sceKernelSendNotificationRequest(int, QSNotification*, size_t, int);
}
#endif
namespace qs {
inline void startup_notice(const std::string& message) noexcept {
    std::fprintf(stderr,"QUENTIN STORE: %s\n",message.c_str());
#ifdef __PROSPERO__
    QSNotification request{};
    std::snprintf(request.message,sizeof request.message,"QUENTIN STORE: %s",message.c_str());
    sceKernelSendNotificationRequest(0,&request,sizeof request,0);
#endif
}
struct StartupLog {
    std::string root,events;
    explicit StartupLog(std::string path):root(std::move(path)){}
    void record(const std::string& event,bool notify=false) noexcept {
        std::fprintf(stderr,"Startup: %s\n",event.c_str());
        try {events+=event+"\n";write_text(root+"/startup.log",events);}catch(...){}
        if(notify)startup_notice(event);
    }
};
}
