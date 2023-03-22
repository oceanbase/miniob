
%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

#include "common/log/log.h"
#include "common/lang/string.h"
#include "sql/parser/parse_defs.h"
#include "sql/parser/yacc_sql.hpp"
#include "sql/parser/lex_sql.h"


int yyerror(YYLTYPE *llocp, ParsedSqlResult *sql_result, yyscan_t scanner, const char *msg)
{
  std::unique_ptr<Command> error_cmd = std::make_unique<Command>(SCF_ERROR);
  error_cmd->error.error_msg = msg;
  error_cmd->error.line = llocp->first_line;
  error_cmd->error.column = llocp->first_column;
  sql_result->add_command(std::move(error_cmd));
  return 0;
}

%}

%define api.pure full
%define parse.error verbose
%locations
%lex-param { yyscan_t scanner }
%parse-param { ParsedSqlResult * sql_result }
%parse-param { void * scanner }

//标识tokens
%token  SEMICOLON
        CREATE
        DROP
        TABLE
        TABLES
        INDEX
        SELECT
        DESC
        SHOW
        SYNC
        INSERT
        DELETE
        UPDATE
        LBRACE
        RBRACE
        COMMA
        TRX_BEGIN
        TRX_COMMIT
        TRX_ROLLBACK
        INT_T
        STRING_T
        FLOAT_T
        HELP
        EXIT
        DOT //QUOTE
        INTO
        VALUES
        FROM
        WHERE
        AND
        SET
        ON
        LOAD
        DATA
        INFILE
        EXPLAIN
        EQ
        LT
        GT
        LE
        GE
        NE

%union {
  Command *command;
  Condition *condition;
  Value *value;
  enum CompOp comp;
  RelAttr *rel_attr;
  std::vector<AttrInfo> *attr_infos;
  AttrInfo *attr_info;
  std::vector<Value> *value_list;
  std::vector<Condition> *condition_list;
  std::vector<RelAttr> *rel_attr_list;
  std::vector<std::string> *relation_list;
  char *string;
  int number;
  float floats;
}

%token <number> NUMBER
%token <floats> FLOAT 
%token <string> ID
%token <string> PATH
%token <string> SSS
%token <string> STAR
%token <string> STRING_V
//非终结符

%type <number>              type
%type <condition>           condition
%type <value>               value
%type <number>              number
%type <comp>                comp_op
%type <rel_attr>            rel_attr
%type <attr_infos>          attr_def_list
%type <attr_info>           attr_def
%type <value_list>          value_list
%type <condition_list>      where
%type <condition_list>      condition_list
%type <rel_attr_list>       select_attr
%type <relation_list>       rel_list
%type <rel_attr_list>       attr_list
%type <command> select
%type <command> insert
%type <command> update
%type <command> delete
%type <command> create_table
%type <command> drop_table
%type <command> show_tables
%type <command> desc_table
%type <command> create_index
%type <command> drop_index
%type <command> sync
%type <command> begin
%type <command> commit
%type <command> rollback
%type <command> load_data
%type <command> explain
%type <command> help
%type <command> exit
%type <command> command_wrapper
// commands should be a list but I use a single command instead
%type <command> commands
%%

commands: command_wrapper opt_semicolon  //commands or sqls. parser starts here.
  {
    std::unique_ptr<Command> sql_command = std::unique_ptr<Command>($1);
    sql_result->add_command(std::move(sql_command));
  }
  ;

command_wrapper:
    select  
  | insert
  | update
  | delete
  | create_table
  | drop_table
  | show_tables
  | desc_table
  | create_index  
  | drop_index
  | sync
  | begin
  | commit
  | rollback
  | load_data
  | explain
  | help
  | exit
    ;

exit:      
    EXIT {
      $$ = new Command(SCF_EXIT);
    };

help:
    HELP {
      $$ = new Command(SCF_HELP);
    };

sync:
    SYNC {
      $$ = new Command(SCF_SYNC);
    }
    ;

begin:
    TRX_BEGIN  {
      $$ = new Command(SCF_BEGIN);
    }
    ;

commit:
    TRX_COMMIT {
      $$ = new Command(SCF_COMMIT);
    }
    ;

rollback:
    TRX_ROLLBACK  {
      $$ = new Command(SCF_ROLLBACK);
    }
    ;

drop_table:    /*drop table 语句的语法解析树*/
    DROP TABLE ID {
      $$ = new Command(SCF_DROP_TABLE);
      $$->drop_table.relation_name = $3;
      free($3);
    };

show_tables:
    SHOW TABLES {
      $$ = new Command(SCF_SHOW_TABLES);
    }
    ;

