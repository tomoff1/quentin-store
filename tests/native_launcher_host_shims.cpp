/* SPDX-License-Identifier: GPL-3.0-or-later
 * Host-only adapters for testing the actual native launcher transfer loop.
 */
#include <winsock2.h>
#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>
static FILE* file_handle;
static SOCKET socket_handle=INVALID_SOCKET;
extern "C" {
int launcher_open(const char* path,int,...) { file_handle=std::fopen(path,"rb");return file_handle?17:-1; }
long launcher_read(int,void* out,std::size_t size) {
    const auto read=std::fread(out,1,size,file_handle);
    return std::ferror(file_handle)?-1:static_cast<long>(read);
}
int launcher_close(int) { const int result=std::fclose(file_handle);file_handle=nullptr;return result; }
int sceNetInit() { WSADATA w;return WSAStartup(MAKEWORD(2,2),&w); }
int sceNetSocket(const char*,int,int,int) {
    socket_handle=socket(AF_INET,SOCK_STREAM,0);return socket_handle==INVALID_SOCKET?-1:42;
}
int sceNetConnect(int,const void* input,unsigned size) {
    if(size!=16) return -1;
    sockaddr_in addr{}; addr.sin_family=AF_INET;
    const auto* bytes=static_cast<const unsigned char*>(input);
    std::memcpy(&addr.sin_port,bytes+2,2);std::memcpy(&addr.sin_addr,bytes+4,4);
    const int result=connect(socket_handle,reinterpret_cast<const sockaddr*>(&addr),sizeof(addr));
    if(result)std::fprintf(stderr,"HOST_SOCKET_ERROR=%d PORT=%hu\n",WSAGetLastError(),ntohs(addr.sin_port));
    return result;
}
int sceNetSend(int,const void* data,std::size_t size,int) {
    if(std::getenv("QS_TEST_FORCE_SEND_FAIL"))return -1;
    // Force partial writes through the production loop.
    if(size>511)size=511;
    return send(socket_handle,static_cast<const char*>(data),static_cast<int>(size),0);
}
int sceNetSetsockopt(int,int,int option,const void* data,unsigned) {
    if(option==0x1109)return 0;
    if(option!=0x1105)return -1;
    DWORD timeout=static_cast<DWORD>(*static_cast<const int*>(data)/1000);
    return setsockopt(socket_handle,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<const char*>(&timeout),sizeof(timeout));
}
int sceNetShutdown(int,int direction) { return shutdown(socket_handle,direction); }
int sceNetSocketClose(int) { return closesocket(socket_handle); }
int sceKernelSendNotificationRequest(unsigned,void* request,std::size_t,int) {
    std::fprintf(stderr,"%s\n",static_cast<const char*>(request)+45);return 0;
}
int sceKernelUsleep(unsigned) { return 0; }
int sceSystemServiceHideSplashScreen() { return 0; }
}
