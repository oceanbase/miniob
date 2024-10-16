%top{
/*
这里的代码会被复制到lex_sql.cpp的最开始位置
定义yy_size_t的原因是因为flex生成的代码，会使用yy_size_t与其他类型的数字
做比较，导致编译报警
*/

#define YY_TYPEDEF_YY_SIZE_T
typedef int yy_size_t;

/* 参考生成的lex_sql.cpp代码，这个宏定义会放在每次运行yylex()最开始的地方 */
#define YY_USER_INIT                                         \
  yycolumn = 0;

/* 参考生成的lex_sql.cpp代码，这个宏定义会放在解析一个token之后，也可以在网上找到大量的参考资料 */
/* 我们在这里设置当前解析的token的位置信息，这样在yacc中就可以使用这些信息了 */
#define YY_USER_ACTION                                       \
do {                                                         \
  yylloc->first_line   = yylloc->last_line = yylineno;       \
  yylloc->first_column = yycolumn;                           \
  yylloc->last_column  = yylloc->first_column + yyleng - 1;  \
  yycolumn += yyleng;                                        \
}                                                            \
while (0);

}

%{
#include<string.h>
#include<stdio.h>

bool pre_token = 1;  // 0表示前面不是 左括号、逗号、比较符、where等会引起当前的 - 被视为负号； 默认为负号

/**
 * flex 代码包含三个部分，使用 %% 分隔
 * 第一个部分的代码是C代码，flex会原样复制到目标文件中
 * 第二个部分是规则部分，使用正则表达式定义一系列规则
 * 第三个部分还是C代码，flex 会复制此段代码
 */

#include "sql/parser/parse_defs.h"
#include "common/log/log.h"
#include "yacc_sql.hpp"

#ifndef register
#define register 
#endif // register

extern int atoi();
extern double atof();

#define RETURN_TOKEN(token) LOG_DEBUG("%s", #token);return token
%}

/* Prevent the need for linking with -lfl */
%option noyywrap
%option nounput
%option noinput
%option bison-bridge
%option reentrant
/* 不区分大小写 */
%option case-insensitive
%option bison-locations

WHITE_SAPCE      [\ \t\b\f]
DIGIT            [0-9]+

