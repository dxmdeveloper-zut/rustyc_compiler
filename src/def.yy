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
%token KRETURN KIF KELSE KFOR
%token GEQ LEQ EQ NEQ
%token KPRINT_I32 KPRINT_F32 KPRINT_STRING
%token DOTDOT DOTDOTEQ
%start stmt_list
%%

stmt_list
    : stmt {;}
    | stmt_list stmt {;}
    ;

stmt
    : variable_decl ';' {;}
    | assignment ';' {compiler.gen_assignment();}
    | KPRINT_I32 '(' wyr ')' ';' {compiler.gen_print(VarType::I32);}
    | KPRINT_F32 '(' wyr ')' ';' {compiler.gen_print(VarType::F32);}
    | KPRINT_STRING '(' wyr ')' ';' {compiler.gen_print(VarType::U8_ARR);}
    | if_expr {;}
    | for_expr {;}
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
    :I32 assignment {compiler.gen_declare(VarType::I32);}
    |F32 assignment {compiler.gen_declare(VarType::F32);}
    |U8 ID '[' ']' '=' wyr { compiler.stack.push_id($2, true); compiler.gen_declare(VarType::U8_ARR); }
    |I32 ID {compiler.gen_declare(VarType::I32, $2);}
    |F32 ID {compiler.gen_declare(VarType::F32, $2);}
    |I32 ID dim_decl {compiler.gen_declare(VarType::I32_ARR, $2);}
    |F32 ID dim_decl {compiler.gen_declare(VarType::F32_ARR, $2);}
    ;
dim_decl
    : '[' size_const ']' {;}
    ;
size_const
    : size_const ',' static_size_value {;}
    | static_size_value {;}
    ;
static_size_value
    : KINT { compiler.static_array_dims.push($1); }
    ;
assignment
    :ID '=' wyr         { compiler.stack.push_id($1, true); }
    |ID arr_idx '=' wyr { compiler.stack.push_id($1); compiler.gen_calc_arr_addr(false); }
    ;
if_expr
	:if_begin code_block { compiler.gen_if_end(); }
	|if_begin code_block else_begin code_block { compiler.gen_if_end(); }
	;
if_begin
    :KIF '(' cond_expr ')' {compiler.gen_if_begin();}
    ;
else_begin
    :KELSE { compiler.gen_else(); }
    ;
for_expr
    :for_begin code_block { compiler.gen_for_end(); }
    ;
for_begin
    :KFOR '(' for_cond ')' { compiler.gen_for_begin(); }
    ;
for_cond
    :I32 ID ':' wyr DOTDOT wyr {compiler.set_for_conditions($2, false);}
    |I32 ID ':' wyr DOTDOTEQ wyr {compiler.set_for_conditions($2, true);}
    |I32 ID ':' wyr DOTDOT wyr ':' KINT {compiler.set_for_conditions($2, false, $8);}
    |I32 ID ':' wyr DOTDOTEQ wyr ':' KINT {compiler.set_for_conditions($2, true, $8);}
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
	:ID			{compiler.stack.push_id($1);}
	|STRING     {compiler.stack.push(ExprElemType::STRING_LITERAL, $1);}
	|KINT		{compiler.stack.push($1);}
	|KFLOAT     {compiler.stack.push($1);}
	|ID arr_idx {compiler.stack.push_id($1); compiler.gen_calc_arr_addr(true);}
	|'(' wyr ')'		{printf("B: wyrazenie w nawiasach\n");}
	;
arr_idx
    : '[' arr_dim_idx ']' {;}
    ;
arr_dim_idx
    : arr_dim_idx ',' wyr { compiler.add_idx_to_arr_idx_stack();}
    | wyr { compiler.add_idx_to_arr_idx_stack();}
    ;
%%
int main(int argc, char **argv) {
    yyparse();

    if (argc <= 1){
        compiler.write_data_region(std::cout);
        compiler.write_text_region(std::cout);
    } else {
        std::ofstream outfile(argv[1]);
        if (!outfile.is_open()) {
            std::cerr << "Error opening output file: " << argv[1] << std::endl;
            return 1;
        }
        compiler.write_data_region(outfile);
        compiler.write_text_region(outfile);
    }

    return 0;
}
