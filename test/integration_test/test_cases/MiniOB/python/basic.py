import __init__
from typing import List, Union
from test_instruction import ResultString
from test_case import TestCase

def create_test_cases() -> TestCase:
  basic_test = TestCase()

  create_table_group = basic_test.add_execution_group('create table')
  create_table_group.add_runtime_ddl_instruction('create table t_basic(id int, age int, name char(4), score float);')
  
  init_data_group = basic_test.add_execution_group('init data', [create_table_group])
  init_data_group.add_runtime_dml_instruction("insert into t_basic values(1,1, 'a', 1.0);") \
                 .add_runtime_dml_instruction("insert into t_basic values(2,2, 'b', 2.0);") \
                 .add_runtime_dml_instruction("insert into t_basic values(4,4, 'c', 3.0);") \
                 .add_runtime_dml_instruction("insert into t_basic values(3,3, 'd', 4.0);") \
                 .add_runtime_dml_instruction("insert into t_basic values(5,5, 'e', 5.5);") \
                 .add_runtime_dml_instruction("insert into t_basic values(6,6, 'f', 6.6);") \
                 .add_runtime_dml_instruction("insert into t_basic values(7,7, 'g', 7.7);")
  init_data_group.add_sort_runtime_dql_instruction('select * from t_basic;')
  
  delete_group = basic_test.add_execution_group('basic delete', [create_table_group])
  delete_group.add_runtime_dml_instruction('delete from t_basic where id=3;')
  delete_group.add_sort_runtime_dql_instruction('select * from t_basic;')

  select_group = basic_test.add_execution_group('basic select', [create_table_group])
  select_group.add_runtime_dql_instruction('select * from t_basic where id=1;')

  select_group.add_sort_runtime_dql_instruction('select * from t_basic where id>=5;')

  select_group.add_sort_runtime_dql_instruction('select * from t_basic where age>1 and age<3;')

  select_group.add_runtime_dql_instruction('select * from t_basic where t_basic.id=1 and t_basic.age=1;')

  select_group.add_runtime_dql_instruction('select * from t_basic where id=1 and age=1;')

  select_group.add_sort_runtime_dql_instruction('select id, age, name, score from t_basic;')

  select_group.add_sort_runtime_dql_instruction('select t_basic.id, t_basic.age, t_basic.name, t_basic.score from t_basic;')

  create_index_group = basic_test.add_execution_group('create index', [create_table_group])
  create_index_group.add_runtime_ddl_instruction("create index i_id on t_basic (id);")
  create_index_group.add_sort_runtime_dql_instruction('select * from t_basic;')

  return basic_test