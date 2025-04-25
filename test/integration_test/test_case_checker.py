import __init__

import sys
from typing import List, Dict, Union
import logging
import importlib
import importlib.util
import os
import os.path
import argparse
from common import mylog
from test_case import TestCase
from test_exception import TestPlatformException
from test_instruction import InstructionGroup, Instruction, RuntimeSqlInstruction, SortInstruction
from test_util import PythonTestCaseFetcher
from mysql_executor.mysql_executor import MysqlConnectConfig
from test_executor import TestExecutor, TestCaseExecutor
from test_result import TestCaseResult

'''
使用Python编写代码非常容易出现运行时错误，同样使用Python编写的测试用例也非常容易出问题。
这里提供了一个检查工具，以在发布线上之前提前运行检查，做一些预防。
示例：
检查某个目录下所有的问题
python3 miniob_test/test_case_checker.py --path miniob_test/test_cases/MiniOB-2023/python/

查看某个测试用例生成的指令
python3 miniob_test/test_case_checker.py --file miniob_test/test_cases/MiniOB-2023/python/function.py
'''

_logger = logging.getLogger('TestChecker')

def __argparse(args:List[str]):
    parser = argparse.ArgumentParser()
    parser.add_argument('--path', dest='path', default='', help='directory or file to list test cases')
    parser.add_argument('--command', dest='command', default='', choices=['try-load', 'dump-instruction', 'pre-check'], help='command to execute')
    parser.add_argument('--debug-log', dest='debug_log', default='', help='debug log file')

    mysql_group = parser.add_argument_group('mysql client config')
    mysql_group.add_argument('--mysql-host', dest='mysql_host', default='127.0.0.1', help='mysql host')
    mysql_group.add_argument('--mysql-port', dest='mysql_port', type=int, default=3306, help='mysql port')
    mysql_group.add_argument('--mysql-socket', dest='mysql_socket', default='/var/run/mysqld/mysqld.sock')
    mysql_group.add_argument('--mysql-user', dest='mysql_user', default='root')
    mysql_group.add_argument('--mysql-password', dest='mysql_password', default='')
    mysql_group.add_argument('--mysql-database', dest='mysql_database', default='test')

    return parser.parse_args(args)

def __get_mysql_config(args) -> MysqlConnectConfig:
    mysql_config = MysqlConnectConfig()
    mysql_config.host = args.mysql_host
    mysql_config.port = args.mysql_port
    mysql_config.unix_socket = args.mysql_socket
    mysql_config.user = args.mysql_user
    mysql_config.password = args.mysql_password
    mysql_config.database = args.mysql_database
    return mysql_config

def __instruction_string(instruction:Instruction) -> str:
    if isinstance(instruction, SortInstruction):
        instruction:SortInstruction = instruction
        s = __instruction_string(instruction.instruction)
        return s + ' -- sort'

    elif isinstance(instruction, RuntimeSqlInstruction):
        return instruction.next_sql()
    else:
        return instruction.show_request()

def __load_test_cases(path:str) -> List[TestCase]:
    fetcher = PythonTestCaseFetcher()
    if os.path.isdir(path):
        fetcher.list_test_cases_from_directory(path)
        test_cases = fetcher.test_cases()
    else:
        test_cases = fetcher.load_file(path)
        if isinstance(test_cases, list):
            print(f'got {len(test_cases)} test cases')
            for case in test_cases:
                print(case.name)
        elif isinstance(test_cases, TestCase):
            print(f'got a test case: {test_cases.name}')
            test_cases = [test_cases]
        else:
            print(f'got no test case or invalid test_case type : {type(test_cases)}')
            test_cases = []

    return test_cases

def __try_load(test_cases:List[TestCase], args) -> None:
    '''
    加载所有测试文件，打印测试用例名称
    '''
    for test_case in test_cases:
        print(test_case.name)
    print(f'{len(test_cases)} test cases loaded')


def __dump_instruction(test_cases:List[TestCase], args) -> None:
    '''
    打印某个测试用例的指令
    '''
    for index, test_case in enumerate(test_cases):
        if not isinstance(test_case, TestCase):
            print(f'invalid instance type: {type(test_case)}, index={index}')
            continue

        test_case:TestCase = test_case
        print(f'case name: {test_case.name}, trx model: {test_case.server_trx_model}')
        for group in test_case.instruction_groups:
            group:InstructionGroup = group
            print(f'  group name: {group.name}')
            for instruction in group.instructions:
                instruction:Instruction = instruction
                print(f'    {__instruction_string(instruction)}')

def __do_pre_check(test_case:TestCase, mysql_config:MysqlConnectConfig) -> bool:
    '''
    检查测试用例是否符合预期
    '''
    if not test_case.name:
        _logger.error('test case name is empty')
        return False

    test_executor = TestCaseExecutor(miniob_config=None, test_config=None, mysql_config=mysql_config, test_case=test_case, dryrun=True)
    result = test_executor.dryrun()
    for result_group in result.result_groups:
        for instruction_result in result_group.results:
            print(f'{instruction_result.instruction.request}, result set lines: {len(instruction_result.expected_messages)}')
            for message in instruction_result.expected_messages:
                print(f'  {message}')

def __pre_check(test_cases:List[TestCase], args) -> None:
    '''
    检查所有测试用例是否符合预期
    '''
    mysql_config = __get_mysql_config(args)

    for index, test_case in enumerate(test_cases):
        if not isinstance(test_case, TestCase):
            print(f'invalid instance type: {type(test_case)}, index={index}')
            continue

        test_case:TestCase = test_case
        print(f'-------- case name: {test_case.name}, trx model: {test_case.server_trx_model}')
        __do_pre_check(test_case, mysql_config)
        print(f'-------- case name: {test_case.name} done')

if __name__ == '__main__':
    '''
    可以使用当前程序进行case语法性质的校验
    示例：
    python3 miniob_test/test_util.py miniob_test/test_cases/MiniOB-2022/python/
    '''
    args = __argparse(sys.argv[1:])
    if args.debug_log:
        mylog.init_log(args.debug_log, level=logging.DEBUG)

    test_cases:List[TestCase] = __load_test_cases(args.path)

    runner_dict = {
        'try-load': __try_load,
        'dump-instruction': __dump_instruction,
        'pre-check': __pre_check
    }

    runner = runner_dict.get(args.command)
    if runner is None:
        print(f'unknown command: {args.command}')
        sys.exit(1)

    runner(test_cases, args)
