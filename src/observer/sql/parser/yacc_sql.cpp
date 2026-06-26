/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 2

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 2 "src/observer/sql/parser/yacc_sql.y"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iterator>

#include "common/log/log.h"
#include "common/lang/string.h"
#include "sql/parser/parse_defs.h"
#include "sql/parser/yacc_sql.hpp"
#include "sql/parser/lex_sql.h"
#include "sql/expr/expression.h"

using namespace std;

string token_name(const char *sql_string, YYLTYPE *llocp)
{
  return string(sql_string + llocp->first_column, llocp->last_column - llocp->first_column + 1);
}

int yyerror(YYLTYPE *llocp, const char *sql_string, ParsedSqlResult *sql_result, yyscan_t scanner, const char *msg)
{
  unique_ptr<ParsedSqlNode> error_sql_node = make_unique<ParsedSqlNode>(SCF_ERROR);
  error_sql_node->error.error_msg = msg;
  error_sql_node->error.line = llocp->first_line;
  error_sql_node->error.column = llocp->first_column;
  sql_result->add_sql_node(std::move(error_sql_node));
  return 0;
}

ArithmeticExpr *create_arithmetic_expression(ArithmeticExpr::Type type,
                                             Expression *left,
                                             Expression *right,
                                             const char *sql_string,
                                             YYLTYPE *llocp)
{
  ArithmeticExpr *expr = new ArithmeticExpr(type, left, right);
  expr->set_name(token_name(sql_string, llocp));
  return expr;
}

UnboundAggregateExpr *create_aggregate_expression(const char *aggregate_name,
                                           Expression *child,
                                           const char *sql_string,
                                           YYLTYPE *llocp)
{
  UnboundAggregateExpr *expr = new UnboundAggregateExpr(aggregate_name, child);
  expr->set_name(token_name(sql_string, llocp));
  return expr;
}


