#!/bin/bash
flex --header-file=lex.yy.h lex_sql.l
`which bison` -d -b yacc_sql yacc_sql.y
