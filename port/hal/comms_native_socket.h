#pragma once
// POSIX adapter for the existing carrier's dynamically loaded socket table.
// Keeps datagram boundaries, nonblocking behavior and truncation rejection
// identical to Winsock. No Nintendo state or platform credentials live here.
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <thread>
#include <climits>

using SOCKET=int;
using WORD=uint16_t;
using DWORD=uint32_t;
using LPVOID=void*;
using LPDWORD=DWORD*;
struct WSADATA {};
using LPWSADATA=WSADATA*;
using LPWSAOVERLAPPED=void*;
using LPWSAOVERLAPPED_COMPLETION_ROUTINE=void*;
#define WSAAPI
#define INVALID_SOCKET (-1)
#define MAKEWORD(a,b) (static_cast<WORD>((a)|((b)<<8)))
#define SIO_UDP_CONNRESET 0

inline unsigned GetTickCount() {
    return static_cast<unsigned>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
inline void Sleep(unsigned milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}
namespace port { namespace native_socket {
inline int startup(WORD,LPWSADATA){return 0;}
inline int make(int domain,int type,int protocol){return ::socket(domain,type,protocol);}
inline int nonblocking(SOCKET fd,long,u_long* enabled) {
    int flags=::fcntl(fd,F_GETFL,0);if(flags<0)return -1;
    return ::fcntl(fd,F_SETFL,*enabled?flags|O_NONBLOCK:flags&~O_NONBLOCK);
}
inline int udp_reset(SOCKET,DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD written,
                     LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE) {
    if(written){*written=0;}return 0; // POSIX has no Winsock UDP-reset ioctl.
}
inline int bind(SOCKET fd,const sockaddr* address,int size) {
    if(size<0){errno=EINVAL;return -1;}return ::bind(fd,address,static_cast<socklen_t>(size));
}
inline int send(SOCKET fd,const char* bytes,int size,int flags,const sockaddr* address,int address_size) {
    if(size<0||address_size<0){errno=EINVAL;return -1;}
    return static_cast<int>(::sendto(fd,bytes,static_cast<size_t>(size),flags,address,static_cast<socklen_t>(address_size)));
}
inline int receive(SOCKET fd,char* bytes,int size,int flags,sockaddr* address,int* address_size) {
    if(size<0||!address_size||*address_size<0){errno=EINVAL;return -1;}
    iovec buffer={bytes,static_cast<size_t>(size)};msghdr message={};
    message.msg_name=address;message.msg_namelen=static_cast<socklen_t>(*address_size);
    message.msg_iov=&buffer;message.msg_iovlen=1;
    ssize_t length=::recvmsg(fd,&message,flags);
    if(length<0)return -1;
    *address_size=static_cast<int>(message.msg_namelen);
    // recvfrom normally silently truncates on POSIX; Winsock rejects that
    // packet. Preserve rejection so a malformed packet cannot become valid.
    if((message.msg_flags&MSG_TRUNC)||length>size||length>INT_MAX){errno=EMSGSIZE;return -1;}
    return static_cast<int>(length);
}
inline int close(SOCKET fd){return ::close(fd);}
inline int error(){return errno;}
inline unsigned long address(const char* text){return ::inet_addr(text);}
inline hostent* resolve(const char* text){return ::gethostbyname(text);}
template<class Table> void fill(Table& api) {
    api.WSAStartup=startup;api.socket=make;api.ioctlsocket=nonblocking;
    api.WSAIoctl=udp_reset;api.bind=bind;api.sendto=send;api.recvfrom=receive;
    api.closesocket=close;api.WSAGetLastError=error;api.inet_addr=address;api.gethostbyname=resolve;
}
} }
