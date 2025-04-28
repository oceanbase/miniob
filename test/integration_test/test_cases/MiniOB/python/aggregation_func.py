import __init__
from typing import List, Union
from test_instruction import ResultString
from test_case import TestCase
import util.myutil

def create_test_cases() -> TestCase:
  aggregation_test = TestCase()
  aggregation_test.name = 'aggregation-func'

  init_group = aggregation_test.add_execution_group('init data')
  init_group.add_runtime_ddl_instruction('CREATE TABLE aggregation_func(id int, num int, price float, addr char(4));')
  
  insert_sqls = [
    "INSERT INTO aggregation_func VALUES (1, 18, 10.0, 'abc');",
    "INSERT INTO aggregation_func VALUES (2, 15, 20.0, 'abc');",
    "INSERT INTO aggregation_func VALUES (3, 12, 30.0, 'def');",
    "INSERT INTO aggregation_func VALUES (4, 15, 30.0, 'dei');"
  ]
  for sql in insert_sqls:
    init_group.add_runtime_dml_instruction(sql)

  count_group = aggregation_test.add_execution_group('count', [init_group])
  count_sqls = [
    'SELECT count(*) FROM aggregation_func;',
    'SELECT count(num) FROM aggregation_func;'
  ]

  for sql in count_sqls:
    count_group.add_runtime_dql_instruction(sql)


  min_group = aggregation_test.add_execution_group('min', [init_group])
  min_sqls = [
    'SELECT min(num) FROM aggregation_func;',
    'SELECT min(price) FROM aggregation_func;',
    'SELECT min(addr) FROM aggregation_func;'
  ]

  for sql in min_sqls:
    min_group.add_runtime_dql_instruction(sql)

  max_group = aggregation_test.add_execution_group('max', [init_group])
  max_sqls = [
    'SELECT max(num) FROM aggregation_func;',
    'SELECT max(price) FROM aggregation_func;',
    'SELECT max(addr) FROM aggregation_func;'
  ]

  for sql in max_sqls:
    max_group.add_runtime_dql_instruction(sql)

  avg_group = aggregation_test.add_execution_group('avg', [init_group])
  avg_sqls = [
    'SELECT avg(num) FROM aggregation_func;',
    'SELECT avg(price) FROM aggregation_func;'
  ]

  for sql in avg_sqls:
    avg_group.add_runtime_dql_instruction(sql)

  error_group = aggregation_test.add_execution_group('error', [init_group])
  error_sqls = [
    "SELECT min(*) FROM aggregation_func;",
    "SELECT max(*) FROM aggregation_func;",
    "SELECT avg(*) FROM aggregation_func;"
  ]
  
  for sql in error_sqls:
    error_group.add_sql_instruction(sql, expected=ResultString.FAILURE)

  error_with_redundant_columns_group = aggregation_test.add_execution_group('error with redundant columns', [init_group])
  error_with_redundant_columns_sqls = util.myutil.split_and_strip_string(
    '''
    SELECT count(*,num) FROM aggregation_func;
    SELECT min(num,price) FROM aggregation_func;
    SELECT max(num,price) FROM aggregation_func;
    SELECT avg(num,price) FROM aggregation_func;
    '''
  )

  for sql in error_with_redundant_columns_sqls:
    error_with_redundant_columns_group.add_sql_instruction(sql, expected=ResultString.FAILURE)

  error_empty_column_group = aggregation_test.add_execution_group('error empty column', [init_group])
  error_empty_column_sqls = util.myutil.split_and_strip_string(
    '''
    SELECT count() FROM aggregation_func;
    SELECT min() FROM aggregation_func;
    SELECT max() FROM aggregation_func;
    SELECT avg() FROM aggregation_func;
    '''
  )

  for sql in error_empty_column_sqls:
    error_empty_column_group.add_sql_instruction(sql, expected=ResultString.FAILURE)

  error_non_exists_column_group = aggregation_test.add_execution_group('error non exists column', [init_group])
  error_non_exists_column_sqls = util.myutil.split_and_strip_string(
    '''
    SELECT count(non_exists_column) FROM aggregation_func;
    SELECT min(non_exists_column) FROM aggregation_func;
    SELECT max(non_exists_column) FROM aggregation_func;
    SELECT avg(non_exists_column) FROM aggregation_func;
    '''
  )

  for sql in error_non_exists_column_sqls:
    error_non_exists_column_group.add_sql_instruction(sql, expected=ResultString.FAILURE)

  many_aggr_group = aggregation_test.add_execution_group('many aggregation functions', [init_group])
  many_aggr_group.add_runtime_dql_instruction('SELECT min(num),max(num),avg(num) FROM aggregation_func;')

  return aggregation_test