/* SPDX-License-Identifier: GPL-3.0-or-later
 * QUENTIN STORE launcher: PS4 FPKG frontend for an existing PS5 ELF loader.
 * A successful send does not prove the payload ran. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#ifdef QS_HOST_TEST
#include <winsock2.h>
#include <ws2tcpip.h>
static void notice(const char* s){fprintf(stderr,"%s\n",s);}
#else
#include <orbis/Net.h>
#include <orbis/libkernel.h>
#include <netinet/in.h>
static void notice(const char* s){OrbisNotificationRequest req;memset(&req,0,sizeof req);req.targetId=-1;snprintf(req.message,sizeof req.message,"QUENTIN STORE: %s",s);sceKernelSendNotificationRequest(0,&req,sizeof req,0);}
#endif
int main(int argc,char** argv) {
    const char* path="/app0/quentin-store.elf";
    unsigned short port=9021;
#ifdef QS_HOST_TEST
    WSADATA w; if(WSAStartup(MAKEWORD(2,2),&w))return 1;
    if(argc>1)path=argv[1];if(argc>2)port=(unsigned short)atoi(argv[2]);
#else
    (void)argc;(void)argv;sceNetInit();
#endif
    FILE* f=fopen(path,"rb");
    if(!f){notice("ELF embarque introuvable");return 2;}
    unsigned char magic[4];
    if(fread(magic,1,4,f)!=4 || memcmp(magic,"\177ELF",4)){fclose(f);notice("ELF embarque invalide");return 3;}
    rewind(f);
    struct sockaddr_in addr;memset(&addr,0,sizeof addr);addr.sin_family=2;
#ifdef QS_HOST_TEST
    addr.sin_port=htons(port);addr.sin_addr.s_addr=htonl(0x7f000001);
    SOCKET s=socket(AF_INET,SOCK_STREAM,0);
    if(s==INVALID_SOCKET){fclose(f);return 4;}
    DWORD timeout=10000;setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof timeout);
    int connected=connect(s,(struct sockaddr*)&addr,sizeof addr);
#else
    addr.sin_len=sizeof addr;addr.sin_port=sceNetHtons(port);sceNetInetPton(2,"127.0.0.1",&addr.sin_addr);
    int s=sceNetSocket("quentin-launcher",2,1,0);
    if(s<0){fclose(f);notice("Creation socket refusee");return 4;}
    struct { long sec,usec; } timeout={10,0};sceNetSetsockopt(s,0xffff,0x1005,&timeout,sizeof timeout);
    int connected=sceNetConnect(s,(OrbisNetSockaddr*)&addr,sizeof addr);
#endif
    int result=0;
    if(connected){notice("Chargeur ELF inaccessible sur 9021. Active-le dans etaHEN.");result=5;}
    char buffer[32768];size_t n;
    while(!result && (n=fread(buffer,1,sizeof buffer,f))){
        size_t offset=0;
        while(offset<n){
#ifdef QS_HOST_TEST
            int sent=send(s,buffer+offset,(int)(n-offset),0);
#else
            int sent=sceNetSend(s,buffer+offset,n-offset,0);
#endif
            if(sent<=0){notice("Envoi interrompu vers le chargeur ELF");result=6;break;}
            offset+=(size_t)sent;
        }
    }
    if(ferror(f)){notice("Lecture de l'ELF impossible");result=7;}
    fclose(f);
#ifdef QS_HOST_TEST
    shutdown(s,SD_SEND);closesocket(s);WSACleanup();
#else
    sceNetShutdown(s,1);sceNetSocketClose(s);
#endif
    if(!result)notice("ELF transmis au chargeur. Lancement du store non confirme.");
    return result;
}
