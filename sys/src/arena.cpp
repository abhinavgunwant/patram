#include "include/arena.hpp"

#if defined(__X86_64__) || defined(_M_X64)
    #define _AMD64_
#elif defined(__aarch64__) || defined(_M_ARM64)
    // TODO: define something here...
#endif

#ifdef _WIN32
    #include <heapapi.h>
#endif

void initArena(Arena& arena, uint32_t capacity) {
    arena.capacity = capacity;
    arena.length = 0;
    arena.pointer = nullptr;

    #ifdef _WIN32
        arena.pointer = (uint8_t*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, capacity);
    #else
        arena.pointer = new uint8_t[capacity];
    #endif
}

void destroyArena(Arena& arena) {
    if (arena.pointer == nullptr) { return; }

    #ifdef _WIN32
        HeapFree(GetProcessHeap(), 0, arena.pointer);
    #else
        delete [] arena.pointer;
    #endif

    arena.pointer = nullptr;
    arena.length = 0;
    arena.capacity = 0;
}

void* arenaAllocate(Arena& arena, uint32_t byte_len) {
    if (arena.length + byte_len <= arena.capacity) {
        void* pointer = arena.pointer + arena.length;

        arena.length += byte_len;

        return pointer;
    }

    return nullptr;
}

