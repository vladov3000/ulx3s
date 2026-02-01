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

static I64 parse_register(Bytes input) {
    if (input.size < 2 || input.data[0] != 'x') {
        return -1;
    }

    I64 value = 0;
    for (I64 i = 1; i < input.size; i++) {
        U8 c = input.data[i];
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

static I64 lex_register(Lexer* lexer) {
    lex_operand(lexer);
    I64 register_ = parse_register(lexer->lexeme_bytes);
    if (register_ == -1) {
        lexer_error(lexer, "Invalid register \"%s\".\n", lexer->lexeme_bytes);
    }
    return register_;
}

static I64 parse_immediate(Lexer* lexer, I64 minimum, I64 maximum) {
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
        lexer_error(lexer, "\"%s\" is not an integer.\n", lexer->lexeme_bytes);
    }

    I64 value = 0;

    while (bytes.size > 0) {
        I64 digit = 0;
        if (!get_digit(bytes.data[0], base, &digit)) {
            lexer_error(lexer, "\"%s\" is not an integer.\n", lexer->lexeme_bytes);
        }
    
        if (negative) {
            digit = -digit;
        }
    
        if (value < (minimum - digit) / base) {
            lexer_error(lexer, "Immediate is too small. Minimum value must be 0x%x.\n", minimum);
        }

        if (value > (maximum - digit) / base) {
            lexer_error(lexer, "Immediate too large. Maximum value must be 0x%x.\n", maximum);
        }

        value = base * value + digit;
        bytes = drop(bytes, 1);
    }

    return value;
}