#line 125 "src/observer/sql/parser/yacc_sql.cpp"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "yacc_sql.hpp"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_SEMICOLON = 3,                  /* SEMICOLON  */
  YYSYMBOL_BY = 4,                         /* BY  */
  YYSYMBOL_CREATE = 5,                     /* CREATE  */
  YYSYMBOL_DROP = 6,                       /* DROP  */
  YYSYMBOL_GROUP = 7,                      /* GROUP  */
  YYSYMBOL_TABLE = 8,                      /* TABLE  */
  YYSYMBOL_TABLES = 9,                     /* TABLES  */
  YYSYMBOL_INDEX = 10,                     /* INDEX  */
  YYSYMBOL_CALC = 11,                      /* CALC  */
  YYSYMBOL_SELECT = 12,                    /* SELECT  */
  YYSYMBOL_DESC = 13,                      /* DESC  */
  YYSYMBOL_SHOW = 14,                      /* SHOW  */
  YYSYMBOL_SYNC = 15,                      /* SYNC  */
  YYSYMBOL_INSERT = 16,                    /* INSERT  */
  YYSYMBOL_DELETE = 17,                    /* DELETE  */
  YYSYMBOL_UPDATE = 18,                    /* UPDATE  */
  YYSYMBOL_LBRACE = 19,                    /* LBRACE  */
  YYSYMBOL_RBRACE = 20,                    /* RBRACE  */
  YYSYMBOL_COMMA = 21,                     /* COMMA  */
  YYSYMBOL_TRX_BEGIN = 22,                 /* TRX_BEGIN  */
  YYSYMBOL_TRX_COMMIT = 23,                /* TRX_COMMIT  */
  YYSYMBOL_TRX_ROLLBACK = 24,              /* TRX_ROLLBACK  */
  YYSYMBOL_INT_T = 25,                     /* INT_T  */
  YYSYMBOL_STRING_T = 26,                  /* STRING_T  */
  YYSYMBOL_FLOAT_T = 27,                   /* FLOAT_T  */
  YYSYMBOL_DATE_T = 28,                    /* DATE_T  */
  YYSYMBOL_VECTOR_T = 29,                  /* VECTOR_T  */
  YYSYMBOL_TEXT_T = 30,                    /* TEXT_T  */
  YYSYMBOL_INNER = 31,                     /* INNER  */
  YYSYMBOL_JOIN = 32,                      /* JOIN  */
  YYSYMBOL_HELP = 33,                      /* HELP  */
  YYSYMBOL_EXIT = 34,                      /* EXIT  */
  YYSYMBOL_DOT = 35,                       /* DOT  */
  YYSYMBOL_INTO = 36,                      /* INTO  */
  YYSYMBOL_VALUES = 37,                    /* VALUES  */
  YYSYMBOL_FROM = 38,                      /* FROM  */
  YYSYMBOL_WHERE = 39,                     /* WHERE  */
  YYSYMBOL_AND = 40,                       /* AND  */
  YYSYMBOL_SET = 41,                       /* SET  */
  YYSYMBOL_ON = 42,                        /* ON  */
  YYSYMBOL_LOAD = 43,                      /* LOAD  */
  YYSYMBOL_DATA = 44,                      /* DATA  */
  YYSYMBOL_INFILE = 45,                    /* INFILE  */
  YYSYMBOL_EXPLAIN = 46,                   /* EXPLAIN  */
  YYSYMBOL_STORAGE = 47,                   /* STORAGE  */
  YYSYMBOL_FORMAT = 48,                    /* FORMAT  */
  YYSYMBOL_PRIMARY = 49,                   /* PRIMARY  */
  YYSYMBOL_KEY = 50,                       /* KEY  */
  YYSYMBOL_ANALYZE = 51,                   /* ANALYZE  */
  YYSYMBOL_FIELDS = 52,                    /* FIELDS  */
  YYSYMBOL_TERMINATED = 53,                /* TERMINATED  */
  YYSYMBOL_ENCLOSED = 54,                  /* ENCLOSED  */
  YYSYMBOL_EQ = 55,                        /* EQ  */
  YYSYMBOL_LT = 56,                        /* LT  */
  YYSYMBOL_GT = 57,                        /* GT  */
  YYSYMBOL_LE = 58,                        /* LE  */
  YYSYMBOL_GE = 59,                        /* GE  */
  YYSYMBOL_NE = 60,                        /* NE  */
  YYSYMBOL_NUMBER = 61,                    /* NUMBER  */
  YYSYMBOL_FLOAT = 62,                     /* FLOAT  */
  YYSYMBOL_ID = 63,                        /* ID  */
  YYSYMBOL_SSS = 64,                       /* SSS  */
  YYSYMBOL_65_ = 65,                       /* '+'  */
  YYSYMBOL_66_ = 66,                       /* '-'  */
  YYSYMBOL_67_ = 67,                       /* '*'  */
  YYSYMBOL_68_ = 68,                       /* '/'  */
  YYSYMBOL_UMINUS = 69,                    /* UMINUS  */
  YYSYMBOL_YYACCEPT = 70,                  /* $accept  */
  YYSYMBOL_commands = 71,                  /* commands  */
  YYSYMBOL_command_wrapper = 72,           /* command_wrapper  */
  YYSYMBOL_exit_stmt = 73,                 /* exit_stmt  */
  YYSYMBOL_help_stmt = 74,                 /* help_stmt  */
  YYSYMBOL_sync_stmt = 75,                 /* sync_stmt  */
  YYSYMBOL_begin_stmt = 76,                /* begin_stmt  */
  YYSYMBOL_commit_stmt = 77,               /* commit_stmt  */
  YYSYMBOL_rollback_stmt = 78,             /* rollback_stmt  */
  YYSYMBOL_drop_table_stmt = 79,           /* drop_table_stmt  */
  YYSYMBOL_analyze_table_stmt = 80,        /* analyze_table_stmt  */
  YYSYMBOL_show_tables_stmt = 81,          /* show_tables_stmt  */
  YYSYMBOL_desc_table_stmt = 82,           /* desc_table_stmt  */
  YYSYMBOL_create_index_stmt = 83,         /* create_index_stmt  */
  YYSYMBOL_drop_index_stmt = 84,           /* drop_index_stmt  */
  YYSYMBOL_create_table_stmt = 85,         /* create_table_stmt  */
  YYSYMBOL_attr_def_list = 86,             /* attr_def_list  */
  YYSYMBOL_attr_def = 87,                  /* attr_def  */
  YYSYMBOL_number = 88,                    /* number  */
  YYSYMBOL_type = 89,                      /* type  */
  YYSYMBOL_primary_key = 90,               /* primary_key  */
  YYSYMBOL_attr_list = 91,                 /* attr_list  */
  YYSYMBOL_insert_stmt = 92,               /* insert_stmt  */
  YYSYMBOL_value_list = 93,                /* value_list  */
  YYSYMBOL_value = 94,                     /* value  */
  YYSYMBOL_storage_format = 95,            /* storage_format  */
  YYSYMBOL_delete_stmt = 96,               /* delete_stmt  */
  YYSYMBOL_update_stmt = 97,               /* update_stmt  */
  YYSYMBOL_select_stmt = 98,               /* select_stmt  */
  YYSYMBOL_from_clause = 99,               /* from_clause  */
  YYSYMBOL_joined_relation = 100,          /* joined_relation  */
  YYSYMBOL_calc_stmt = 101,                /* calc_stmt  */
  YYSYMBOL_expression_list = 102,          /* expression_list  */
  YYSYMBOL_expression = 103,               /* expression  */
  YYSYMBOL_aggregate_expression = 104,     /* aggregate_expression  */
  YYSYMBOL_rel_attr = 105,                 /* rel_attr  */
  YYSYMBOL_relation = 106,                 /* relation  */
  YYSYMBOL_where = 107,                    /* where  */
  YYSYMBOL_condition_list = 108,           /* condition_list  */
  YYSYMBOL_condition = 109,                /* condition  */
  YYSYMBOL_comp_op = 110,                  /* comp_op  */
  YYSYMBOL_group_by = 111,                 /* group_by  */
  YYSYMBOL_load_data_stmt = 112,           /* load_data_stmt  */
  YYSYMBOL_fields_terminated_by = 113,     /* fields_terminated_by  */
  YYSYMBOL_enclosed_by = 114,              /* enclosed_by  */
  YYSYMBOL_explain_stmt = 115,             /* explain_stmt  */
  YYSYMBOL_set_variable_stmt = 116,        /* set_variable_stmt  */
  YYSYMBOL_opt_semicolon = 117             /* opt_semicolon  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if 1

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) \
             + YYSIZEOF (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  69
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   179

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  70
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  48
/* YYNRULES -- Number of rules.  */
#define YYNRULES  109
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  202

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   320


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,    67,    65,     2,    66,     2,    68,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      69
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   222,   222,   230,   231,   232,   233,   234,   235,   236,
     237,   238,   239,   240,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   250,   254,   260,   265,   271,   277,   283,
     289,   295,   302,   308,   315,   326,   334,   355,   361,   370,
     377,   386,   389,   390,   391,   392,   393,   394,   398,   401,
     408,   412,   424,   434,   440,   447,   451,   455,   463,   466,
     473,   484,   498,   528,   532,   547,   552,   566,   575,   580,
     591,   594,   597,   600,   603,   607,   610,   613,   618,   624,
     630,   636,   640,   648,   670,   673,   679,   682,   687,   694,
     706,   718,   730,   745,   746,   747,   748,   749,   750,   756,
     759,   767,   790,   793,   800,   803,   809,   817,   826,   827
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "SEMICOLON", "BY",
  "CREATE", "DROP", "GROUP", "TABLE", "TABLES", "INDEX", "CALC", "SELECT",
  "DESC", "SHOW", "SYNC", "INSERT", "DELETE", "UPDATE", "LBRACE", "RBRACE",
  "COMMA", "TRX_BEGIN", "TRX_COMMIT", "TRX_ROLLBACK", "INT_T", "STRING_T",
  "FLOAT_T", "DATE_T", "VECTOR_T", "TEXT_T", "INNER", "JOIN", "HELP",
  "EXIT", "DOT", "INTO", "VALUES", "FROM", "WHERE", "AND", "SET", "ON",
  "LOAD", "DATA", "INFILE", "EXPLAIN", "STORAGE", "FORMAT", "PRIMARY",
  "KEY", "ANALYZE", "FIELDS", "TERMINATED", "ENCLOSED", "EQ", "LT", "GT",
  "LE", "GE", "NE", "NUMBER", "FLOAT", "ID", "SSS", "'+'", "'-'", "'*'",
  "'/'", "UMINUS", "$accept", "commands", "command_wrapper", "exit_stmt",
  "help_stmt", "sync_stmt", "begin_stmt", "commit_stmt", "rollback_stmt",
  "drop_table_stmt", "analyze_table_stmt", "show_tables_stmt",
  "desc_table_stmt", "create_index_stmt", "drop_index_stmt",
  "create_table_stmt", "attr_def_list", "attr_def", "number", "type",
  "primary_key", "attr_list", "insert_stmt", "value_list", "value",
  "storage_format", "delete_stmt", "update_stmt", "select_stmt",
  "from_clause", "joined_relation", "calc_stmt", "expression_list",
  "expression", "aggregate_expression", "rel_attr", "relation", "where",
  "condition_list", "condition", "comp_op", "group_by", "load_data_stmt",
  "fields_terminated_by", "enclosed_by", "explain_stmt",
  "set_variable_stmt", "opt_semicolon", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-151)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      82,    23,    33,   -11,   -11,   -44,    -3,  -151,     6,   -14,
     -31,  -151,  -151,  -151,  -151,  -151,    -2,    -6,    82,    41,
      63,    59,  -151,  -151,  -151,  -151,  -151,  -151,  -151,  -151,
    -151,  -151,  -151,  -151,  -151,  -151,  -151,  -151,  -151,  -151,
    -151,  -151,  -151,     8,     9,    12,    20,   -11,  -151,  -151,
     -10,  -151,   -11,  -151,  -151,  -151,    14,  -151,  -151,    46,
    -151,  -151,    22,    27,    45,    34,    56,  -151,    28,  -151,
    -151,  -151,    83,    69,  -151,    70,    -8,   -11,    50,  -151,
     -11,   -11,   -11,   -11,   -11,    51,    87,    88,    66,   -25,
      67,  -151,    71,    72,    73,  -151,     2,  -151,  -151,   -57,
     -57,  -151,  -151,  -151,   -16,    95,  -151,   111,   -35,  -151,
      77,  -151,   101,   -12,   117,  -151,   120,  -151,  -151,    51,
     133,   109,   -25,   107,    62,    62,  -151,   103,   -25,   136,
    -151,  -151,  -151,  -151,  -151,  -151,   126,   -42,   127,    85,
      95,   142,  -151,    51,    24,  -151,  -151,  -151,  -151,  -151,
    -151,  -151,   -35,   -35,   -35,    88,    86,    89,   102,  -151,
     104,   134,   -11,   113,  -151,   -25,  -151,  -151,  -151,  -151,
    -151,  -151,   105,  -151,   139,   137,   112,  -151,  -151,  -151,
     -35,  -151,   100,   108,  -151,    98,   110,  -151,   159,   160,
    -151,   145,   147,   106,   114,   115,    98,  -151,  -151,  -151,
    -151,  -151
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,    26,     0,     0,
       0,    27,    28,    29,    25,    24,     0,     0,     0,     0,
       0,   108,    23,    22,    15,    16,    17,    18,     9,    10,
      11,    12,    13,    14,     8,     5,     7,     6,     4,     3,
      19,    20,    21,     0,     0,     0,     0,     0,    55,    56,
      81,    57,     0,    76,    77,    67,    68,    79,    78,     0,
      33,    32,     0,     0,     0,     0,     0,   106,     0,     1,
     109,     2,     0,     0,    30,     0,     0,     0,     0,    75,
       0,     0,     0,     0,     0,     0,     0,    84,     0,     0,
       0,    31,     0,     0,     0,    74,     0,    82,    69,    70,
      71,    72,    73,    83,    84,    63,    65,     0,    86,    60,
       0,   107,     0,     0,    48,    37,     0,    35,    80,     0,
      99,     0,     0,    81,     0,     0,    85,    87,     0,     0,
      42,    43,    44,    45,    46,    47,    40,     0,     0,     0,
      64,     0,    62,     0,     0,    53,    93,    94,    95,    96,
      97,    98,     0,     0,    86,    84,     0,     0,     0,    38,
      58,     0,     0,     0,    52,     0,    90,    92,    89,    91,
      88,    61,   102,    41,     0,     0,     0,    36,    34,   100,
      86,    54,     0,   104,    39,     0,     0,    66,     0,     0,
     101,    50,     0,     0,     0,     0,     0,    49,    59,   103,
     105,    51
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -151,  -151,   150,  -151,  -151,  -151,  -151,  -151,  -151,  -151,
    -151,  -151,  -151,  -151,  -151,  -151,  -151,    35,  -151,  -151,
    -151,   -26,  -151,  -151,   -88,  -151,  -151,  -151,  -151,  -151,
      52,  -151,    -4,    26,  -151,  -106,    30,  -101,  -150,  -151,
      49,  -151,  -151,  -151,  -151,  -151,  -151,  -151
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,   114,   115,   174,   136,
     138,   192,    35,   144,    54,   177,    36,    37,    38,   104,
     105,    39,    55,    56,    57,    58,   106,   109,   126,   127,
     152,   142,    40,   183,   190,    41,    42,    71
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      59,   111,   125,   120,   170,   119,    61,   158,    47,    77,
      83,    84,    95,   130,   131,   132,   133,   134,   135,    60,
     124,   113,   118,   108,    63,    78,    48,    49,   123,    51,
     187,    43,    64,    44,   145,    80,    48,    49,    66,    51,
     155,    45,    62,    46,   164,   165,   167,   169,   125,    68,
      48,    49,    50,    51,   171,    52,    53,    81,    82,    83,
      84,    65,    70,    69,   166,   168,   124,    81,    82,    83,
      84,    72,    73,    76,   125,    74,    98,   181,    79,    81,
      82,    83,    84,    75,    85,    86,    88,     1,     2,    89,
      87,    91,   124,     3,     4,     5,     6,     7,     8,     9,
      10,    90,    92,    96,    11,    12,    13,    99,   100,   101,
     102,    93,    94,    97,   103,    14,    15,   146,   147,   148,
     149,   150,   151,    16,   107,    17,   121,   108,    18,   110,
     122,   112,   128,    19,   113,   116,   117,   129,   137,   139,
     141,   143,    78,   154,   156,   157,   162,   160,   161,   172,
     173,   176,   175,   188,   178,   180,   185,   182,   179,   184,
     186,   191,   189,   194,   195,   193,   196,   197,    67,   198,
     201,   140,   159,   163,   153,     0,     0,     0,   199,   200
};

