#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <unistd.h>

#define INFO  "\x1b[32;1mInfo \x1b[0m "
#define WARN  "\x1b[33;1mWarn \x1b[0m "
#define ERROR "\x1b[31;1mError\x1b[0m "

#define length(array) sizeof(array) / sizeof((array)[0])

typedef uint8_t  U8;
typedef uint32_t U32;
typedef int32_t  I32;
typedef int64_t  I64;

typedef struct iovec Iovec;

typedef struct {
    U8* data;
    I64 size;    
} Bytes;

#define make_bytes(input) \
    ((Bytes) { (U8*) input, (I64) strlen(input) })

static bool starts_with(Bytes base, char* prefix) {
    I64 prefix_size = strlen(prefix);
    return base.size >= prefix_size && memcmp(base.data, prefix, prefix_size) == 0;
}

static bool ends_with(Bytes base, char* suffix) {
    I64 suffix_size = strlen(suffix);
    return base.size >= suffix_size && memcmp(base.data + base.size - suffix_size, suffix, suffix_size) == 0;
}

static bool bytes_equal(Bytes a, Bytes b) {
    return a.size == b.size && memcmp(a.data, b.data, a.size) == 0;
}

static Bytes take(Bytes input, I64 count) {
    if (count < 0) {
        count += input.size;
    }
    input.size = count;
    return input;
}

static Bytes drop(Bytes input, I64 count) {
    input.data += count;
    input.size -= count;
    return input;
}

static bool is_space(U8 c) {
    return c == ' ' || c == '\n';
}

static bool is_digit(U8 c) {
    return '0' <= c && c <= '9';
}

static bool get_digit(U8 c, I64 base, I64* digit) {
    I64 digit_storage = 0;
    if (digit == NULL) {
        digit = &digit_storage;
    }

    bool ok = true;
    if (is_digit(c)) {
        *digit = c - '0';
    } else if ('a' <= c && c <= 'f' + base - 1) {
        *digit = c - 'a' + 10;
    } else if ('A' <= c && c <= 'A' + base - 1) {
        *digit = c - 'A' + 10;
    } else {
        ok = false;
    }
    return ok;
}

static Iovec make_iovec(const char* input) {
    return (Iovec) { (U8*) input, strlen(input) };
}

static Bytes get_error() {
    return make_bytes(strerror(errno));
}

static Bytes i64_to_string(I64 input, I64 base, U8 storage[20]) {
    U8*  end      = &storage[20];
    U8*  start    = end;
    bool negative = input < 0;
    input         = negative ? -input : input;

    do {
        I64 digit = input % base;
        start     = start - 1;
        *start    = digit < 10 ? digit + '0' : digit + 'A' - 10;
        input     = input / base;
    } while (input > 0);

    if (negative) {
        start  = start - 1;
        *start = '-';
    }

    return (Bytes) { start, end - start };
}

static Bytes left_pad(Bytes input, U8 byte, I64 output_size) {
    I64 padding = output_size - input.size;
    if (padding > 0) {
        input.data -= padding;
        input.size += padding;
        memset(input.data, byte, padding);
    }
    return input;
}

static Bytes prepend(Bytes input, const char* prefix) {
    Bytes prefix_bytes = make_bytes(prefix);
    memcpy((char*) &input.data[-prefix_bytes.size], prefix_bytes.data, prefix_bytes.size);
    input.data -= prefix_bytes.size;
    input.size += prefix_bytes.size;
    return input;
}

static Bytes append(Bytes input, char* suffix) {
    Bytes suffix_bytes = make_bytes(suffix);
    memcpy((char*) &input.data[input.size], (char*) suffix_bytes.data, suffix_bytes.size);
    input.size += suffix_bytes.size;   
    return input;
}

static I64 test_bit(I64 input, I64 index) {
    return (input >> index) & 1;
}

static I64 slice_bits(I64 input, I64 start, I64 end) {
    I64 width = end - start + 1;
    I64 mask  = (1 << width) - 1;
    return (input >> start) & mask;
}
