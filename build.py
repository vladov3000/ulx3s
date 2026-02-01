from argparse import ArgumentParser
import os
import subprocess

def main():
    parser = ArgumentParser()
    parser.add_argument("--assembler", action="store_true")
    parser.add_argument("--debug", action="store_true")
    parser.add_argument("--simulator", action="store_true")
    parser.add_argument("--synthesize", action="store_true")
    parser.add_argument("--program", action="store_true")

    arguments = parser.parse_args()

    try:
        os.mkdir("output")
    except FileExistsError:
        pass

    commands = []

    if arguments.simulator:
        commands.extend(
            ( ("yosys", "scripts/simulator.ys")
            , ( "clang++"
                , "-std=c++20"
                , "-I", "code"
                , "-o", "output/simulator"
                , "-g"
                , "code/simulator/main.cpp"
                )
            )
        )

    if arguments.assembler:
        commands.append(
            ( "clang"
            , "-Wall"
            , "-g"
            , "-o", "output/assembler"
            , "code/assembler/main.c"
            )
        )

    if arguments.synthesize or arguments.program:
        commands.extend(
            ( ("yosys", "scripts/synthesize.ys")
            , ( "nextpnr-ecp5"
                , "--85k"
                , "--json", "output/Top.json"
                , "--lpf", "ulx3s.lpf"
                , "--textcfg", "output/ulx3s.config"
                , "--package", "CABGA381"
                )
            , ("ecppack", "output/ulx3s.config", "output/ulx3s.bit")
            )
        )
        if arguments.program:
            commands.append(("fujprog", "output/ulx3s.bit"))

    for command in commands:
        print(" ".join(command))
        subprocess.run(command, check=True)

if __name__ == "__main__":
    main()
