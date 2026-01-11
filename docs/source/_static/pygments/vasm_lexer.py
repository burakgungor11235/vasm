"""
Varm/ASM Lexer for Pygments
===========================

Custom lexer for varm assembly language syntax highlighting.
Provides highlighting for:
- Instructions (MOV, ADD, LDR, STR, etc.)
- Condition codes (EQ, NE, GT, LT, etc.)
- Registers (R0-R15, SP, LR, PC, FP, IP)
- Labels and identifiers
- Immediate values
- Comments
"""

from pygments.lexer import RegexLexer, words
from pygments import token


class VarmLexer(RegexLexer):
    """Lexer for varm assembly language."""

    name = "varm"
    aliases = ["varm", "vasm"]
    filenames = ["*.varm", "*.vasm"]
    mimetypes = ["text/x-varm", "text/x-vasm"]

    # Register names (including aliases)
    registers = (
        "r0",
        "r1",
        "r2",
        "r3",
        "r4",
        "r5",
        "r6",
        "r7",
        "r8",
        "r9",
        "r10",
        "r11",
        "r12",
        "r13",
        "r14",
        "r15",
        "sp",
        "lr",
        "pc",
        "fp",
        "ip",
    )

    # Condition codes
    conditions = (
        "eq",
        "ne",
        "gt",
        "lt",
        "ge",
        "le",
        "hi",
        "ls",
        "cc",
        "cs",
        "pl",
        "mi",
        "vs",
        "vc",
    )

    # Data processing instructions
    data_proc = (
        "add",
        "sub",
        "rsb",
        "adc",
        "sbc",
        "rsc",
        "and",
        "orr",
        "eor",
        "bic",
        "orn",
        "mov",
        "mvn",
        "cmp",
        "cmn",
        "tst",
        "teq",
    )

    # Multiply instructions
    multiply = ("mul", "mla", "mls", "umull", "umlal", "smull", "smlal")

    # Load/store instructions
    load_store = ("ldr", "str", "ldrb", "strb", "ldrh", "strh", "ldrsb", "ldrsh")

    # Branch instructions
    branch = ("b", "bl", "bx", "blx")

    # Status register instructions
    status = ("mrs", "msr")

    # System instructions
    system = ("swi", "svc", "halt", "nop", "equ")

    # Pseudo-instructions
    pseudo = ("adr", "adrl", "org", "req", "global", "local")

    # Data directives
    data_directives = (
        "byte",
        "word",
        "half",
        "ascii",
        "asciz",
        "space",
        "data",
        "text",
    )

    tokens = {
        "root": [
            # Comments - must be first
            (r";.*$", token.Comment.Single),
            # Labels (at start of line)
            (r"^[a-zA-Z_][a-zA-Z0-9_]*:", token.Name.Label, "label"),
            # Immediate values (decimal, hex, binary)
            (r"#[+-]?\d+", token.Number.Integer),
            (r"0x[0-9a-fA-F]+", token.Number.Hex),
            (r"0b[01]+", token.Number.Bin),
            (r"0o[0-7]+", token.Number.Oct),
            # Strings
            (r'"[^"]*"', token.String),
            # Character literals (including escape sequences)
            (r"'[^']*'", token.String.Char),
            # =label pseudo-op for address loading
            (r"=\s*[a-zA-Z_][a-zA-Z0-9_]*", token.Name.Function),
            (r"=\s*0x[0-9a-fA-F]+", token.Number),
            (r"=\s*#[+-]?\d+", token.Number),
            # Directives (starting with .)
            (r"\.[a-zA-Z_][a-zA-Z0-9_]*", token.Name.Builtin),
            # Condition codes (case-insensitive using (?i))
            (
                r"(?i)\b(eq|ne|gt|lt|ge|le|hi|ls|cc|cs|pl|mi|vs|vc)\b",
                token.Keyword.Operand,
            ),
            # Data processing instructions (case-insensitive)
            (
                r"(?i)\b(add|sub|rsb|adc|sbc|rsc|and|orr|eor|bic|orn|mov|mvn|cmp|cmn|tst|teq)\b",
                token.Keyword,
            ),
            # Multiply instructions (case-insensitive)
            (r"(?i)\b(mul|mla|mls|umull|umlal|smull|smlal)\b", token.Keyword),
            # Load/store instructions (case-insensitive)
            (
                r"(?i)\b(ldr|str|ldrb|strb|ldrh|strh|ldrsb|ldrsh|ldm|stm)\b",
                token.Keyword,
            ),
            # Branch instructions (case-insensitive)
            (r"(?i)\b(b|bl|bx|blx)\b", token.Keyword),
            # Status register instructions (case-insensitive)
            (r"(?i)\b(mrs|msr)\b", token.Keyword),
            # System instructions (case-insensitive)
            (r"(?i)\b(swi|svc|halt|nop|equ)\b", token.Keyword),
            # Pseudo-instructions (case-insensitive)
            (r"(?i)\b(adr|adrl|org|req|global|local)\b", token.Name.Builtin),
            # Registers (case-insensitive)
            (
                r"(?i)\b(r0|r1|r2|r3|r4|r5|r6|r7|r8|r9|r10|r11|r12|r13|r14|r15|sp|lr|pc|fp|ip)\b",
                token.Name.Builtin,
            ),
            # Identifiers and labels
            (r"[a-zA-Z_][a-zA-Z0-9_]*", token.Name),
            # Operators
            (r"[=+\-*/<>&\^|]", token.Operator),
            # Punctuation
            (r"[\[\]{},.:]", token.Punctuation),
            # Whitespace
            (r"\s+", token.Text),
        ],
        "label": [
            (r"[a-zA-Z_][a-zA-Z0-9_]*", token.Name.Label, "#pop"),
            (r"\s+", token.Text),
            (r":", token.Punctuation),
            (r".", token.Text),
        ],
    }
