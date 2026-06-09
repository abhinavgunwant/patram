#ifndef PATRAM_SYS_ARENA
#define PATRAM_SYS_ARENA

#include <cstdint>

struct Arena {
    uint8_t* pointer;
    uint32_t length;
    uint32_t capacity;
};

void initArena(Arena&, uint32_t capacity);
void destroyArena(Arena&);
void* arenaAllocate(Arena&, uint32_t byte_len);

#endif // PATRAM_SYS_ARENA

