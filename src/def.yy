%{
#define INFILE_ERROR 1
#define OUTFILE_ERROR 2

#include "parser.hpp"

extern "C" int yyparse(void);
extern "C" int yyerror(const char *, ...);
extern "C" int yylex(void);


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
    | assignment ';' {; /* TODO: verify if left side variable is declared */}
    | KPRINT_I32 '(' wyr ')' ';' {gen_code_print(VarType::I32);}
    | KPRINT_F32 '(' wyr ')' ';' {gen_code_print(VarType::F32);}
    | KPRINT_STRING '(' wyr ')' ';' {gen_code_print(VarType::U8_ARR);}
    | if_expr {;}
    ;

code_block
    : stmt ';' {;}
    | '{' stmt_list '}' {;}
    ;

wyr
	:wyr '+' skladnik	{gen_code('+');}
	|wyr '-' skladnik	{gen_code('-');}
	|skladnik		{; /*printf("B: wyrazenie pojedyncze \n"); */ }
	;
variable_decl
    :I32 assignment {;}
    |F32 assignment {;}
    |U8 '[' ']' assignment { /* TODO: wrong order. fix  print_str(var) doesn't work*/;}
    |I32 ID {;}
    |F32 ID {;}
    ;
assignment
    :ID '=' wyr     { stack.push({$1, ExprElemType::ID}); gen_code('=');}
    ;
if_expr
	:if_begin code_block {gen_code_if_end();}
	;
if_begin
    :KIF '(' cond_expr ')' {gen_code_if_begin(cond_expr_op);}
    ;
cond_expr
    : wyr EQ wyr  { cond_expr_op = CondExprOp::EQ  ;};
    | wyr NEQ wyr { cond_expr_op = CondExprOp::NEQ ;};
    | wyr '<' wyr { cond_expr_op = CondExprOp::LT  ;};
    | wyr LEQ wyr { cond_expr_op = CondExprOp::LEQ ;};
    | wyr '>' wyr { cond_expr_op = CondExprOp::GT  ;};
    | wyr GEQ wyr { cond_expr_op = CondExprOp::EQ  ;};
    ;
skladnik
	:skladnik '*' czynnik	{gen_code('*');}
	|skladnik '/' czynnik	{gen_code('/');}
	|czynnik		{;}
	;
czynnik
	:ID			{push_id($1);}
	|KINT		{stack.push({std::to_string($1), ExprElemType::NUMBER, VarType::I32});}
	|KFLOAT     {stack.push({std::to_string($1), ExprElemType::NUMBER, VarType::F32});}
	|STRING     {push_string_literal($1);}
	|'(' wyr ')'		{printf("B: wyrazenie w nawiasach\n");}
	;
%%
//int main(int argc, char *argv[])
//{
//}