ID               [A-Za-z_]+[A-Za-z0-9_]*
DOT              \.
QUOTE            [\'\"]
%x STR

/* 规则匹配的优先级：*/
/* 1. 匹配的规则长的优先 */
/* 2. 写在最前面的优先 */

/* yylval 就可以认为是 yacc 中 %union 定义的结构体(union 结构) */
/*   */

%%

{WHITE_SAPCE}                                                          
\n;

"-"                                     {
                                            if(pre_token == 0) {  
                                                yylval->string = strdup(yytext); 
                                                return yytext[0];
                                            }
                                            else {
                                            }
                                        }

[\-]?{DIGIT}+                           {
                                            pre_token = 0;              
                                            yylval->number=atoi(yytext);
                                            RETURN_TOKEN(NUMBER);
                                        }

[\-]?{DIGIT}+{DOT}{DIGIT}+              {
                                            pre_token = 0;
                                            yylval->floats=(float)(atof(yytext)); 
                                            RETURN_TOKEN(FLOAT);
                                        }

";"                                     pre_token = 0;  RETURN_TOKEN(SEMICOLON);
MAX                                     pre_token = 0;  RETURN_TOKEN(MAX);
SUM                                     pre_token = 0;  RETURN_TOKEN(SUM);
MIN                                     pre_token = 0;  RETURN_TOKEN(MIN);
AVG                                     pre_token = 0;  RETURN_TOKEN(AVG);
COUNT                                   pre_token = 0;  RETURN_TOKEN(COUNT);
{DOT}                                   pre_token = 0;  RETURN_TOKEN(DOT);
EXIT                                    pre_token = 0;  RETURN_TOKEN(EXIT);
HELP                                    pre_token = 0;  RETURN_TOKEN(HELP);
DESC                                    pre_token = 0;  RETURN_TOKEN(DESC);
CREATE                                  pre_token = 0;  RETURN_TOKEN(CREATE);
DROP                                    pre_token = 0;  RETURN_TOKEN(DROP);
TABLE                                   pre_token = 0;  RETURN_TOKEN(TABLE);
TABLES                                  pre_token = 0;  RETURN_TOKEN(TABLES);
INDEX                                   pre_token = 0;  RETURN_TOKEN(INDEX);
ON                                      pre_token = 0;  RETURN_TOKEN(ON);
SHOW                                    pre_token = 0;  RETURN_TOKEN(SHOW);
SYNC                                    pre_token = 0;  RETURN_TOKEN(SYNC);
SELECT                                  pre_token = 0;  RETURN_TOKEN(SELECT);
CALC                                    pre_token = 0;  RETURN_TOKEN(CALC);
FROM                                    pre_token = 0;  RETURN_TOKEN(FROM);
WHERE                                   pre_token = 0;  RETURN_TOKEN(WHERE);
AND                                     pre_token = 0;  RETURN_TOKEN(AND);
INSERT                                  pre_token = 0;  RETURN_TOKEN(INSERT);
INTO                                    pre_token = 0;  RETURN_TOKEN(INTO);
VALUES                                  pre_token = 0;  RETURN_TOKEN(VALUES);
DELETE                                  pre_token = 0;  RETURN_TOKEN(DELETE);
UPDATE                                  pre_token = 0;  RETURN_TOKEN(UPDATE);
SET                                     pre_token = 0;  RETURN_TOKEN(SET);
BEGIN                                   pre_token = 0;  RETURN_TOKEN(TRX_BEGIN);
COMMIT                                  pre_token = 0;  RETURN_TOKEN(TRX_COMMIT);
ROLLBACK                                pre_token = 0;  RETURN_TOKEN(TRX_ROLLBACK);
INT                                     pre_token = 0;  RETURN_TOKEN(INT_T);
CHAR                                    pre_token = 0;  RETURN_TOKEN(STRING_T);
FLOAT                                   pre_token = 0;  RETURN_TOKEN(FLOAT_T);
DATE                                    pre_token = 0;  RETURN_TOKEN(DATE_T);
LOAD                                    pre_token = 0;  RETURN_TOKEN(LOAD);
DATA                                    pre_token = 0;  RETURN_TOKEN(DATA);
INFILE                                  pre_token = 0;  RETURN_TOKEN(INFILE);
EXPLAIN                                 pre_token = 0;  RETURN_TOKEN(EXPLAIN);
GROUP                                   pre_token = 0;  RETURN_TOKEN(GROUP);
BY                                      pre_token = 0;  RETURN_TOKEN(BY);
STORAGE                                 pre_token = 0;  RETURN_TOKEN(STORAGE);
FORMAT                                  pre_token = 0;  RETURN_TOKEN(FORMAT);
{ID}                                    pre_token = 0;  yylval->string=strdup(yytext); RETURN_TOKEN(ID);
"("                                     pre_token = 1;  RETURN_TOKEN(LBRACE);
")"                                     pre_token = 0;  RETURN_TOKEN(RBRACE);

","                                     pre_token = 1;  RETURN_TOKEN(COMMA);
"="                                     pre_token = 1;  RETURN_TOKEN(EQ);
"<="                                    pre_token = 1;  RETURN_TOKEN(LE);
"<>"                                    pre_token = 1;  RETURN_TOKEN(NE);
"!="                                    pre_token = 1;  RETURN_TOKEN(NE);
"<"                                     pre_token = 1;  RETURN_TOKEN(LT);
">="                                    pre_token = 1;  RETURN_TOKEN(GE);
">"                                     pre_token = 1;  RETURN_TOKEN(GT);

"+" |
"*" |
"/"                                     {pre_token = 0;   return yytext[0]; }
\"[^"]*\"                               pre_token = 0;  yylval->string = strdup(yytext); RETURN_TOKEN(SSS);
'[^']*\'                                pre_token = 0;  yylval->string = strdup(yytext); RETURN_TOKEN(SSS);

.                                       pre_token = 0;  LOG_DEBUG("Unknown character [%c]",yytext[0]); return yytext[0];

%%

void scan_string(const char *str, yyscan_t scanner) {
  yy_switch_to_buffer(yy_scan_string(str, scanner), scanner);
}