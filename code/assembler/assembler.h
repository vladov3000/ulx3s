typedef enum {
    OPCODE_IMM    = 0b0010011,
    OPCODE_LUI    = 0b0110111,
    OPCODE_AUIPC  = 0b0010111,
    OPCODE_OP     = 0b0110011,
    OPCODE_JAL    = 0b1101111,
    OPCODE_JALR   = 0b1100111,
    OPCODE_BRANCH = 0b1100011,
    OPCODE_LOAD   = 0b0000011,
    OPCODE_STORE  = 0b0100011,
} Opcode;

#define FUNCT3_ADD  0b000
#define FUNCT3_SUB  ((1 << 18) | 0b000)
#define FUNCT3_SLL  0b001
#define FUNCT3_SLT  0b010
#define FUNCT3_SLTU 0b011
#define FUNCT3_XOR  0b100
#define FUNCT3_SRL  0b101
#define FUNCT3_SRA  ((1 << 18) | 0b101)
#define FUNCT3_OR   0b110
#define FUNCT3_AND  0b111

#define FUNCT3_BEQ  0b000
#define FUNCT3_BNE  0b001
#define FUNCT3_BLT  0b100
#define FUNCT3_BGE  0b101
#define FUNCT3_BLTU 0b110
#define FUNCT3_BGEU 0b111

#define FUNCT3_B   0b000
#define FUNCT3_H   0b001
#define FUNCT3_W   0b010
#define FUNCT3_LBU 0b100
#define FUNCT3_LHU 0b101

typedef struct {
    Bytes  operation;
    Opcode opcode;
    I64    funct3;
} Schema;

static const Schema schemas[] = {
    { make_bytes("addi")  , OPCODE_IMM   , FUNCT3_ADD  },
    { make_bytes("slti")  , OPCODE_IMM   , FUNCT3_SLT  },
    { make_bytes("sltiu") , OPCODE_IMM   , FUNCT3_SLTU },
    { make_bytes("andi")  , OPCODE_IMM   , FUNCT3_AND  },
    { make_bytes("ori")   , OPCODE_IMM   , FUNCT3_OR   },
    { make_bytes("xori")  , OPCODE_IMM   , FUNCT3_XOR  },
    { make_bytes("slli")  , OPCODE_IMM   , FUNCT3_SLL  },
    { make_bytes("srli")  , OPCODE_IMM   , FUNCT3_SRL  },
    { make_bytes("srai")  , OPCODE_IMM   , FUNCT3_SRA  },
    { make_bytes("lui")   , OPCODE_LUI   , 0           },
    { make_bytes("auipc") , OPCODE_AUIPC , 0           },
    { make_bytes("add")   , OPCODE_OP    , FUNCT3_ADD  },
    { make_bytes("sub")   , OPCODE_OP    , FUNCT3_SUB  },
    { make_bytes("slt")   , OPCODE_OP    , FUNCT3_SLT  },
    { make_bytes("sltu")  , OPCODE_OP    , FUNCT3_SLTU },
    { make_bytes("and")   , OPCODE_OP    , FUNCT3_AND  },
    { make_bytes("or")    , OPCODE_OP    , FUNCT3_OR   },
    { make_bytes("xor")   , OPCODE_OP    , FUNCT3_XOR  },
    { make_bytes("sll")   , OPCODE_OP    , FUNCT3_SLL  },
    { make_bytes("srl")   , OPCODE_OP    , FUNCT3_SRL  },
    { make_bytes("sra")   , OPCODE_OP    , FUNCT3_SRA  },
    { make_bytes("jal")   , OPCODE_JAL   , 0           },
    { make_bytes("jalr")  , OPCODE_JALR  , 0           },
    { make_bytes("beq")   , OPCODE_BRANCH, FUNCT3_BEQ  },
    { make_bytes("bne")   , OPCODE_BRANCH, FUNCT3_BNE  },
    { make_bytes("blt")   , OPCODE_BRANCH, FUNCT3_BLT  },
    { make_bytes("bltu")  , OPCODE_BRANCH, FUNCT3_BLTU },
    { make_bytes("bge")   , OPCODE_BRANCH, FUNCT3_BGE  },
    { make_bytes("bgeu")  , OPCODE_BRANCH, FUNCT3_BGEU },
    { make_bytes("lb")    , OPCODE_LOAD  , FUNCT3_B    },
    { make_bytes("lh")    , OPCODE_LOAD  , FUNCT3_H    },
    { make_bytes("lw")    , OPCODE_LOAD  , FUNCT3_W    },
    { make_bytes("lbu")   , OPCODE_LOAD  , FUNCT3_LBU  },
    { make_bytes("lhu")   , OPCODE_LOAD  , FUNCT3_LHU  },
    { make_bytes("sb")    , OPCODE_STORE , FUNCT3_B    },
    { make_bytes("sh")    , OPCODE_STORE , FUNCT3_H    },
    { make_bytes("sw")    , OPCODE_STORE , FUNCT3_W    },
};

static const Schema* find_schema(Bytes operation) {
    for (I64 i = 0; i < length(schemas); i++) {
        const Schema* schema = &schemas[i];
        if (bytes_equal(operation, schema->operation)) {
            return schema;
        }
    }
    return NULL;
}

typedef struct {
    Bytes name;
    I64   offset;
} Label;

typedef struct {
    Label* data;
    I64    size;
} Labels;

static Label* find_label(Labels* labels, Bytes name) {
    for (I64 i = 0; i < labels->size; i++) {
        Label* label = &labels->data[i];
        if (bytes_equal(label->name, name)) {
            return label;
        }
    }
    return NULL;
}

