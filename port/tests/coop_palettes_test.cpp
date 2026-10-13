#include "hal/coop_palettes.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <cstdio>
int main(int argc,char** argv) {
    for(int c=0;c<4;++c){auto original=coop_palettes::stock(c),parsed=original;
        original[2]="13579b";original[0]="02468a";
        std::string ini=coop_palettes::write(original,c);assert(coop_palettes::read(ini,parsed)&&parsed==original);
        assert(ini.find("PANTS_R = 19")!=std::string::npos&&ini.find("CAP_R = 2")!=std::string::npos);
        auto invalid=ini;invalid.replace(invalid.find("PANTS_R = 19"),12,"PANTS_R = 256");assert(!coop_palettes::read(invalid,parsed));
        assert(!coop_palettes::read(ini+"CAP_R = 2\n",parsed));assert(!coop_palettes::read("[PALETTE]\nCAP_R = 1\n",parsed));}
    for(int n=1;n<argc;++n){std::ifstream f(argv[n],std::ios::binary);std::string ini((std::istreambuf_iterator<char>(f)),{});
        coop_palettes::Colors colors,again;assert(coop_palettes::read(ini,colors));assert(coop_palettes::read(coop_palettes::write(colors,0),again)&&colors==again);}
    puts("PASS: CoopDX INI imports, channel mapping, round trips and malformed preset rejection");
}
