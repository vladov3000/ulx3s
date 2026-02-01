#include <cxxrtl/cxxrtl.h>
#include <cxxrtl/cxxrtl_vcd.h>
#include <fstream>

#include "../base/prelude.h"
#include "../base/buffer.h"
#include "../base/extra.h"
#include "../../output/Cpu.hpp"

using cxxrtl_design::p_Cpu;

static const char* help_message =
    "Usage: simulator FIRMWARE_PATH\n"
    "\n"
    "       Runs the machine code at FIRMWARE_PATH and prints the\n"
    "       CPU state after 1000 cycles.\n"
    "\n"
    "       --help   Prints this message.\n";

int main(int argc, char** argv) {
    Buffer console = make_buffer(STDOUT_FILENO, getpagesize());

    print_help(&console, argc, argv, help_message);

    if (argc < 2) {
        print(&console, ERROR "Missing FIRMWARE_PATH.\n");
        flush_and_exit(&console, EXIT_FAILURE);
    }

    char* firmware_path = argv[1];
    Bytes memory        = read_file(&console, firmware_path);

    p_Cpu cpu;

    cxxrtl::debug_items all_debug_items;
    cpu.debug_info(&all_debug_items, NULL, "");

    cxxrtl::vcd_writer vcd;
    vcd.timescale(1, "us");
    vcd.add_without_memories(all_debug_items);

    std::ofstream waves("output/waves.vcd");

    value<1>& clock = cpu.p_clock;
    for (I64 cycle = 0; cycle < 1000; cycle++) {
        cpu.p_reset.set(cycle == 0);

        U32  read_address = cpu.p_read__address.get<U32>() % memory.size;
        U32  read_data    = *(U32*) &memory.data[read_address];
        cpu.p_read__data  = value<32>(read_data);

        U32 write_address = cpu.p_write__address.get<U32>() % memory.size;
        U32 write_data    = cpu.p_write__data.get<U32>();
        U32 write_enable  = cpu.p_write__enable.get<U32>();
        for (I64 i = 0; i < 4; i++) {
            if (test_bit(write_enable, i)) {
                memory.data[write_address + i] = write_data >> (8 * i);
            }
        }

        clock.set(true);
        cpu.step();

        vcd.sample(2 * cycle);

        clock.set(false);
        cpu.p_reset.set(false);
        cpu.step();

        vcd.sample(2 * cycle + 1);

        waves << vcd.buffer;
        vcd.buffer.clear();
    }

    for (I64 i = 0; i < 32; i++) {
        U8    storage[20] = {};
        Bytes i_bytes     = i64_to_string(i, 10, storage);
        i_bytes           = prepend(i_bytes, "x");
        i_bytes           = left_pad(i_bytes, ' ', 4);
        I64   value       = cpu.memory_p_registers[i].get<U32>();
        print(&console, "%s = 0x%x\n", i_bytes, value);
    }

    flush(&console);
}