desc_table:
    DESC ID  {
      $$ = new Command(SCF_DESC_TABLE);
      $$->desc_table.relation_name = $2;
      free($2);
    }
    ;

create_index:    /*create index 语句的语法解析树*/
    CREATE INDEX ID ON ID LBRACE ID RBRACE
    {
      $$ = new Command(SCF_CREATE_INDEX);
      CreateIndex &create_index = $$->create_index;
      create_index.index_name = $3;
      create_index.relation_name = $5;
      create_index.attribute_name = $7;
      free($3);
      free($5);
      free($7);
    }
    ;

drop_index:      /*drop index 语句的语法解析树*/
    DROP INDEX ID ON ID
    {
      $$ = new Command(SCF_DROP_INDEX);
      $$->drop_index.index_name = $3;
      $$->drop_index.relation_name = $5;
      free($3);
      free($5);
    }
    ;
create_table:    /*create table 语句的语法解析树*/
    CREATE TABLE ID LBRACE attr_def attr_def_list RBRACE
    {
      $$ = new Command(SCF_CREATE_TABLE);
      CreateTable &create_table = $$->create_table;
      create_table.relation_name = $3;
      free($3);

      std::vector<AttrInfo> *src_attrs = $6;

      if (src_attrs != nullptr) {
        create_table.attr_infos.swap(*src_attrs);
      }
      create_table.attr_infos.emplace_back(*$5);
      std::reverse(create_table.attr_infos.begin(), create_table.attr_infos.end());
      delete $5;
    }
    ;
attr_def_list:
    /* empty */
    {
      $$ = nullptr;
    }
    | COMMA attr_def attr_def_list
    {
      if ($3 != nullptr) {
        $$ = $3;
      } else {
        $$ = new std::vector<AttrInfo>;
      }
      $$->emplace_back(*$2);
      delete $2;
    }
    ;
    
attr_def:
    ID type LBRACE number RBRACE 
    {
      $$ = new AttrInfo;
      $$->type = (AttrType)$2;
      $$->name = $1;
      $$->length = $4;
      free($1);
    }
    | ID type
    {
      $$ = new AttrInfo;
      $$->type = (AttrType)$2;
      $$->name = $1;
      $$->length = 4;
      free($1);
    }
    ;
number:
    NUMBER {$$ = $1;}
    ;
type:
    INT_T      { $$=INTS; }
    | STRING_T { $$=CHARS; }
    | FLOAT_T  { $$=FLOATS; }
    ;
insert:        /*insert   语句的语法解析树*/
    INSERT INTO ID VALUES LBRACE value value_list RBRACE 
    {
      $$ = new Command(SCF_INSERT);
      $$->insertion.relation_name = $3;
      if ($7 != nullptr) {
        $$->insertion.values.swap(*$7);
      }
      $$->insertion.values.emplace_back(*$6);
      std::reverse($$->insertion.values.begin(), $$->insertion.values.end());
      delete $6;
      free($3);
    }
    ;

value_list:
    /* empty */
    {
      $$ = nullptr;
    }
    | COMMA value value_list  { 
      if ($3 != nullptr) {
        $$ = $3;
      } else {
        $$ = new std::vector<Value>;
      }
      $$->emplace_back(*$2);
      delete $2;
    }
    ;
value:
    NUMBER {
      $$ = new Value;
      $$->type = INTS;
      $$->int_value = $1;
    }
    |FLOAT {
      $$ = new Value;
      $$->type = FLOATS;
      $$->float_value = $1;
    }
    |SSS {
      char *tmp = common::substr($1,1,strlen($1)-2);
      $$ = new Value;
      $$->type = CHARS;
      $$->string_value = tmp;
      free(tmp);
    }
    ;
    
delete:    /*  delete 语句的语法解析树*/
    DELETE FROM ID where 
    {
      $$ = new Command(SCF_DELETE);
      $$->deletion.relation_name = $3;
      if ($4 != nullptr) {
        $$->deletion.conditions.swap(*$4);
        delete $4;
      }
      free($3);
    }
    ;
update:      /*  update 语句的语法解析树*/
    UPDATE ID SET ID EQ value where 
    {
      $$ = new Command(SCF_UPDATE);
      $$->update.relation_name = $2;
      $$->update.attribute_name = $4;
      $$->update.value = *$6;
      if ($7 != nullptr) {
        $$->update.conditions.swap(*$7);
        delete $7;
      }
      free($2);
      free($4);
    }
    ;
