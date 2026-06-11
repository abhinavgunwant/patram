#include "include/arena.hpp"

#if defined(__X86_64__) || defined(_M_X64)
    #define _AMD64_
#elif defined(__aarch64__) || defined(_M_ARM64)
    // TODO: define something here...
#endif

#ifdef _WIN32
    #include <heapapi.h>
#endif

ArenaBlock* initArenaBlock(ArenaBlock* rootBlock, uint32_t capacity, uint8_t index, bool firstInit) {
    size_t reserve = firstInit ? sizeof(ArenaBlock) * 4 : 0;
    uint8_t* reservePointer = nullptr;

    #ifdef _WIN32
        reservePointer = (uint8_t*) HeapAlloc(
            GetProcessHeap(),
            HEAP_ZERO_MEMORY,
            capacity + reserve
        );
    #else
        reservePointer = new uint8_t[capacity + reserve];
    #endif

    if (reservePointer == nullptr) { return nullptr; }

    ArenaBlock* block;

    if (firstInit) {
        block = (ArenaBlock*) reservePointer;
    } else {
        block = rootBlock + sizeof(ArenaBlock) * index;
    }

    block->capacity = capacity;
    block->length = 0;
    block->next = nullptr;
    block->pointer = reservePointer + reserve;

    return block;
}

void destroyArenaBlock(Arena& arena, ArenaBlock* block) {
    if (block == nullptr || block->pointer == nullptr) { return; }

    destroyArenaBlock(arena, block->next);

    bool isRoot = (block == arena.rootBlock);

    uint8_t* ptr = isRoot ? (uint8_t*) block : block->pointer;

    #ifdef _WIN32
        HeapFree(GetProcessHeap(), 0, ptr);
    #else
        delete [] arena.pointer;
    #endif

    if (isRoot) {
        arena.rootBlock = nullptr;
        return;
    }

    block->pointer = nullptr;
    block->length = 0;
    block->capacity = 0;
    block->next = nullptr;
}

void* blockAllocate(ArenaBlock* rootBlock, ArenaBlock* block, uint32_t byteLen, uint8_t index) {
    if (block != nullptr && block->length + byteLen <= block->capacity) {
        void* pointer = block->pointer + block->length;

        block->length += byteLen;

        return pointer;
    }

    if (index < 3) {
        if (block->next == nullptr) {
            block->next = initArenaBlock(rootBlock, block->capacity, index + 1, false);
        }

        return blockAllocate(rootBlock, block->next, byteLen, index + 1);
    }

    return nullptr;
}

void initArena(Arena& arena, uint32_t capacity) {
    arena.rootBlock = initArenaBlock(nullptr, capacity, 0, true);
}

void destroyArena(Arena& arena) {
    destroyArenaBlock(arena, arena.rootBlock);
}

void* arenaAllocate(Arena& arena, uint32_t byteLen) {
    return blockAllocate(arena.rootBlock, arena.rootBlock, byteLen, 0);
}