static const yytype_int16 yycheck[] =
{
       4,    89,   108,   104,   154,    21,     9,    49,    19,    19,
      67,    68,    20,    25,    26,    27,    28,    29,    30,    63,
     108,    63,    20,    39,    38,    35,    61,    62,    63,    64,
     180,     8,    63,    10,   122,    21,    61,    62,    44,    64,
     128,     8,    36,    10,    20,    21,   152,   153,   154,     8,
      61,    62,    63,    64,   155,    66,    67,    65,    66,    67,
      68,    63,     3,     0,   152,   153,   154,    65,    66,    67,
      68,    63,    63,    47,   180,    63,    80,   165,    52,    65,
      66,    67,    68,    63,    38,    63,    41,     5,     6,    55,
      63,    63,   180,    11,    12,    13,    14,    15,    16,    17,
      18,    45,    19,    77,    22,    23,    24,    81,    82,    83,
      84,    42,    42,    63,    63,    33,    34,    55,    56,    57,
      58,    59,    60,    41,    37,    43,    31,    39,    46,    63,
      19,    64,    55,    51,    63,    63,    63,    36,    21,    19,
       7,    32,    35,    40,     8,    19,     4,    20,    63,    63,
      61,    47,    50,    53,    20,    42,    19,    52,   162,    20,
      48,    63,    54,     4,     4,    55,    21,    20,    18,    63,
     196,   119,   137,   143,   125,    -1,    -1,    -1,    64,    64
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     5,     6,    11,    12,    13,    14,    15,    16,    17,
      18,    22,    23,    24,    33,    34,    41,    43,    46,    51,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    92,    96,    97,    98,   101,
     112,   115,   116,     8,    10,     8,    10,    19,    61,    62,
      63,    64,    66,    67,    94,   102,   103,   104,   105,   102,
      63,     9,    36,    38,    63,    63,    44,    72,     8,     0,
       3,   117,    63,    63,    63,    63,   103,    19,    35,   103,
      21,    65,    66,    67,    68,    38,    63,    63,    41,    55,
      45,    63,    19,    42,    42,    20,   103,    63,   102,   103,
     103,   103,   103,    63,    99,   100,   106,    37,    39,   107,
      63,    94,    64,    63,    86,    87,    63,    63,    20,    21,
     107,    31,    19,    63,    94,   105,   108,   109,    55,    36,
      25,    26,    27,    28,    29,    30,    89,    21,    90,    19,
     100,     7,   111,    32,    93,    94,    55,    56,    57,    58,
      59,    60,   110,   110,    40,    94,     8,    19,    49,    87,
      20,    63,     4,   106,    20,    21,    94,   105,    94,   105,
     108,   107,    63,    61,    88,    50,    47,    95,    20,   102,
      42,    94,    52,   113,    20,    19,    48,   108,    53,    54,
     114,    63,    91,    55,     4,     4,    21,    20,    63,    64,
      64,    91
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    70,    71,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    86,    87,
      87,    88,    89,    89,    89,    89,    89,    89,    90,    90,
      91,    91,    92,    93,    93,    94,    94,    94,    95,    95,
      96,    97,    98,    99,    99,   100,   100,   101,   102,   102,
     103,   103,   103,   103,   103,   103,   103,   103,   103,   103,
     104,   105,   105,   106,   107,   107,   108,   108,   108,   109,
     109,   109,   109,   110,   110,   110,   110,   110,   110,   111,
     111,   112,   113,   113,   114,   114,   115,   116,   117,   117
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     3,     2,     2,     8,     5,     8,     1,     3,     5,
       2,     1,     1,     1,     1,     1,     1,     1,     0,     6,
       1,     3,     7,     1,     3,     1,     1,     1,     0,     4,
       4,     7,     6,     1,     3,     1,     6,     2,     1,     3,
       3,     3,     3,     3,     3,     2,     1,     1,     1,     1,
       4,     1,     3,     1,     0,     2,     0,     1,     3,     3,
       3,     3,     3,     1,     1,     1,     1,     1,     1,     0,
       3,     9,     0,     4,     0,     3,     2,     4,     0,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (&yylloc, sql_string, sql_result, scanner, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YYLOCATION_PRINT

#  if defined YY_LOCATION_PRINT

   /* Temporary convenience wrapper in case some people defined the
      undocumented and private YY_LOCATION_PRINT macros.  */
#   define YYLOCATION_PRINT(File, Loc)  YY_LOCATION_PRINT(File, *(Loc))

#  elif defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
}

#   define YYLOCATION_PRINT  yy_location_print_

    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT(File, Loc)  YYLOCATION_PRINT(File, &(Loc))

#  else

#   define YYLOCATION_PRINT(File, Loc) ((void) 0)
    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT  YYLOCATION_PRINT

#  endif
# endif /* !defined YYLOCATION_PRINT */


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Location, sql_string, sql_result, scanner); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, const char * sql_string, ParsedSqlResult * sql_result, void * scanner)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (yylocationp);
  YY_USE (sql_string);
  YY_USE (sql_result);
  YY_USE (scanner);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, const char * sql_string, ParsedSqlResult * sql_result, void * scanner)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YYLOCATION_PRINT (yyo, yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp, sql_string, sql_result, scanner);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,
                 int yyrule, const char * sql_string, ParsedSqlResult * sql_result, void * scanner)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)],
                       &(yylsp[(yyi + 1) - (yynrhs)]), sql_string, sql_result, scanner);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule, sql_string, sql_result, scanner); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
  YYLTYPE *yylloc;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
  if (!yypact_value_is_default (yyn))
    {
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  In other words, skip the first -YYN actions for
         this state because they are default actions.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;
      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yyx;
      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
# endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
      char const *yyp = yystr;
      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
#endif


static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
        break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysize1
          = yysize + yytnamerr (YY_NULLPTR, yytname[yyarg[yyi]]);
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yytname[yyarg[yyi++]]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, const char * sql_string, ParsedSqlResult * sql_result, void * scanner)
{
  YY_USE (yyvaluep);
  YY_USE (yylocationp);
  YY_USE (sql_string);
  YY_USE (sql_result);
  YY_USE (scanner);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yykind)
    {
    case YYSYMBOL_attr_def_list: /* attr_def_list  */
#line 152 "src/observer/sql/parser/yacc_sql.y"
            { delete ((*yyvaluep).attr_infos); }
#line 1478 "src/observer/sql/parser/yacc_sql.cpp"
        break;

    case YYSYMBOL_primary_key: /* primary_key  */
#line 159 "src/observer/sql/parser/yacc_sql.y"
            { delete ((*yyvaluep).key_list); }
#line 1484 "src/observer/sql/parser/yacc_sql.cpp"
        break;

    case YYSYMBOL_attr_list: /* attr_list  */
#line 159 "src/observer/sql/parser/yacc_sql.y"
            { delete ((*yyvaluep).key_list); }
#line 1490 "src/observer/sql/parser/yacc_sql.cpp"
        break;

    case YYSYMBOL_value_list: /* value_list  */
#line 155 "src/observer/sql/parser/yacc_sql.y"
            { delete ((*yyvaluep).value_list); }
#line 1496 "src/observer/sql/parser/yacc_sql.cpp"
        break;

    case YYSYMBOL_value: /* value  */
#line 150 "src/observer/sql/parser/yacc_sql.y"
            { delete ((*yyvaluep).value); }
#line 1502 "src/observer/sql/parser/yacc_sql.cpp"
        break;

    case YYSYMBOL_from_clause: /* from_clause  */
#line 149 "src/observer/sql/parser/yacc_sql.y"
            { delete ((*yyvaluep).from_clause); }
#line 1508 "src/observer/sql/parser/yacc_sql.cpp"
        break;

    case YYSYMBOL_joined_relation: /* joined_relation  */
#line 149 "src/observer/sql/parser/yacc_sql.y"
            { delete ((*yyvaluep).from_clause); }
#line 1514 "src/observer/sql/parser/yacc_sql.cpp"
        break;

    case YYSYMBOL_expression_list: /* expression_list  */
#line 154 "src/observer/sql/parser/yacc_sql.y"
            { delete ((*yyvaluep).expression_list); }
#line 1520 "src/observer/sql/parser/yacc_sql.cpp"
        break;

    case YYSYMBOL_expression: /* expression  */
#line 153 "src/observer/sql/parser/yacc_sql.y"
            { delete ((*yyvaluep).expression); }
#line 1526 "src/observer/sql/parser/yacc_sql.cpp"
        break;

    case YYSYMBOL_aggregate_expression: /* aggregate_expression  */
#line 153 "src/observer/sql/parser/yacc_sql.y"
            { delete ((*yyvaluep).expression); }
#line 1532 "src/observer/sql/parser/yacc_sql.cpp"
        break;

    case YYSYMBOL_rel_attr: /* rel_attr  */
#line 151 "src/observer/sql/parser/yacc_sql.y"
            { delete ((*yyvaluep).rel_attr); }
#line 1538 "src/observer/sql/parser/yacc_sql.cpp"
        break;

    case YYSYMBOL_where: /* where  */
#line 156 "src/observer/sql/parser/yacc_sql.y"
            { delete ((*yyvaluep).condition_list); }
#line 1544 "src/observer/sql/parser/yacc_sql.cpp"
        break;

    case YYSYMBOL_condition_list: /* condition_list  */
#line 156 "src/observer/sql/parser/yacc_sql.y"
            { delete ((*yyvaluep).condition_list); }
#line 1550 "src/observer/sql/parser/yacc_sql.cpp"
        break;

    case YYSYMBOL_condition: /* condition  */
#line 148 "src/observer/sql/parser/yacc_sql.y"
            { delete ((*yyvaluep).condition); }
#line 1556 "src/observer/sql/parser/yacc_sql.cpp"
        break;

    case YYSYMBOL_group_by: /* group_by  */
#line 154 "src/observer/sql/parser/yacc_sql.y"
            { delete ((*yyvaluep).expression_list); }
#line 1562 "src/observer/sql/parser/yacc_sql.cpp"
        break;

      default:
        break;
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (const char * sql_string, ParsedSqlResult * sql_result, void * scanner)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

/* Location data for the lookahead symbol.  */
static YYLTYPE yyloc_default
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
YYLTYPE yylloc = yyloc_default;

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  yylsp[0] = yylloc;
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yyls1, yysize * YYSIZEOF (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, &yylloc, scanner);
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* commands: command_wrapper opt_semicolon  */
#line 223 "src/observer/sql/parser/yacc_sql.y"
  {
    unique_ptr<ParsedSqlNode> sql_node = unique_ptr<ParsedSqlNode>((yyvsp[-1].sql_node));
    sql_result->add_sql_node(std::move(sql_node));
  }
#line 1871 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 24: /* exit_stmt: EXIT  */
#line 254 "src/observer/sql/parser/yacc_sql.y"
         {
      (void)yynerrs;  // 这么写为了消除yynerrs未使用的告警。如果你有更好的方法欢迎提PR
      (yyval.sql_node) = new ParsedSqlNode(SCF_EXIT);
    }
#line 1880 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 25: /* help_stmt: HELP  */
#line 260 "src/observer/sql/parser/yacc_sql.y"
         {
      (yyval.sql_node) = new ParsedSqlNode(SCF_HELP);
    }
#line 1888 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 26: /* sync_stmt: SYNC  */
#line 265 "src/observer/sql/parser/yacc_sql.y"
         {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SYNC);
    }
