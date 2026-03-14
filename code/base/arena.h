typedef struct Arena Arena;

struct Arena {
    Buffer* console;
    U8*     memory;
    I64     size;
    I64     used;
};

static U8* os_allocate(Buffer* console, I64 size) {
    U8* memory = try_os_allocate(size);
    if (memory == MAP_FAILED) {
        print(console, ERROR "Out of memory.\n");
        flush_and_exit(console, EXIT_FAILURE);
    }
    return memory;
}

static Arena make_arena(Buffer* console, I64 size) {
    return (Arena) { console, os_allocate(console, size), size };
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
