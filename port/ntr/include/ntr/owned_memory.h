#pragma once
#include <cstddef>
#include <cstdint>
namespace ntr {
// Require a DS-owned reservation before publishing data at guest addresses.
bool io_owns_range(uintptr_t address, size_t size);
}