#line 1896 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 27: /* begin_stmt: TRX_BEGIN  */
#line 271 "src/observer/sql/parser/yacc_sql.y"
               {
      (yyval.sql_node) = new ParsedSqlNode(SCF_BEGIN);
    }
#line 1904 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 28: /* commit_stmt: TRX_COMMIT  */
#line 277 "src/observer/sql/parser/yacc_sql.y"
               {
      (yyval.sql_node) = new ParsedSqlNode(SCF_COMMIT);
    }
#line 1912 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 29: /* rollback_stmt: TRX_ROLLBACK  */
#line 283 "src/observer/sql/parser/yacc_sql.y"
                  {
      (yyval.sql_node) = new ParsedSqlNode(SCF_ROLLBACK);
    }
#line 1920 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 30: /* drop_table_stmt: DROP TABLE ID  */
#line 289 "src/observer/sql/parser/yacc_sql.y"
                  {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DROP_TABLE);
      (yyval.sql_node)->drop_table.relation_name = (yyvsp[0].cstring);
    }
#line 1929 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 31: /* analyze_table_stmt: ANALYZE TABLE ID  */
#line 295 "src/observer/sql/parser/yacc_sql.y"
                     {
      (yyval.sql_node) = new ParsedSqlNode(SCF_ANALYZE_TABLE);
      (yyval.sql_node)->analyze_table.relation_name = (yyvsp[0].cstring);
    }
