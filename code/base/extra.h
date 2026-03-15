static I64 read_all(I32 fd, U8* buffer, I64 size) {
    I64 to_read = size;
    while (to_read > 0) {
        I64 bytes_read = read(fd, buffer, to_read);
        if (bytes_read == -1) {
            return -1;
        }
        buffer  += bytes_read;
        to_read -= bytes_read;
    }
    return size;
}

static Bytes read_file(Buffer* console, const char* path) {
    Bytes path_bytes = make_bytes(path);

    I32 input_fd = open(path, O_RDONLY);
    if (input_fd == -1) {
        print(console, ERROR "Failed to open \"%s\": %s.\n", path_bytes, get_error());
        flush_and_exit(console, EXIT_FAILURE);
    }

    struct stat info = {};
    if (fstat(input_fd, &info) == -1) {
        print(console, ERROR "Failed to stat \"%s\": %s.\n", path_bytes, get_error());
        flush_and_exit(console, EXIT_FAILURE);
    }

    I64 size   = info.st_size;
    U8* memory = (U8*) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, input_fd, 0);
    if (memory == MAP_FAILED) {
        print(console, ERROR "Failed to memory map \"%s\": %s.\n", path_bytes, get_error());
        flush_and_exit(console, EXIT_FAILURE);
    }

    return (Bytes) { memory, size };
}

static void print_help(Buffer* console, int argc, char** argv, const char* help_message) {
    for (I64 i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print(console, help_message);
            flush_and_exit(console, EXIT_SUCCESS);
        }
    }
}

static Buffer open_output(Buffer* console, char* output_path) {
    I32 output_fd = STDOUT_FILENO;
    if (strcmp(output_path, "-") != 0) {
        output_fd = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (output_fd == -1) {
            print(console, ERROR "Failed to open \"%s\": %s.\n", make_bytes(output_path), get_error());
            flush_and_exit(console, EXIT_FAILURE);
        }
    }
    return make_buffer(output_fd, getpagesize());
}
