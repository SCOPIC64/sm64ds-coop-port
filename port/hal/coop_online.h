#pragma once
// Port-owned online lobby client. No endpoint or encryption key is compiled in.
// DLLs load after io_init, avoiding mappings inside the emulated DS address space.
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#include <wincrypt.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <fstream>
#include <stdexcept>
#include <climits>
#include <cstring>
#include <chrono>

namespace coop_online {
enum { MAX_PLAYERS = 7 };
struct Json {
    enum Kind { NIL, STRING, NUMBER, BOOL, OBJECT, ARRAY } kind = NIL;
    std::string text;
    int number = 0;
    std::map<std::string, Json> object;
    std::vector<Json> array;
    const Json& at(const char* key) const {
        auto i = object.find(key); static const Json empty;
        return i == object.end() ? empty : i->second;
    }
    std::string str() const { return kind == STRING ? text : ""; }
    int integer(int fallback = 0) const { return kind == NUMBER ? number : fallback; }
};
struct Parser {
    const std::string& s; size_t p = 0;
    explicit Parser(const std::string& data) : s(data) {}
    void ws() { while (p < s.size() && (s[p]==' ' || s[p]=='\r' || s[p]=='\n' || s[p]=='\t')) ++p; }
    char take() { if (p >= s.size()) throw std::runtime_error("Invalid server response"); return s[p++]; }
    std::string string() {
        if (take() != '"') throw std::runtime_error("Invalid server response");
        std::string out;
        for (;;) {
            char c=take(); if(c=='"') return out;
            if (static_cast<unsigned char>(c)<32) throw std::runtime_error("Invalid server response");
            if(c=='\\') {
                c=take();
                if(c=='u') {
                    unsigned v=0;
                    for(int n=0;n<4;++n) { char h=take(); int d=h>='0'&&h<='9'?h-'0':h>='a'&&h<='f'?h-'a'+10:h>='A'&&h<='F'?h-'A'+10:-1; if(d<0) throw std::runtime_error("Invalid server response"); v=v*16+d; }
                    c=v>=32&&v<=126?static_cast<char>(v):'?';
                } else if(c=='n') c='\n'; else if(c=='r') c='\r'; else if(c=='t') c='\t';
                else if(c=='b') c='\b'; else if(c=='f') c='\f';
                else if(c!='"' && c!='\\' && c!='/') throw std::runtime_error("Invalid server response");
            }
            out+=c;
        }
    }
    Json value(int depth=0) {
        if(depth>24) throw std::runtime_error("Invalid server response");
        ws(); Json j; char c=take();
        if(c=='"') { --p; j.kind=Json::STRING; j.text=string(); }
        else if(c=='{' || c=='[') {
            j.kind=c=='{'?Json::OBJECT:Json::ARRAY; ws();
            char end=c=='{'?'}':']'; if(p<s.size() && s[p]==end) { ++p; return j; }
            for(;;) {
                ws(); std::string key;
                if(c=='{') { key=string(); ws(); if(take()!=':') throw std::runtime_error("Invalid server response"); }
                Json child=value(depth+1);
                if(c=='{') { if(!j.object.emplace(key,child).second) throw std::runtime_error("Invalid server response"); }
                else j.array.push_back(child);
                ws(); char next=take(); if(next==end) break; if(next!=',') throw std::runtime_error("Invalid server response");
            }
        } else if(c=='-' || (c>='0'&&c<='9')) {
            bool negative=c=='-'; unsigned long long n=0;
            if(negative) c=take();
            if(c<'0'||c>'9') throw std::runtime_error("Invalid server response");
            n=c-'0'; while(p<s.size()&&s[p]>='0'&&s[p]<='9') { n=n*10+s[p++]-'0'; if(n>2147483648ULL) throw std::runtime_error("Invalid server response"); }
            if((!negative&&n>INT_MAX)||n>2147483648ULL) throw std::runtime_error("Invalid server response");
            j.kind=Json::NUMBER; j.number=negative?static_cast<int>(-static_cast<long long>(n)):static_cast<int>(n);
        } else {
            const char* word=c=='t'?"true":c=='f'?"false":c=='n'?"null":"";
            size_t n=std::strlen(word); if(!n || s.compare(p-1,n,word)) throw std::runtime_error("Invalid server response");
            p+=n-1; j.kind=c=='n'?Json::NIL:Json::BOOL; j.number=c=='t';
        }
        return j;
    }
    Json parse() { if(s.size()>65536) throw std::runtime_error("Server response too large"); Json j=value(); ws(); if(p!=s.size()) throw std::runtime_error("Invalid server response"); return j; }
};
inline std::string quote(const std::string& value) {
    std::string out="\""; for(unsigned char c:value) { if(c<' '||c>126) throw std::runtime_error("Use printable letters and numbers"); if(c=='"'||c=='\\') out+='\\'; out+=c; } return out+'"';
}
inline std::string read_file(const std::string& path) {
    std::ifstream file(path,std::ios::binary); if(!file) return "";
    std::string data; char block[2048];
    while(file) { file.read(block,sizeof block); data.append(block,static_cast<size_t>(file.gcount())); if(data.size()>16384) throw std::runtime_error("Server profile too large"); }
    return data;
}
struct Dpapi {
    HMODULE dll = LoadLibraryA("crypt32.dll");
    decltype(&CryptProtectData) protect = dll?reinterpret_cast<decltype(protect)>(GetProcAddress(dll,"CryptProtectData")):nullptr;
    decltype(&CryptUnprotectData) unprotect = dll?reinterpret_cast<decltype(unprotect)>(GetProcAddress(dll,"CryptUnprotectData")):nullptr;
    decltype(&CryptStringToBinaryA) decode = dll?reinterpret_cast<decltype(decode)>(GetProcAddress(dll,"CryptStringToBinaryA")):nullptr;
    decltype(&CryptBinaryToStringA) encode = dll?reinterpret_cast<decltype(encode)>(GetProcAddress(dll,"CryptBinaryToStringA")):nullptr;
    ~Dpapi() { if(dll) FreeLibrary(dll); }
    std::string decrypt(const std::string& text) {
        DWORD size=0; if(!unprotect||!decode||!decode(text.c_str(),static_cast<DWORD>(text.size()),CRYPT_STRING_BASE64,nullptr,&size,nullptr,nullptr)) throw std::runtime_error("Reinstall the server profile on this Windows account");
        std::vector<BYTE> bytes(size); if(!decode(text.c_str(),static_cast<DWORD>(text.size()),CRYPT_STRING_BASE64,bytes.data(),&size,nullptr,nullptr)) throw std::runtime_error("Invalid encrypted server profile");
        DATA_BLOB in={size,bytes.data()},out={};
        if(!unprotect(&in,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out)) throw std::runtime_error("Reinstall the server profile on this Windows account");
        std::string result(reinterpret_cast<char*>(out.pbData),out.cbData); SecureZeroMemory(out.pbData,out.cbData); LocalFree(out.pbData); return result;
    }
    std::string encrypt(const std::string& text) {
        DATA_BLOB in={static_cast<DWORD>(text.size()),reinterpret_cast<BYTE*>(const_cast<char*>(text.data()))},out={};
        if(!protect||!encode||!protect(&in,L"SM64DS relay profile",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out)) throw std::runtime_error("Windows could not protect the server profile");
        DWORD size=0; BOOL ok=encode(out.pbData,out.cbData,CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,nullptr,&size);
        std::string result(size,'\0'); if(ok) ok=encode(out.pbData,out.cbData,CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,&result[0],&size);
        LocalFree(out.pbData); if(!ok) throw std::runtime_error("Windows could not protect the server profile");
        while(!result.empty()&&result.back()==0) result.pop_back(); return result;
    }
};
struct Profile {
    std::string lobby, relay, path, discovery_key;
    static bool endpoint(const std::string& text) {
        size_t colon=text.find(':'); if(colon==std::string::npos||colon==0||colon>253||text.find(':',colon+1)!=std::string::npos) return false;
        for(size_t i=0;i<colon;++i) { char c=text[i]; if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='-')) return false; }
        if(colon+1==text.size()) return false;
        unsigned port=0; for(size_t i=colon+1;i<text.size();++i) { if(text[i]<'0'||text[i]>'9') return false; port=port*10+text[i]-'0'; if(port>65535) return false; } return port>0;
    }
    void parse(const std::string& text) {
        Json j=Parser(text).parse(); lobby=j.at("lobby_url").str(); relay=j.at("relay_addr").str(); discovery_key=j.at("discovery_key").str();
        if(!endpoint(relay)) throw std::runtime_error("Install a valid relay profile");
        if(!discovery_key.empty()) {
            if(discovery_key.size()!=368) throw std::runtime_error("Invalid server identity key");
            for(char c:discovery_key) if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='+'||c=='/'||c=='=')) throw std::runtime_error("Invalid server identity key");
            if(lobby.empty()) return;
        }
        while(!lobby.empty()&&lobby.back()=='/') lobby.pop_back();
        bool https=lobby.compare(0,8,"https://")==0;
        bool local=lobby.compare(0,17,"http://127.0.0.1:")==0;
        std::string host=https?lobby.substr(8):local?lobby.substr(7):"";
        if(host.find(':')==std::string::npos) host+=":443";
        if(!endpoint(host)||!endpoint(relay)||host.find('/')!=std::string::npos) throw std::runtime_error("Install a valid HTTPS relay profile");
    }
    void load() {
        const char* app=getenv("APPDATA"); if(!app||!*app) throw std::runtime_error("Windows AppData is unavailable");
        std::string dir=std::string(app)+"\\SM64DS-Coop"; path=dir+"\\server.txt";
        std::string saved=read_file(path); Dpapi key;
        if(!saved.empty()) { parse(key.decrypt(saved)); return; }
        char exe[MAX_PATH]; DWORD n=GetModuleFileNameA(nullptr,exe,MAX_PATH); if(!n||n==MAX_PATH) throw std::runtime_error("Server profile is not installed");
        std::string folder(exe,n); size_t slash=folder.find_last_of("\\/");
        std::string source=read_file(folder.substr(0,slash+1)+"server-profile.json");
        if(source.empty()) throw std::runtime_error("Server not set up yet - install the relay profile");
        parse(source);
        if(!CreateDirectoryA(dir.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS) throw std::runtime_error("Could not create AppData server folder");
        std::string encrypted=key.encrypt(source);
        std::ofstream file(path,std::ios::binary|std::ios::trunc); file<<encrypted<<'\n'; if(!file) throw std::runtime_error("Could not save the encrypted server profile");
    }
};
// Standard RSA/SHA-256 signatures authenticate changing HTTPS lobby addresses.
// Private keys remain in the server owner's Windows key store.
struct Identity {
    HMODULE dll=LoadLibraryA("advapi32.dll"); HCRYPTPROV provider=0; HCRYPTKEY key=0;
#define COOP_CRYPTO(name) decltype(&name) name##_ = dll?reinterpret_cast<decltype(&name)>(GetProcAddress(dll,#name)):nullptr
    COOP_CRYPTO(CryptAcquireContextA); COOP_CRYPTO(CryptReleaseContext); COOP_CRYPTO(CryptGenRandom);
    COOP_CRYPTO(CryptImportKey); COOP_CRYPTO(CryptDestroyKey); COOP_CRYPTO(CryptCreateHash);
    COOP_CRYPTO(CryptHashData); COOP_CRYPTO(CryptDestroyHash); COOP_CRYPTO(CryptVerifySignatureA);
#undef COOP_CRYPTO
    Identity() {
        if(!CryptAcquireContextA_||!CryptReleaseContext_||!CryptGenRandom_||!CryptImportKey_||!CryptDestroyKey_||!CryptCreateHash_||!CryptHashData_||!CryptDestroyHash_||!CryptVerifySignatureA_||!CryptAcquireContextA_(&provider,nullptr,nullptr,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)) throw std::runtime_error("Windows identity verification unavailable");
    }
    ~Identity() { if(key) CryptDestroyKey_(key); if(provider) CryptReleaseContext_(provider,0); if(dll) FreeLibrary(dll); }
    void random(BYTE* data,DWORD size) { if(!CryptGenRandom_(provider,size,data)) throw std::runtime_error("Windows random generator unavailable"); }
    bool verify(const std::string& encoded,const BYTE* data,DWORD size,const BYTE* signature) {
        Dpapi codec; DWORD n=0;
        if(!codec.decode||!codec.decode(encoded.c_str(),static_cast<DWORD>(encoded.size()),CRYPT_STRING_BASE64,nullptr,&n,nullptr,nullptr)||n!=276) return false;
        std::vector<BYTE> blob(n);
        if(!codec.decode(encoded.c_str(),static_cast<DWORD>(encoded.size()),CRYPT_STRING_BASE64,blob.data(),&n,nullptr,nullptr)||blob[0]!=PUBLICKEYBLOB||blob[1]!=CUR_BLOB_VERSION||std::memcmp(blob.data()+8,"RSA1",4)) return false;
        if(key) {CryptDestroyKey_(key);key=0;}
        if(!CryptImportKey_(provider,blob.data(),n,0,0,&key)) return false;
        HCRYPTHASH hash=0; if(!CryptCreateHash_(provider,CALG_SHA_256,0,0,&hash)) return false;
        bool ok=CryptHashData_(hash,data,size,0)&&CryptVerifySignatureA_(hash,signature,256,key,nullptr,0);
        CryptDestroyHash_(hash);return ok;
    }
};
struct Discovery {
    HMODULE dll=LoadLibraryA("ws2_32.dll"); bool started=false; SOCKET sock=INVALID_SOCKET;
#define COOP_SOCKET(name) decltype(&name) name##_ = dll?reinterpret_cast<decltype(&name)>(GetProcAddress(dll,#name)):nullptr
    COOP_SOCKET(WSAStartup); COOP_SOCKET(WSACleanup); COOP_SOCKET(socket); COOP_SOCKET(closesocket);
    COOP_SOCKET(getaddrinfo); COOP_SOCKET(freeaddrinfo); COOP_SOCKET(connect); COOP_SOCKET(setsockopt); COOP_SOCKET(send); COOP_SOCKET(recv);
#undef COOP_SOCKET
    ~Discovery() { if(sock!=INVALID_SOCKET) closesocket_(sock); if(started) WSACleanup_(); if(dll) FreeLibrary(dll); }
    void resolve(Profile& profile) {
        if(profile.discovery_key.empty()) return;
        if(!WSAStartup_||!WSACleanup_||!socket_||!closesocket_||!getaddrinfo_||!freeaddrinfo_||!connect_||!setsockopt_||!send_||!recv_) throw std::runtime_error("Windows relay discovery unavailable");
        WSADATA info={}; if(WSAStartup_(MAKEWORD(2,2),&info)) throw std::runtime_error("Windows relay discovery unavailable"); started=true;
        size_t colon=profile.relay.find(':'); std::string host=profile.relay.substr(0,colon),port=profile.relay.substr(colon+1);
        addrinfo hints={},*address=nullptr; hints.ai_family=AF_INET;hints.ai_socktype=SOCK_DGRAM;hints.ai_protocol=IPPROTO_UDP;
        if(getaddrinfo_(host.c_str(),port.c_str(),&hints,&address)||!address) throw std::runtime_error("Could not find the public game relay");
        sock=socket_(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
        bool connected=sock!=INVALID_SOCKET&&!connect_(sock,address->ai_addr,static_cast<int>(address->ai_addrlen));freeaddrinfo_(address);
        if(!connected) throw std::runtime_error("Could not reach the public game relay");
        DWORD timeout=1500; if(setsockopt_(sock,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<char*>(&timeout),sizeof timeout)) throw std::runtime_error("Could not configure relay discovery");
        Identity identity; BYTE request[20]={'S','M','D','C'};identity.random(request+4,16);
        for(int attempt=0;attempt<3;++attempt) {
            if(send_(sock,reinterpret_cast<char*>(request),sizeof request,0)!=sizeof request) continue;
            BYTE response[1024]; int n=recv_(sock,reinterpret_cast<char*>(response),sizeof response,0);
            if(n<=276||n>788||std::memcmp(response,"SMDS",4)||std::memcmp(response+4,request+4,16)) continue;
            DWORD body=static_cast<DWORD>(n-256);if(!identity.verify(profile.discovery_key,response,body,response+body)) continue;
            Json json=Parser(std::string(reinterpret_cast<char*>(response+20),body-20)).parse();
            Profile verified;verified.parse("{\"lobby_url\":"+quote(json.at("lobby_url").str())+",\"relay_addr\":"+quote(profile.relay)+"}");
            profile.lobby=verified.lobby;return;
        }
        throw std::runtime_error("Server offline - check WGS and both tunnels");
    }
};
struct Http {
    HMODULE dll=LoadLibraryA("winhttp.dll");
#define COOP_HTTP(name) decltype(&name) name##_ = dll?reinterpret_cast<decltype(&name)>(GetProcAddress(dll,#name)):nullptr
    COOP_HTTP(WinHttpOpen); COOP_HTTP(WinHttpCloseHandle); COOP_HTTP(WinHttpCrackUrl);
    COOP_HTTP(WinHttpConnect); COOP_HTTP(WinHttpOpenRequest); COOP_HTTP(WinHttpSetTimeouts);
    COOP_HTTP(WinHttpSetOption); COOP_HTTP(WinHttpSendRequest); COOP_HTTP(WinHttpReceiveResponse);
    COOP_HTTP(WinHttpQueryHeaders); COOP_HTTP(WinHttpReadData);
#undef COOP_HTTP
    ~Http() { if(dll) FreeLibrary(dll); }
    struct Handle { HINTERNET h; Http* api; ~Handle(){if(h) api->WinHttpCloseHandle_(h);} };
    Json post(const Profile& profile,const char* verb,const std::string& body) {
        if(!WinHttpOpen_||!WinHttpCloseHandle_||!WinHttpCrackUrl_||!WinHttpConnect_||!WinHttpOpenRequest_||!WinHttpSetTimeouts_||!WinHttpSetOption_||!WinHttpSendRequest_||!WinHttpReceiveResponse_||!WinHttpQueryHeaders_||!WinHttpReadData_) throw std::runtime_error("Windows networking unavailable");
        std::string url=profile.lobby+"/port/lobby/"+verb;
        std::wstring wide(url.begin(),url.end()); URL_COMPONENTS parts={}; parts.dwStructSize=sizeof parts; parts.dwHostNameLength=parts.dwUrlPathLength=static_cast<DWORD>(-1);
        if(!WinHttpCrackUrl_(wide.c_str(),0,0,&parts)) throw std::runtime_error("Invalid relay profile");
        std::wstring host(parts.lpszHostName,parts.dwHostNameLength),path(parts.lpszUrlPath,parts.dwUrlPathLength);
        Handle session={WinHttpOpen_(L"SM64DS-Coop/1",WINHTTP_ACCESS_TYPE_NO_PROXY,nullptr,nullptr,0),this};
        if(!session.h) throw std::runtime_error("Could not reach lobby server");
        WinHttpSetTimeouts_(session.h,2000,2000,2000,2000);
        Handle conn={WinHttpConnect_(session.h,host.c_str(),parts.nPort,0),this};
        if(!conn.h) throw std::runtime_error("Could not reach lobby server");
        Handle request={WinHttpOpenRequest_(conn.h,L"POST",path.c_str(),nullptr,nullptr,nullptr,parts.nScheme==INTERNET_SCHEME_HTTPS?WINHTTP_FLAG_SECURE:0),this};
        if(!request.h) throw std::runtime_error("Could not reach lobby server");
        DWORD redirect=WINHTTP_OPTION_REDIRECT_POLICY_NEVER; WinHttpSetOption_(request.h,WINHTTP_OPTION_REDIRECT_POLICY,&redirect,sizeof redirect);
        if(!WinHttpSendRequest_(request.h,L"Content-Type: application/json\r\n",static_cast<DWORD>(-1),const_cast<char*>(body.data()),static_cast<DWORD>(body.size()),static_cast<DWORD>(body.size()),0)||!WinHttpReceiveResponse_(request.h,nullptr)) throw std::runtime_error("Lobby unreachable - check connection");
        DWORD status=0,size=sizeof status;
        if(!WinHttpQueryHeaders_(request.h,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&status,&size,nullptr)) throw std::runtime_error("Invalid server response");
        std::string response; char block[4096]; DWORD got=0;
        for(;;) { if(!WinHttpReadData_(request.h,block,sizeof block,&got)) throw std::runtime_error("Lobby connection interrupted"); if(!got) break; response.append(block,got); if(response.size()>65536) throw std::runtime_error("Server response too large"); }
        Json j=Parser(response).parse();
        if(status!=200) { std::string error=j.at("error").str(); if(error=="room_full") error="Room is full (7 players)"; else if(error=="no_such_room") error="Room not found - check the room code"; else if(error=="not_enough_players") error="Wait for at least one other player"; else if(error=="in_match") error="That room has already started"; else if(error.empty()) error="Lobby request refused"; throw std::runtime_error(error); }
        return j;
    }
};
struct Plan {
    std::string role, code, relay, names;
    int players=0,slot=0;
    bool valid=false;
    static Plan from(const Json& j,const Profile& profile) {
        Plan p; p.role=j.at("role").str(); p.code=j.at("code").str(); p.relay=j.at("relay").str(); p.names=j.at("names").str(); p.players=j.at("players").integer(); p.slot=j.at("slot").integer(-1);
        if((p.role!="parent"&&p.role!="child")||p.players<2||p.players>MAX_PLAYERS||p.slot<0||p.slot>=p.players||(p.role=="parent")!=(p.slot==0)||p.relay!=profile.relay||p.code.size()!=8||j.at("session_type").str()!="party") throw std::runtime_error("Lobby sent an incompatible session");
        for(char c:p.code) if(!((c>='A'&&c<='Z')||(c>='0'&&c<='9'))) throw std::runtime_error("Lobby sent an invalid room key");
        if(p.names.size()>1024) throw std::runtime_error("Lobby sent invalid player names");
        p.valid=true; return p;
    }
};
struct Snapshot { std::string room,status,state; std::vector<std::string> names; bool host=false,busy=false; Plan plan; };
class Client {
    std::mutex mutex; std::condition_variable wake;
    bool stopping=false; int command=0; std::string input; Snapshot published;
    Profile profile; std::string room,token,match; bool host=false; int cursor=0;
    std::thread worker; // Start only after all shared state has initialized.
    void publish(Snapshot s) { std::lock_guard<std::mutex> lock(mutex); published=s; }
    std::string auth() const { return "\"v\":4,\"room\":"+quote(room)+",\"token\":"+quote(token); }
    Snapshot view(const Json& data) {
        Snapshot s; s.room=room; s.host=host; const Json& v=data.at("view"); s.state=v.at("state").str();
        cursor=data.at("cursor").integer(cursor); match=v.at("match").str();
        for(const Json& m:v.at("members").array) s.names.push_back(m.at("display").str());
        if(s.names.size()>MAX_PLAYERS) throw std::runtime_error("This server exceeds the seven-player limit");
        if(s.state=="closed") { room.clear();token.clear(); s.room.clear(); s.status="The host closed the room"; }
        if(data.at("plan").kind==Json::OBJECT) s.plan=Plan::from(data.at("plan"),profile);
        if(s.state=="arming") s.status="Preparing the shared game...";
        return s;
    }
    void loop() {
        Http http; bool loaded=false;
        for(;;) {
            int job; std::string value;
            { std::unique_lock<std::mutex> lock(mutex); wake.wait_for(lock,std::chrono::milliseconds(800),[this]{return stopping||command;}); if(stopping) break; job=command; value=input; command=0; }
            if(!job && room.empty()) continue;
            try {
                if(!loaded) { profile.load(); loaded=true; }
                Json result;
                if(job==1||job==2) {
                    if(!room.empty()) throw std::runtime_error("Leave the current room first");
                    Discovery discovery;discovery.resolve(profile);
                    host=job==1;
                    std::string body="{\"v\":4,\"nick\":"+quote(value.substr(value.find('|')+1))+",\"pre_ok\":true";
                    body+=host?",\"session_type\":\"party\"":",\"room\":"+quote(value.substr(0,value.find('|')));
                    result=http.post(profile,host?"create":"join",body+"}");
                    room=result.at("room").str();token=result.at("token").str();
                    if(room.size()!=6||token.size()!=32) throw std::runtime_error("Lobby sent an invalid room");
                    if(host) http.post(profile,"params","{"+auth()+",\"map\":1,\"win_mode\":\"time\",\"match_players\":7}");
                } else if(job==3) http.post(profile,"start","{"+auth()+"}");
                else if(job==4) { if(!room.empty()) {try{http.post(profile,"leave","{"+auth()+"}");}catch(...) {}} room.clear();token.clear();publish(Snapshot{});continue; }
                if(!room.empty()) {
                    result=http.post(profile,"poll","{"+auth()+",\"cursor\":"+std::to_string(cursor)+",\"wait\":0}");
                    Snapshot s=view(result);
                    if(s.state=="arming" && !match.empty()) http.post(profile,"ready","{"+auth()+",\"match\":"+quote(match)+"}");
                    publish(s);
                }
            } catch(const std::exception& error) {
                Snapshot s; {std::lock_guard<std::mutex> lock(mutex);s=published;}
                s.room=room;s.host=host;
                s.busy=false;s.status=error.what();publish(s);
                if(!room.empty()&&!profile.discovery_key.empty()) {try{Discovery discovery;discovery.resolve(profile);}catch(...) {}}
            }
        }
        if(loaded&&!room.empty()) { try { http.post(profile,"leave","{"+auth()+"}"); } catch(...) {} }
    }
public:
    Client() { worker=std::thread([this]{loop();}); }
    ~Client() { {std::lock_guard<std::mutex> lock(mutex);stopping=true;} wake.notify_all();if(worker.joinable()) worker.join(); }
    bool request(int job,const std::string& value="") { std::lock_guard<std::mutex> lock(mutex); if(command||published.busy) return false;command=job;input=value;published.busy=true;published.status="Connecting to lobby...";wake.notify_all();return true; }
    Snapshot snapshot() { std::lock_guard<std::mutex> lock(mutex);return published; }
};
} // namespace coop_online
