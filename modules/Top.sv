module Top
    ( input  logic      clk_25mhz
    , input  logic[6:0] btn
    , output logic      wifi_gpio0
    , output logic[7:0] led
    );

    logic[31:0] read_address;
    logic[31:0] read_data;
    logic[31:0] write_address;
    logic[31:0] write_data;
    logic[3:0]  write_enable;
    Memory memory
        ( .clock(clk_25mhz)
        , .read_address
        , .read_data
        , .write_address
        , .write_data
        , .write_enable
        , .led
        );

    Cpu cpu
        ( .clock(clk_25mhz)
        , .reset(btn[3]) // UP
        , .read_address
        , .read_data
        , .write_address
        , .write_data
        , .write_enable
        );

    assign wifi_gpio0 = 1;

endmodule
