import os
import os.path
import logging
import traceback
import socket
import shutil

from typing import Dict, List

_logger = logging.getLogger('myutil')

def remove_empty_strings(strs: List[str]):
  ret = []
  for s in strs:
    s = s.strip()
    if len(s) != 0:
      ret.append(s)
  return ret

def split_and_strip_string(s: str) -> List[str]:
  '''
  拆分s字符串，通常是按照换行符拆分，然后把每一行的前后空字符都去掉。
  然后去掉字符串前面和后面的空字符串
  '''
  strings = s.splitlines()
  return strip_string_list([ single_string.strip() for single_string in strings ])

def lstrip_string_list(strs: List[str]) -> List[str]:
  '''
  去掉字符串列表中最前面的空字符串
  '''
  while len(strs) > 0:
    s = strs[0].strip()
    if len(s) == 0:
      strs = strs[1:]
    else:
      break

  return strs

def rstrip_string_list(strs: List[str]) -> List[str]:
  '''
  去掉字符串列表中最后面的空字符串
  '''
  while len(strs) > 0:
    s = strs[-1].strip()
    if len(s) == 0:
      strs = strs[ : -1]
    else:
      break
  return strs

def strip_string_list(strs: List[str]) -> List[str]:
  '''
  去掉字符串列表中最前面和最后面的空字符串
  '''
  strs = lstrip_string_list(strs)
  strs = rstrip_string_list(strs)
  return strs

def count_directory_size(directory: str):
  '''
  count the size of files in directory(in bytes)
  return 0 if directory is not exists
  '''

  if not os.path.exists(directory):
    return 0

  bytes_count = 0
  for f in os.listdir(directory):
    f = os.path.join(directory, f)
    if os.path.isdir(f):
      bytes_count += count_directory_size(f)
    else:
      bytes_count += os.path.getsize(f)

  return bytes_count

def compare_file(file1, file2):
  try:
    with open(file1, 'r') as f1, open(file2, 'r') as f2:
      for line1 in f1.readlines():
        line2 = f2.readline()
        if line1.upper() != line2.upper():
          return False
  except Exception as ex:
    _logger.info('compare file exception=%s', traceback.format_exc())  
    return False

  return True

def list_ipv4():
  '''
   list all ipv4 addresses
   应该返回一个IPv4列表，但是现在简单处理，只返回一个IP地址
   NOTE: gethostname在MAC机器上，只能返回本地回环地址
  '''
  hostname = socket.gethostname()
  host = socket.gethostbyname(hostname)
  return [host]

def get_integer(val):
  if val is None:
    return None

  if isinstance(val, int):
    return val
  try:
    return int(val)
  except Exception as ex:
    _logger.error('cannot convert val[%s] to integer. exception=%s', str(val), str(ex))
    return None

def remove_sub_files(path:str):
  try:
    files = os.listdir(path)
    for file in files:
      full_path = os.path.join(path, file)
      if os.path.isfile(full_path):
        os.remove(full_path)
      elif os.path.isdir(full_path):
        shutil.rmtree(full_path, ignore_errors=True)

    _logger.info('remove sub files of %s', path)

  except Exception as ex:
    _logger.info('error occurs while removing sub files of %s. ex=%s', path, str(ex))
  
def str_to_dict(s:str) -> Dict[str, str]:
  '''
  将字符串转换成字典
  字符串的格式是
  key1=value1,key2=value2,key3=value3
  如果有重复的键值，将会直接覆盖掉
  '''
  fields = s.split(',')
  ret:Dict[str,str] = dict()
  for field in fields:
    field = field.strip()
    kv = field.split('=')
    if len(kv) == 2:
      key = kv[0].strip()
      value = kv[1].strip()
      ret[key] = value

  return ret

def __test_split_and_strip_string():
  insert_sqls = split_and_strip_string(
    '''
    insert into t_group_by values(3, 1.0, 'a');
    insert into t_group_by_2 values(1, 10);

    
    ''')
  expected_sqls = [
          "insert into t_group_by values(3, 1.0, 'a');",
          "insert into t_group_by_2 values(1, 10);"
          ]
  if len(insert_sqls) != len(expected_sqls):
    print(f'invalid lens {len(insert_sqls)}, expected {len(expected_sqls)}')
    return

  for it1, it2 in zip(insert_sqls, expected_sqls):
    if it1 != it2:
      print(f'invalid string. it1={it1}, it2={it2}')
  #print(f'left string is equal to the right string')

def __test_strip_string_list():
  strings = [
    '   \t',
    '\n',
    'abcd',
    '\n',
    ' '
  ]
  strip_string_list(strings)
  print(f'strings after strip is "{strings}"')
  strings = strip_string_list(strings)
  print(f'2 strings after strip is "{strings}"')
if __name__ == '__main__':
  print(list_ipv4())
  __test_split_and_strip_string()
  __test_strip_string_list()