typedef struct {
    Labels labels;
    Lexer  lexer;
    I64    pc;
} Assembler;

static I64 parse_label(Assembler* assembler) {
    Labels* labels = &assembler->labels;
    I64     output = 0;

    if (labels != NULL) {
        Lexer* lexer  = &assembler->lexer;
        Bytes  lexeme = lexer->lexeme_bytes;
        Label* label  = find_label(labels, lexeme);
        if (label == NULL) {
            lexer_error(lexer, "Label \"%s\" is not defined.\n", lexeme);
        }
        output = assembler->pc - label->offset;
    }

    return output;
}

static I64 parse_offset(Assembler* assembler, I64 minimum, I64 maximum) {
    Lexer*            lexer  = &assembler->lexer;
    I64               output = 0;
    ParseNumberResult result = try_parse_number(lexer, minimum, maximum, &output);
    if (result != PARSE_NUMBER_OK) {
        output = parse_label(assembler);
        if (output < minimum) {
            lexer_error(lexer, "Offset is too small. Minimum value must be 0x%x.\n", minimum);
        }
        if (output > maximum) {
            lexer_error(lexer, "Offset too large. Maximum value must be 0x%x.\n", maximum);
        }
    }
    return output;
}

static I64 next_instruction(Assembler* assembler) {
    Lexer* lexer = &assembler->lexer;

    const Schema* schema = find_schema(lexer->lexeme_bytes);
    if (schema == NULL) {
        lexer_error(lexer, "Invalid operation \"%s\".\n", lexer->lexeme_bytes);
    }

    Opcode opcode    = schema->opcode;
    I64    funct3    = schema->funct3;
    I64    rd        = 0;
    I64    rs1       = 0;
    I64    rs2       = 0;
    I64    parsed    = 0;
    I64    immediate = 0;
    
    bool parse_rd
        =  opcode == OPCODE_IMM
        || opcode == OPCODE_LUI
        || opcode == OPCODE_AUIPC
        || opcode == OPCODE_OP
        || opcode == OPCODE_JAL
        || opcode == OPCODE_JALR
        || opcode == OPCODE_LOAD;

    if (parse_rd) {
        rd = lex_register(lexer);
    }

    bool parse_rs1
        =  opcode == OPCODE_IMM
        || opcode == OPCODE_OP
        || opcode == OPCODE_JALR
        || opcode == OPCODE_BRANCH
        || opcode == OPCODE_LOAD
        || opcode == OPCODE_STORE;

    if (parse_rs1) {
        rs1 = lex_register(lexer);
    }

    lex_operand(lexer);
    switch (opcode) {

    case OPCODE_IMM:
    case OPCODE_JALR:
    case OPCODE_LOAD:
        parsed    = parse_number(lexer, 0, 0x7FF);
        immediate = parsed << 20;
        break;

    case OPCODE_LUI:
    case OPCODE_AUIPC:
        immediate = parse_number(lexer, 0, 0xFFFFF000);
        if (immediate & 0xFFF) {
            lexer_error(lexer, "Offset must be a multiple of 0x1000.\n");
        }
        break;

    case OPCODE_OP:
        rs2 = parse_register(lexer);
        break;

    case OPCODE_JAL:
        parsed = parse_offset(assembler, 0, 0xFFFFF);
        if (parsed % 2 != 0) {
            lexer_error(lexer, "jal offset must be a multiple of 2.\n");
        }
        immediate |= slice_bits(parsed, 12, 19) << 12;
        immediate |= test_bit(parsed, 11)       << 20;
        immediate |= slice_bits(parsed, 1, 10)  << 21;
        immediate |= test_bit(parsed, 20)       << 31;
        break;

    case OPCODE_BRANCH:
        rs2   = parse_register(lexer);
        lex_operand(lexer);
        parsed = parse_number(lexer, 0, 0xFFF);
        if (parsed & 1) {
            lexer_error(lexer, "Branch offset must be a multiple of 2.\n");
        }
        immediate |= test_bit(parsed, 11)      << 7;
        immediate |= slice_bits(parsed, 1, 4)  << 8;
        immediate |= slice_bits(parsed, 5, 10) << 25;
        immediate |= test_bit(parsed, 12)      << 31;
        break;

    case OPCODE_STORE:
        rs2        = parse_register(lexer);
        lex_operand(lexer);
        parsed     = parse_number(lexer, 0, 0xFFF);
        immediate |= slice_bits(parsed, 0, 4) << 7;
        immediate |= slice_bits(parsed, 5, 11) << 25;
        break;

    default:
        assert(false);

    }

    I64 instruction = 0;
    instruction    |= opcode;
    instruction    |= rd     << 7;
    instruction    |= funct3 << 12;
    instruction    |= rs1    << 15;
    instruction    |= rs2    << 20;
    instruction    |= immediate;

    assembler->pc += 4;
    return instruction;
}

static void compute_label_offsets(Assembler* assembler) {
    Lexer*  lexer        = &assembler->lexer;
    Buffer* console      = lexer->console;
    Arena   labels_arena = make_arena(console, getpagesize());

    while (lex(lexer)) {
        Bytes lexeme = lexer->lexeme_bytes;
        if (ends_with(lexeme, ":")) {
            Label* label  = push(&labels_arena, Label);
            label->name   = take(lexeme, -1);
            label->offset = assembler->pc;
        } else {
            next_instruction(assembler);
        }
    }

    assembler->labels = (Labels) {
        .data = (Label*) labels_arena.memory,
        .size = labels_arena.used / sizeof(Label),
    };
}
