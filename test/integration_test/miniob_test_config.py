#!/bin/env python3

import os
import sys
import logging
import traceback
import getpass

import __init__
from common.config_reader import ConfigurationReader
from common import myrc as rc
from util import myutil
import test_info

_logger = logging.getLogger('miniob_test_config')


class Configuration:
    DEFAULT = None

    def __init__(self, filename):
        self.__filename = filename

    def __str__(self):
        return f'Configuration{{{self.to_string()}}}'

    def to_string(self, pretty=False):
        delimiter = '\n' if pretty else ','

        return f'test_config={self.__test_config.to_string(pretty)}{delimiter}' \
               f'task_config={self.__task_config.to_string(pretty)}{delimiter}' \
               f'repo_config={self.__repo_config.to_string(pretty)}{delimiter}'

    def init(self) -> rc.Result:
        try:
            config = ConfigurationReader(self.__filename)
            self.__config = config
            self.__test_config = TestEnvConfiguration(config)
            self.__mysql_connect_config = MysqlConnectConfiguration(config)
            self.__task_config = TestTaskConfiguration(config)
            self.__repo_config = RepoConfiguration(config)
            return rc.Result.success()
        except Exception as ex:
            _logger.error('failed to read config. exception=%s', traceback.format_exc())
            return rc.Result.failed('failed to read config=%s', self.__filename)

    def test_config(self):
        return self.__test_config

    def mysql_connect_config(self):
        return self.__mysql_connect_config

    def src_task_config(self):
        return self.__task_config

    def task_config(self, test_task):
        return TestTaskConfigWrapper(self.__task_config, test_task)

    def repo_config(self):
        return self.__repo_config

    @staticmethod
    def set_default(config):
        Configuration.DEFAULT: Configuration = config


