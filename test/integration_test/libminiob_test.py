#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import logging
import traceback
import optparse
import shutil
import getpass
import pwd
import grp
from typing import Tuple, List

import __init__
from test_info import TestTask
from test_case import TestCase
from test_result import TestResult
from test_result import TestCaseResult
from common import myrc as rc, myrc
from common import mylog
from util import myshell
from util import mycompile
from test_executor import TestConfig, TestExecutor
import miniob_test_config

_logger = logging.getLogger('libminiob_test')

'''
miniob 官方测试平台使用
miniob 测试时，可以单独作为一个库，针对某个测试任务，给出测试参数，直接执行测试
示例：
python3 ./libminiob_test.py --repo=https://gitee.com/oceanbase/miniob --branch=master --commit-id=xx
python3 /root/source/ob-game-test/miniob_test/libminiob_test.py --log=/root/miniob_test/log/test.log \
  -c miniob_test/conf.ini --player hnwyllmm --repo https://gitee.com/oceanbase-game-test/miniob --no-cleanup --no-clone --no-compile
'''
class MiniobTestOptions:
    def __init__(self):
        self.report_only = False
        self.test_case_base_dir = ''
        self.test_case_scores = None
        self.test_result_base_dir = ''
        self.test_result_tmp_dir = ''
        self.test_user = "root"

        self.test_cases = None
        self.db_base_dir = ''
        self.db_data_dir = ''
        self.db_config = ''
        self.server_port = -1
        self.use_unix_socket = True
        self.server_started = False
        self.debug = False

        self.target_dir = ''
        self.core_path:str = None

