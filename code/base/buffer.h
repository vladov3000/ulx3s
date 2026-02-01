typedef struct {
    I32 fd;
    I64 buffered;
    U8* memory;
    I64 size;
} Buffer;

static Buffer make_buffer(I32 fd, I64 size) {
    Buffer buffer = {};

    I32 protection = PROT_READ   | PROT_WRITE;
    I32 flags      = MAP_PRIVATE | MAP_ANON;
    U8* memory     =  mmap(NULL, size, protection, flags, -1, 0);

    if (memory == MAP_FAILED) {
        // We implementing printing in terms of `Buffer`, so we have
        // to directly write to standard output here.
        Iovec messages[] = {
            make_iovec(ERROR "Failed to allocate memory: "),
            make_iovec(strerror(errno)),
            make_iovec("\n"),
        };
        writev(STDOUT_FILENO, messages, length(messages));
        exit(EXIT_FAILURE);
    }

    buffer.fd     = fd;
    buffer.memory = memory;
    buffer.size   = size;
    return buffer;
}

static bool flush(Buffer* buffer) {
    if (buffer->buffered > 0) {
        ssize_t bytes_written = write(buffer->fd, buffer->memory, buffer->buffered);
        buffer->buffered      = 0;
        if (bytes_written == -1) {
            return false;
        }
    }
    return true;
}

static void flush_and_exit(Buffer* buffer, I32 exit_code) {
    flush(buffer);
    exit(exit_code);
}

static bool write_u8(Buffer* buffer, U8 input) {
    if (buffer->buffered == buffer->size) {
        return flush(buffer);
    }
    buffer->memory[buffer->buffered] = input;
    buffer->buffered++;
    return true;
}

static bool write_bytes(Buffer* buffer, Bytes input) {
    if (buffer->buffered + input.size > buffer->size) {
        return flush(buffer);
    }
    memcpy(&buffer->memory[buffer->buffered], input.data, input.size);
    buffer->buffered += input.size;
    return true;
}

static void print_i64(Buffer* buffer, I64 base, I64 input) {
    U8 storage[20] = {};
    write_bytes(buffer, i64_to_string(input, base, storage));
}

static void printv(Buffer* buffer, const char* format, va_list arguments) {
    while (true) {
        U8 c = format[0];
        if (c == 0) {
            break;
        }

        if (c == '%' && format[1] == 'c') {
            write_u8(buffer, va_arg(arguments, I32));
            format += 2;
        } else if (c == '%' && format[1] == 's') {
            write_bytes(buffer, va_arg(arguments, Bytes));
            format += 2;
        } else if (c == '%' && format[1] == 'i') {
            print_i64(buffer, 10, va_arg(arguments, I64));
            format += 2;
        } else if (c == '%' && format[1] == 'x') {
            print_i64(buffer, 16, va_arg(arguments, I64));
            format += 2;
        } else {
            write_u8(buffer, *format);
            format++;
        }
    }
}

static void print(Buffer* buffer, const char* format, ...) {
    va_list arguments;
    va_start(arguments, format);
    printv(buffer, format, arguments);
    va_end(arguments);
}
