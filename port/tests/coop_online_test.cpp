#define WIN32_LEAN_AND_MEAN
#include "hal/coop_session.h"
#include <cassert>
#include <memory>
#include <cstdio>
using namespace coop_online;
int main(int argc,char**argv) {
    // Persisted data is protected by the current user's Windows key, not a
    // constant embedded in this source or the release executable.
    std::string plain="{\"lobby_url\":\"https://relay.example.com\",\"relay_addr\":\"relay.example.com:41234\"}";
    Dpapi key; std::string encoded=key.encrypt(plain);
    assert(encoded.find("relay.example.com")==std::string::npos);
    assert(key.decrypt(encoded)==plain);
    bool rejected=false;
    try { encoded[encoded.size()/2]=encoded[encoded.size()/2]=='A'?'B':'A';key.decrypt(encoded); } catch(...) {rejected=true;}
    assert(rejected);
    Profile p;p.parse(plain);
    for(const char* bad:{"{}","{\"lobby_url\":\"http://relay.example.com\",\"relay_addr\":\"relay.example.com:41234\"}","{\"lobby_url\":\"https://relay.example.com/path\",\"relay_addr\":\"relay.example.com:41234\"}"}) {
        rejected=false;try{Profile t;t.parse(bad);}catch(...){rejected=true;}assert(rejected);
    }
    Json j=Parser("{\"players\":7,\"role\":\"parent\",\"slot\":0,\"session_type\":\"party\",\"code\":\"ABCDEFGH\",\"relay\":\"relay.example.com:41234\"}").parse();
    const std::string shared=encode_save(fresh_save());
    j.object["adventure_save"].kind=Json::STRING;j.object["adventure_save"].text=shared;
    assert(Plan::from(j,p).valid);
    const bool export_startup=argc==3&&std::string(argv[1])=="--export-startup";
    if(export_startup) {
        j.object["players"].number=2;Plan plan=Plan::from(j,p);
        std::ofstream envs(argv[2]);envs<<"{";
        for(int slot=0;slot<2;++slot) {
            plan.role=slot?"child":"parent";plan.slot=slot;
            if(slot) envs<<",";
            envs<<quote(plan.role)<<":{";bool first=true;
            for(const auto& field:room_environment(plan,0,slot?2:1,123+slot)) {
                if(!first)envs<<",";first=false;
                envs<<quote(field.first)<<":"<<quote(field.second);
            }
            envs<<"}";
        }
        envs<<"}";assert(envs.good());
    }
    for(int count:{2,7}) {
        j.object["players"].number=count;
        Plan plan=Plan::from(j,p);
        auto host=room_environment(plan,2,1,123);
        plan.role="child";plan.slot=1;
        auto child=room_environment(plan,0,2,124);
        for(const char* key:{"SM64DS_BOOT_FILE","SM64DS_LEVEL","SM64DS_NETMODE","SM64DS_SHARED_SAVE"}) assert(host.at(key)==child.at(key));
        assert(host.at("SM64DS_LEVEL")=="1"&&host.at("SM64DS_BOOT_FILE").empty());
        assert(host.at("SM64DS_NETMODE")=="lockstep");
        assert(host.at("SM64DS_SHARED_SAVE_SLOT")=="2"&&child.at("SM64DS_SHARED_SAVE_SLOT")=="0");
        assert(host.at("SM64DS_CHARACTER")=="1"&&child.at("SM64DS_CHARACTER")=="2");
        assert(child.at("SM64DS_SAVE_PATH")!=room_environment(plan,0,2,125).at("SM64DS_SAVE_PATH"));
    }
    j.object["adventure_save"].text=std::string(136,'f');rejected=false;try{Plan::from(j,p);}catch(...){rejected=true;}assert(rejected);
    j.object["adventure_save"].text=shared;
    auto payload=fresh_save();payload[0x14]=255;payload[0x41]=1;
    std::vector<unsigned char> chip(8192,255);
    for(int off:{128,4224}) {
        unsigned crc=coop_frontend::save_checksum(payload.data());chip[off]=crc&255;chip[off+1]=crc>>8;
        std::memcpy(chip.data()+off+2,"ds mario",8);std::memcpy(chip.data()+off+10,payload.data(),payload.size());
    }
    const std::string save_test="room-save-test-"+std::to_string(GetCurrentProcessId())+".sav";
    std::ofstream(save_test,std::ios::binary).write(reinterpret_cast<const char*>(chip.data()),chip.size());
    assert(room_save(save_test.c_str(),1)==encode_save(payload));
    assert(room_save(save_test.c_str(),0)==shared);
    chip[128]^=1;std::ofstream(save_test,std::ios::binary).write(reinterpret_cast<const char*>(chip.data()),chip.size());
    assert(room_save(save_test.c_str(),1)==encode_save(payload));
    chip[4224]^=1;std::ofstream(save_test,std::ios::binary).write(reinterpret_cast<const char*>(chip.data()),chip.size());
    rejected=false;try{room_save(save_test.c_str(),1);}catch(...){rejected=true;}assert(rejected);std::remove(save_test.c_str());
    j.object["players"].number=8;rejected=false;try{Plan::from(j,p);}catch(...){rejected=true;}assert(rejected);
    j.object["players"].number=7;j.object["relay"].text="other.example.com:41234";rejected=false;try{Plan::from(j,p);}catch(...){rejected=true;}assert(rejected);
    for(const char* bad:{"{\"a\":1,\"a\":2}","[1,]","{\"x\":1}junk","[99999999999999999]"}) { rejected=false;try{Parser(bad).parse();}catch(...){rejected=true;}assert(rejected); }
    if(argc==4||argc==5) { // Cross-language signatures and authenticated UDP discovery.
        Profile discovery;discovery.parse(read_file(argv[1]));
        if(argc==5) {Profile installed;installed.load();assert(installed.relay==discovery.relay&&installed.discovery_key==discovery.discovery_key);discovery=installed;}
        std::string packet=read_file(argv[2]);assert(packet.size()>276);
        Identity identity;DWORD body=static_cast<DWORD>(packet.size()-256);
        assert(identity.verify(discovery.discovery_key,reinterpret_cast<const BYTE*>(packet.data()),body,reinterpret_cast<const BYTE*>(packet.data()+body)));
        packet[20]^=1;
        assert(!identity.verify(discovery.discovery_key,reinterpret_cast<const BYTE*>(packet.data()),body,reinterpret_cast<const BYTE*>(packet.data()+body)));
        Discovery network;network.resolve(discovery);assert(discovery.lobby==argv[3]);
    }
    if(!export_startup&&(argc==2||argc==3)) { // Full real HTTP client lifecycle.
        assert(getenv("APPDATA"));
        std::string dir=std::string(getenv("APPDATA"))+"\\SM64DS-Coop";
        CreateDirectoryA(getenv("APPDATA"),nullptr);CreateDirectoryA(dir.c_str(),nullptr);
        std::string local=argc==3?read_file(argv[1]):"{\"lobby_url\":\"http://127.0.0.1:"+std::string(argv[1])+"\",\"relay_addr\":\"127.0.0.1:51990\"}";
        std::ofstream(dir+"\\server.txt")<<key.encrypt(local);
        Profile cached;cached.load();Profile expected;expected.parse(local);assert(cached.relay==expected.relay);
        std::vector<std::unique_ptr<Client>> clients;
        auto wait=[](Client& c,bool room,bool plan) {
            DWORD start=GetTickCount();Snapshot s;
            do { s=c.snapshot(); if(plan?s.plan.valid:room?!s.room.empty():!s.busy&&s.room.empty()) return s; Sleep(50); }while(GetTickCount()-start<30000);
            fprintf(stderr,"Client timeout: %s\n",s.status.c_str());assert(false);return s;
        };
        clients.emplace_back(new Client);clients[0]->request(1,"|Luigi",shared);Snapshot s=wait(*clients[0],true,false);
        std::string room=s.room;
        for(int n=1;n<7;++n) {clients.emplace_back(new Client);clients.back()->request(2,room+"|Player"+std::to_string(n));wait(*clients.back(),true,false);}
        Client extra;extra.request(2,room+"|Extra");
        DWORD start=GetTickCount();do {s=extra.snapshot();if(!s.busy&&!s.status.empty())break;Sleep(50);}while(GetTickCount()-start<15000);
        assert(s.room.empty()&&s.status.find("full")!=std::string::npos);
        clients[0]->request(3);std::string code;
        for(int n=0;n<7;++n) {s=wait(*clients[n],true,true);assert(s.plan.players==7&&s.plan.slot==n);if(!n)code=s.plan.code;assert(s.plan.code==code&&s.plan.save_data==shared);}
        clients.clear();
    }
    puts("PASS: native online client checks");
}
