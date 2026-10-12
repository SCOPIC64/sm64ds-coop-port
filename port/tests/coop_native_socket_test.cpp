#include "hal/comms_native_socket.h"
#include <array>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>

int main() {
    using namespace port::native_socket;
    std::array<SOCKET,7> sockets;
    std::array<sockaddr_in,7> endpoints={};
    for(int i=0;i<7;++i) {
        sockets[i]=make(AF_INET,SOCK_DGRAM,IPPROTO_UDP);assert(sockets[i]>=0);
        u_long enabled=1;assert(nonblocking(sockets[i],0,&enabled)==0);
        auto& endpoint=endpoints[i];endpoint.sin_family=AF_INET;endpoint.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
        assert(port::native_socket::bind(sockets[i],reinterpret_cast<sockaddr*>(&endpoint),sizeof endpoint)==0);
        socklen_t size=sizeof endpoint;assert(getsockname(sockets[i],reinterpret_cast<sockaddr*>(&endpoint),&size)==0);
    }
    char buffer[64];sockaddr_in from={};int from_size=sizeof from;
    assert(receive(sockets[0],buffer,sizeof buffer,0,reinterpret_cast<sockaddr*>(&from),&from_size)==-1);
    assert(errno==EAGAIN||errno==EWOULDBLOCK);
    // Seven independent native endpoints preserve every byte of a game block.
    for(int i=0;i<7;++i) {
        std::array<char,32> block;for(int b=0;b<32;++b)block[b]=static_cast<char>(i*32+b);
        assert(send(sockets[i],block.data(),32,0,reinterpret_cast<sockaddr*>(&endpoints[0]),sizeof(sockaddr_in))==32);
        int length=-1;unsigned deadline=GetTickCount()+1000;
        while(length<0&&static_cast<int>(deadline-GetTickCount())>0) {
            from_size=sizeof from;length=receive(sockets[0],buffer,sizeof buffer,0,reinterpret_cast<sockaddr*>(&from),&from_size);
            if(length<0)Sleep(1);
        }
        assert(length==32 && std::memcmp(buffer,block.data(),32)==0);
        assert(from.sin_port==endpoints[i].sin_port);
    }
    std::vector<char> oversized(1024,'x');
    assert(send(sockets[1],oversized.data(),1024,0,reinterpret_cast<sockaddr*>(&endpoints[0]),sizeof(sockaddr_in))==1024);
    unsigned deadline=GetTickCount()+1000;int length=-1;
    do {
        from_size=sizeof from;
        length=receive(sockets[0],buffer,sizeof buffer,0,reinterpret_cast<sockaddr*>(&from),&from_size);
        if(length<0&&(errno==EAGAIN||errno==EWOULDBLOCK))Sleep(1);else break;
    }while(static_cast<int>(deadline-GetTickCount())>0);
    assert(length==-1&&errno==EMSGSIZE);
    for(auto fd:sockets)assert(port::native_socket::close(fd)==0);
    std::puts("PASS: native UDP seven endpoints, exact blocks, nonblocking reads and oversized-packet rejection");
}