class TestEnvConfiguration:
    def __init__(self, config: ConfigurationReader):
        self.__config = config
        self.__section_name = 'test_env'
        self.__run_once_and_exit = False

        home_dir = os.environ.get('HOME', '/root')
        self.__default_test_base_dir = os.path.join(home_dir, 'miniob_test')

    def to_string(self, pretty=False):
        delimiter = '\n' if pretty else ','
        return f'env={self.env()},alipay_task_fetcher_name={self.alipay_task_fetcher_name()}{delimiter}' \
               f'alipay_task_fetcher_mock_file={self.alipay_task_fetcher_mock_file()}{delimiter}' \
               f'alipay_task_service_url={self.alipay_task_service_url()}{delimiter}' \
               f'task_queue_size={self.task_queue_size()},test_base_dir={self.test_base_dir()}{delimiter}' \
               f'test_suite={self.test_suite()},player={self.player()},case_name_map_file={self.case_name_map_file()}{delimiter}' \
               f'test_user={self.test_user()},auto_test_file={self.auto_test_file()},test_case_base_dir={self.test_case_base_dir()}{delimiter}' \
               f'compile_make_args={self.compile_make_args()}{delimiter}' \
               f'compile_cmake_args={self.compile_cmake_args()},run_once_and_exit={self.run_once_and_exit()},' \
               f'core_path={self.core_path()}, use_ccache={self.use_ccache()}'

    def env(self):
        return self.__config.get(self.__section_name, "env", "prod")

    def alipay_task_fetcher_name(self):
        return self.__config.get(self.__section_name, "alipay_task_fetcher_name", "default")

    def alipay_task_fetcher_mock_file(self):
        mock_file = self.__config.get(self.__section_name, "alipay_task_fetcher_mock_file", "alipay_mock.txt")
        return os.path.join(self.__auto_test_dir(), mock_file)

    def task_mock_file(self):
        mock_file = self.__config.get(self.__section_name, "task_mock_file", "mock_task.txt")
        return os.path.join(self.__auto_test_dir(), mock_file)

    def task_handle_name(self):
        return self.__config.get(self.__section_name, "task_handle_name", "default")

    def alipay_task_service_url(self):
        return self.__config.get(self.__section_name, "alipay_task_service_url", None)

    def task_queue_size(self):
        return self.__config.get_int(self.__section_name, "task_queue_size", 10)

    def test_base_dir(self):
        return self.__config.get(self.__section_name, "test_base_dir", self.__default_test_base_dir)

    def test_suite(self):
        '''
        test_suite is a name to exchange with `train` platform.
        eg: miniob, miniob-2022, oceanbase-2022
        '''
        return self.__config.get(self.__section_name, "test_suite", "miniob")

    def player(self):
        return self.__config.get(self.__section_name, "player", "")

    def case_name_map_file(self):
        return self.__config.get(self.__section_name, "case_name_map", "")

    def test_user(self):
        current_user = getpass.getuser()
        return self.__config.get(self.__section_name, "user", current_user)

    def log_file(self):
        return os.path.join(self.test_base_dir(), 'log', 'test.log')

    def log_level(self):
        return self.__config.get(self.__section_name, 'log_level', 'info')

    def __auto_test_dir(self):
        return os.path.join(self.test_base_dir(), 'auto_test')

    def auto_test_file(self):
        return os.path.join(self.__auto_test_dir(), 'test_task.txt')

    def test_case_base_dir(self):
        return os.path.join(self.test_base_dir(), 'test_cases')

    def test_case_type(self):
        return self.__config.get(self.__section_name, 'test_case_type', 'python')

    def compile_make_args(self):
        args = self.__config.get(self.__section_name, 'make_args', default='-j1', reparse=True)
        return myutil.remove_empty_strings(args.split(';'))

    def compile_cmake_args(self):
        args = self.__config.get(self.__section_name, 'cmake_args',
                                 default="-DCMAKE_COMMON_FLAGS='-w -fmax-errors=1';" \
                                         "-DWITH_UNIT_TESTS=OFF",
                                 reparse=True)
        return myutil.remove_empty_strings(args.split(';'))

    def use_ccache(self):
        return self.__config.get_bool(self.__section_name, 'use_ccache', 'false')

    def set_run_once_and_exit(self, run_once: bool):
        self.__run_once_and_exit = run_once

    def run_once_and_exit(self):
        return self.__run_once_and_exit

    def core_path(self):
        return self.__config.get(self.__section_name, 'core_path', default=None)

    def test_token(self):
        ## test=xxx,pre=xxxx,prod=xxx
        value = os.environ.get('TRAIN_API_TOKEN')
        return value


class MysqlConnectConfiguration:
    def __init__(self, config: ConfigurationReader):
        self.__config = config
        self.__section_name = 'mysql_connect'

    def enabled(self):
        return self.__config.get_bool(self.__section_name, 'enabled', 'false')

    def user(self):
        return self.__config.get(self.__section_name, 'user', 'root')

    def password(self):
        return self.__config.get(self.__section_name, 'password', '')

    def host(self):
        return self.__config.get(self.__section_name, 'host', '127.0.0.1')

    def port(self):
        return self.__config.get_int(self.__section_name, 'port', '3306')

    def unix_socket(self):
        return self.__config.get(self.__section_name, 'unix_socket', '/tmp/mysql.sock')

    def __str__(self) -> str:
        return f'enabled={self.enabled()}, user={self.user()}, password={self.password()}, ' \
               f'host={self.host()}, port={self.port()}, unix_socket={self.unix_socket()}'


