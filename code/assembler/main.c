#include "../base/prelude.h"
#include "../base/buffer.h"
#include "../base/arena.h"
#include "../base/extra.h"
#include "lexer.h"
#include "assembler.h"

typedef enum {
    FORMAT_BINARY,
    FORMAT_ARRAY,
    FORMAT_HEX,
} Format;

static const char* help_message =
    "Usage: assembler [--array] [--help] [--hex] INPUT_PATH OUTPUT_PATH\n"
    "\n"
    "       Assembles the code at INPUT_PATH into flat machine code at OUTPUT_PATH.\n"
    "       An OUTPUT_PATH of \"-\" is standard output.\n"
    "\n"
    "       --array Output a c array.\n"
    "       --help  Prints this message.\n"
    "       --hex   Output hex instead of flat machine code.\n";

static void handle_write_failure(Buffer* console, char* output_path) {
    print(console, ERROR "Failed to write to \"%s\": %s.\n", make_bytes(output_path), get_error());
    flush_and_exit(console, EXIT_FAILURE);
}

int main(int argc, char** argv) {
    Buffer console = make_buffer(STDOUT_FILENO, getpagesize());

    print_help(&console, argc, argv, help_message);

    I64    argument_index = 1;
    Format output_format  = FORMAT_BINARY;
    while (argument_index < argc - 2) {
        char* argument = argv[argument_index];
        if (strcmp(argument, "--array") == 0) {
            output_format = FORMAT_ARRAY;
        } else if (strcmp(argument, "--hex") == 0) {
            output_format = FORMAT_HEX;    
        } else {
            print(&console, ERROR "Invalid option \"%s\".\n", make_bytes(argument));
            flush_and_exit(&console, EXIT_FAILURE);
        }
        argument_index++;
    }

    if (argument_index > argc - 2) {
        print(&console, ERROR "Missing INPUT_PATH and OUTPUT_PATH.\n");
        flush_and_exit(&console, EXIT_FAILURE);
    }

    char* input_path  = argv[argument_index];
    char* output_path = argv[argument_index + 1];

    Bytes input = read_file(&console, input_path);
    
    I32 output_fd = STDOUT_FILENO;
    if (strcmp(output_path, "-") != 0) {
        output_fd = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (output_fd == -1) {
            print(&console, ERROR "Failed to open \"%s\": %s.\n", make_bytes(output_path), get_error());
            flush_and_exit(&console, EXIT_FAILURE);
        }
    }

    Bytes path          = make_bytes(input_path);
    Lexer lexer         = make_lexer(&console, path, input);
    Arena symbols_arena = make_arena(&console, getpagesize());
    I64   pc            = 0;

    while (lex(&lexer)) {
        if (ends_with(lexer.lexeme_bytes, ":")) {
            Symbol* symbol = push(&symbols_arena, Symbol);
            symbol->name   = take(lexer.lexeme_bytes, -1);
            symbol->offset = pc;
        } else {
            next_instruction(&lexer);
            pc += 4;
        }
    }

    Symbols symbols = (Symbols) {
        (Symbol*) symbols_arena.memory,
        symbols_arena.used / sizeof(Symbol)
    };
    Buffer  output  = make_buffer(output_fd, getpagesize());

    if (output_format == FORMAT_ARRAY) {
        if (!write_bytes(&output, make_bytes("static const U32 instructions[] = {\n"))) {
            handle_write_failure(&console, output_path);
        }
    }

    lexer = make_lexer(&console, path, input);
    while (lex(&lexer)) {
        if (ends_with(lexer.lexeme_bytes, ":")) {
            continue;
        }

        I64 instruction = next_instruction(&lexer);

        // 20 bytes necessary for i64_to_string. Extra 2 bytes for comma and newline.
        U8    storage[22] = {};
        Bytes to_write    = {};
        switch (output_format) {

        case FORMAT_ARRAY:
            to_write = i64_to_string(instruction, 16, storage);
            to_write = left_pad(to_write, '0', 8);
            to_write = prepend(to_write, "    0x");
            to_write = append(to_write, ",\n");
            break;

        case FORMAT_BINARY:
            memcpy(storage, &instruction, 4);
            to_write = (Bytes) { storage, 4 };
            break;

        case FORMAT_HEX:
            to_write = i64_to_string(instruction, 16, storage);
            to_write = left_pad(to_write, '0', 8);
            to_write = append(to_write, "\n");
            break;

        }

        if (!write_bytes(&output, to_write)) {
            handle_write_failure(&console, output_path);
        }
    }

    if (output_format == FORMAT_ARRAY) {
        if (!write_bytes(&output, make_bytes("};\n"))) {
            handle_write_failure(&console, output_path);
        }
    }

    if (!flush(&output)) {
        handle_write_failure(&console, output_path);
    }

    flush(&console);
}