class Tester:
    def __init__(self, test_task: TestTask, config:miniob_test_config.Configuration = None):
        self.__test_task = test_task
        self.__config:miniob_test_config.Configuration = config
        self.__logger = _logger

        if not config:
            self.__config = miniob_test_config.Configuration.DEFAULT

        self.__no_cleanup = False
        self.__no_compile = False

        '''
        self.__no_cleanup = True
        self.__no_compile = True
        '''

        self.__no_start_server = False

    def set_no_cleanup(self):
        self.__no_cleanup = True

    def set_no_compile(self):
        self.__no_compile = True
    def set_no_start_server(self):
        self.__no_start_server = True

    def close(self):
        pass

    def check_test_task(self) -> rc.Result:
        if not self.__test_task.git_repo:
            _logger.info("invalid task. git repo is none. task=%s", str(self.__test_task))
            return rc.Result.failed('invalid task. no git repo').with_rc(rc.ReturnCode.INVALID_ARGUMENT)

        player = self.__test_task.player
        if not player:
            player = 'unknown'
        self.__test_task.player = player

        self.__logger = _logger.getChild(player)
        return rc.Result.success()

    def __cleanup(self) -> rc.Result:
        task_config = self.__config.task_config(self.__test_task)
        work_dir = task_config.base_dir()
        try:
            shutil.rmtree(work_dir, ignore_errors=True)
            os.makedirs(work_dir, exist_ok=True)
            self.__logger.info('recreate base dir: %s', work_dir)
            return rc.Result.success()
        except Exception as ex:
            self.__logger.info('failed to recreate base dir: %s. exception=%s', work_dir, str(ex))
            return rc.Result.failed('failed to recreate base dir: %s', work_dir)

    def test(self) -> rc.Result:
        try:
            result = self._test()
            test_result = self.transform_result(result)
            return rc.Result.success(test_result).with_rc(test_result.return_code)
        except Exception as ex:
            return rc.Result.failed('exception in test: %s', traceback.format_exc())

    def transform_result(self, result: rc.Result) -> TestResult:
        test_case_results: List[TestCaseResult] = result.body()
        test_result: TestResult = TestResult(test_task=self.__test_task, case_results=test_case_results)

        task_config = self.__config.task_config(self.__test_task)
        repo_config = self.__config.repo_config()
        source_dir: str = task_config.source_dir()

        if result is None:
            test_result.set_result_code(rc.ReturnCode.FAILED)
        elif not result:
            test_result.set_message(result.error_message())
            test_result.set_result_code(result.rc())
            # 把编译错误/git clone等报错信息通过basic测试用例推送到前端
            basic_test_case = TestCase()
            basic_test_case.name = "basic"
            fake_basic_result = TestCaseResult(basic_test_case)
            fake_basic_result.result_groups = []
            fake_basic_result.test_exception = result.error_message()[0:4096]
            test_result.test_case_result.append(fake_basic_result)

        return test_result

    def _test(self) -> rc.Result:
        self.__logger.info('start test: %s', str(self.__test_task))
        result = rc.Result.success()
        test_config = self.__config.test_config()
        test_user = test_config.test_user()

        result = result if self.__no_cleanup else self.__cleanup()
        if not result:
            return result.append_error('cleanup failed')

        result = self.__create_miniob_test_config()
        if not result:
            return result.append_error('failed to create options')
        miniob_test_config: TestConfig = result.body()

        result.with_body(None)

        try:
            test_executor = TestExecutor(test_config=miniob_test_config, logger=self.__logger)
            test_case_results = test_executor.execute()
            self.__logger.info("miniob test done. result=%s",
                               ','.join((map(lambda test_case:str(test_case), test_case_results))))
            return result.with_body(test_case_results)
        except Exception as ex:
            self.__logger.warn('failed to run miniob test. exception=%s', traceback.format_exc())
            return result.with_rc(rc.ReturnCode.FAILED).append_error(str(ex))

    def __create_miniob_test_config(self) -> rc.Result:
        test_config = self.__config.test_config()
        task_config = self.__config.task_config(self.__test_task)
        source_dir: str = task_config.source_dir()
        build_dir: str = task_config.build_dir()
        cmake_args = test_config.compile_cmake_args()
        mysql_connect_config = self.__config.mysql_connect_config()

        miniob_test_config = TestConfig()
        # 为了支持一个测试程序可以同时测多个test_suite，测试用例的目录就多加一层
        miniob_test_config.test_case_dir   = os.path.join(os.path.dirname(os.path.abspath(__file__)), "test_cases", self.__test_task.test_suite)
        if test_config.test_case_type() == 'python':
            miniob_test_config.test_case_dir = os.path.join(miniob_test_config.test_case_dir, 'python')

        miniob_test_config.test_case_type  = test_config.test_case_type()
        miniob_test_config.test_user       = test_config.test_user()
        miniob_test_config.test_cases      = self.__test_task.cases
        miniob_test_config.observer_path   = os.path.join(task_config.db_base_dir(), 'bin', 'observer')
        miniob_test_config.db_data_dir     = task_config.db_data_dir()
        miniob_test_config.db_config       = task_config.db_config()
        miniob_test_config.use_unix_socket = True
        miniob_test_config.core_path       = test_config.core_path()

        miniob_test_config.source_dir = source_dir
        miniob_test_config.build_dir  = build_dir
        miniob_test_config.use_ccache = test_config.use_ccache()
        miniob_test_config.cmake_args = test_config.compile_cmake_args()
        miniob_test_config.make_args  = test_config.compile_make_args()

        miniob_test_config.mysql_enabled     = mysql_connect_config.enabled()
        miniob_test_config.mysql_host        = mysql_connect_config.host()
        miniob_test_config.mysql_port        = mysql_connect_config.port()
        miniob_test_config.mysql_unix_socket = mysql_connect_config.unix_socket()
        miniob_test_config.mysql_user        = mysql_connect_config.user()
        miniob_test_config.mysql_password    = mysql_connect_config.password()
        miniob_test_config.mysql_db          = f'miniob_test_{self.__test_task.player}'

        return rc.Result.success(miniob_test_config)

    def switch_user(self, test_user: str) -> rc.Result:
        self.__logger.info('switch user begin')
        result = rc.Result.success()

        current_user = getpass.getuser()
        if test_user == current_user:
            self.__logger.info('The user of testing is current user: %s', current_user)
            return result

        passwd = pwd.getpwnam(test_user)
        user_home, group_id = passwd.pw_dir, passwd.pw_gid
        group_name = grp.getgrgid(group_id).gr_name

        # 指定用户的HOME目录下有些数据可能是当前程序运行时复制过去的，而那个用户可能没有权限
        # 比如当前用户是root用户，复制了一些文件过去，就需要切换一下所有权
        commands = ["chown", "-R", f'{test_user}:{group_name}', user_home]
        returncode, _, errors = myshell.run_shell_command(commands)
        if returncode == -1:
            return result.append_error('failed to chown. ', errors)
        self.__logger.info('chown done')

        self.__logger.info('switch user done')
        return result

    def compile(self) -> rc.Result:
        self.__logger.info('compile begin')
        test_config = self.__config.test_config()
        task_config = self.__config.task_config(self.__test_task)
        source_dir: str = task_config.source_dir()
        build_dir: str = task_config.build_dir()
        test_user: str = test_config.test_user()
        cmake_args = test_config.compile_cmake_args()

        if test_config.use_ccache():
            cmake_args.append('-DCMAKE_C_COMPILER_LAUNCHER=ccache')
            cmake_args.append('-DCMAKE_CXX_COMPILER_LAUNCHER=ccache')

            # 这里通过环境变量设置ccache的配置，就会在全局生效，因此miniob-test不能支持一次运行多个选手的任务
            # 现在使用同一个源码目录，不再设置basedir
            # os.environ['CCACHE_BASEDIR'] = source_dir

            # 从最终的代码来看，使用ccache的话，只需要改配置文件即可，不需要修改这些东西。
            # cmake的参数是可以配置的。
            self.__logger.info('user ccache')
        else:
            self.__logger.info('does not use ccache')

        compiler = mycompile.CompileRunner(source_dir=source_dir, build_dir=build_dir)
        result = compiler.run_cmake(cmake_args, test_user)
        if not result:
            return result.append_error('failed to run cmake')

        self.__logger.info('make begin')
        make_args = test_config.compile_make_args()
        result = compiler.run_make(make_args, test_user)
        if not result:
            return result.append_error('failed to run make')

        self.__logger.info('compile done')
        return result

