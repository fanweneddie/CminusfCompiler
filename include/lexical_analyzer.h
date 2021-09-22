#ifndef _LEXICAL_ANALYZER_H_
#define _LEXICAL_ANALYZER_H_

#include <stdio.h>
#define __THROW
#define __wur

extern int fileno (FILE *__stream) __THROW __wur;

#ifndef YYTOKENTYPE
#define YYTOKENTYPE
/* the integer code of token in cminus */
typedef enum cminus_token_type {
    // operation
    ADD = 259,
    SUB = 260,
    MUL = 261,
    DIV = 262,
    LT = 263,
    LTE = 264,
    GT = 265,
    GTE = 266,
    EQ = 267,
    NEQ = 268,
    ASSIN = 269,
    // punctuation
    SEMICOLON = 270,
    COMMA = 271,
    LPARENTHESE = 272,
    RPARENTHESE = 273,
    LBRACKET = 274,
    RBRACKET = 275,
    LBRACE = 276,
    RBRACE = 277,
    // key word
    ELSE = 278,
    IF = 279,
    INT = 280,
    FLOAT = 281,
    RETURN = 282,
    VOID = 283,
    WHILE = 284,
    // ID and NUM
    IDENTIFIER = 285,
    INTEGER = 286,
    FLOATPOINT = 287,
    ARRAY = 288,
    LETTER = 289,
    // others
    EOL = 290,
    COMMENT = 291,
    BLANK = 292,
    ERROR = 258

} Token;


typedef struct{
    char text[256];
    int token;
    int lines;
    int pos_start;
    int pos_end;
} Token_Node;

#define MAX_NUM_TOKEN_NODE 1024

void analyzer(char* input_file, Token_Node* token_stream);

#endif /* YYTOKENTYPE */
#endif /* lexical_analyzer.h */
