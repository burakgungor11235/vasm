Lexer Design
============

.. warning::

   **varm is NOT stable.** The lexer implementation may change. Token types,
   tokenization rules, and maximum token limits are not guaranteed to remain
   compatible across versions.

1. Lexer Overview
-----------------

The lexer (also called a tokenizer or scanner) converts raw assembly source
text into a sequence of tokens. This tokenization step separates syntactic
analysis from lexical analysis, following the traditional compiler pipeline:

::

    Source Code → Lexer → Tokens → Parser → AST/Binary

Why a Lexer is Needed
~~~~~~~~~~~~~~~~~~~~~

Assembly source code contains characters with different semantic meanings:

- Identifiers: ``mov``, ``r0``, ``my_label``
- Numbers: ``42``, ``0xFF``, ``0b1010``
- Punctuation: ``,``, ``[``, ``]``, ``#``
- Comments: ``; this is ignored``

The lexer classifies each character sequence into tokens, making parsing
simpler and more efficient. The parser works with structured tokens rather
than raw text.

2. Token Types
--------------

The lexer produces the following token types (defined in ``include/assembler.h``):

=================  =============================================================
Token Type         Description
=================  =============================================================
TOKEN_EOF          End of file marker
TOKEN_INSTRUCTION  Assembly mnemonic (e.g., ``mov``, ``add``, ``ldr``)
TOKEN_DIRECTIVE    Assembler directive (e.g., ``.text``, ``.word``)
TOKEN_LABEL        Label definition (e.g., ``my_label:``)
TOKEN_REGISTER     Register name (currently parsed as identifier)
TOKEN_IMMEDIATE    Numeric constant (e.g., ``#42``, ``0xFF``)
TOKEN_IDENTIFIER   Symbol name (labels, register aliases)
TOKEN_STRING       Quoted string literal (e.g., ``"hello"``)
TOKEN_COMMA        Comma separator (``,``)
TOKEN_EQUAL        Equals sign (``=`` for pseudo-instructions)
TOKEN_LBRACKET     Left bracket (``[``)
TOKEN_RBRACKET     Right bracket (``]``)
TOKEN_EXCLAM       Exclamation mark (``!`` for writeback)
TOKEN_HASH         Hash/pound sign (``#`` for immediates)
TOKEN_NEWLINE      Line terminator
=================  =============================================================

Token Structure
~~~~~~~~~~~~~~~

::

    typedef struct {
        token_type_t type;   // Token classification
        char*        value;  // String value (allocated, must be freed)
        int          line;   // Source line number (1-indexed)
        int          column; // Column position in source line
    } token_t;

3. Finite State Machine
-----------------------

The lexer uses a simple state machine based on the current character:

::

    ┌─────────────────────────────────────────────────────────────────┐
    │                      INITIAL STATE                              │
    ├─────────────────────────────────────────────────────────────────┤
    │                                                                  │
    │   ┌─────────────┐    whitespace    ┌─────────────────────┐    │
    │   │   ' '       │──────────────────→│     (discard)       │    │
    │   │   '\t'      │                   └─────────────────────┘    │
    │   │   '\r'      │                                              │
    │   └─────────────┘                                              │
    │                                                                  │
    │   ┌─────────────┐    '\n'          ┌─────────────────────┐    │
    │   │   '\n'      │──────────────────→│   TOKEN_NEWLINE     │────┼──→ INITIAL
    │   └─────────────┘                   └─────────────────────┘    │
    │                                                                  │
    │   ┌─────────────┐    ';'            ┌─────────────────────┐    │
    │   │   ';'       │──────────────────→│   IN_COMMENT        │────┼──→ (skip to '\n')
    │   └─────────────┘                   └─────────────────────┘    │
    │                                                                  │
    │   ┌─────────────┐    [a-zA-Z_]      ┌─────────────────────┐    │
    │   │  letter/'_' │──────────────────→│   IN_IDENTIFIER     │────┼──→ TOKEN_LABEL/INSTRUCTION/
    │   └─────────────┘                   └─────────────────────┘    │   TOKEN_DIRECTIVE/IDENTIFIER
    │                                                                  │
    │   ┌─────────────┐    [0-9-]          ┌─────────────────────┐    │
    │   │ digit/'-'   │──────────────────→│    IN_NUMBER        │────┼──→ TOKEN_IMMEDIATE
    │   └─────────────┘                   └─────────────────────┘    │
    │                                                                  │
    │   ┌─────────────┐    '"'             ┌─────────────────────┐    │
    │   │   '"'       │──────────────────→│   IN_STRING         │────┼──→ TOKEN_STRING
    │   └─────────────┘                   └─────────────────────┘    │
    │                                                                  │
    │   ┌─────────────┐    "'"             ┌─────────────────────┐    │
    │   │   "'"       │──────────────────→│   IN_CHAR           │────┼──→ TOKEN_IMMEDIATE
    │   └─────────────┘                   └─────────────────────┘    │
    │                                                                  │
    │   ┌─────────────┐    '#'             ┌─────────────────────┐    │
    │   │   '#'       │──────────────────→│   TOKEN_HASH        │────┼──→ INITIAL
    │   └─────────────┘                   └─────────────────────┘    │
    │                                                                  │
    │   ┌─────────────┐    ','             ┌─────────────────────┐    │
    │   │   ','       │──────────────────→│   TOKEN_COMMA       │────┼──→ INITIAL
    │   └─────────────┘                   └─────────────────────┘    │
    │                                                                  │
    │   ┌─────────────┐    '='             ┌─────────────────────┐    │
    │   │   '='       │──────────────────→│   TOKEN_EQUAL       │────┼──→ INITIAL
    │   └─────────────┘                   └─────────────────────┘    │
    │                                                                  │
    │   ┌─────────────┐    '['             ┌─────────────────────┐    │
    │   │   '['       │──────────────────→│ TOKEN_LBRACKET      │────┼──→ INITIAL
    │   └─────────────┘                   └─────────────────────┘    │
    │                                                                  │
    │   ┌─────────────┐    ']'             ┌─────────────────────┐    │
    │   │   ']'       │──────────────────→│ TOKEN_RBRACKET      │────┼──→ INITIAL
    │   └─────────────┘                   └─────────────────────┘    │
    │                                                                  │
    │   ┌─────────────┐    '!'             ┌─────────────────────┐    │
    │   │   '!'       │──────────────────→│ TOKEN_EXCLAM        │────┼──→ INITIAL
    │   └─────────────┘                   └─────────────────────┘    │
    │                                                                  │
    │   ┌─────────────┐    other           ┌─────────────────────┐    │
    │   │  unknown    │──────────────────→│   (discard)         │────┼──→ INITIAL
    │   └─────────────┘                   └─────────────────────┘    │
    │                                                                  │
    └─────────────────────────────────────────────────────────────────┘

4. Regular Expressions for Tokens
----------------------------------

The lexer uses these patterns to identify tokens:

Identifier
~~~~~~~~~~

::

    [a-zA-Z_][a-zA-Z0-9_]*

Examples: ``mov``, ``r0``, ``my_label``, ``_start``

Number
~~~~~~

::

    0x[0-9A-Fa-f]+    (hexadecimal)
    0b[01]+           (binary)
    -?[0-9]+          (decimal, including negative)
    [0-9]+            (unsigned decimal)

Examples: ``0xFF``, ``0b1010``, ``42``, ``-10``

Register
~~~~~~~~

::

    r[0-9]+           (r0-r15)
    sp                (r13, stack pointer)
    lr                (r14, link register)
    pc                (r15, program counter)

Note: Registers are currently parsed as identifiers and converted later.

String
~~~~~~

::

    "([^"\\]|\\.)*"

