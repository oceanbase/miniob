#!/bin/env python3

import os
import sys
import logging
import subprocess
import getpass
from typing import List, Dict

import __init__
from util import myutil

_logger = logging.getLogger('myshell')

def get_runuser_command(commands:List[str], user: str):
  # 如果使用 --preserve-environment 那么使用root用户切换到miniob运行时，
  # HOME 和 USER 环境变量会被保留，是/root和root，这样会导致miniob的配置文件路径不对
  return f'runuser -u {user}  -- {" ".join(commands)}'

def run_shell_command(command_args:List[str], cwd=os.getcwd(), env:Dict[str, str]=None, timeout=None, record_stdout=True, record_stderr=True, 
                      shell=False, user="root", start_new_session=False):
  '''
  运行shell命令，返回命令的执行结果码和输出到控制台的信息
  返回的控制台信息是每行一个字符串的字符串列表
  如果控制台信息输出比较多，不适合用PIPE
  NOTE: 返回的信息固定使用UTF-8解码
  @param command_args 是shell命令的每个字符串列表，比如 ['git','log','-1','--date=raw']
  subprocess.TimeoutExpired 超时抛出异常
  '''

  _logger.debug("running command: '%s' in cwd=%s", ' '.join(command_args), str(cwd))

  current_user = getpass.getuser()
  if user != current_user:
    command_args = get_runuser_command(command_args, user)
    shell = True

  stdout = subprocess.PIPE if record_stdout else subprocess.DEVNULL # will not record stdout
  stderr = subprocess.PIPE if record_stderr else subprocess.DEVNULL
    
  command_process = subprocess.Popen(command_args, cwd=cwd, env=env, stdout=stdout, stderr=stdout, 
      shell=shell, start_new_session=start_new_session)
  # communicate 返回的是bytes，不是str
  stdout, stderr = command_process.communicate(timeout=timeout) # 用communicate去通讯，防止卡死
  normal_outputs = []
  error_outputs = []
  if stdout != None:
    stdout = stdout.decode('UTF-8')
    normal_outputs = stdout.split('\n')
  if stderr != None:
    stderr = stderr.decode('UTF-8')
    error_outputs = stderr.split('\n')
    if len(error_outputs) == 1 and len(error_outputs[0]) == 0:
      error_outputs = []

  return_code = command_process.wait(timeout=timeout)
  if not return_code is None:
    return return_code, normal_outputs, error_outputs
  else:
    return -1, normal_outputs, error_outputs

if __name__ == '__main__':
  #rc, outputs, errors = run_shell_command(sys.argv[1:])
  #print('rc=%d' % rc)
  #if len(outputs) != 0:
  #  print(outputs) 
  #if len(errors) != 0:
  #  print(errors)

  commands = ['ps', '-o', 'pid,ruser,args', '-C', 'observer', '--no-headers']
  command = ' '.join(commands)
  r, outputs, errors = run_shell_command(command, shell=True)
  print(f'commands={command}')
  print(f'r={r},outputs={outputs}, errors={errors}')