#line 1938 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 32: /* show_tables_stmt: SHOW TABLES  */
#line 302 "src/observer/sql/parser/yacc_sql.y"
                {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SHOW_TABLES);
    }
#line 1946 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 33: /* desc_table_stmt: DESC ID  */
#line 308 "src/observer/sql/parser/yacc_sql.y"
             {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DESC_TABLE);
      (yyval.sql_node)->desc_table.relation_name = (yyvsp[0].cstring);
    }
#line 1955 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 34: /* create_index_stmt: CREATE INDEX ID ON ID LBRACE ID RBRACE  */
#line 316 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CREATE_INDEX);
      CreateIndexSqlNode &create_index = (yyval.sql_node)->create_index;
      create_index.index_name = (yyvsp[-5].cstring);
      create_index.relation_name = (yyvsp[-3].cstring);
      create_index.attribute_name = (yyvsp[-1].cstring);
    }
#line 1967 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 35: /* drop_index_stmt: DROP INDEX ID ON ID  */
#line 327 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DROP_INDEX);
      (yyval.sql_node)->drop_index.index_name = (yyvsp[-2].cstring);
      (yyval.sql_node)->drop_index.relation_name = (yyvsp[0].cstring);
    }
#line 1977 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 36: /* create_table_stmt: CREATE TABLE ID LBRACE attr_def_list primary_key RBRACE storage_format  */
#line 335 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CREATE_TABLE);
      CreateTableSqlNode &create_table = (yyval.sql_node)->create_table;
      create_table.relation_name = (yyvsp[-5].cstring);
      //free($3);

      create_table.attr_infos.swap(*(yyvsp[-3].attr_infos));
      delete (yyvsp[-3].attr_infos);

      if ((yyvsp[-2].key_list) != nullptr) {
        create_table.primary_keys.swap(*(yyvsp[-2].key_list));
        delete (yyvsp[-2].key_list);
      }
      if ((yyvsp[0].cstring) != nullptr) {
        create_table.storage_format = (yyvsp[0].cstring);
      }
    }
#line 1999 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 37: /* attr_def_list: attr_def  */
#line 356 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.attr_infos) = new vector<AttrInfoSqlNode>;
      (yyval.attr_infos)->emplace_back(*(yyvsp[0].attr_info));
      delete (yyvsp[0].attr_info);
    }
#line 2009 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 38: /* attr_def_list: attr_def_list COMMA attr_def  */
#line 362 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.attr_infos) = (yyvsp[-2].attr_infos);
      (yyval.attr_infos)->emplace_back(*(yyvsp[0].attr_info));
      delete (yyvsp[0].attr_info);
    }
#line 2019 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 39: /* attr_def: ID type LBRACE number RBRACE  */
#line 371 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.attr_info) = new AttrInfoSqlNode;
      (yyval.attr_info)->type = (AttrType)(yyvsp[-3].number);
      (yyval.attr_info)->name = (yyvsp[-4].cstring);
      (yyval.attr_info)->length = (yyvsp[-1].number);
    }
#line 2030 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 40: /* attr_def: ID type  */
#line 378 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.attr_info) = new AttrInfoSqlNode;
      (yyval.attr_info)->type = (AttrType)(yyvsp[0].number);
      (yyval.attr_info)->name = (yyvsp[-1].cstring);
      (yyval.attr_info)->length = ((yyval.attr_info)->type == AttrType::TEXTS ? 4096 : 4);
    }
