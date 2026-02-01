module Cpu
    ( input  logic       clock
    , input  logic       reset
    , output logic[31:0] read_address
    , input  logic[31:0] read_data
    , output logic[31:0] write_address
    , output logic[31:0] write_data
    , output logic[3:0]  write_enable
    );

    logic[31:0] registers[31:0];
    logic[31:0] pc;

    logic[31:0] instruction;
    logic       loading;
    always_ff @(posedge clock)
        if (is_load && !loading) instruction <= instruction;
        else                     instruction <= read_data;

    typedef enum logic[6:0]
        { OPCODE_IMM    = 7'b0010011
        , OPCODE_LUI    = 7'b0110111
        , OPCODE_AUIPC  = 7'b0010111
        , OPCODE_OP     = 7'b0110011
        , OPCODE_JAL    = 7'b1101111
        , OPCODE_JALR   = 7'b1100111
        , OPCODE_BRANCH = 7'b1100011
        , OPCODE_LOAD   = 7'b0000011
        , OPCODE_STORE  = 7'b0100011
        // TODO Add remaining opcodes.
        } Opcode;

    // TODO Handle invalid opcodes.
    Opcode opcode;
    assign opcode = instruction[6:0];

    logic[4:0] rd;
    assign rd = instruction[11:7];

    logic[2:0] funct3;
    assign funct3 = instruction[14:12];

    logic[4:0] rs1;
    assign rs1 = instruction[19:15];

    logic[4:0] rs2;
    assign rs2 = instruction[24:20];

    logic[6:0] funct7;
    assign funct7 = instruction[31:25];

    logic[31:0] immediate_i;
    assign immediate_i = { { 21 { instruction[31] } }, instruction[31:20] };

    logic[31:0] immediate_s;
    assign immediate_s =
        { { 21 { instruction[31] } }
        , instruction[30:25]
        , instruction[11:7]
        };

    logic[31:0] immediate_b;
    assign immediate_b =
        { { 20 { instruction[31] } }
        , instruction[7]
        , instruction[30:25]
        , instruction[11:8]
        , 1'b0
        };

    logic[31:0] immediate_u;
    assign immediate_u = { instruction[31:12], 12'b0 };

    logic[31:0] immediate_j;
    assign immediate_j =
        { { 10 { instruction[31] } }
        , instruction[19:12]
        , instruction[20]
        , instruction[30:21]
        , 1'b0
        };

    // TODO Validate the rest of the bits are set to 0 for OPCODE_IMM
    //      instructions.
    logic[4:0] shift_amount;
    assign shift_amount = instruction[24:20];

    logic arithmetic_shift;
    assign arithmetic_shift = instruction[30];

    logic subtract;
    assign subtract = instruction[30];

    logic is_jal;
    assign is_jal = opcode == OPCODE_JAL;

    logic is_jalr;
    assign is_jalr = opcode == OPCODE_JALR;

    logic is_load;
    assign is_load = opcode == OPCODE_LOAD;

    logic is_store;
    assign is_store = opcode == OPCODE_STORE;

    always_ff @(posedge clock)
        loading <= is_load;

    logic[31:0] rs1_value;
    assign rs1_value = registers[rs1];

    logic[31:0] rs2_value;
    assign rs2_value = registers[rs2];

    logic[31:0] operand0;
    always_comb
        case (opcode)
            OPCODE_IMM   ,
            OPCODE_OP    ,
            OPCODE_LUI   ,
            OPCODE_JALR  ,
            OPCODE_BRANCH,
            OPCODE_LOAD  ,
            OPCODE_STORE : operand0 = rs1_value;
            OPCODE_AUIPC : operand0 = pc;
            default      : operand0 = 0;
        endcase

    logic[31:0] operand1;
    always_comb
        case (opcode)
            OPCODE_IMM   ,
            OPCODE_JALR  ,
            OPCODE_LOAD  : operand1 = immediate_i;
            OPCODE_LUI   ,
            OPCODE_AUIPC : operand1 = immediate_u;
            OPCODE_OP    : operand1 = subtract ? -rs2_value : rs2_value;
            OPCODE_BRANCH: operand1 = rs2_value;
            OPCODE_STORE : operand1 = immediate_s;
            default      : operand1 = 0;
        endcase

    typedef enum logic[3:0]
        { OPERATION_ADD
        , OPERATION_SIGNED_LESS_THAN
        , OPERATION_LESS_THAN
        , OPERATION_AND
        , OPERATION_OR
        , OPERATION_XOR
        , OPERATION_SHIFT_LEFT
        , OPERATION_SHIFT_RIGHT
        , OPERATION_SIGNED_SHIFT_RIGHT
        , OPERATION_EQUAL
        , OPERATION_GREATER_EQUAL
        , OPERATION_SIGNED_GREATER_EQUAL
        } Operation;

    Operation operation;
    always_comb
        case (opcode)
            OPCODE_IMM,
            OPCODE_OP :
                case (funct3)
                    3'b000 : operation = OPERATION_ADD;
                    3'b001 : operation = OPERATION_SHIFT_LEFT;
                    3'b010 : operation = OPERATION_SIGNED_LESS_THAN;
                    3'b011 : operation = OPERATION_LESS_THAN;
                    3'b100 : operation = OPERATION_XOR;
                    3'b101 : operation = arithmetic_shift
                        ? OPERATION_SIGNED_SHIFT_RIGHT
                        : OPERATION_SHIFT_RIGHT;
                    3'b110 : operation = OPERATION_OR;
                    3'b111 : operation = OPERATION_AND;
                    default: operation = OPERATION_ADD;
                endcase
            OPCODE_BRANCH:
                case (funct3)
                    3'b000 : operation = OPERATION_EQUAL;
                    3'b001 : operation = OPERATION_XOR;
                    // TODO Handle the case when funct3 = 3'b010.
                    3'b100 : operation = OPERATION_LESS_THAN;
                    3'b101 : operation = OPERATION_GREATER_EQUAL;
                    3'b110 : operation = OPERATION_SIGNED_LESS_THAN;
                    3'b111 : operation = OPERATION_SIGNED_GREATER_EQUAL;
                    default: operation = 0;
                endcase
            // TODO Handle unknown opcodes.
            default: operation = OPERATION_ADD;
        endcase

    logic unsigned_load;
    assign unsigned_load = funct3[2];

    logic[3:0] memory_write_enable;
    always_comb
        case (funct3[1:0])
            2'b00  : memory_write_enable = 4'b0001;
            2'b01  : memory_write_enable = 4'b0011;
            2'b10  : memory_write_enable = 4'b1111;
            default: memory_write_enable = 4'b0000;
        endcase

    assign write_enable = is_store ? memory_write_enable : 4'h0;

    logic[31:0] operation_output;
    always_comb
        case (operation)
            OPERATION_ADD:
                operation_output = operand0 + operand1;
            OPERATION_SIGNED_LESS_THAN:
                operation_output = $signed(operand0) < $signed(operand1);
            OPERATION_LESS_THAN:
                operation_output = operand0 < operand1;
            OPERATION_AND:
                operation_output = operand0 & operand1;
            OPERATION_OR:
                operation_output = operand0 | operand1;
            OPERATION_XOR:
                operation_output = operand0 ^ operand1;
            OPERATION_SHIFT_LEFT:
                operation_output = operand0 << shift_amount;
            OPERATION_SHIFT_RIGHT:
                operation_output = operand0 >> shift_amount;
            OPERATION_SIGNED_SHIFT_RIGHT:
                operation_output = $signed(operand0) >> shift_amount;
            OPERATION_EQUAL:
                operation_output = operand0 == operand1;
            OPERATION_GREATER_EQUAL:
                operation_output = operand0 >= operand1;
            OPERATION_SIGNED_GREATER_EQUAL:
                operation_output = $signed(operand0) >= $signed(operand1);
            default:
                operation_output = 0;
        endcase

    logic take_branch;
    assign take_branch = opcode == OPCODE_BRANCH && operation_output;

    logic[31:0] target_address_offset;
    always_comb
        if (is_load && !loading) target_address_offset = 0;
        else if (is_jal)         target_address_offset = immediate_j;
        else if (take_branch)    target_address_offset = immediate_b;
        else                     target_address_offset = 4;

    logic[31:0] target_address;
    always_comb
        if (is_jalr) target_address = { operation_output[31:1], 1'b0 };
        else         target_address = pc + target_address_offset;

    logic[31:0] register_write_data;
    always_comb
        // x0 must always be 0.
        if (rd == 0)                register_write_data = 0;
        else if (is_jal || is_jalr) register_write_data = pc + 4;
        else if (loading)           register_write_data = read_data;
        else                        register_write_data = operation_output;

    always_ff @(posedge clock)
        registers[rd] <= is_store ? registers[rd] : register_write_data;

    assign write_address = operation_output;
    assign write_data    = rs2_value;

    logic[31:0] next_pc;
    assign next_pc = reset ? 0 : target_address;

    // TODO Generate an instruction address misaligned exception if
    //      next program counter is not a multiple of 4.
    always_ff @(posedge clock)
        pc <= next_pc;

    assign read_address = is_load ? operation_output : next_pc;

endmodule
