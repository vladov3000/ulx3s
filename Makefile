Top.json: Top.ys Top.sv
	yosys Top.ys

ulx3s.config: Top.json
	nextpnr-ecp5 --85k --json Top.json --lpf ulx3s.lpf --textcfg ulx3s.config --package CABGA381

ul3xs.bit: ulx3s.config
	ecppack ulx3s.config ulx3s.bit

program: ul3xs.bit
	fujprog ulx3s.bit

clean:
	rm -f Top.json ulx3s.config ulx3s.bit

.PHONY: clean program