#line 2041 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 41: /* number: NUMBER  */
#line 386 "src/observer/sql/parser/yacc_sql.y"
           {(yyval.number) = (yyvsp[0].number);}
#line 2047 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 42: /* type: INT_T  */
#line 389 "src/observer/sql/parser/yacc_sql.y"
               { (yyval.number) = static_cast<int>(AttrType::INTS); }
#line 2053 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 43: /* type: STRING_T  */
#line 390 "src/observer/sql/parser/yacc_sql.y"
               { (yyval.number) = static_cast<int>(AttrType::CHARS); }
#line 2059 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 44: /* type: FLOAT_T  */
#line 391 "src/observer/sql/parser/yacc_sql.y"
               { (yyval.number) = static_cast<int>(AttrType::FLOATS); }
#line 2065 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 45: /* type: DATE_T  */
#line 392 "src/observer/sql/parser/yacc_sql.y"
               { (yyval.number) = static_cast<int>(AttrType::DATES); }
#line 2071 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 46: /* type: VECTOR_T  */
#line 393 "src/observer/sql/parser/yacc_sql.y"
               { (yyval.number) = static_cast<int>(AttrType::VECTORS); }
#line 2077 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 47: /* type: TEXT_T  */
#line 394 "src/observer/sql/parser/yacc_sql.y"
               { (yyval.number) = static_cast<int>(AttrType::TEXTS); }
#line 2083 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 48: /* primary_key: %empty  */
#line 398 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.key_list) = nullptr;
    }
#line 2091 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 49: /* primary_key: COMMA PRIMARY KEY LBRACE attr_list RBRACE  */
#line 402 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.key_list) = (yyvsp[-1].key_list);
    }
#line 2099 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 50: /* attr_list: ID  */
#line 408 "src/observer/sql/parser/yacc_sql.y"
       {
      (yyval.key_list) = new vector<string>();
      (yyval.key_list)->push_back((yyvsp[0].cstring));
    }
#line 2108 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 51: /* attr_list: ID COMMA attr_list  */
#line 412 "src/observer/sql/parser/yacc_sql.y"
                         {
      if ((yyvsp[0].key_list) != nullptr) {
        (yyval.key_list) = (yyvsp[0].key_list);
      } else {
        (yyval.key_list) = new vector<string>;
      }

      (yyval.key_list)->insert((yyval.key_list)->begin(), (yyvsp[-2].cstring));
    }
#line 2122 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 52: /* insert_stmt: INSERT INTO ID VALUES LBRACE value_list RBRACE  */
#line 425 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_INSERT);
      (yyval.sql_node)->insertion.relation_name = (yyvsp[-4].cstring);
      (yyval.sql_node)->insertion.values.swap(*(yyvsp[-1].value_list));
      delete (yyvsp[-1].value_list);
    }
#line 2133 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 53: /* value_list: value  */
#line 435 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.value_list) = new vector<Value>;
      (yyval.value_list)->emplace_back(*(yyvsp[0].value));
      delete (yyvsp[0].value);
    }
#line 2143 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 54: /* value_list: value_list COMMA value  */
#line 440 "src/observer/sql/parser/yacc_sql.y"
                             { 
      (yyval.value_list) = (yyvsp[-2].value_list);
      (yyval.value_list)->emplace_back(*(yyvsp[0].value));
      delete (yyvsp[0].value);
    }
#line 2153 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 55: /* value: NUMBER  */
#line 447 "src/observer/sql/parser/yacc_sql.y"
           {
      (yyval.value) = new Value((int)(yyvsp[0].number));
      (yyloc) = (yylsp[0]);
    }
#line 2162 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 56: /* value: FLOAT  */
#line 451 "src/observer/sql/parser/yacc_sql.y"
           {
      (yyval.value) = new Value((float)(yyvsp[0].floats));
      (yyloc) = (yylsp[0]);
    }
#line 2171 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 57: /* value: SSS  */
#line 455 "src/observer/sql/parser/yacc_sql.y"
         {
      char *tmp = common::substr((yyvsp[0].cstring),1,strlen((yyvsp[0].cstring))-2);
      (yyval.value) = new Value(tmp);
      free(tmp);
    }
#line 2181 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 58: /* storage_format: %empty  */
#line 463 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.cstring) = nullptr;
    }
#line 2189 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 59: /* storage_format: STORAGE FORMAT EQ ID  */
#line 467 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.cstring) = (yyvsp[0].cstring);
    }
#line 2197 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 60: /* delete_stmt: DELETE FROM ID where  */
#line 474 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DELETE);
      (yyval.sql_node)->deletion.relation_name = (yyvsp[-1].cstring);
      if ((yyvsp[0].condition_list) != nullptr) {
        (yyval.sql_node)->deletion.conditions.swap(*(yyvsp[0].condition_list));
        delete (yyvsp[0].condition_list);
      }
    }
#line 2210 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 61: /* update_stmt: UPDATE ID SET ID EQ value where  */
#line 485 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_UPDATE);
      (yyval.sql_node)->update.relation_name = (yyvsp[-5].cstring);
      (yyval.sql_node)->update.attribute_name = (yyvsp[-3].cstring);
      (yyval.sql_node)->update.value = *(yyvsp[-1].value);
      delete (yyvsp[-1].value);
      if ((yyvsp[0].condition_list) != nullptr) {
        (yyval.sql_node)->update.conditions.swap(*(yyvsp[0].condition_list));
        delete (yyvsp[0].condition_list);
      }
    }
#line 2226 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 62: /* select_stmt: SELECT expression_list FROM from_clause where group_by  */
#line 499 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SELECT);
      if ((yyvsp[-4].expression_list) != nullptr) {
        (yyval.sql_node)->selection.expressions.swap(*(yyvsp[-4].expression_list));
        delete (yyvsp[-4].expression_list);
      }

      if ((yyvsp[-2].from_clause) != nullptr) {
        (yyval.sql_node)->selection.relations.swap((yyvsp[-2].from_clause)->relations);
        (yyval.sql_node)->selection.conditions.swap((yyvsp[-2].from_clause)->conditions);
        delete (yyvsp[-2].from_clause);
      }

      if ((yyvsp[-1].condition_list) != nullptr) {
        (yyval.sql_node)->selection.conditions.insert(
            (yyval.sql_node)->selection.conditions.end(),
            std::make_move_iterator((yyvsp[-1].condition_list)->begin()),
            std::make_move_iterator((yyvsp[-1].condition_list)->end()));
        delete (yyvsp[-1].condition_list);
      }

      if ((yyvsp[0].expression_list) != nullptr) {
        (yyval.sql_node)->selection.group_by.swap(*(yyvsp[0].expression_list));
        delete (yyvsp[0].expression_list);
      }
    }
#line 2257 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 63: /* from_clause: joined_relation  */
#line 529 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.from_clause) = (yyvsp[0].from_clause);
    }
