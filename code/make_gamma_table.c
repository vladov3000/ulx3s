#include "base/prelude.h"
#include "base/buffer.h"
#include "base/extra.h"

static const char* help_message =
    "Usage: make_gamma_table [OUTPUT_PATH]\n"
    "\n"
    "       Monitors apply a transformation of color ^ 2.2.\n"
    "       This program generates a .mem table of an inverse of that operation:\n"
    "         value = 255 * (index / 255) ^ (1 / 2.2)\n"
    "       Will output to standard output if OUTPUT_PATH is missing.\n";

int main(int argc, char** argv) {
    Buffer console = make_console();

    print_help(&console, argc, argv, help_message);

    char*  output_path = argc < 2 ? "-" : argv[1];
    Buffer output      = open_output(&console, output_path);

    for (I64 i = 0; i < 256; i++) {
        F64 normalized = pow(i / 255., 1 / 2.2);
        I64 corrected  = round(255 * normalized);

        if (corrected < 10) {
            write_u8(&output, '0');
        }
        print(&output, "%x", corrected);
        U8 seperator = i % 16 == 15 ? '\n' : ' ';
        write_u8(&output, seperator);
    }

    flush(&output);
    flush(&console);
}
