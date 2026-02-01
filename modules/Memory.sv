module Memory
    ( input  logic       clock
    , input  logic[31:0] read_address
    , output logic[31:0] read_data
    , input  logic[31:0] write_address
    , input  logic[31:0] write_data
    , input  logic[3:0]  write_enable
    , output logic[7:0]  led
    );

    logic is_led_write;
    assign is_led_write = write_address == 32'hFFFFFFFF;

    Ram #
        ( .WIDTH(32)
        , .DEPTH(18 * 1024 / 32)
        , .MEMORY_FILE("default.mem")
        ) memory
        ( .clock
        , .read_address(read_address[9:0])
        , .read_data(read_data)
        , .write_address(write_address[9:0])
        , .write_data(write_data)
        , .write_enable(is_led_write ? 4'h0 : write_enable)
        );

    always_ff @(posedge clock)
        led <= is_led_write && write_enable[0] ? write_data : 4'hA;

endmodule