#line 2265 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 64: /* from_clause: from_clause COMMA joined_relation  */
#line 533 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.from_clause) = (yyvsp[-2].from_clause);
      if ((yyvsp[0].from_clause) != nullptr) {
        (yyval.from_clause)->relations.insert((yyval.from_clause)->relations.end(), (yyvsp[0].from_clause)->relations.begin(), (yyvsp[0].from_clause)->relations.end());
        (yyval.from_clause)->conditions.insert(
            (yyval.from_clause)->conditions.end(),
            std::make_move_iterator((yyvsp[0].from_clause)->conditions.begin()),
            std::make_move_iterator((yyvsp[0].from_clause)->conditions.end()));
        delete (yyvsp[0].from_clause);
      }
    }
#line 2281 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 65: /* joined_relation: relation  */
#line 548 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.from_clause) = new FromSqlNode;
      (yyval.from_clause)->relations.emplace_back((yyvsp[0].cstring));
    }
#line 2290 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 66: /* joined_relation: joined_relation INNER JOIN relation ON condition_list  */
#line 553 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.from_clause) = (yyvsp[-5].from_clause);
      (yyval.from_clause)->relations.emplace_back((yyvsp[-2].cstring));
      if ((yyvsp[0].condition_list) != nullptr) {
        (yyval.from_clause)->conditions.insert(
            (yyval.from_clause)->conditions.end(),
            std::make_move_iterator((yyvsp[0].condition_list)->begin()),
            std::make_move_iterator((yyvsp[0].condition_list)->end()));
        delete (yyvsp[0].condition_list);
      }
    }
#line 2306 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 67: /* calc_stmt: CALC expression_list  */
#line 567 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CALC);
      (yyval.sql_node)->calc.expressions.swap(*(yyvsp[0].expression_list));
      delete (yyvsp[0].expression_list);
    }
#line 2316 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 68: /* expression_list: expression  */
#line 576 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.expression_list) = new vector<unique_ptr<Expression>>;
      (yyval.expression_list)->emplace_back((yyvsp[0].expression));
    }
#line 2325 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 69: /* expression_list: expression COMMA expression_list  */
#line 581 "src/observer/sql/parser/yacc_sql.y"
    {
      if ((yyvsp[0].expression_list) != nullptr) {
        (yyval.expression_list) = (yyvsp[0].expression_list);
      } else {
        (yyval.expression_list) = new vector<unique_ptr<Expression>>;
      }
      (yyval.expression_list)->emplace((yyval.expression_list)->begin(), (yyvsp[-2].expression));
    }
#line 2338 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 70: /* expression: expression '+' expression  */
#line 591 "src/observer/sql/parser/yacc_sql.y"
                              {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::ADD, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 2346 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 71: /* expression: expression '-' expression  */
#line 594 "src/observer/sql/parser/yacc_sql.y"
                                {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::SUB, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 2354 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 72: /* expression: expression '*' expression  */
#line 597 "src/observer/sql/parser/yacc_sql.y"
                                {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::MUL, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 2362 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 73: /* expression: expression '/' expression  */
#line 600 "src/observer/sql/parser/yacc_sql.y"
                                {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::DIV, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 2370 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 74: /* expression: LBRACE expression RBRACE  */
#line 603 "src/observer/sql/parser/yacc_sql.y"
                               {
      (yyval.expression) = (yyvsp[-1].expression);
      (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 2379 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 75: /* expression: '-' expression  */
#line 607 "src/observer/sql/parser/yacc_sql.y"
                                  {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::NEGATIVE, (yyvsp[0].expression), nullptr, sql_string, &(yyloc));
    }
#line 2387 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 76: /* expression: '*'  */
#line 610 "src/observer/sql/parser/yacc_sql.y"
          {
      (yyval.expression) = new StarExpr();
    }
#line 2395 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 77: /* expression: value  */
#line 613 "src/observer/sql/parser/yacc_sql.y"
            {
      (yyval.expression) = new ValueExpr(*(yyvsp[0].value));
      (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
      delete (yyvsp[0].value);
    }
#line 2405 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 78: /* expression: rel_attr  */
#line 618 "src/observer/sql/parser/yacc_sql.y"
               {
      RelAttrSqlNode *node = (yyvsp[0].rel_attr);
      (yyval.expression) = new UnboundFieldExpr(node->relation_name, node->attribute_name);
      (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
      delete (yyvsp[0].rel_attr);
    }
#line 2416 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 79: /* expression: aggregate_expression  */
#line 624 "src/observer/sql/parser/yacc_sql.y"
                           {
      (yyval.expression) = (yyvsp[0].expression);
    }
#line 2424 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 80: /* aggregate_expression: ID LBRACE expression RBRACE  */
#line 630 "src/observer/sql/parser/yacc_sql.y"
                                {
      (yyval.expression) = create_aggregate_expression((yyvsp[-3].cstring), (yyvsp[-1].expression), sql_string, &(yyloc));
    }
#line 2432 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 81: /* rel_attr: ID  */
#line 636 "src/observer/sql/parser/yacc_sql.y"
       {
      (yyval.rel_attr) = new RelAttrSqlNode;
      (yyval.rel_attr)->attribute_name = (yyvsp[0].cstring);
    }
#line 2441 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 82: /* rel_attr: ID DOT ID  */
#line 640 "src/observer/sql/parser/yacc_sql.y"
                {
      (yyval.rel_attr) = new RelAttrSqlNode;
      (yyval.rel_attr)->relation_name  = (yyvsp[-2].cstring);
      (yyval.rel_attr)->attribute_name = (yyvsp[0].cstring);
    }
#line 2451 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 83: /* relation: ID  */
#line 648 "src/observer/sql/parser/yacc_sql.y"
       {
      (yyval.cstring) = (yyvsp[0].cstring);
    }
#line 2459 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 84: /* where: %empty  */
#line 670 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.condition_list) = nullptr;
    }
#line 2467 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 85: /* where: WHERE condition_list  */
#line 673 "src/observer/sql/parser/yacc_sql.y"
                           {
      (yyval.condition_list) = (yyvsp[0].condition_list);  
    }
#line 2475 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 86: /* condition_list: %empty  */
#line 679 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.condition_list) = nullptr;
    }
#line 2483 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 87: /* condition_list: condition  */
#line 682 "src/observer/sql/parser/yacc_sql.y"
                {
      (yyval.condition_list) = new vector<ConditionSqlNode>;
      (yyval.condition_list)->emplace_back(*(yyvsp[0].condition));
      delete (yyvsp[0].condition);
    }
#line 2493 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 88: /* condition_list: condition AND condition_list  */
#line 687 "src/observer/sql/parser/yacc_sql.y"
                                   {
      (yyval.condition_list) = (yyvsp[0].condition_list);
      (yyval.condition_list)->emplace_back(*(yyvsp[-2].condition));
      delete (yyvsp[-2].condition);
    }
#line 2503 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 89: /* condition: rel_attr comp_op value  */
#line 695 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.condition) = new ConditionSqlNode;
      (yyval.condition)->left_is_attr = 1;
      (yyval.condition)->left_attr = *(yyvsp[-2].rel_attr);
      (yyval.condition)->right_is_attr = 0;
      (yyval.condition)->right_value = *(yyvsp[0].value);
      (yyval.condition)->comp = (yyvsp[-1].comp);

      delete (yyvsp[-2].rel_attr);
      delete (yyvsp[0].value);
    }
#line 2519 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 90: /* condition: value comp_op value  */
#line 707 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.condition) = new ConditionSqlNode;
      (yyval.condition)->left_is_attr = 0;
      (yyval.condition)->left_value = *(yyvsp[-2].value);
      (yyval.condition)->right_is_attr = 0;
      (yyval.condition)->right_value = *(yyvsp[0].value);
      (yyval.condition)->comp = (yyvsp[-1].comp);

      delete (yyvsp[-2].value);
      delete (yyvsp[0].value);
    }
#line 2535 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 91: /* condition: rel_attr comp_op rel_attr  */
#line 719 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.condition) = new ConditionSqlNode;
      (yyval.condition)->left_is_attr = 1;
      (yyval.condition)->left_attr = *(yyvsp[-2].rel_attr);
      (yyval.condition)->right_is_attr = 1;
      (yyval.condition)->right_attr = *(yyvsp[0].rel_attr);
      (yyval.condition)->comp = (yyvsp[-1].comp);

      delete (yyvsp[-2].rel_attr);
      delete (yyvsp[0].rel_attr);
    }
#line 2551 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 92: /* condition: value comp_op rel_attr  */
#line 731 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.condition) = new ConditionSqlNode;
      (yyval.condition)->left_is_attr = 0;
      (yyval.condition)->left_value = *(yyvsp[-2].value);
      (yyval.condition)->right_is_attr = 1;
      (yyval.condition)->right_attr = *(yyvsp[0].rel_attr);
      (yyval.condition)->comp = (yyvsp[-1].comp);

      delete (yyvsp[-2].value);
      delete (yyvsp[0].rel_attr);
    }
#line 2567 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 93: /* comp_op: EQ  */
#line 745 "src/observer/sql/parser/yacc_sql.y"
         { (yyval.comp) = EQUAL_TO; }
#line 2573 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 94: /* comp_op: LT  */
#line 746 "src/observer/sql/parser/yacc_sql.y"
         { (yyval.comp) = LESS_THAN; }
#line 2579 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 95: /* comp_op: GT  */
#line 747 "src/observer/sql/parser/yacc_sql.y"
         { (yyval.comp) = GREAT_THAN; }
#line 2585 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 96: /* comp_op: LE  */
#line 748 "src/observer/sql/parser/yacc_sql.y"
         { (yyval.comp) = LESS_EQUAL; }
#line 2591 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 97: /* comp_op: GE  */
#line 749 "src/observer/sql/parser/yacc_sql.y"
         { (yyval.comp) = GREAT_EQUAL; }
#line 2597 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 98: /* comp_op: NE  */
#line 750 "src/observer/sql/parser/yacc_sql.y"
         { (yyval.comp) = NOT_EQUAL; }
#line 2603 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 99: /* group_by: %empty  */
#line 756 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.expression_list) = nullptr;
    }
