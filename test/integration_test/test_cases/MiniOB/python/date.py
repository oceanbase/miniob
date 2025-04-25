import __init__
from typing import List, Union
from test_instruction import ResultString
from test_case import TestCase
import util.myutil

def create_test_cases() -> TestCase:
  date_test = TestCase()

  init_data_group = date_test.add_execution_group('init data')
  create_table_sqls = util.myutil.split_and_strip_string(
    '''
    CREATE TABLE date_table(id int, u_date date);
    CREATE INDEX index_id on date_table(u_date);
    ''')
  for sql in create_table_sqls:
    init_data_group.add_runtime_ddl_instruction(sql)

  insert_sqls = util.myutil.split_and_strip_string(
    '''
    INSERT INTO date_table VALUES (1,'2020-01-21');
    INSERT INTO date_table VALUES (2,'2020-10-21');
    INSERT INTO date_table VALUES (3,'2020-1-01');
    INSERT INTO date_table VALUES (4,'2020-01-1');
    INSERT INTO date_table VALUES (5,'2019-12-21');
    INSERT INTO date_table VALUES (6,'2016-2-29');
    INSERT INTO date_table VALUES (7,'1970-1-1');
    INSERT INTO date_table VALUES (8,'2000-01-01');
    INSERT INTO date_table VALUES (9,'2038-1-19');
    ''')
  for sql in insert_sqls:
    init_data_group.add_runtime_dml_instruction(sql)

  compare_sqls = util.myutil.split_and_strip_string(
    '''
    SELECT * FROM date_table WHERE u_date>'2020-1-20';
    SELECT * FROM date_table WHERE u_date<'2019-12-31';
    SELECT * FROM date_table WHERE u_date='2020-1-1';
    ''')
  compare_group = date_test.add_execution_group('compare', [init_data_group])
  for sql in compare_sqls:
    compare_group.add_sort_runtime_dql_instruction(sql)

  delete_group = date_test.add_execution_group('delete', [init_data_group])
  delete_group.add_runtime_dml_instruction("DELETE FROM date_table WHERE u_date>'2012-2-29';")
  delete_group.add_sort_runtime_dql_instruction("SELECT * FROM date_table;")

  invalid_date_group = date_test.add_execution_group('invalid date', [init_data_group])
  invalid_date_sqls = util.myutil.split_and_strip_string(
    '''
    SELECT * FROM date_table WHERE u_date='2017-2-29';
    SELECT * FROM date_table WHERE u_date='2017-21-29';
    SELECT * FROM date_table WHERE u_date='2017-12-32';
    SELECT * FROM date_table WHERE u_date='2017-11-31';
    ''')
  for sql in invalid_date_sqls:
    invalid_date_group.add_sql_instruction(sql, expected=ResultString.FAILURE)

  insert_invalid_date_group = date_test.add_execution_group('insert invalid date', [init_data_group])
  insert_invalid_date_sqls = util.myutil.split_and_strip_string(
    '''
    INSERT INTO date_table VALUES (10,'2017-2-29');
    INSERT INTO date_table VALUES (11,'2017-21-29');
    INSERT INTO date_table VALUES (12,'2017-12-32');
    INSERT INTO date_table VALUES (13,'2017-11-31');
    ''')
  for sql in insert_invalid_date_sqls:
    insert_invalid_date_group.add_runtime_dql_instruction(sql)

  return date_test