Examples: ``"hello"``, ``"line\n"``

Character Literal
~~~~~~~~~~~~~~~~~

::

    '.'

Examples: ``'A'``, ``'\n'``

5. Implementation Details
--------------------------

Location: ``src/asm/lexer.c``

Main Functions
~~~~~~~~~~~~~~

``int tokenize(const char* input, token_t* tokens, int max_tokens)``
    Tokenizes the input string. Returns the number of tokens created.

``void free_tokens(token_t* tokens, int count)``
    Frees all dynamically allocated token values.

Token Limits
~~~~~~~~~~~~

::

    #define MAX_TOKENS 4096

The maximum token count is 4096. Programs exceeding this limit will have
truncated tokenization.

Helper Functions
~~~~~~~~~~~~~~~~

::

    static int is_whitespace(char c)
        Returns true if c is space, tab, or carriage return.

    static int is_identifier_start(char c)
        Returns true if c can start an identifier (letter or underscore).

    static int is_identifier_char(char c)
        Returns true if c can continue an identifier (alphanumeric or underscore).

    static int is_number_start(char c)
        Returns true if c can start a number (digit or minus sign).

    static char* strdup_len(const char* str, int len)
        Allocates and copies a string of specified length.

6. Lexer Algorithm (Pseudocode)
-------------------------------

