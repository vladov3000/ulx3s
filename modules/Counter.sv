module Counter #
    ( parameter int COUNT = 1
    )
    ( input  logic clock
    , output logic toggle
    );

    logic[$clog2(COUNT) - 1:0] count;

    logic done;
    assign done = count == COUNT - 1;

    always_ff @(posedge clock)
        count <= done ? 0 : count + 1;

    always_ff @(posedge clock)
        toggle <= done ? ~toggle : toggle;

endmodule
