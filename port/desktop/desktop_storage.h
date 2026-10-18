#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace sm64ds::desktop {
// Directory-descriptor based storage: entries cannot escape through a name or
// symlink. Atomic replacement preserves the previous save on a failed write.
class Storage {
    int directory_=-1;
public:
    ~Storage();
    Storage()=default;
    Storage(const Storage&)=delete;
    Storage& operator=(const Storage&)=delete;
    bool open(const std::string& existing_directory,std::string& error);
    bool read(const std::string& name,std::vector<uint8_t>& data,
              size_t limit,std::string& error) const;
    bool write(const std::string& name,const void* data,size_t size,
               std::string& error) const;
};
}
