typedef struct Arena Arena;

struct Arena {
    Buffer* console;
    U8*     memory;
    I64     size;
    I64     used;
};

static Arena make_arena(Buffer* console, I64 size) {
    I32 protection = PROT_READ   | PROT_WRITE;
    I32 flags      = MAP_PRIVATE | MAP_ANON;
    U8* memory     = (U8*) mmap(NULL, size, protection, flags, -1, 0);
    if (memory == MAP_FAILED) {
        print(console, ERROR "Out of memory.\n");
        flush_and_exit(console, EXIT_FAILURE);
    }
    return (Arena) { console, memory, size };
}

#define push(arena, type) ((type*) push_bytes((arena), sizeof(type)))

static U8* push_bytes(Arena* arena, I64 size) {
    if (arena->used + size > arena->size) {
        print(arena->console, ERROR "Out of memory.\n");
        flush_and_exit(arena->console, EXIT_FAILURE);
    }
    U8* output   = &arena->memory[arena->used];
    arena->used += size;
    return output;
}
