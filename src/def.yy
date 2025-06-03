%{
#define INFILE_ERROR 1
#define OUTFILE_ERROR 2

#include "../src/Compiler.hpp" // TODO: CMAKE modification

extern "C" int yyparse(void);
extern "C" int yyerror(const char *, ...);
extern "C" int yylex(void);

Compiler compiler;

%}
%union 
{char *text;
int	ival;
float fval;};
%type <text> wyr
%token <text> ID
%token <ival> KINT
%token <fval> KFLOAT
%token <text> STRING
%token U0 U8 I32 F32
%token KRETURN KIF KFOR
%token GEQ LEQ EQ NEQ
%token KPRINT_I32 KPRINT_F32 KPRINT_STRING
%start stmt_list
%%

stmt_list
    : stmt {;}
    | stmt_list stmt {;}
    ;

stmt
    : variable_decl ';' {;}
    | assignment ';' {;  compiler.gen_assignment();}
    | KPRINT_I32 '(' wyr ')' ';' {compiler.gen_print(VarType::I32);}
    | KPRINT_F32 '(' wyr ')' ';' {compiler.gen_print(VarType::F32);}
    | KPRINT_STRING '(' wyr ')' ';' {compiler.gen_print(VarType::U8_ARR);}
    | if_expr {;}
    ;

code_block
    : stmt ';' {;}
    | '{' stmt_list '}' {;}
    ;

wyr
	:wyr '+' skladnik	{compiler.gen_arithmetic('+');}
	|wyr '-' skladnik	{compiler.gen_arithmetic('-');}
	|skladnik		{; /*printf("B: wyrazenie pojedyncze \n"); */ }
	;
variable_decl
    :I32 assignment {compiler.gen_declare(VarType::I32, true);}
    |F32 assignment {compiler.gen_declare(VarType::I32, true);}
    |U8 '[' ']' assignment {compiler.gen_declare(VarType::U8_ARR, true) /* TODO: wrong order */;}
    |I32 ID {compiler.gen_declare(VarType::I32, false);}
    |F32 ID {compiler.gen_declare(VarType::F32, false);}
    ;
assignment
    :ID '=' wyr     { compiler.stack.push(ExprElemType::ID, $1);}
    ;
if_expr
	:if_begin code_block { compiler.gen_if_end(); }
	;
if_begin
    :KIF '(' cond_expr ')' {compiler.gen_if_begin();}
    ;
cond_expr
    : wyr EQ wyr  { compiler.set_cond_expr_op(CondExprOp::EQ ) ;};
    | wyr NEQ wyr { compiler.set_cond_expr_op(CondExprOp::NEQ) ;};
    | wyr '<' wyr { compiler.set_cond_expr_op(CondExprOp::LT ) ;};
    | wyr LEQ wyr { compiler.set_cond_expr_op(CondExprOp::LEQ) ;};
    | wyr '>' wyr { compiler.set_cond_expr_op(CondExprOp::GT ) ;};
    | wyr GEQ wyr { compiler.set_cond_expr_op(CondExprOp::EQ ) ;};
    ;
skladnik
	:skladnik '*' czynnik	{compiler.gen_arithmetic('*');}
	|skladnik '/' czynnik	{compiler.gen_arithmetic('/');}
	|czynnik		{;}
	;
czynnik
	:ID			{compiler.stack.push(ExprElemType::ID, $1);}
	|KINT		{compiler.stack.push(ExprElemType::NUMBER, std::to_string($1), VarType::I32);}
	|KFLOAT     {compiler.stack.push(ExprElemType::NUMBER, std::to_string($1), VarType::F32);}
	|STRING     {compiler.stack.push(ExprElemType::STRING_LITERAL, $1);}
	|'(' wyr ')'		{printf("B: wyrazenie w nawiasach\n");}
	;
%%
int main(int argc, char **argv) {
    yyparse();

    compiler.write_data_region(std::cout);
    compiler.write_text_region(std::cout);
    return 0;
}
