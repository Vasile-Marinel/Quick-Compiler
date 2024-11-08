#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "lexer.h"

int iTk;	// the iterator in tokens
Token* consumed;	// the last consumed token

// same as err, but also prints the line of the current token
void tkerr(const char* fmt, ...) {
	fprintf(stderr, "error in line %d: ", (iTk > 0) ? tokens[iTk - 1].line : tokens[iTk].line);
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

bool consume(int code) {
	//printf("consume(%s)", tkCodeName(code));
	if (tokens[iTk].code == code) {
		consumed = &tokens[iTk++];
		//printf(" => consumed\n");
		return true;
	}
	//printf(" => found %s\n", tkCodeName(tokens[iTk].code));
	return false;
}


// baseType ::= TYPE_INT | TYPE_REAL | TYPE_STR
bool baseType() {
	puts("#baseType");
	if (consume(TYPE_INT)) {
		return true;
	}
	else if (consume(TYPE_REAL)) {
		return true;
	}
	else if (consume(TYPE_STR)) {
		return true;
	}
	else tkerr("tip de data invalid");
	return false;
}

bool block();
bool defVar();
bool expr();

/*factor ::= INT
			| REAL
			| STR
			| LPAR expr RPAR
			| ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?*/
bool factor() {
	int start = iTk;
	puts("#factor");
	if (consume(INT) || consume(REAL) || consume(STR)) {
		return true;
	}
	else if (consume(LPAR)) {
		if (expr()) {
			if (consume(RPAR)) {
				return true;
			}
			else tkerr("lipseste ) dupa expresie");
		}
		else tkerr("expresie invalida intre paranteze");
	}
	else if (consume(ID)) {
		if (consume(LPAR)) {
			if (expr()) {
				while (consume(COMMA)) {
					if (!expr()) {
						tkerr("expresie invalida dupa , in apelarea de functie");
					}
				}
			}
			if (consume(RPAR)) {
				return true;
			}
			else tkerr("lipseste ) in apelarea de functie");
		}
		return true;
	}
	iTk = start;
	return false;
}

// exprPrefix ::= ( SUB | NOT )? factor
bool exprPrefix() {
	int start = iTk;
	puts("#esprPrefix");
	if (consume(SUB) || consume(NOT)) {
		if (factor()) {
			return true;
		}
	}
	iTk = start;
	return factor();
}

// exprMul ::= exprPrefix ( ( MUL | DIV ) exprPrefix )*
bool exprMul() {
	int start = iTk;
	puts("#exprMul");
	if (exprPrefix()) {
		while (consume(MUL) || consume(DIV)) {
			if (!exprPrefix()) {
				tkerr("expresie invalida dupa operatorul de inmultire/impartire");
			}
		}
		return true;
	}
	iTk = start;
	return false;
}

// exprAdd ::= exprMul ( ( ADD | SUB ) exprMul )*
bool exprAdd() {
	int start = iTk;
	puts("#exprAdd");
	if (exprMul()) {
		while (consume(ADD) || consume(SUB)) {
			if (!exprMul()) {
				tkerr("expresie invalida dupa operatorul de adaugare/scadere");
			}
		}
		return true;
	}
	iTk = start;
	return false;
}

// exprComp ::= exprAdd ( ( LESS | EQUAL ) exprAdd )?
bool exprComp() {
	int start = iTk;
	puts("#exprComp");
	if (exprAdd()) {
		while (consume(LESS) || consume(EQUAL)) {
			if (!exprAdd()) {
				tkerr("expresie invalida dupa operatorul de comparare");
			}
		}
		return true;
	}
	iTk = start;
	return false;
}

// exprAssign ::= ( ID ASSIGN )? exprComp
bool exprAssign() {
	int start = iTk;
	puts("#exprAssign");
	if (consume(ID)) {
		if (consume(ASSIGN)) {
			if (exprComp()) {
				return true;
			}
			else {
				tkerr("expresie invalida in atribuire");
				return false;
			}
		}
	}
	iTk = start;
	return exprComp();
}

// exprLogic ::= exprAssign ( ( AND | OR ) exprAssign )*
bool exprLogic() {
	int start = iTk;
	puts("#exprLogic");
	if (exprAssign()) {
		while (consume(AND) || consume(OR)) {
			if (!exprAssign()) {
				tkerr("expresie invalida dupa operatorul logic");
			}
		}
		return true;
	}
	iTk = start;
	return false;
}

// expr ::= exprLogic
bool expr() {
	int start = iTk;
	puts("#expr");
	if (exprLogic()) {
		return true;
	}
	iTk = start;
	return false;

}

/*instr ::= expr? SEMICOLON
			| IF LPAR expr RPAR block ( ELSE block )? END
			| RETURN expr SEMICOLON
			| WHILE LPAR expr RPAR block END*/
bool instr() {
	int start = iTk;
	puts("#instr");
	if (expr()) {
		if (consume(SEMICOLON)) {
			return true;
		}
		else tkerr("lipseste ; dupa expresie");
	}

	if (consume(SEMICOLON)) {
		return true;
	}

	if (consume(IF)) {
		if (consume(LPAR)) {
			if (expr()) {
				if (consume(RPAR)) {
					if (block()) {
						if (consume(ELSE)) {
							if (block()) { }
							else tkerr("cod invalid dupa else");
						}
						if (consume(END)) {
							return true;
						}
						else tkerr("instructiunea nu a fost inchisa corespunzator");
					}
					else tkerr("cod invalid dupa if");
				}
				else tkerr("lipseste ) dupa conditia din if");
			}
			else tkerr("conditie invalida dupa ( in if");
		}
		else tkerr("lipseste ( din instructiunea if");
	}

	if (consume(RETURN)) {
		if (!expr()) {
			tkerr("expresie invalida dupa return");
		}
		if (consume(SEMICOLON)) {
			return true;
		}
		else tkerr("lipseste ; dupa expresia din return");
	}

	if (consume(WHILE)) {
		if (consume(LPAR)) {
			if (expr()) {
				if (consume(RPAR)) {
					if (block()) {
						if (consume(END)) {
							return true;
						}
						else tkerr("instructiunea while nu este inchisa");
					}
					else tkerr("cod invalid in while");
				}
				else tkerr("lipseste ) dupa conditia din while");
			}
			else tkerr("conditie invalida in while");
		}
		else tkerr("lipseste ( dupa while");
	}

	iTk = start;
	return false;
}

// block ::= instr+
bool block() {
	int start = iTk;
	puts("#block");
	if (instr()) {
		while(instr()) {}
		return true;
	}
	iTk = start;
	return false;
}

// funcParam ::= ID COLON baseType
bool funcParam() {
	int start = iTk;
	puts("#funcParam");
	if (consume(ID)) {
		if (consume(COLON)) {
			if (baseType()) {
				return true;
			}
			else tkerr("tip de data al parametrului invalid");
		}
		else tkerr("lipseste : dupa numele parametrului");
	}
	iTk = start;
	return false;
}

// funcParams ::= funcParam ( COMMA funcParam )*
bool funcParams() {
	int start = iTk;
	puts("#funcParams");
	if (funcParam()) {
		while (consume(COMMA)) {
			if (!funcParam()) {
				tkerr("parametrul functiei este invalid dupa ,");
				return false;
			}
		}
		return true;
	}
	iTk = start;
	return false;
}

// defFunc ::= FUNCTION ID LPAR funcParams? RPAR COLON baseType defVar* block END
bool defFunc() {
	int start = iTk;
	puts("#defFunc");
	if (consume(FUNCTION)) {
		if (consume(ID)) {
			if (consume(LPAR)) {
				if (funcParams()) {}
				if (consume(RPAR)) {
					if (consume(COLON)) {
						if (baseType()) {
							while (defVar()) {}
							if (block()) {
								if (consume(END)) {
									return true;
								}
								else tkerr("functia nu este inchisa");
							}
							else tkerr("functia este goala");
						}
						else tkerr("tip de data invalid");
					}
					else tkerr("lipseste : dupa ) in functie");
				}
				else tkerr("lipseste ) dupa parametrii");
			}
			else tkerr("lipseste ( dupa numele functiei");
		}
		else tkerr("lipseste numele functiei");
	}
	iTk = start;
	return false;
}

// defVar ::= VAR ID COLON baseType SEMICOLON
bool defVar() {
	int start = iTk;
	puts("#defVar");
	if (consume(VAR)) {
		if (consume(ID)) {
			if (consume(COLON)) {
				if (baseType()) {
					if (consume(SEMICOLON)) {
						return true;
					}
					else tkerr("lipseste ; dupa tipul de data");
				}
				else tkerr("tipul invalid pentru variabila");
			}
			else tkerr("lipseste : dupa numele variabilei");
		}
		else tkerr("lipseste numele variabilei");
	}
	iTk = start;
	return false;
}

// program ::= ( defVar | defFunc | block )* FINISH
bool program() {
	puts("#program");
	for (;;) {
		if (defVar()) {}
		else if (defFunc()) {}
		else if (block()) {}
		else break;
	}
	if (consume(FINISH)) {
		return true;
	}
	else tkerr("syntax error");
	return false;
}

void parse() {
	iTk = 0;
	program();
}
