module Top
    ( output logic[7:0] led
    , output logic      wifi_gpio0
    );

    assign led        = 8'hF0;
    assign wifi_gpio0 = 1'b1;

endmodule
