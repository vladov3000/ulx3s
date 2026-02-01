module Ram #
    ( parameter int WIDTH       = 32
    , parameter int DEPTH       = 32
    , parameter     MEMORY_FILE = "default.mem"
    )
    ( input  logic                      clock
    , input  logic[$clog2(DEPTH) - 1:0] read_address
    , output logic[WIDTH - 1:0]         read_data
    , input  logic[$clog2(DEPTH) - 1:0] write_address
    , input  logic[WIDTH - 1:0]         write_data
    , input  logic[WIDTH / 8 - 1:0]     write_enable
    );

    logic[WIDTH - 1:0] memory[DEPTH - 1:0];

    initial $readmemh(MEMORY_FILE, memory);

    for (genvar i = 0; i < $bits(write_enable); i = i + 1)
        always_ff @(posedge clock)
            if (write_enable[i]) begin
                memory[write_address][8 * (i + 1) - 1:8 * i] <=
                    write_data[8 * (i + 1) - 1:8 * i];
            end

    always_ff @(posedge clock)
        read_data <= memory[read_address];

endmodule
