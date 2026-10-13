// Optional menu textures are decoded once; gameplay never performs file IO for them.
static const coop_frontend::MenuTextures* coop_menu_textures() {
    const auto* definition=sm64ds::packs::menu_theme();
    if(!definition)return nullptr;
    static std::string key;static bool ready=false;
    static coop_frontend::MenuTextures theme;
    std::string next=definition->panel+'\n'+definition->button+'\n'+definition->selected+'\n'+definition->font;
    if(key==next)return ready?&theme:nullptr;
    key=next;ready=false;theme={};theme.border=definition->border;
    auto load=[](const std::string& path,coop_frontend::MenuImage& image) {
        FILE* file=fopen(path.c_str(),"rb");if(!file)return false;
        if(fseek(file,0,SEEK_END)){fclose(file);return false;}
        long count=ftell(file);if(count<24||count>8*1024*1024){fclose(file);return false;}
        rewind(file);std::vector<unsigned char> data(static_cast<size_t>(count));
        bool read=fread(data.data(),1,data.size(),file)==data.size();fclose(file);
        const unsigned char png[]={137,80,78,71,13,10,26,10};
        if(!read||memcmp(data.data(),png,8)||memcmp(data.data()+12,"IHDR",4))return false;
        auto dimension=[&](int offset){return (unsigned(data[offset])<<24)|(unsigned(data[offset+1])<<16)|(unsigned(data[offset+2])<<8)|data[offset+3];};
        if(!dimension(16)||!dimension(20)||dimension(16)>2048||dimension(20)>2048)return false;
        return coop_media::image_memory(data.data(),static_cast<int>(data.size()),image.pixels,image.width,image.height);
    };
    if(!load(definition->panel,theme.panel)||!load(definition->button,theme.button)||
       !load(definition->selected,theme.selected)||!load(definition->font,theme.font)||
       theme.font.width!=288||theme.font.height!=108) {
        fprintf(stderr,"[menu-theme] %s: invalid PNG/font grid; using stock menu\n",definition->pack_id.c_str());
        theme={};return nullptr;
    }
    for(const auto* image:{&theme.panel,&theme.button,&theme.selected})
        if(image->width<=theme.border*2||image->height<=theme.border*2) {
            fprintf(stderr,"[menu-theme] %s: border exceeds image; using stock menu\n",definition->pack_id.c_str());
            theme={};return nullptr;
        }
    theme.prepare_font();ready=true;
    fprintf(stderr,"[menu-theme] loaded %s (external menu textures/font)\n",definition->pack_id.c_str());
    return &theme;
}
