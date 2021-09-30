%option noyywrap
%{
/***************** declaration and option  begin *****************/
#include <stdio.h>
#include <stdlib.h>

#include "lexical_analyzer.h"
// the line that the word is at
int lines;
// the start position of the word
int pos_start;
// the end position of the word
int pos_end;


/***************** declaration and option  end *****************/
%}

%%
 /******************TODO*********************/
 /*** fill in the match of flex  start ******/
 
 // 1. operation  2. punctuation  3. key word  4. ID and NUM  5. others
\+ 			{ return ADD;	 }
\- 			{ return SUB; 	}
\*			{ return MUL; 	}
\/ 			{ return DIV; 	}
\< 			{ return LT;  	}
\<\= 		{ return LTE; 	}
\>			{ return GT;  	}
\>\=		{ return GTE; 	}
\=\=		{ return EQ;  	}
\!\=		{ return NEQ; 	}
\=			{ return ASSIGN; }

\;			{ return SEMICOLON; 	}
\,			{ return COMMA;			}
\(			{ return LPARENTHESE;	}
\)			{ return RPARENTHESE;	}
\[			{ return LBRACKET;		}
\]			{ return RBRACKET;		}
\{			{ return LBRACE;		}
\}			{ return RBRACE;		}

else		{ return ELSE;		}
if			{ return IF;		}
int			{ return INT;		}
float		{ return FLOAT;		}
return		{ return RETURN;	}
void		{ return VOID;		}
while		{ return WHILE;		}

[a-zA-Z]+							{ return IDENTIFIER;	}
[0-9]+								{ return INTEGER;		}
[0-9]+\.|[0-9]*\.[0-9]+				{ return FLOATPOINT;	}
\[\]								{ return ARRAY;			}
[a-zA-Z]							{ return LETTER;		}
(\r)*\n								{ return EOL;			}
"/*"([^\*]|(\*)*[^\*/])*(\*)*"*/" 	{ return COMMENT;		}
[ \t]+								{ return BLANK;			}
. 									{ return ERROR;			} 




 /**** fill in the match of flex  end ******/
%%
/**************** C code start *************/

/// \brief analysize a *.cminus file
///
///	\param input_file, the path of file to analyze
/// \param token_stream, array of Token_Node, which stores the result of analysis
void analyzer(char* input_file, Token_Node* token_stream){
	if(!(yyin = fopen(input_file,"r"))){
		printf("[ERR] No input file\n");
		exit(1);
	}
	printf("[START]: Read from: %s\n", input_file);

	int token;
	int index = 0;
	lines = 1;
	pos_start = 1;
	pos_end = 1;

	while(token = yylex()){
		int len = strlen(yytext);
		switch(token){
			// for comments, check whether there is '\n'
			// if so, increment lines and reset pos_start and pos_end
			case COMMENT:
				for (int i = 0; i < len; ++i) {
					if (yytext[i] == '\n') {
						lines += 1;
						pos_start = 1;
						pos_end = 1;
					} else {
						pos_end += 1;
					}
				}
				break;
			// for blank, increment pos_end
			case BLANK:
				pos_end += len;
				break;
			// for end of line, increment lines and reset pos_start and pos_end
			case EOL:
				lines += 1;
				pos_start = 1;
				pos_end = 1;
				break;
			case ERROR:
				printf("[ERR]: unable to analysize %s at %d line, from %d to %d\n", yytext, lines, pos_start, pos_end);
			default :
				if (token == ERROR){
					sprintf(token_stream[index].text, "[ERR]: unable to analysize %s at %d line, from %d to %d", yytext, lines, pos_start, pos_end);
				} else {
					strcpy(token_stream[index].text, yytext);
				}

				// update pos_start and pos_end
				pos_start = pos_end;
				pos_end = pos_start + len;
				// record the info of that token
				token_stream[index].token = token;
				token_stream[index].lines = lines;
				token_stream[index].pos_start = pos_start;
				token_stream[index].pos_end = pos_end;
				index++;
				if (index >= MAX_NUM_TOKEN_NODE){
					printf("%d has too many tokens (> %d)", input_file, MAX_NUM_TOKEN_NODE);
					exit(1);
				}
		}
	}
	printf("[END]: Analysis completed.\n");
	return;
}



/**************** C code end *************/