def __init_options(argv = sys.argv[1:]):
    parser = optparse.OptionParser()
    parser.add_option('-c', '--config', action='store', dest='config', default='/home/miniob/miniob_test/etc/conf.ini',
                      help='config file')
    parser.add_option('-l', '--log', action='store', dest='log', default=None)
    parser.add_option('', '--task-id', action='store', dest='task_id', default='default')
    parser.add_option('', '--player', action='store', dest='player', default='default_player')
    parser.add_option('', '--repo', action='store', dest='repo', default='https://github.com/oceanbase/miniob')
    parser.add_option('', '--branch', action='store', dest='branch', default='main')
    parser.add_option('', '--commit-id', action='store', dest='commit_id', default='')
    parser.add_option('', '--no-cleanup', action='store_true', dest='no_cleanup',
                      help='skip the clone')
    parser.add_option('', '--no-compile', action='store_true', dest='no_compile',
                      help='skip the compile')
    parser.add_option('', '--no-start-server', action='store_true', dest='no_start_server',
                      help='skip the start server')
    parser.add_option('', '--test-suite', action='store', dest='test_suite', default='MiniOB')

    options, args = parser.parse_args(argv)
    return options

if __name__ == '__main__':
    os.setpgrp()
    options = __init_options()
    mylog.init_log(options.log, level=logging.DEBUG)

    test_task = TestTask(task_id=options.task_id,
                         player=options.player,
                         git_repo=options.repo,
                         branch=options.branch,
                         commit_id=options.commit_id,
                         test_suite=options.test_suite)

    config = miniob_test_config.Configuration(options.config)
    config.init()

    tester = Tester(test_task, config=config)
    if options.no_cleanup:
        tester.set_no_cleanup()
    if options.no_compile:
        tester.set_no_compile()
    if options.no_start_server:
        tester.set_no_start_server()

    test_result = tester.test()
    tester.close()
    _logger.info('result = %s', str(test_result))