select:        /*  select 语句的语法解析树*/
    SELECT select_attr FROM ID rel_list where
    {
      $$ = new Command(SCF_SELECT);
      if ($2 != nullptr) {
        $$->selection.attributes.swap(*$2);
        delete $2;
      }
      if ($5 != nullptr) {
        $$->selection.relations.swap(*$5);
        delete $5;
      }
      $$->selection.relations.push_back($4);
      std::reverse($$->selection.relations.begin(), $$->selection.relations.end());

      if ($6 != nullptr) {
        $$->selection.conditions.swap(*$6);
        delete $6;
      }
      free($4);
    }
    ;

select_attr:
    STAR {
      $$ = new std::vector<RelAttr>;
      RelAttr attr;
      attr.relation_name = "";
      attr.attribute_name = "*";
      $$->emplace_back(attr);
    }
    | rel_attr attr_list {
      if ($2 != nullptr) {
        $$ = $2;
      } else {
        $$ = new std::vector<RelAttr>;
      }
      $$->emplace_back(*$1);
      delete $1;
    }
    ;

rel_attr:
    ID {
      $$ = new RelAttr;
      $$->attribute_name = $1;
      free($1);
    }
    | ID DOT ID {
      $$ = new RelAttr;
      $$->relation_name = $1;
      $$->attribute_name = $3;
      free($1);
      free($3);
    }
    ;

attr_list:
    /* empty */
    {
      $$ = nullptr;
    }
    | COMMA rel_attr attr_list {
      if ($3 != nullptr) {
        $$ = $3;
      } else {
        $$ = new std::vector<RelAttr>;
      }

      $$->emplace_back(*$2);
      delete $2;
    }
    ;

rel_list:
    /* empty */
    {
      $$ = nullptr;
    }
    | COMMA ID rel_list {
      if ($3 != nullptr) {
        $$ = $3;
      } else {
        $$ = new std::vector<std::string>;
      }

      $$->push_back($2);
      free($2);
    }
    ;
where:
    /* empty */
    {
      $$ = nullptr;
    }
    | WHERE condition_list {
      $$ = $2;  
    }
    ;
condition_list:
    /* empty */
    {
      $$ = nullptr;
    }
    | condition {
      $$ = new std::vector<Condition>;
      $$->emplace_back(*$1);
      delete $1;
    }
    | condition AND condition_list {
      $$ = $3;
      $$->emplace_back(*$1);
      delete $1;
    }
    ;
condition:
    rel_attr comp_op value
    {
      $$ = new Condition;
      $$->left_is_attr = 1;
      $$->left_attr = *$1;
      $$->right_is_attr = 0;
      $$->right_value = *$3;
      $$->comp = $2;

      delete $1;
      delete $3;
    }
    | value comp_op value 
    {
      $$ = new Condition;
      $$->left_is_attr = 0;
      $$->left_value = *$1;
      $$->right_is_attr = 0;
      $$->right_value = *$3;
      $$->comp = $2;

      delete $1;
      delete $3;
    }
    | rel_attr comp_op rel_attr
    {
      $$ = new Condition;
      $$->left_is_attr = 1;
      $$->left_attr = *$1;
      $$->right_is_attr = 1;
      $$->right_attr = *$3;
      $$->comp = $2;

      delete $1;
      delete $3;
    }
    | value comp_op rel_attr
    {
      $$ = new Condition;
      $$->left_is_attr = 0;
      $$->left_value = *$1;
      $$->right_is_attr = 1;
      $$->right_attr = *$3;
      $$->comp = $2;

      delete $1;
      delete $3;
    }
    ;

comp_op:
      EQ { $$ = EQUAL_TO; }
    | LT { $$ = LESS_THAN; }
    | GT { $$ = GREAT_THAN; }
    | LE { $$ = LESS_EQUAL; }
    | GE { $$ = GREAT_EQUAL; }
    | NE { $$ = NOT_EQUAL; }
    ;

load_data:
    LOAD DATA INFILE SSS INTO TABLE ID 
    {
      $$ = new Command(SCF_LOAD_DATA);
      $$->load_data.relation_name = $7;
      $$->load_data.file_name = $4;
      free($7);
    }
    ;

explain:
    EXPLAIN command_wrapper
    {
      $$ = new Command(SCF_EXPLAIN);
      $$->explain.cmd = std::unique_ptr<Command>($2);
    }
    ;

opt_semicolon: /*empty*/
    | SEMICOLON
    ;
%%
//_____________________________________________________________________
extern void scan_string(const char *str, yyscan_t scanner);

int sql_parse(const char *s, ParsedSqlResult *sql_result){
  yyscan_t scanner;
  yylex_init(&scanner);
  scan_string(s, scanner);
  int result = yyparse(sql_result, scanner);
  yylex_destroy(scanner);
  return result;
}
