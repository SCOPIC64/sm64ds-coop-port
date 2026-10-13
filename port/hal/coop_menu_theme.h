#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>
#include <array>

namespace coop_frontend {
struct MenuImage {
    std::vector<uint32_t> pixels;
    int width=0, height=0;
};
struct MenuTextures {
    MenuImage panel, button, selected, font;
    int border=3;
    std::array<int,95> advances{};
    // The editable font PNG is a 16-column grid of 18x18 cells, ASCII 32..126.
    int advance(unsigned ch, int size) const {
        if(ch>='a'&&ch<='z') ch-=32;
        if(ch<32||ch>126)ch='?';
        if(ch==' ')return 5*size;
        return advances[ch-32]*size;
    }
    void prepare_font() {
        for(int cell=0;cell<95;++cell) {
            int right=0;
            for(int y=0;y<18;++y)for(int x=0;x<18;++x)
                if(font.pixels[(cell/16*18+y)*font.width+cell%16*18+x]>>24)right=(std::max)(right,x+1);
            advances[cell]=(std::max)(3,(right*10+17)/18+1);
        }
    }
};
}
