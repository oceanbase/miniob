import __init__
from typing import List, Union
from test_instruction import ResultString
from test_case import TestCase
import util.myutil

def create_test_cases() -> TestCase:
  drop_table_test = TestCase()
  drop_table_test.name = 'drop-table'

  drop_empty_table_group = drop_table_test.add_execution_group('drop empty table')
  drop_empty_table_group.add_runtime_ddl_instruction('create table Drop_table_1(id int);')
  drop_empty_table_group.add_runtime_ddl_instruction('drop table Drop_table_1;')

  drop_non_empty_table_group = drop_table_test.add_execution_group('drop non empty table')
  drop_non_empty_table_group.add_runtime_ddl_instruction('create table Drop_table_2(id int);')
  drop_non_empty_table_group.add_runtime_dml_instruction('insert into Drop_table_2 values(1);')
  drop_non_empty_table_group.add_runtime_ddl_instruction('drop table Drop_table_2;')

  check_drop_table_group = drop_table_test.add_execution_group('check drop table')
  check_drop_table_group.add_runtime_ddl_instruction('create table Drop_table_3(id int, t_name char(4));')
  check_drop_table_group.add_runtime_dml_instruction("insert into Drop_table_3 values(1,'OB');")
  check_drop_table_group.add_sort_runtime_dql_instruction('select * from Drop_table_3;')
  check_drop_table_group.add_runtime_ddl_instruction('drop table Drop_table_3;')
  check_drop_table_group.add_runtime_dml_instruction("insert into Drop_table_3 values(2,'OC');")
  check_drop_table_group.add_sort_runtime_dql_instruction('select * from Drop_table_3;')
  check_drop_table_group.add_runtime_dml_instruction('delete from Drop_table_3 where id = 3;')
  check_drop_table_group.add_runtime_ddl_instruction('create table Drop_table_3(id int, t_name char(4));')
  check_drop_table_group.add_sort_runtime_dql_instruction('select * from Drop_table_3;')

  drop_non_exists_table_group = drop_table_test.add_execution_group('drop non exists table')
  drop_non_exists_table_group.add_runtime_ddl_instruction('create table Drop_table_4(id int, t_name char(4));')
  drop_non_exists_table_group.add_runtime_ddl_instruction('drop table Drop_table_4;')
  drop_non_exists_table_group.add_runtime_ddl_instruction('drop table Drop_table_4;')
  drop_non_exists_table_group.add_runtime_ddl_instruction('drop table Drop_table_4_1;')

  create_dropped_table_group = drop_table_test.add_execution_group('create dropped table')
  create_dropped_table_group.add_runtime_ddl_instruction('create table Drop_table_5(id int, t_name char(4));')
  create_dropped_table_group.add_runtime_ddl_instruction('drop table Drop_table_5;')
  create_dropped_table_group.add_runtime_ddl_instruction('create table Drop_table_5(id int, t_name char(4));')
  create_dropped_table_group.add_sort_runtime_dql_instruction('select * from Drop_table_5;')

  drop_table_with_index_group = drop_table_test.add_execution_group('drop table with index')
  drop_table_with_index_group.add_runtime_ddl_instruction('create table Drop_table_6(id int, t_name char(4));')
  drop_table_with_index_group.add_runtime_ddl_instruction('create index index_id on Drop_table_6(id);')
  drop_table_with_index_group.add_runtime_dml_instruction("insert into Drop_table_6 values(1,'OB');")
  drop_table_with_index_group.add_sort_runtime_dql_instruction('select * from Drop_table_6;')
  drop_table_with_index_group.add_runtime_ddl_instruction('drop table Drop_table_6;')
  drop_table_with_index_group.add_sort_runtime_dql_instruction('select * from Drop_table_6;')

  return drop_table_test