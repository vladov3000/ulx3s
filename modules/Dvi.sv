// https://en.wikipedia.org/wiki/Digital_Visual_Interface
module Dvi #
    // VGA Parameters for 25 MHz clock and a 640 x 480 resolution.
    ( parameter int COLUMNS                = 800
    , parameter int ROWS                   = 525
    , parameter int ACTIVE_COLUMNS         = 640
    , parameter int ACTIVE_ROWS            = 480
    , parameter int HORIZONTAL_FRONT_PORCH = 16
    , parameter int HORIZONTAL_BACK_PORCH  = 48
    , parameter int VERTICAL_FRONT_PORCH   = 10
    , parameter int VERTICAL_BACK_PORCH    = 33
    )
    ( input  logic                        clock_25_MHz
    , input  logic                        clock_250_MHz
    , input  logic[7:0]                   red
    , input  logic[7:0]                   green
    , input  logic[7:0]                   blue
    , output logic[$clog2(COLUMNS) - 1:0] column
    , output logic[$clog2(ROWS)    - 1:0] row
    , output logic[3:0]                   gpdi_dp
    );

    logic last_column;
    assign last_column = column == COLUMNS - 1;

    always_ff @(posedge clock_25_MHz)
        if (last_column) column <= 0;
        else             column <= column + 1;

    always_ff @(posedge clock_25_MHz)
        if (last_column) row <= row == ROWS - 1 ? 0 : row + 1;
        else             row <= row;

    logic left, right, top, bottom;
    assign left   = column >= ACTIVE_COLUMNS + HORIZONTAL_FRONT_PORCH;
    assign right  = column <  COLUMNS - HORIZONTAL_BACK_PORCH;
    assign top    = row    >= ACTIVE_ROWS + VERTICAL_FRONT_PORCH;
    assign bottom = row    <  ROWS - VERTICAL_BACK_PORCH;

    logic[1:0] control;
    assign control =
        { left & right  // hsync
        , top  & bottom // vsync
        };

    logic blank;
    assign blank = column >= ACTIVE_COLUMNS || row >= ACTIVE_ROWS;

    logic[9:0] encoded_red;
    TmdsEncoder red_tmds_encoder
        ( .clock(clock_25_MHz)
        , .data(red)
        , .control(2'b0)
        , .blank
        , .encoded(encoded_red)
        );

    logic[9:0] encoded_green;
    TmdsEncoder green_tmds_encoder
        ( .clock(clock_25_MHz)
        , .data(green)
        , .control(2'b0)
        , .blank
        , .encoded(encoded_green)
        );

    logic[9:0] encoded_blue;
    TmdsEncoder blue_tmds_encoder
        ( .clock(clock_25_MHz)
        , .data(blue)
        , .control
        , .blank
        , .encoded(encoded_blue)
        );

    logic[9:0] encoded_red_buffer;
    always_ff @(posedge clock_25_MHz) encoded_red_buffer <= encoded_red;

    logic[9:0] encoded_green_buffer;
    always_ff @(posedge clock_25_MHz) encoded_green_buffer <= encoded_green;

    logic[9:0] encoded_blue_buffer;
    always_ff @(posedge clock_25_MHz) encoded_blue_buffer <= encoded_blue;

    logic sync_25_MHz;
    always_ff @(posedge clock_25_MHz)
        sync_25_MHz <= ~sync_25_MHz;

    logic sync_250_MHz;
    always_ff @(posedge clock_250_MHz)
        sync_250_MHz <= sync_25_MHz;

    logic sync_pulsed;
    assign sync_pulsed = sync_250_MHz != sync_25_MHz;

    logic[3:0] tmds_clock;

    logic tmds_clock_done;
    assign tmds_clock_done = tmds_clock == 9;

    always_ff @(posedge clock_250_MHz)
        if (tmds_clock_done) tmds_clock <= 0;
        else                 tmds_clock <= tmds_clock + 1;

    logic[9:0] red_buffer;
    always_ff @(posedge clock_250_MHz)
        if (tmds_clock_done) red_buffer <= encoded_red_buffer;
        else                 red_buffer <= { 1'b0, red_buffer[9:1] };

    logic[9:0] green_buffer;
    always_ff @(posedge clock_250_MHz)
        if (tmds_clock_done) green_buffer <= encoded_green_buffer;
        else                 green_buffer <= { 1'b0, green_buffer[9:1] };

    logic[9:0] blue_buffer;
    always_ff @(posedge clock_250_MHz)
        if (tmds_clock_done) blue_buffer <= encoded_blue_buffer;
        else                 blue_buffer <= { 1'b0, blue_buffer[9:1] };

    always_ff @(posedge clock_250_MHz) gpdi_dp[0] <= blue_buffer [0];
    always_ff @(posedge clock_250_MHz) gpdi_dp[1] <= green_buffer[0];
    always_ff @(posedge clock_250_MHz) gpdi_dp[2] <= red_buffer  [0];
    always_ff @(posedge clock_250_MHz) gpdi_dp[3] <= tmds_clock[3];
    // ^ Toggles every 5 clock cycles.

endmodule
