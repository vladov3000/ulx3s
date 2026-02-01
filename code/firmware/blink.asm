# LEDs are controlled by the least significant byte of 0xFFFFFFFF.
main:
    addi  x2 x0 0xA
    lui   x1 0xFFFFF000
    sw    x1 x2 0xFFF
    jal   x0 0