#line 2611 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 100: /* group_by: GROUP BY expression_list  */
#line 760 "src/observer/sql/parser/yacc_sql.y"
    {
      // group by 的表达式范围与select查询值的表达式范围是不同的，比如group by不支持 *
      // 但是这里没有处理。
      (yyval.expression_list) = (yyvsp[0].expression_list);
    }
#line 2621 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 101: /* load_data_stmt: LOAD DATA INFILE SSS INTO TABLE ID fields_terminated_by enclosed_by  */
#line 768 "src/observer/sql/parser/yacc_sql.y"
    {
      char *tmp_file_name = common::substr((yyvsp[-5].cstring), 1, strlen((yyvsp[-5].cstring)) - 2);
      
      (yyval.sql_node) = new ParsedSqlNode(SCF_LOAD_DATA);
      (yyval.sql_node)->load_data.relation_name = (yyvsp[-2].cstring);
      (yyval.sql_node)->load_data.file_name = tmp_file_name;
      if ((yyvsp[-1].cstring) != nullptr) {
        char *tmp = common::substr((yyvsp[-1].cstring),1,strlen((yyvsp[-1].cstring))-2);
        (yyval.sql_node)->load_data.terminated = (yyvsp[-1].cstring);
        free(tmp);
      }
      if ((yyvsp[0].cstring) != nullptr) {
        char *tmp = common::substr((yyvsp[0].cstring),1,strlen((yyvsp[0].cstring))-2);
        (yyval.sql_node)->load_data.enclosed = (yyvsp[0].cstring);
        free(tmp);
      }
      free(tmp_file_name);
    }
#line 2644 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 102: /* fields_terminated_by: %empty  */
#line 790 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.cstring) = nullptr;
    }
#line 2652 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 103: /* fields_terminated_by: FIELDS TERMINATED BY SSS  */
#line 794 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.cstring) = (yyvsp[0].cstring);
    }
#line 2660 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 104: /* enclosed_by: %empty  */
#line 800 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.cstring) = nullptr;
    }
#line 2668 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 105: /* enclosed_by: ENCLOSED BY SSS  */
#line 804 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.cstring) = (yyvsp[0].cstring);
    }
#line 2676 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 106: /* explain_stmt: EXPLAIN command_wrapper  */
#line 810 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_EXPLAIN);
      (yyval.sql_node)->explain.sql_node = unique_ptr<ParsedSqlNode>((yyvsp[0].sql_node));
    }
#line 2685 "src/observer/sql/parser/yacc_sql.cpp"
    break;

  case 107: /* set_variable_stmt: SET ID EQ value  */
#line 818 "src/observer/sql/parser/yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SET_VARIABLE);
      (yyval.sql_node)->set_variable.name  = (yyvsp[-2].cstring);
      (yyval.sql_node)->set_variable.value = *(yyvsp[0].value);
      delete (yyvsp[0].value);
    }
#line 2696 "src/observer/sql/parser/yacc_sql.cpp"
    break;


#line 2700 "src/observer/sql/parser/yacc_sql.cpp"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken, &yylloc};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (&yylloc, sql_string, sql_result, scanner, yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  yyerror_range[1] = yylloc;
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc, sql_string, sql_result, scanner);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp, sql_string, sql_result, scanner);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, sql_string, sql_result, scanner, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc, sql_string, sql_result, scanner);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp, sql_string, sql_result, scanner);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}

#line 829 "src/observer/sql/parser/yacc_sql.y"

//_____________________________________________________________________
extern void scan_string(const char *str, yyscan_t scanner);

int sql_parse(const char *s, ParsedSqlResult *sql_result) {
  yyscan_t scanner;
  std::vector<char *> allocated_strings;
  yylex_init_extra(static_cast<void*>(&allocated_strings),&scanner);
  scan_string(s, scanner);
  int result = yyparse(s, sql_result, scanner);

  for (char *ptr : allocated_strings) {
    free(ptr);
  }
  allocated_strings.clear();

  yylex_destroy(scanner);
  return result;
}