class TestTaskConfiguration:
    '''
    基于一些基础配置，获取某个运行任务在运行时的一些环境配置
    '''

    def __init__(self, config: ConfigurationReader):
        self.__config = config
        self.__section_name = 'test_task'

        home_dir = "/tmp/miniob"
        self.__default_base_dir = os.path.join(home_dir, 'miniob_test/players')

    def to_string(self, pretty=False):
        delimiter = '\n' if pretty else ','
        return f'base_dir={self.__default_base_dir}'

    def base_dir(self, task_name):
        return self.__config.get(self.__section_name, "base_dir", self.__default_base_dir)
        # return os.path.join(self.__config.get(self.__section_name, "base_dir", self.__default_base_dir), task_name)

    def source_dir(self, task_name):
        return os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')

    def build_dir(self, task_name):
        return os.path.join(self.base_dir(task_name), 'build')

    def db_base_dir(self, task_name):
        return self.build_dir(task_name)

    def db_data_dir(self, task_name):
        return os.path.join(self.base_dir(task_name), 'data')

    def db_config(self, task_name):
        return os.path.join(self.source_dir(task_name), 'etc', 'observer.ini')

    def result_tmp_dir(self, task_name):
        return os.path.join(self.base_dir(task_name), 'result_tmp')
        #return os.path.join(self.db_data_dir(task_name), 'result_tmp')

    def result_tmp_file(self, task_name, case_name):
        return os.path.join(self.result_tmp_dir(task_name), case_name + '.result.tmp')


class TestTaskConfigWrapper:
    def __init__(self, config: TestTaskConfiguration, test_task: test_info.TestTask):
        self.__config = config
        self.__task_name = test_task.player if test_task.player else test_info.TestTask.default_player()

    def base_dir(self):
        return self.__config.base_dir(self.__task_name)

    def source_dir(self):
        return self.__config.source_dir(self.__task_name)

    def build_dir(self):
        return self.__config.build_dir(self.__task_name)

    def db_base_dir(self):
        return self.__config.db_base_dir(self.__task_name)

    def db_data_dir(self):
        return self.__config.db_data_dir(self.__task_name)

    def db_config(self):
        return self.__config.db_config(self.__task_name)

    def result_tmp_dir(self):
        return self.__config.result_tmp_dir(self.__task_name)

    def result_tmp_file(self, case_name):
        return self.__config.result_tmp_file(self.__task_name, case_name)


class RepoConfiguration:
    '''
    仓库相关的一些配置
    '''

    def __init__(self, config: ConfigurationReader):
        self.__config = config
        self.__section_name = 'repo'

    def __str__(self):
        return f'RepoConfiguration{{{self.to_string()}}}'

    def to_string(self, pretty=False):
        return f'token={self.gitee_token()},user={self.gitee_user()},clone_timeout={self.clone_timeout()}' \
               f', accept_public_repo={self.accept_public_repo()}'

    def github_user(self):
        value = self.__config.get(self.__section_name, 'github_user',
                                  default='xx', reparse=True)
        if not value:
            # read from environment
            return os.environ.get('GITHUB_USER')
        return value

    def github_token(self):
        value = self.__config.get(self.__section_name, 'github_token',
                                  default=None, reparse=True)
        if not value:
            return os.environ.get('GITHUB_TOKEN')
        return value

    def gitee_token(self):
        value = self.__config.get(self.__section_name, 'gitee_token',
                                  default=None, reparse=True)
        if not value:
            value = os.environ.get('GITEE_TOKEN')
        return value

    def gitee_user(self):
        value = self.__config.get(self.__section_name, 'gitee_user',
                                  default=None, reparse=True)
        if not value:
            value = os.environ.get('GITEE_USER')
        return value

    def clone_timeout(self):
        return self.__config.get_int(self.__section_name, 'clone_timeout_seconds', '600', reparse=True)

    def accept_public_repo(self) -> bool:
        return self.__config.get_bool(self.__section_name, 'accept_public_repo', 'true', reparse=True)


if __name__ == '__main__':
    config_file = None
    if len(sys.argv) >= 2:
        config_file = sys.argv[1]
    else:
        print('config file is None')
        exit(1)

    config = Configuration(config_file)
    result = config.init()
    print(f'init config result={result.to_string()}')
    print(config.to_string(pretty=True))
