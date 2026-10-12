#pragma once
#include "coop_online.h"
#include "coop_save_preview.h"
#include <array>
#include <cerrno>

namespace coop_online {
using SavePayload = std::array<unsigned char, 0x44>;
inline SavePayload fresh_save() {
    SavePayload data{};
    std::memcpy(data.data(), "8000", 4);
    data[8] = 8; data[0x41] = 3;
    return data;
}
inline SavePayload decode_save(const std::string& hex) {
    if (hex.size() != 0x44 * 2) throw std::runtime_error("Invalid shared save");
    SavePayload data{};
    auto digit=[](char c) { return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1; };
    for (size_t n=0;n<data.size();++n) {
        int a=digit(hex[2*n]), b=digit(hex[2*n+1]);
        if(a<0||b<0) throw std::runtime_error("Invalid shared save");
        data[n]=static_cast<unsigned char>(16*a+b);
    }
    if(std::memcmp(data.data(),"8000",4)||data[0x41]>3||data[0x42]>2)
        throw std::runtime_error("Invalid shared save");
    return data;
}
inline std::string encode_save(const SavePayload& data) {
    const char* digits="0123456789abcdef"; std::string hex;
    for(unsigned char b:data) { hex+=digits[b>>4]; hex+=digits[b&15]; }
    return hex;
}
inline std::string room_save(const char* path,int slot) {
    if(slot<0||slot>2) throw std::runtime_error("Invalid save slot");
    SavePayload data=fresh_save();
    FILE* f=std::fopen(path,"rb");
    if(!f) { if(errno==ENOENT) return encode_save(data); throw std::runtime_error("Could not read the selected save"); }
    unsigned char chip[8193]; size_t size=std::fread(chip,1,sizeof chip,f); std::fclose(f);
    auto preview=coop_frontend::preview_save(chip,size,slot);
    if(preview.damaged) throw std::runtime_error("The selected save is damaged");
    if(preview.exists) for(int copy=0;copy<2;++copy) {
        const unsigned char* record=chip+copy*4096+slot*128;
        if(!std::memcmp(record+2,"ds mario",8)&&
           (record[0]|(record[1]<<8))==coop_frontend::save_checksum(record+10)) {
            std::memcpy(data.data(),record+10,data.size()); break;
        }
    }
    return encode_save(decode_save(encode_save(data)));
}
inline std::map<std::string,std::string> room_environment(const Plan& plan,int save_slot,int character,unsigned pid) {
    if(!plan.valid||save_slot<0||save_slot>2||character<0||character>3)
        throw std::runtime_error("Invalid online startup plan");
    decode_save(plan.save_data);
    const bool host=plan.role=="parent";
    std::map<std::string,std::string> env={
        {"SM64DS_BOOT_FILE",""},{"SM64DS_SKIP_MENU","1"},
        {"SM64DS_LEVEL","1"},{"SM64DS_COMMS_ROLE",plan.role},
        {"SM64DS_COMMS_PORT","51765"},{"SM64DS_COMMS_RELAY",plan.transport_relay.empty()?plan.relay:plan.transport_relay},
        {"SM64DS_COMMS_CODE",plan.code},{"SM64DS_VS_PLAYERS",std::to_string(plan.players)},
        {"SM64DS_VS_NAMES",plan.names},{"SM64DS_COMMS_FANOUT","1"},
        {"SM64DS_NETMODE","lockstep"},{"SM64DS_PARTY","1"},{"SM64DS_SYNC","1"},
        {"SM64DS_COMMS_WAIT_MS","300000"},{"SM64DS_COMMS_TIMEOUT_MS","15000"},
        {"SM64DS_CHARACTER",std::to_string(character)},
        {"SM64DS_SHARED_SAVE",plan.save_data},
        {"SM64DS_SHARED_SAVE_SLOT",std::to_string(host?save_slot:0)}
    };
    if(plan.transport_relay=="127.0.0.1:41234") {
        // Keep adaptive sizing enabled: an internet friend still needs enough
        // shared delay for their measured path. Local-only rooms need no 5-frame floor.
        env["SM64DS_COMMS_DELAY_FLOOR"]="1";
    }
    env["SM64DS_PLAYER_PALETTES"]=plan.palettes;
    env["SM64DS_PLAYER_CHARACTERS"]=plan.characters;
    if(host) env["SM64DS_COMMS_BIND_ANY"]="1";
    else {
        env["SM64DS_COMMS_SLOT"]=std::to_string(plan.slot);
        env["SM64DS_SAVE_PATH"]="save/coop-join-"+std::to_string(pid)+".sav";
    }
    return env;
}
}
