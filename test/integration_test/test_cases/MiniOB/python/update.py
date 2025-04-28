import __init__
from typing import List, Union
from test_instruction import ResultString
from test_case import TestCase
import util.myutil

def create_test_cases() -> TestCase:
  update_test = TestCase()
  update_test.name = 'update'

  init_data_group = update_test.add_execution_group('init data')
  init_data_group.add_runtime_ddl_instruction('CREATE TABLE Update_table_1(id int, t_name char(4), col1 int, col2 int);') \
                 .add_runtime_ddl_instruction("CREATE INDEX index_id on Update_table_1(id);")

  for i in range(1, 4):
    init_data_group.add_runtime_dml_instruction(f"INSERT INTO Update_table_1 VALUES ({i}, 'N{i}', {i}, {i});")

  update_one_row_group = update_test.add_execution_group('update one row', [init_data_group])
  update_one_row_group.add_runtime_dml_instruction("UPDATE Update_table_1 SET t_name='N01' WHERE id=1;")
  update_one_row_group.add_sort_runtime_dql_instruction("SELECT * FROM Update_table_1;")

  update_rows_group = update_test.add_execution_group('update rows', [init_data_group])
  update_rows_group.add_runtime_dml_instruction("UPDATE Update_table_1 SET col2=0 WHERE col1=1;")
  update_rows_group.add_sort_runtime_dql_instruction("SELECT * FROM Update_table_1;")

  update_index_group = update_test.add_execution_group('update index', [init_data_group])
  update_index_group.add_runtime_dml_instruction("UPDATE Update_table_1 SET id=4 WHERE t_name='N3';")
  update_index_group.add_sort_runtime_dql_instruction("SELECT * FROM Update_table_1;")

  update_without_condition_group = update_test.add_execution_group('update without conditions', [init_data_group])
  update_without_condition_group.add_runtime_dml_instruction("UPDATE Update_table_1 SET col1=0;")
  update_without_condition_group.add_sort_runtime_dql_instruction("SELECT * FROM Update_table_1;")

  update_with_condition_group = update_test.add_execution_group('update with conditions', [init_data_group])
  update_with_condition_group.add_runtime_dml_instruction("UPDATE Update_table_1 SET t_name='N02' WHERE col1=0 AND col2=0;")
  update_with_condition_group.add_sort_runtime_dql_instruction("SELECT * FROM Update_table_1;")

  update_with_sub_query_group = update_test.add_execution_group('update non-exists table')
  update_with_sub_query_group.add_sql_instruction("UPDATE Update_table_2 SET t_name='N01' WHERE id=1;", expected=ResultString.FAILURE)

  update_non_exists_column_group = update_test.add_execution_group('update non-exists column', [init_data_group])
  update_non_exists_column_group.add_sql_instruction("UPDATE Update_table_1 SET t_name_false='N01' WHERE id=1;", expected=ResultString.FAILURE)

  update_with_invalid_condition_group = update_test.add_execution_group('update with invalid condition', [init_data_group])
  update_with_invalid_condition_group.add_sql_instruction("UPDATE Update_table_1 SET t_name='N01' WHERE id_false=1;", expected=ResultString.FAILURE)

  update_in_vain_group = update_test.add_execution_group('update in vain', [init_data_group])
  update_in_vain_group.add_runtime_dml_instruction("UPDATE Update_table_1 SET t_name='N01' WHERE id=100;")
  update_in_vain_group.add_sort_runtime_dql_instruction("SELECT * FROM Update_table_1;")

  update_with_invalid_value_group = update_test.add_execution_group('update with invalid value', [init_data_group])
  update_with_invalid_value_group.add_runtime_dml_instruction("UPDATE Update_table_1 SET col1='N01' WHERE id=1;")

  return update_test