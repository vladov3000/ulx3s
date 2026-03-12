typedef struct {
    Buffer* console;
    Bytes   path;
    I64     line;
    I64     column;
    U8*     current;
    U8*     end;
    I64     lexeme_line;
    I64     lexeme_column;
    Bytes   lexeme_bytes;
} Lexer;

static Lexer make_lexer(Buffer* console, Bytes path, Bytes text) {
    return (Lexer) {
        .console = console,
        .path    = path,
        .line    = 1,
        .column  = 1,
        .current = text.data,
        .end     = &text.data[text.size],
    };
}

static bool lexer_done(Lexer* lexer) {
    return lexer->current == lexer->end;
}

static void lexer_step(Lexer* lexer) {
    if (*lexer->current == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    lexer->current++;
}

static U8 lexer_current(Lexer* lexer) {
    return *lexer->current;
}

static void clear_space(Lexer* lexer) {
    while (!lexer_done(lexer)) {
        if (lexer_current(lexer) == '#') {
            while (!lexer_done(lexer) && lexer_current(lexer) != '\n') {
                lexer_step(lexer);
            }
        }

        if (!is_space(lexer_current(lexer))) {
            return;
        }

        lexer_step(lexer);
    }
}

static bool lex(Lexer* lexer) {
    clear_space(lexer);
    if (lexer_done(lexer)) {
        return false;
    }

    U8* start            = lexer->current;
    lexer->lexeme_line   = lexer->line;
    lexer->lexeme_column = lexer->column;

    while (!lexer_done(lexer) && !is_space(lexer_current(lexer))) {
        lexer_step(lexer);
    }

    lexer->lexeme_bytes = (Bytes) { start, lexer->current - start };
    return true;
}

static void lexer_error(Lexer* lexer, const char* format, ...) {
    va_list arguments;
    va_start(arguments, format);

    Buffer* console = lexer->console;
    print(console, ERROR "%s:%i:%i: ", lexer->path, lexer->lexeme_line, lexer->lexeme_column);
    printv(console, format, arguments);

    flush_and_exit(console, EXIT_FAILURE);
    va_end(arguments);
}

static void lex_operand(Lexer* lexer) {
    if (!lex(lexer)) {
        lexer_error(lexer, "Missing operand.\n");
    }
}

static I64 try_parse_register(Lexer* lexer) {
    Bytes bytes = lexer->lexeme_bytes;

    if (bytes.size < 2 || bytes.data[0] != 'x') {
        return -1;
    }

    I64 value = 0;
    for (I64 i = 1; i < bytes.size; i++) {
        U8 c = bytes.data[i];
        if (!is_digit(c)) {
            return -1;
        }

        I64 digit = c - '0';
        value     = 10 * value + digit;
        if (value > 31) {
            return -1;
        }
    }

    return value;
}

static I64 parse_register(Lexer* lexer) {
    I64 register_ = try_parse_register(lexer);
    if (register_ == -1) {
        lexer_error(lexer, "Invalid register \"%s\".\n", lexer->lexeme_bytes);
    }
    return register_;
}

static I64 lex_register(Lexer* lexer) {
    lex_operand(lexer);
    return parse_register(lexer);
}

typedef enum {
    PARSE_NUMBER_OK,
    PARSE_NUMBER_NOT_INTEGER,
    PARSE_NUMBER_TOO_SMALL,
    PARSE_NUMBER_TOO_LARGE,
} ParseNumberResult;

static ParseNumberResult try_parse_number(Lexer* lexer, I64 minimum, I64 maximum, I64* output) {
    Bytes   bytes    = lexer->lexeme_bytes;
    bool    negative = false;

    if (starts_with(bytes, "-")) {
        bytes    = drop(bytes, 1);
        negative = true;
    }

    I64 base = 10;
    if (starts_with(bytes, "0x")) {
        bytes = drop(bytes, 2);
        base  = 16;
    }

    if (bytes.size == 0 || !get_digit(bytes.data[0], base, NULL)) {
        return PARSE_NUMBER_NOT_INTEGER;
    }

    I64 value = 0;
    while (bytes.size > 0) {
        I64 digit = 0;
        if (!get_digit(bytes.data[0], base, &digit)) {
            return PARSE_NUMBER_NOT_INTEGER;
        }
    
        if (negative) {
            digit = -digit;
        }
    
        if (value < (minimum - digit) / base) {
            return PARSE_NUMBER_TOO_SMALL;
        }

        if (value > (maximum - digit) / base) {
            return PARSE_NUMBER_TOO_LARGE;
        }

        value = base * value + digit;
        bytes = drop(bytes, 1);
    }

    *output = value;
    return PARSE_NUMBER_OK;
}

static I64 parse_number(Lexer* lexer, I64 minimum, I64 maximum) {
    I64               output = 0;
    ParseNumberResult result = try_parse_number(lexer, minimum, maximum, &output);
    switch (result) {

    case PARSE_NUMBER_NOT_INTEGER:
        lexer_error(lexer, "\"%s\" is not an integer.\n", lexer->lexeme_bytes);

    case PARSE_NUMBER_TOO_SMALL:
        lexer_error(lexer, "Immediate is too small. Minimum value must be 0x%x.\n", minimum);

    case PARSE_NUMBER_TOO_LARGE:
        lexer_error(lexer, "Immediate too large. Maximum value must be 0x%x.\n", maximum);

    default:
        return output;

    }
}
