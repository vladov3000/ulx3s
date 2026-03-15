// https://en.wikipedia.org/wiki/Transition-minimized_differential_signaling
module TmdsEncoder
    ( input  logic      clock
    , input  logic[7:0] data
    , input  logic[1:0] control
    , input  logic      blank
    , output logic[9:0] encoded
    );

    logic[8:0] xored;
    assign xored[0] = data[0];
    assign xored[1] = data[1] ^ xored[0];
    assign xored[2] = data[2] ^ xored[1];
    assign xored[3] = data[3] ^ xored[2];
    assign xored[4] = data[4] ^ xored[3];
    assign xored[5] = data[5] ^ xored[4];
    assign xored[6] = data[6] ^ xored[5];
    assign xored[7] = data[7] ^ xored[6];
    assign xored[8] = 1;

    logic[8:0] xnored;
    assign xnored[0] = data[0];
    assign xnored[1] = data[1] ~^ xnored[0];
    assign xnored[2] = data[2] ~^ xnored[1];
    assign xnored[3] = data[3] ~^ xnored[2];
    assign xnored[4] = data[4] ~^ xnored[3];
    assign xnored[5] = data[5] ~^ xnored[4];
    assign xnored[6] = data[6] ~^ xnored[5];
    assign xnored[7] = data[7] ~^ xnored[6];
    assign xnored[8] = 0;

    logic[3:0] ones;
    assign ones = $countones(data);

    logic xnored_transitions_less;
    assign xnored_transitions_less = ones > 4 || (ones == 4 && data[0] == 0);

    logic[8:0] chosen;
    assign chosen = xnored_transitions_less ? xnored : xored;

    logic[8:0] inverted;
    assign inverted = ~chosen;

    logic signed[3:0] chosen_ones;
    assign chosen_ones = $countones(chosen[7:0]);

    logic signed[3:0] disparity;
    assign disparity = chosen_ones - 4'h4;

    logic signed[3:0] bias;

    logic no_disparity;
    assign no_disparity = bias == 0 || disparity == 0;

    logic same_sign;
    assign same_sign = (bias < 0) == (disparity < 0);

    logic[9:0] encoded_data;
    always_comb
        if (no_disparity)
            encoded_data = chosen[8] ? { 2'b01, chosen[7:0]   }
                                     : { 2'b10, inverted[7:0] };
        else if (same_sign)
            encoded_data = { 1'b1, chosen[8], inverted[7:0] };
        else
            encoded_data = { 1'b0, chosen };

    logic[9:0] encoded_control;
    always_comb
        case (control)
            2'b00  : encoded_control = 10'b0010101011;
            2'b01  : encoded_control = 10'b0010101010;
            2'b10  : encoded_control = 10'b1101010100;
            default: encoded_control = 10'b1101010101;
        endcase

    assign encoded = blank ? encoded_control : encoded_data;

    always_ff @(posedge clock)
        if (blank)
            bias <= 0; // Control encodings are balanced.
        else if (no_disparity)
            bias <= bias + chosen[8] ? disparity : -disparity;
        else if (same_sign)
            bias <= bias + chosen[8] - disparity;
        else
            bias <= bias - inverted[8] + disparity;

endmodule
