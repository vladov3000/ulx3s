addi  x1 x0 1
slti  x2 x0 1
sltiu x3 x0 1
andi  x4 x1 0
ori   x5 x1 0
xori  x6 x1 0
slli  x7 x1 1
srli  x8 x1 1
srai  x9 x1 1
lui   x10 0xFFFFF000
auipc x11 0xFFFFF000
add   x12 x0 x1
slt   x13 x0 x1
sltu  x14 x0 x1
and   x15 x1 x0
or    x16 x1 x0
xor   x17 x1 x0
sll   x18 x1 x1
srl   x19 x1 x1
sra   x20 x1 x1
sub   x21 x1 x1
sra   x22 x1 x1
jal   x23 8
addi  x23 x0 0
auipc x24 0
addi  x24 x24 12
jalr  x24 x24 8
addi  x24 x0 0
beq   x0 x1 8
addi  x25 x0 1
bne   x0 x1 8
addi  x26 x0 1
blt   x0 x1 8
addi  x27 x0 1
bltu  x0 x1 8
addi  x28 x0 1
bge   x0 x1 8
addi  x29 x0 1
bgeu  x0 x1 8
addi  x30 x0 1
sw    x0 x1 0
lw    x31 x0 0
jal   x0 0