::

    function tokenize(input, tokens, max_tokens):
        token_count = 0
        line = 1
        p = input

        while p[0] != '\0' and token_count < max_tokens:
            if is_whitespace(p[0]):
                p = p + 1
                continue

            if p[0] == '\n':
                tokens[token_count] = {TOKEN_NEWLINE, NULL, line, column}
                token_count++
                line++
                p = p + 1
                continue

            if p[0] == ';':
                while p[0] != '\0' and p[0] != '\n':
                    p = p + 1
                continue

            if p[0] == '#':
                tokens[token_count] = {TOKEN_HASH, NULL, line, column}
                token_count++
                p = p + 1
                continue

            if p[0] == ',':
                tokens[token_count] = {TOKEN_COMMA, NULL, line, column}
                token_count++
                p = p + 1
                continue

            if is_number_start(p[0]):
                start = p
                while is_number_start(p[0]) or p[0] in 'xXa-fA-F':
                    p = p + 1
                value = strdup_len(start, p - start)
                tokens[token_count] = {TOKEN_IMMEDIATE, value, line, column}
                token_count++
                continue

            if is_identifier_start(p[0]):
                start = p
                while is_identifier_char(p[0]):
                    p = p + 1

                if p[0] == ':':
                    name = strdup_len(start, p - start)
                    tokens[token_count] = {TOKEN_LABEL, name, line, column}
                    token_count++
                    p = p + 1
                else:
                    name = strdup_len(start, p - start)
                    type = classify_identifier(name)  // instruction/directive/identifier
                    tokens[token_count] = {type, name, line, column}
                    token_count++
                continue

            // Unknown character
            p = p + 1

        tokens[token_count] = {TOKEN_EOF, NULL, line, column}
        token_count++

        return token_count

Identifier Classification
~~~~~~~~~~~~~~~~~~~~~~~~~

When an identifier is found, it is classified by comparing against known
instruction and directive names:

::

    static const char* instructions[] = {
        "mov", "mvn", "add", "adc", "sub", "sbc", "rsb", "rsc",
        "and", "eor", "orr", "bic", "cmp", "cmn", "tst", "teq",
        "mul", "mla", "ldr", "ldrb", "str", "strb", "b", "bl",
        "bx", "halt", "swi", "nop", "push", "pop", "call", "ret",
        "beq", "bne", "bcs", "bhs", "bcc", "blo", "bmi", "bpl",
        "bvs", "bvc", "bhi", "bls", "bge", "blt", "bgt", "ble",
        NULL
    };

    static const char* directives[] = {
        ".text", ".data", ".word", ".byte", ".ascii", ".asciz",
        ".space", ".align", ".equ", ".set", ".global", NULL
    };

7. Complexity Analysis
----------------------

Time Complexity: **O(n)** where n = input length

- Single pass through input characters
- Each character examined exactly once
- String comparisons for classification are O(k) where k = identifier length
- Overall: O(n)

Space Complexity: **O(t)** where t = number of tokens

- Tokens array: fixed size (MAX_TOKENS = 4096)
- Token values: sum of lengths of all identifiers/strings/numbers
- Each token stores: type (4 bytes), value pointer (8 bytes), line (4 bytes), column (4 bytes)
- Base token structure: 20 bytes per token (on 64-bit)
- Maximum: ~80KB for token array + values

.. warning::

   The lexer performs case-insensitive comparison for instruction and directive
   names, but case-sensitive comparison for identifiers. This may change in
   future versions.
