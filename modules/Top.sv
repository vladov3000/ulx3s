module Top
    ( input  logic      clk_25mhz
    , input  logic[6:0] btn
    , output logic      wifi_gpio0
    , output logic[7:0] led
    , output logic[3:0] gpdi_dp
    , output logic      gpdi_hpd
    );

    // Prevent board from rebooting.
    assign wifi_gpio0 = 1;

    logic clock_feedback;
    logic clock_250_MHz;
    logic clock_25_MHz;

    EHXPLLL #
        ( .CLKI_DIV(1)
        , .FEEDBK_PATH("CLKOP")
        // The VCO in the PLL must be kept between 400 - 800 MHz, so
        // we set its frequency to 25 MHz * 10 * 2 = 500 MHz.
        , .CLKFB_DIV(10)
        // 500 MHz / 2  = 250 Mhz
        , .CLKOP_ENABLE("ENABLED")
        , .CLKOP_DIV(2)
        // 500 MHz / 20 = 25 MHz
        , .CLKOS_ENABLE("ENABLED")
        , .CLKOS_DIV(20)
        ) pll
        ( .CLKI(clk_25mhz)
        , .CLKFB(clock_250_MHz)
        , .PHASESEL0(1'b0)
        , .PHASESEL1(1'b0)
        , .PHASEDIR(1'b0)
        , .PHASESTEP(1'b0)
        , .PHASELOADREG(1'b0)
        , .STDBY(1'b0)
        , .RST(1'b0)
        , .ENCLKOP(1'b1)
        , .ENCLKOS(1'b1)
        , .ENCLKOS2(1'b0)
        , .ENCLKOS3(1'b0)
        , .PLLWAKESYNC(1'b0)
        , .CLKOP(clock_250_MHz)
        , .CLKOS(clock_25_MHz)
        );

    logic[31:0] read_address;
    logic[31:0] read_data;
    logic[31:0] write_address;
    logic[31:0] write_data;
    logic[3:0]  write_enable;
    Memory memory
        ( .clock(clock_25_MHz)
        , .read_address
        , .read_data
        , .write_address
        , .write_data
        , .write_enable
        , .led
        );

    Cpu cpu
        ( .clock(clock_25_MHz)
        , .reset(btn[3]) // UP
        , .read_address
        , .read_data
        , .write_address
        , .write_data
        , .write_enable
        );

    logic[7:0] red;
    logic[7:0] green;
    logic[7:0] blue;
    logic[9:0] column;
    logic[9:0] row;
    Dvi dvi
        ( .clock_25_MHz
        , .clock_250_MHz
        , .red(gamma[red])
        , .green(gamma[green])
        , .blue(gamma[blue])
        , .column
        , .row
        , .gpdi_dp
        );

    logic[7:0] gamma[0:255];
    initial $readmemh("output/gamma_table.mem", gamma);

    localparam int COLUMNS  = 640;
    localparam int ROWS     = 480;
    localparam int BOX_SIZE = 256;
    localparam int BOX_X    = COLUMNS / 2 - BOX_SIZE / 2;
    localparam int BOX_Y    = ROWS    / 2 - BOX_SIZE / 2;
    localparam int BOX_X2   = BOX_X + BOX_SIZE;
    localparam int BOX_Y2   = BOX_Y + BOX_SIZE;

    logic in_box_x;
    assign in_box_x = BOX_X < column && column < BOX_X2;

    logic in_box_y;
    assign in_box_y = BOX_Y < row && row < BOX_Y2;

    logic in_box;
    assign in_box = in_box_x && in_box_y;

    assign red   = in_box ? (BOX_X2 - column) : 8'h11;
    assign green = in_box ? (BOX_Y2 - row   ) : 8'h11;
    assign blue  = in_box ? (row    - BOX_Y ) : 8'h11;

    assign gpdi_hpd = 1;

endmodule
