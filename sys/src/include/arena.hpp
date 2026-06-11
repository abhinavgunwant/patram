#ifndef PATRAM_SYS_ARENA
#define PATRAM_SYS_ARENA

#include <cstdint>

struct ArenaBlock {
    uint32_t length;
    uint32_t capacity;
    uint8_t* pointer;
    ArenaBlock* next;
};

struct Arena {
    ArenaBlock* rootBlock;
};

void initArena(Arena&, uint32_t capacity);
void destroyArena(Arena&);
void* arenaAllocate(Arena&, uint32_t byte_len);

#endif // PATRAM_SYS_ARENA

