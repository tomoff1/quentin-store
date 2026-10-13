/* SPDX-License-Identifier: GPL-3.0-or-later
 * QUENTIN STORE native PS5 launcher. Uses a separately running ELF loader.
 * A successful TCP transfer does not establish payload execution.
 */
#include "demo_renderer.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>
#ifdef QS_HOST_TEST
#include <cstdlib>
#endif

extern "C" {
int open(const char*, int, ...);
long read(int, void*, std::size_t);
int close(int);
int sceNetInit();
int sceNetSocket(const char*, int, int, int);
int sceNetConnect(int, const void*, unsigned);
int sceNetSend(int, const void*, std::size_t, int);
int sceNetSetsockopt(int, int, int, const void*, unsigned);
int sceNetShutdown(int, int);
int sceNetSocketClose(int);
int sceKernelSendNotificationRequest(unsigned, void*, std::size_t, int);
int sceKernelUsleep(unsigned);
int sceSystemServiceHideSplashScreen();
int sceSystemServiceLoadExec(const char*, const char* const*);
}

namespace {
struct Notification { std::uint8_t reserved[45]; char message[3075]; };
void notice(std::string_view message) noexcept {
    Notification n{};
    const auto count = message.size() < sizeof(n.message)-1 ? message.size() : sizeof(n.message)-1;
    for (std::size_t i=0; i<count; ++i) n.message[i]=message[i];
    sceKernelSendNotificationRequest(0, &n, sizeof(n), 0);
}
const char* error_message = "ERREUR INCONNUE";
#ifndef QS_HOST_TEST
void failure_scene(ps5::demo::Canvas& c) noexcept {
    using ps5::demo::Color;
    c.clear(Color::background);
    c.text(120, 130, "QUENTIN STORE PS5", 10, Color::white);
    c.text(120, 360, error_message, 5, Color::yellow);
    c.text(120, 570, "ACTIVE LE CHARGEUR ELF SUR LE PORT 9021", 4, Color::cyan);
    c.text(120, 650, "PUIS FERME ET RELANCE CETTE APPLICATION", 4, Color::white);
    c.text(120, 830, "VERSION NATIVE EXPERIMENTALE 01.000.000", 3, Color::white);
}
#endif
int fail(const char* text) noexcept {
    error_message=text;
    notice(text);
#ifdef QS_HOST_TEST
    return 2;
#else
    ps5::demo::run(failure_scene, text);
#endif
}
}

int main() {
    notice("QUENTIN STORE PS5: lanceur natif execute");
    const char* path="/app0/assets/quentin-store.payload";
    unsigned short port=9021;
#ifdef QS_HOST_TEST
    if(const char* value=std::getenv("QS_TEST_PAYLOAD")) path=value;
    if(const char* value=std::getenv("QS_TEST_PORT")) port=static_cast<unsigned short>(std::atoi(value));
#endif
    notice("QUENTIN STORE PS5: lecture du store embarque");
    const int f=open(path,0);
    if(f<0) return fail("ELF EMBARQUE INTROUVABLE");
    unsigned char magic[4];
    if(read(f,magic,4)!=4 || magic[0]!=0x7f || magic[1]!='E' || magic[2]!='L' || magic[3]!='F') {
        close(f); return fail("ELF EMBARQUE INVALIDE");
    }
    close(f);
    const int payload=open(path,0);
    if(payload<0) return fail("LECTURE ELF IMPOSSIBLE");
    notice("QUENTIN STORE PS5: ELF valide. Connexion au chargeur 9021");
    // FreeBSD/SCE sockaddr_in, including its length byte. Values are network byte order.
    struct Address { std::uint8_t length,family; std::uint16_t port; std::uint32_t host; std::uint8_t zero[8]; };
    Address address{16,2,static_cast<std::uint16_t>((port>>8)|((port&255)<<8)),0x0100007f,{}};
    (void)sceNetInit();
    const int socket=sceNetSocket("quentin-native",2,1,6);
    if(socket<0) { close(payload); return fail("CREATION SOCKET REFUSEE"); }
    const int timeout_us=10000000;
    if(sceNetSetsockopt(socket,0xffff,0x1105,&timeout_us,sizeof(timeout_us))<0 ||
       sceNetSetsockopt(socket,0xffff,0x1109,&timeout_us,sizeof(timeout_us))<0) {
        sceNetSocketClose(socket); close(payload); return fail("CONFIGURATION SOCKET REFUSEE");
    }
    if(sceNetConnect(socket,&address,sizeof(address))!=0) {
        sceNetSocketClose(socket); close(payload); return fail("CHARGEUR ELF 9021 INACCESSIBLE");
    }
    notice("QUENTIN STORE PS5: chargeur connecte. Transmission en cours");
    unsigned char buffer[32768];
    long received;
    while((received=read(payload,buffer,sizeof(buffer)))>0) {
        std::size_t offset=0;
        while(offset<static_cast<std::size_t>(received)) {
            const int sent=sceNetSend(socket,buffer+offset,static_cast<std::size_t>(received)-offset,0);
            if(sent<=0) { sceNetSocketClose(socket); close(payload); return fail("ENVOI ELF INTERROMPU"); }
            offset+=static_cast<std::size_t>(sent);
        }
    }
    close(payload);
    (void)sceNetShutdown(socket,1);
    (void)sceNetSocketClose(socket);
    if(received<0) return fail("LECTURE ELF INTERROMPUE");
    notice("QUENTIN STORE PS5: ELF transmis au chargeur 9021. Execution du store non confirmee.");
    // Leave VideoOut free for the payload application. On success do not create a framebuffer.
    (void)sceSystemServiceHideSplashScreen();
#ifdef QS_HOST_TEST
    return 0;
#else
    // Use the Shell-mediated exit used by the upstream native self-update sample.
    // Returning through the compatibility libc exit path is not supported here.
    (void)sceSystemServiceLoadExec("exit", nullptr);
    for (;;) (void)sceKernelUsleep(1000000);
#endif
}
