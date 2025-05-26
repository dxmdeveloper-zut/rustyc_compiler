%{
#include <stdlib.h>
#include <string.h>
#include "def.tab.hh"
#define INFILE_ERROR 1
#define OUTFILE_ERROR 2
extern int yylineno;
int yyerror(char *,...);
%}
%%

\r  {;}


\,  {return ',';}
\#  {return '#';}

\:  {return ':';}
\;  {return ';';}

if  {return KIF;}
for {return KFOR;}

\(  {return '(';}
\)  {return ')';}
\[  {return '[';}
\]  {return ']';}
\{  {return '{';}
\}  {return '}';}

\>      {return '>';}
\<      {return '<';}
\>\=    {return GEQ;}
\<\=    {return LEQ;}
\=\=    {return EQ;}
\!\=    {return NEQ;}


([1-9][0-9]*)|0		{
				yylval.ival = atoi(yytext);
				return KINT;
			}	


([1-9][0-9]*|0)\.[0-9]+     {
                yylval.fval = atof(yytext);
                return KFLOAT;
            }

u0  {return U0;}
u8  {return U8;}
i32 {return I32;}
f32 {return F32;}

print_i32 {return KPRINT_I32;}
print_f32 {return KPRINT_F32;}
print_str {return KPRINT_STRING;}

return  {return KRETURN;}


\+			{return '+';} 
\*			{return '*';}
\/			{return '/';}
\-			{return '-';}
\=			{return '=';}

\".*\"  {yylval.text = strdup(yytext); return STRING;}



[A-Za-z_][A-Za-z0-9_]*	{
				yylval.text = strdup(yytext);
				return ID;
			}

\ |\t			{;}
\n			{yylineno++;}
.			{yyerror("Blad leksykalny\n");}



\/\/.*       {;}



%%
int yyerror(char *msg,...)
{
	printf("%d: %s", yylineno, msg);
	exit(1);
}

