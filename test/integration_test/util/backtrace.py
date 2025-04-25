#!/bin/env python3

import os
import re
import subprocess
from typing import List

'''
给定一个可执行文件和core文件，获取core的栈信息

可以从指定的目录下，找到最新的文件，以此文件作为core文件
'''

def get_lastest_file(path:str, file_pattern:str):
  '''
  从指定的目录中找到某个最新的文件
  文件名字符合file_pattern命名格式
  '''
  
  files = os.listdir(path)
  latest_file_modified_time = 0.0
  latest_file = None
  for file in files:
    matched = re.search(file_pattern, file)
    if not matched:
      continue

    full_file_path = os.path.join(path, file)
    file_modified_time = os.path.getmtime(full_file_path)
    if file_modified_time > latest_file_modified_time:
      latest_file = full_file_path
      latest_file_modified_time = file_modified_time

  return latest_file

def get_core_backtrace(*, exec_file:str, core_file:str) -> List[str]:
  '''
  获取指定core文件的栈信息
  args:
    exec_file 产生core的进程的二进制文件，比如observer
    core_file 要获取栈信息的coredump文件
  
  将栈信息按行分隔返回列表
  '''
  backtrace_start_key_words = 'backtrace start'
  commands = ['gdb', exec_file, core_file, '-ex', f'echo {backtrace_start_key_words}\n', '-ex', 'backtrace', '--batch']
  process = subprocess.run(commands, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
  if process.returncode != 0 or (not process.stdout):
    raise Exception(f'failed to fetch backtrace info from core file({core_file}). error={process.stderr}, returncode={process.returncode}')

  output = process.stdout
  lines = output.splitlines()

  backtrace_lines = []
  backtrace_started = False
  for line in lines:
    line = line.strip()
    if backtrace_started:
      backtrace_lines.append(line)
      continue

    if line.startswith(backtrace_start_key_words):
      backtrace_started = True

  if not backtrace_lines:
    raise Exception('backtrace information not found')

  return backtrace_lines

if __name__ == '__main__':
  path = '.'
  core_file_pattern = 'core'
  core_file = get_lastest_file(path, core_file_pattern)
  exec_file = 'a.out'
  bt = get_core_backtrace(exec_file=exec_file, core_file=core_file)
  print('\n'.join(bt))
