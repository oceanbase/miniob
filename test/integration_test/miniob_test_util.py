#!/bin/env python3

import grp
from util import myshell, backtrace, myutil
import signal
import os
import json
import psutil
import sys
import logging
import subprocess
import socket
import select
import time
import shutil
import getpass
import pwd
#import timeout_decorator
from enum import Enum
from miniob_test.test_result import TestCaseResult, EvalResult
from view_result_diff import ViewResultDiff
from typing import List, Dict, Tuple
#import eventlet
#from timeout_decorator import TimeoutError
try:
    from optparse import OptionParser
except:
    print("cannot load optparse module")
    exit(1)

"""
为OceanBase 大赛测试平台设计的自动化测试程序
测试流程：
获取源码 -> 
编译源码 ->
获取测试用例文件 ->
启动observer ->
执行测试用例 ->
对比执行结果与预先设置的结果文件

- 获取源码的方式：支持通过git获取，也可以指定源码的zip压缩包路径
- 编译源码：可以指定编译的cmake和make参数。也可以跳过这个步骤。
- 测试用例文件：测试用例文件都以.test结尾，当前放在test目录下，分为necessary和option(后续可以考虑删除)
- 测试结果文件：预先设置的结果文件，以.result结尾，放在result目录下
- 启动observer: 启动observer，使用unix socket，这样可以每个observer使用自己的socket文件
- 执行测试用例：测试用例文件中，每行都是一个命令。命令可以是SQL语句，也可以是预先定义的命令，比如 echo，sort等
- 评分文件：当前为 case-scores.json 文件，内容为json格式，描述每个case的分值
- 测试：使用参数直接连接已经启动的observer

"""

_logger = logging.getLogger("MiniOBTestUtil")

class TimeoutException(BaseException):
    def __init__(self, value="Timed Out"):
        self.value = value

    def __str__(self):
        return repr(self.value)

class Result(Enum):
    true = True
    false = False
    timeout = 0

class GlobalConfig:
    default_encoding = "UTF-8"
    debug = False

class ResultWriter:
    '''
    写数据到指定文件，当前用于输出测试结果
    '''

    def __init__(self, file):
        self.__file = file

    def __exit__(self, exc_type, exc_value, exc_tb):
        self.close()

    def close(self):
        if self.__file is not None:
            self.__file.close()
            self.__file = None

    def write(self, arg: str):
        self.__file.write(bytes(arg.upper(), GlobalConfig.default_encoding))

    def write_line(self, arg: str):
        self.write(str(arg).upper())
        self.write('\n')

class MiniObServer:
    '''
    用来控制miniob的服务器程序。负责程序的启停和环境的初始化和清理工作

    获取core堆栈信息使用说明：
    在测试过程中，Observer可能会挂掉，会有core文件，可以将core的栈信息当做错误信息返回用户，帮助用户查找问题。
    使用core的功能需要遵守一些约定：
    - core文件都放在某个目录下；如果是放在工作目录下，指定为 '.'，不建议指定为当前目录；
    - 在启动observer进程之前，会清空core目录下所有的文件。如果指定了当前工作目录，要小心；
    - 如果是不同的用户来测试，选择一个所有用户都有权限读写的目录最好；
    - 在查找core文件时，会在当前目录下，按照进程号，找最新的一个 core.`pid` 的文件。因此core目录下最好不要放其它文件，尤其是core开头的文件
    - core文件存放目录通过 /proc/sys/kernel/core_pattern 来配置
    - 配置  /proc/sys/kernel/core_uses_pid 为1，让core文件名称带上进程号
    - 注意配置 ulimit -c unlimited 防止core文件大小受限不能落下来
    '''

    def __init__(self, *,
                 base_dir: str,
                 data_dir: str,
                 config_file: str,
                 server_port: int,
                 server_socket: str,
                 clean_data_dir: bool,
                 logger:logging.Logger,
                 core_path:str=None):
        self.__check_base_dir(base_dir)
        self.__check_data_dir(data_dir, clean_data_dir)

        self.__base_dir = base_dir
        self.__data_dir = data_dir

        if config_file == None:
            config_file = self.__default_config(base_dir)
        self.__check_config(config_file)
        self.__config = config_file
        self.__server_port = server_port
        self.__server_socket = server_socket.strip()

        self.__process = None
        self.__core_path = core_path
        self.__logger:logging.Logger = logger

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, exc_tb):
        if self.__process is not None:
            self.stop_all_servers()
            self.clean()
            self.__process = None

    def __observer_path(self, base_dir: str):
        '''
        observer程序所在路径
        '''
        return base_dir + "/bin/observer"

    def __default_config(self, base_dir: str):
        return base_dir + "/etc/observer.ini"

    def __check_base_dir(self, base_dir: str):
        if not(os.path.isdir(base_dir)):
            raise(Exception("failed to check base directory. " + base_dir + " is not a directory"))

        observer_path = self.__observer_path(base_dir)
        if not(os.path.isfile(observer_path)):
            raise(Exception("observer not exists: " + observer_path))

    def __check_data_dir(self, data_dir: str, clean_data_dir: bool):
        if os.path.exists(data_dir) and clean_data_dir:
            shutil.rmtree(data_dir)

        os.makedirs(data_dir, exist_ok=True)
        if not(os.path.isdir(data_dir)):
            raise(Exception(data_dir + " is not a directory or failed to create"))

    def __check_config(self, config_file: str):
        if not(os.path.isfile(config_file)):
            raise(Exception("config file does not exists: " + config_file))

    def init_server(self):
        self.__logger.info("miniob-server inited")
        # do nothing now

    def stop_all_servers(self, wait_seconds=10.0):
        process_name = 'observer'
        observer_path = self.__observer_path(self.__base_dir)
        for p in psutil.process_iter():
            try:
                if p.name() != process_name:
                    continue
                cmdline = p.cmdline()
                if len(cmdline) >= 1:
                    binary_path = cmdline[0]
                    if binary_path != observer_path:
                        self.__logger.warn('got an observer process but it is not mine. skip. cmdline=%s, my path=%s',
                                           str(cmdline), observer_path)
                        continue

                try:
                    p.terminate()
                    p.wait(wait_seconds)
                except psutil.TimeoutExpired as ex:
                    self.__logger.warn('failed to wait observer terminated, kill it now. process=%s', str(p))
                    p.kill()
                self.__logger.info('process has been killed: %s', str(p))
            except Exception as ex:
                self.__logger.warn('got exception while try to get process info. exception=%s', str(ex))

    def start_server(self, test_user: str = ''):
        '''
        启动服务端程序，并使用探测端口的方式检测程序是否正常启动
        调试模式如果可以使用调试器启动程序就好了
        '''

        if self.__process != None:
            self.__logger.warn("Server has already been started")
            return False

        if self.__core_path:
            myutil.remove_sub_files(self.__core_path)

        time_begin = time.time()
        self.__logger.debug("use '%s' as observer work path", os.getcwd())
        observer_command = [self.__observer_path(self.__base_dir), '-f', self.__config]
        if len(self.__server_socket) > 0:
            observer_command.append('-s')
            observer_command.append(self.__server_socket)
        else:
            observer_command.append('-p')
            observer_command.append(str(self.__server_port))

        current_user = getpass.getuser()
        if not test_user:
            test_user = current_user

        if test_user != current_user:
            passwd = pwd.getpwnam(test_user)
            user_home, group_id = passwd.pw_dir, passwd.pw_gid
            group_name = grp.getgrgid(group_id).gr_name

            # 看起来这个命令多此一举，但是指定用户的HOME目录下，有些数据可能是当前脚本运行用户创建的，比如root用户
            # 所以指定用户可能没有权限
            chown_commands = ["chown", "-R", f'{test_user}:{group_name}', user_home]
            returncode, _, errors = myshell.run_shell_command(chown_commands)
            if returncode != 0:
                self.__logger.error('chown error, %s', '\n'.join(errors))
                return False

        shell = False
        # 这里做了切换用户的处理。但是应该是当前不是指定用户时才做切换
        if test_user != current_user:
            observer_command = "runuser -l " + test_user  + " -c \"cd " + self.__data_dir + " && " + ' '.join(observer_command) + "\""
            shell = True
            self.__logger.info("observer command: %s", observer_command)
        else:
            self.__logger.info("observer command: %s", ' '.join(observer_command))

        process = subprocess.Popen(observer_command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                   cwd=self.__data_dir, shell=shell, preexec_fn=os.setsid)
        #cwd=self.__data_dir, shell=shell, start_new_session=True, preexec_fn=os.setsid)
        return_code = process.poll()
        if return_code != None:
            self.__logger.error("Failed to start observer, exit with code %d", return_code)
            return False

        self.__logger.info('start subprocess with pid=%d', process.pid)
        #os.setpgid(process.pid, GlobalConfig.group_id)

        self.__process = process
        time.sleep(0.2)
        if not self.__wait_server_started(10):
            time_span = time.time() - time_begin
            self.__logger.error("Failed to start server in %f seconds", time_span)
            return False

        time_span = time.time() - time_begin
        self.__logger.info("miniob-server started in %f seconds", time_span)
        return True

    def stop_server(self):
        '''
        由于observer可能是使用shell=True的方式启动的，所以在使用subprocess.terminate终止
        进程时会失败，无法终止子进程，经过测试，使用 `start_new_session=True` 或
        preexec_fn=os.setsid 的方式启动，然后killpg的方式杀死进程组也失败了，所以改用
        psutil遍历所有进程然后杀死相关的进程的方法，参考stop_all_servers函数
        '''

        if self.__process == None:
            self.__logger.warning("Server has not been started")
            return True

        self.__logger.warning('terminate process')
        #self.__process.terminate()
        os.killpg(os.getpgid(self.__process.pid), signal.SIGTERM)
        return_code = -1
        try:
            return_code = self.__process.wait(10)
            if return_code is None:
                os.killpg(os.getpgid(self.__process.pid), signal.SIGKILL)
                self.__logger.warning("Failed to stop server: %s. process=%s", self.__base_dir, str(self.__process))
                return False
        except Exception as ex:
            # self.__process.kill()
            os.killpg(os.getpgid(self.__process.pid), signal.SIGKILL)
            self.__logger.warning("wait server exit timedout: %s. process=%s", self.__base_dir, str(self.__process))
            return False

        self.__logger.info("miniob-server exit with code %d. pid=%s", return_code, str(self.__process.pid))
        return True

    def clean(self):
        '''
        清理数据目录（如果没有配置调试模式）
        调试模式可能需要查看服务器程序运行的日志
        '''

        if GlobalConfig.debug is False:
            shutil.rmtree(self.__data_dir)
        self.__logger.info("miniob-server cleaned")

    def __check_unix_socket_server(self):
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
            errno = s.connect_ex(self.__server_socket)
            if errno == 0:
                return True
            else:
                self.__logger.debug("Failed to connect to server. err=%d:%s", errno, os.strerror(errno))
                return False

    def __check_tcp_socket_server(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            errno = s.connect_ex(('127.0.0.1', self.__server_port))
            if errno == 0:
                return True
            else:
                self.__logger.debug("Failed to connect to server. err=%d:%s", errno, os.strerror(errno))
                return False

    def __wait_server_started(self, timeout_seconds: int):
        deadline = time.time() + timeout_seconds

        while time.time() <= deadline:
            result = False
            if len(self.__server_socket) > 0:
                result = self.__check_unix_socket_server()
            else:
                result = self.__check_tcp_socket_server()
            if result:
                return result
            time.sleep(0.5)

        return False

    def coredump_info(self) -> str:
        '''
        如果observer core了，就返回core信息
        如果没有core，或者没有找到core信息，或者获取失败了，都返回None
        '''

        if not self.__process:
            self.__logger.debug('cannot got coredump info as there is no process')
            return None

        core_file_pattern = f'core'
        path = self.__core_path
        core_file = backtrace.get_lastest_file(path, core_file_pattern)
        if not core_file:
            self.__logger.debug('cannot find core file. path=%s, pattern=%s', path, core_file_pattern)
            return None

        try:
            backtraces = backtrace.get_core_backtrace(exec_file=self.__observer_path(self.__base_dir), core_file=core_file)
            self.__logger.debug('got backtrace of core file: %s', str(backtraces))
            return '\n'.join(backtraces[ : 10])
        except Exception as ex:
            self.__logger.info('failed to get core backtrace. core file=%s, ex=%s', core_file, str(ex))
            return None

class MiniObClient:
    '''
    测试客户端。使用TCP连接，向服务器发送命令并反馈结果
    '''

    def __init__(self, *,
                 server_port: int=-1,
                 server_socket: str='',
                 time_limit:int = 10,
                 logger:logging.Logger=None):
        if (server_port < 0 or server_port > 65535) and server_socket is None:
            raise(Exception("Invalid server port: " + str(server_port)))

        if logger:
            self.__logger:logging.Logger = logger
        else:
            self.__logger:logging.Logger = _logger

        self.__server_port = server_port
        self.__server_socket = server_socket.strip()
        self.__socket = None
        self.__buffer_size = 8192

        sock = None
        if len(self.__server_socket) > 0:
            sock = self.__init_unix_socket(self.__server_socket)
        else:
            sock = self.__init_tcp_socket(self.__server_port)

        self.__socket = sock
        if sock != None:
            self.__socket.setblocking(False)
            #self.__socket.settimeout(time_limit) # do not work

            self.__time_limit = time_limit
            self.__poller = select.poll()
            self.__poller.register(self.__socket, select.POLLIN | select.POLLPRI | select.POLLHUP | select.POLLERR)

    def __init_tcp_socket(self, server_port:int):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        errno = s.connect_ex(('127.0.0.1', server_port))
        if errno != 0:
            self.__logger.error("Failed to connect to server with port %d. errno=%d:%s",
                                server_port, errno, os.strerror(errno))
            s = None
        return s

    def __init_unix_socket(self, server_socket: str):
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        errno = sock.connect_ex(server_socket)
        if errno != 0:
            self.__logger.error("Failed to connect to server with address '%s'. errno=%d:%s",
                                server_socket, errno, os.strerror(errno))
            sock = None
        return sock

    def is_valid(self):
        return self.__socket is not None

    def __recv_response(self):
        result = ''

        while True:
            events = self.__poller.poll(self.__time_limit * 1000)
            if len(events) == 0:
                raise Exception('Poll timeout after %d second(s)' % self.__time_limit)

            (_, event) = events[0]
            if event & (select.POLLHUP | select.POLLERR):
                msg = "Failed to receive from server. poll return POLLHUP(%s) or POLLERR(%s)" % ( str(event & select.POLLHUP), str(event & select.POLLERR))
                self.__logger.info(msg)
                raise Exception(msg)

            data = self.__socket.recv(self.__buffer_size)
            if len(data) > 0:
                result_tmp = data.decode(encoding= GlobalConfig.default_encoding)
                self.__logger.debug("receive from server[size=%d]: '%s'", len(data), result_tmp)
                if data[len(data) - 1] == 0:
                    result += result_tmp[0:-2]
                    return result.strip() + '\n'
                else:
                    result += result_tmp # TODO 返回数据量比较大的时候，python可能会hang住
                    # 可以考虑返回列表
            else:
                self.__logger.info("receive from server error. result len=%d", len(data))
                raise Exception("receive return error. the connection may be closed")


    def run_sql(self, sql: str):
        try:
            data = str.encode(sql, GlobalConfig.default_encoding)
            self.__socket.sendall(data)
            self.__socket.sendall(b'\0')
            self.__logger.debug("send command to server(size=%d) '%s'", len(data) + 1, sql)
            result = self.__recv_response()
            self.__logger.debug("receive result from server '%s'", result)
            return True, result
        except Exception as ex:
            self.__logger.error("Failed to send message to server: '%s'", str(ex))
            return False, None

    def close(self):
        if self.__socket is not None:
            self.__socket.close()
            self.__socket = None

class CommandRunner:
    __default_client_name = "default"
    __command_prefix = "--"
    __comment_prefix = "#"

    def __init__(self, *,
                 result_writer: ResultWriter,
                 server_port: int,
                 unix_socket: str,
                 logger:logging.Logger=None):
        self.__result_writer = result_writer
        self.__clients:Dict[str, MiniObClient] = {}

        if logger:
            self.__logger:logging.Logger = logger
        else:
            self.__logger:logging.Logger = _logger

        # create default client
        default_client = MiniObClient(server_port=server_port, server_socket=unix_socket, logger=self.__logger)
        if not( default_client.is_valid()):
            self.__is_valid = False
        else:
            self.__is_valid = True
            self.__clients[self.__default_client_name] = default_client

        self.__current_client = default_client
        self.__server_port = server_port
        self.__unix_socket = unix_socket

    def is_valid(self):
        return self.__is_valid

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, exc_tb):
        self.close()

    def close(self):
        for client in self.__clients.values():
            client.close()
        self.__clients.clear()
        self.__current_client = None

    def run_connection(self, name: str):
        '''
        切换当前连接
        '''

        client = self.__clients[name]
        if client == None:
            self.__logger.error("No such client named %s", name)
            return False

        self.__current_client = client
        return True

    def run_connect(self, name: str):
        '''
        创建一个连接。每个连接有一个名字，可以使用使用connection name来切换当前的连接
        '''
        name = name.strip()
        if len(name) == 0:
            self.__logger.error("Found empty client name")
            return False

        client = self.__clients.get(name)
        if client != None:
            self.__logger.error("Client with name %s already exists", name)
            return False

        client = MiniObClient(server_port=self.__server_port, server_socket=self.__unix_socket, logger=self.__logger)
        if not(client.is_valid()):
            self.__logger.error("Failed to create client with name: %s", name)
            return False

        self.__clients[name] = client
        return True

    def run_echo(self, arg: str):
        '''
        echo 命令。参数可以是#开头的注释，这里不关心
        '''

        self.__result_writer.write_line(arg)
        return True

    def run_restart(self, arg: str):
        '''
        restart 命令。关闭client连接
        '''

        self.close()
        return True

    def do_restart(self):
        '''
        重新打开默认client连接
        '''

        default_client = MiniObClient(server_port=self.__server_port, server_socket=self.__unix_socket, logger=self.__logger)
        if not( default_client.is_valid()):
            self.__logger.error("Failed to recreate default client")
            return False
        else:
            self.__clients[self.__default_client_name] = default_client

        self.__current_client = default_client
        return True

    def run_sql(self, sql):
        self.__result_writer.write_line(sql)
        result, data = self.__current_client.run_sql(sql)
        if result is False:
            return False
        self.__result_writer.write(data)
        return True
    

    def run_ensure(self, command: str, sql: str):
        self.__result_writer.write_line(command + " " + sql)
        result, data = self.__current_client.run_sql("explain " + sql)
        if result is False:
            return False
        if command == "ensure:hashjoin":
            if len(data.split("HASH_JOIN")) != 2:
                return False
        elif command == "ensure:hashjoin*2":
            if len(data.split("HASH_JOIN")) != 3:
                return False
        elif command == "ensure:hashjoin*4":
            if len(data.split("HASH_JOIN")) != 5:
                return False
        elif command == "ensure:nlj":
            if len(data.split("NESTED_LOOP_JOIN")) != 2:
                return False
        elif command == "ensure:nlj*2":
            if len(data.split("NESTED_LOOP_JOIN")) != 3:
                return False
        else:
            _logger.error("No such ensure command %s", command)
            return False
        return True

    def run_sort(self, sql):
        self.__result_writer.write_line(sql)
        result, data = self.__current_client.run_sql(sql)
        if result is False:
            return False
        data_l = data.strip().split('\n')
        data_l.sort(key=str.upper)
        data = '\n'.join(data_l) + '\n'
        self.__result_writer.write(data)
        return result

    def run_command(self, command_line: str):
        '''
        执行一条命令。命令的参数使用空格分开, 第一个字符串是命令类型
        '''
        command_line = command_line[len(self.__command_prefix) : ]
        command_line = command_line.lstrip()
        args = command_line.split(' ', 1)
        command = args[0]
        restart = False

        command_arg = ''
        if len(args) > 1:
            command_arg = args[1]

        result = True
        if 'echo' == command:
            result = self.run_echo(command_arg)
        elif 'connect' == command:
            result = self.run_connect(command_arg)
        elif 'connection' == command:
            result = self.run_connection(command_arg)
        elif 'sort' == command:
            result = self.run_sort(command_arg)
        elif command.startswith('ensure'):
            result = self.run_ensure(command, command_arg)
        elif 'restart' == command:
            result = self.run_restart(command_arg)
            restart = True
        else:
            _logger.error("No such command %s", command)
            result = False

        return result, restart

    def run_anything(self, argline: str) -> Tuple[bool, bool]:
        '''
        运行一个SQL命令
        返回两个值，第一个表示命令是否执行成功，第二个表示是否需要重启
        TODO 命令是否运行成功应该区分是测试程序导致的失败，还是由于用户的程序导致的失败
        当前都认为是用户程序导致的失败
        '''
        argline = argline.strip()
        if len(argline) == 0:
            self.__result_writer.write_line('') # 读取到一个空行，也写入一个空行
            return True, False

        if argline.startswith(self.__comment_prefix):
            return True, False

        if argline.startswith(self.__command_prefix):
            return self.run_command(argline)

        return self.run_sql(argline), False

class TestCase:

    def __init__(self, is_necessary: bool, score: int):
        self.__name = ''
        self.__necessary = is_necessary
        self.__score = score
        self.__lines = []

    def init_with_file(self, name, filename):
        self.__name = name
        with open(filename, mode='r') as f:
            self.__lines = f.readlines()
        return True

    def init_with_content(self, name, lines):
        self.__name = name
        self.__lines = lines
        return True

    def command_lines(self):
        return self.__lines

    def get_name(self):
        return self.__name

    def is_necessary(self):
        return self.__necessary

    def get_score(self):
        return self.__score

    def result_file(self, base_dir):
        subdir = ''
        #if self.__necessary:
        #  subdir = self.NECESSARY_DIR
        #else:
        #  subdir = self.OPTION_DIR
        return base_dir + "/" + subdir + "/" + self.__name + ".result"

    def tmp_result_file(self, base_dir):
        result_file = self.result_file(base_dir)
        return result_file + '.tmp'

class TestCaseLister:

    def __init__(self, suffix = None):
        if suffix != None:
            self.__suffix = suffix
        else:
            self.__suffix = ".test"

    def list_by_test_score_file(self, test_scores, test_case_file_dir: str):
        '''
        从test-score文件中加载所有测试用例
        '''
        test_cases = []
        test_score_infos = test_scores.get_all()
        for case_name, test_score in test_score_infos.items():
            test_case = TestCase(test_score.is_necessary(), test_score.score())
            test_case_file = test_case_file_dir + '/' + case_name + self.__suffix
            test_case.init_with_file(case_name, test_case_file)
            test_cases.append(test_case)

        return test_cases

    def list_directory(self, base_dir : str, is_necessary: bool):
        test_case_files = []

        is_dir = os.path.isdir(base_dir)
        if False == is_dir:
            raise(Exception("Failed to list directory while getting test cases. " + base_dir + " is not a directory"))

        files = os.listdir(base_dir)
        for filename in files:
            _logger.debug("find file %s", filename)
            if filename.startswith('.'):
                continue

            full_path = base_dir + "/" + filename
            is_file = os.path.isfile(full_path)
            if False == is_file:
                continue
            if filename.endswith(self.__suffix):
                test_case_files.append(filename)

        test_cases = []
        for test_case_file in test_case_files:
            full_path = base_dir + "/" + test_case_file
            test_case_name = test_case_file[0 : -len(self.__suffix)]
            test_case = TestCase(is_necessary, 0)
            test_case.init_with_file(test_case_name, full_path)
            test_cases.append(test_case)
            _logger.debug("got a test case file %s", str(test_case_file))

        return test_cases

    def list_all(self, base_dir, test_names):
        is_dir = os.path.isdir(base_dir)
        if False == is_dir:
            raise("Failed to list all test cases. " + base_dir + " is not a directory")

        test_cases = []
        for test_name in test_names:
            full_path = base_dir + "/" + test_name + self.__suffix
            if not(os.path.isfile(full_path)):
                raise(Exception(full_path + " is not a file"))

            test_case = TestCase(False, 0)
            test_case.init_with_file(test_name, full_path)
            test_cases.append(test_case)
            _logger.debug("got a test case %s", test_case)

        return test_cases

class TestScore:
    def __init__(self, is_necessary: bool, score: int):
        self.__necessary = is_necessary
        self.__score = score

    def is_necessary(self):
        return self.__necessary
    def score(self):
        return self.__score

class TestScores:
    def __init__(self):
        self.__scores = {}
        self.__is_valid = False

    def is_valid(self):
        return self.__is_valid

    def init_file(self, fp):
        score_infos = json.load(fp)
        self.__init(score_infos)

    def init_content(self, content: str):
        score_infos = json.loads(content)
        self.__init(score_infos)

    def __init(self, score_info_dict: dict):
        scores = {}
        for name, score_info in score_info_dict.items():
            scores[name] = TestScore(score_info['necessary'], score_info['score'])

        self.__scores = scores
        self.__is_valid = True

    def is_necessary(self, name):
        if name in self.__scores.keys():
            return self.__scores[name].is_necessary()

        return None

    def acquire_score(self, name):
        if name in self.__scores.keys():
            return self.__scores[name].score()

        return None

    def get_all(self):
        return self.__scores

class TestSuite:

    def __init__(self):
        self.__report_only = False # 本次测试为了获取测试结果，不是为了校验结果
        self.__test_case_base_dir = "./test"
        self.__test_result_base_dir = "./result"
        self.__test_result_tmp_dir = "./result/tmp" # 生成的结果存放的临时目录
        self.__test_user = "root"
        self.__db_server_base_dir = None
        self.__db_data_dir = None
        self.__db_config = None
        self.__server_port = 0
        self.__use_unix_socket = False # 如果指定unix socket，那么就不再使用TCP连接
        self.__core_path:str = None
        self.__need_start_server = True
        self.__test_names = None # 如果指定测试哪些Case，就不再遍历所有的cases
        self.__miniob_server = None
        self.__test_case_scores = TestScores()

        self.__logger:logging.Logger = None

    def set_logger(self, logger:logging.Logger):
        self.__logger = logger

    def set_test_names(self, tests):
        self.__test_names = tests

    def set_test_case_base_dir(self, test_case_base_dir):
        self.__test_case_base_dir = test_case_base_dir

    def set_test_result_base_dir(self, test_result_base_dir):
        self.__test_result_base_dir = test_result_base_dir

    def set_test_result_tmp_dir(self, test_result_tmp_dir: str):
        self.__test_result_tmp_dir = test_result_tmp_dir
        os.makedirs(test_result_tmp_dir, exist_ok=True)
        self.__logger.info('create result tmp directory: %s', test_result_tmp_dir)
        if not(os.path.isdir(test_result_tmp_dir)):
            raise(Exception("Failed to set test result temp directory. " + test_result_tmp_dir + " is not a directory or failed to create"))

    def set_test_user(self, user: str):
        self.__test_user = user

    def set_test_case_scores(self, scores_path: str):
        if not scores_path:
            pass
        else:
            with open(scores_path) as fp:
                self.__test_case_scores.init_file(fp)

    def set_db_server_base_dir(self, db_server_base_dir):
        self.__db_server_base_dir = db_server_base_dir

    def set_db_data_dir(self, db_data_dir):
        self.__db_data_dir = db_data_dir

    def set_db_config(self, db_config):
        self.__db_config = db_config

    def set_server_port(self, server_port):
        self.__server_port = server_port

    def set_use_unix_socket(self, use_unix_socket: bool):
        self.__use_unix_socket = use_unix_socket

    def set_core_path(self, core_path:str):
        self.__core_path = core_path

    def donot_need_start_server(self):
        self.__need_start_server = False

    def set_report_only(self, report_only):
        self.__report_only = report_only

    def __compare_files(self, file1, file2):
        '''
        对比两个文件
        忽略文件维护的空行
        python 在读取到文件EOF时，会返回''
        '''
        differ: ViewResultDiff = ViewResultDiff()
        cmp_result = differ.get_diff_view(file1, file2)
        return not cmp_result

    def run_case(self, test_case:TestCase, timeout=20) -> TestCaseResult:

        try:
            ret = self.__run_case(test_case)
            return ret
        except TimeoutException as ex:
            test_case_result = TestCaseResult(test_case.get_name(), test_result=True, case_result=False, message='Timedout')
            return test_case_result

    def __run_case(self, test_case: TestCase) -> TestCaseResult:
        result_tmp_file_name = test_case.tmp_result_file(self.__test_result_tmp_dir)

        test_case_result:TestCaseResult = TestCaseResult(test_case.get_name(), test_result=True)

        unix_socket = ''
        if self.__use_unix_socket:
            unix_socket = self.__get_unix_socket_address()

        with open(result_tmp_file_name, mode='wb') as result_file:
            result_writer = ResultWriter(result_file)

            with CommandRunner(result_writer=result_writer,
                               server_port=self.__server_port,
                               unix_socket=unix_socket,
                               logger=self.__logger) as command_runner:
                if command_runner.is_valid() == False:
                    test_case_result.case_result = False
                    test_case_result.message = "The state of client is not valid. Maybase the server is failed"
                    return test_case_result

                for command_line in test_case.command_lines():
                    result, restart = command_runner.run_anything(command_line)
                    if result is False:
                        self.__logger.error("Failed to run command %s in case %s", command_line, test_case.get_name())
                        test_case_result.case_result = False
                        return test_case_result
                    if restart is True:
                        self.__clean_server_if_need()
                        # 如果是需要重启的测试，不删除之前的测试数据
                        result, message = self.__start_server_if_need(False)
                        if result is False:
                            self.__logger.error("Failed to restart server")
                            test_case_result.case_result = False
                            test_case_result.message = message
                            return test_case_result
                        result = command_runner.do_restart()
                        if result is False:
                            self.__logger.error("Failed to restart client")
                            test_case_result.case_result = False
                            test_case_result.message = "Failed to restart client. Maybe the server started failed"
                            return test_case_result

        result_file_name = test_case.result_file(self.__test_result_base_dir)
        if self.__report_only:
            os.rename(result_tmp_file_name, result_file_name)
            test_case_result.case_result = True
            return test_case_result
        else:
            result = self.__compare_files(result_tmp_file_name, result_file_name)
            if not GlobalConfig.debug:
                #os.remove(result_tmp_file_name)
                pass
            test_case_result.case_result = result
            return test_case_result

    def __get_unix_socket_address(self):
        return self.__db_data_dir + '/miniob.sock'

    def __get_all_test_cases(self) -> List[TestCase]:
        test_case_lister = TestCaseLister()
        test_cases = []
        if self.__test_case_scores.is_valid():
            test_cases = test_case_lister.list_by_test_score_file(self.__test_case_scores, self.__test_case_base_dir)
        else:
            test_cases = test_case_lister.list_directory(self.__test_case_base_dir, False) # is_necessary always be false if scores_path is invalid

        if not self.__test_names: # 没有指定测试哪个case
            self.__logger.info('got %d cases in directory %s', len(test_cases), self.__test_case_base_dir)
            return test_cases

        # 指定了测试case，就从中捞出来
        # 找出指定了要测试某个case，但是没有发现
        test_case_result = []
        for case_name in self.__test_names:
            found = False
            for test_case in test_cases:
                if test_case.get_name() == case_name:
                    test_case_result.append(test_case)
                    self.__logger.debug("got case: " + case_name)
                    found = True
            if found == False:
                self.__logger.error("No such test case with name '%s'", case_name)
                return []

        self.__logger.info("got %d test cases", len(test_case_result))
        return test_case_result

    def run(self, eval_result: EvalResult):

        # 找出所有需要测试Case
        test_cases = self.__get_all_test_cases()

        if test_cases is None or len(test_cases) == 0:
            self.__logger.info("Cannot find any test cases")
            return True

        self.__logger.info("Starting observer server")

        # 测试每个Case
        success_count = 0
        failure_count = 0
        for test_case in test_cases:
            try:
                # 每个case都清理并重启一下服务端，这样可以方式某个case core之后，还能测试其它case
                self.__clean_server_if_need()

                case_name: str = test_case.get_name()
                result, message = self.__start_server_if_need(True)
                if result is False:
                    eval_result.append(TestCaseResult(case_name, test_result = True, case_result = False, message=message))
                    return False

                self.__logger.info(test_case.get_name() + " starting ...")
                test_case_result:TestCaseResult = self.run_case(test_case)
                if self.__miniob_server:
                    coredump_info = self.__miniob_server.coredump_info()
                    if coredump_info:
                        test_case_result.message += coredump_info

                eval_result.append(test_case_result)
                if test_case_result.case_result:
                    self.__logger.info("Case passed: %s", test_case.get_name())
                    success_count += 1
                else:
                    if not test_case_result.case_result:
                        self.__logger.info("Case failed: %s", test_case.get_name())
                        failure_count += 1

            except Exception as ex:
                self.__logger.error("Failed to run case %s", test_case.get_name())
                eval_result.append(TestCaseResult(test_case.get_name(), test_result=False, case_result=False, message=str(ex)))
                self.__clean_server_if_need()
                raise ex

        self.__logger.info("All done. %d passed, %d failed", success_count, failure_count)
        self.__logger.debug(eval_result.get_message())

        self.__clean_server_if_need()
        return True

    def __start_server_if_need(self, clean_data_dir: bool) -> Tuple[bool, str]:
        if self.__miniob_server is not None:
            return (True, '')

        if self.__need_start_server:
            unix_socket = ''
            if self.__use_unix_socket:
                unix_socket = self.__get_unix_socket_address()

            miniob_server = MiniObServer(base_dir=self.__db_server_base_dir,
                                         data_dir=self.__db_data_dir,
                                         config_file=self.__db_config,
                                         server_port=self.__server_port,
                                         server_socket=unix_socket,
                                         clean_data_dir=clean_data_dir,
                                         core_path=self.__core_path,
                                         logger=self.__logger)
            miniob_server.init_server()
            result = miniob_server.start_server(self.__test_user)
            if result is False:
                self.__logger.error("Failed to start db server")
                coredump_info = miniob_server.coredump_info()
                miniob_server.stop_all_servers()
                miniob_server.clean()
                return (False, coredump_info)
            self.__miniob_server = miniob_server

        return (True, '')

    def __clean_server_if_need(self):
        if self.__miniob_server is not None:
            self.__miniob_server.stop_all_servers()
            # 不再清理掉中间结果。如果从解压代码开始，那么执行的中间结果不需要再清理，所有的数据都在临时目录
            # self.__miniob_server.clean()
            self.__miniob_server = None

def __init_options():
    options_parser = OptionParser()
    # 是否仅仅生成结果，而不对结果做校验。一般在新生成一个case时使用
    options_parser.add_option('', '--report-only', action='store_true', dest='report_only', default=False,
                              help='just report the result')
    # 测试case文件存放的目录
    options_parser.add_option('', '--test-case-dir', action='store', type='string', dest='test_case_base_dir', default='test',
                              help='the directory that contains the test files')
    # 测试case文件存放的目录
    options_parser.add_option('', '--test-case-scores', action='store', type='string', dest='test_case_scores', default='score.json',
                              help='a json file that records score of the test cases')
    # 测试结果文件存放目录
    options_parser.add_option('', '--test-result-dir', action='store', type='string', dest='test_result_base_dir', default='result',
                              help='the directory that contains the test result files')
    # 生成的测试结果文件临时目录
    options_parser.add_option('', '--test-result-tmp-dir', action='store', type='string', dest='test_result_tmp_dir', default='result/tmp',
                              help='the directory that contains the generated test result files')

    # 测试哪些用例。不指定就会扫描test-case-dir目录下面的所有测试用例。指定的话，就从test-case-dir目录下面按照名字找
    options_parser.add_option('', '--test-cases', action='store', type='string', dest='test_cases',
                              help='test cases. If none, we will iterate the test case directory. Split with \',\' if more than one')

    # 测试时服务器程序基础路径，下面包含bin/observer执行主程序和etc/observer.ini配置文件
    options_parser.add_option('', '--db-base-dir', action='store', type='string', dest='db_base_dir',
                              help='the directory of miniob database which db-base-dir/bin contains the binary executor file')

    # 测试时服务器程序的数据文件存放目录
    options_parser.add_option('', '--db-data-dir', action='store', type='string', dest='db_data_dir', default='miniob_data_test',
                              help='the directory of miniob database\'s data for test')

    # 服务程序配置文件
    options_parser.add_option('', '--db-config', action='store', type='string', dest='db_config',
                              help='the configuration of db for test. default is base_dir/etc/observer.ini')
    # 服务程序端口号，客户端也使用这个端口连接服务器。目前还不具备通过配置文件解析端口配置的能力
    options_parser.add_option('', '--server-port', action='store', type='int', dest='server_port', default=6789,
                              help='the server port. should be the same with the value in the config')
    options_parser.add_option('', '--use-unix-socket', action='store_true', dest='use_unix_socket',
                              help='If true, server-port will be ignored and will use a random address socket.')

    # 可以手动启动服务端程序，然后添加这个选项，就不会再启动服务器程序。一般调试时使用
    options_parser.add_option('', '--server-started', action='store_true', dest='server_started', default=False,
                              help='Whether the server is already started. If true, we will not start the server')

    options_parser.add_option('', '--core-path', action='store', dest='core_path', default='',
                              help='The directory that store the core files. It should be configurated by /proc/sys/kernel/core_pattern')

    # 是否启动调试模式。调试模式不会清理服务器的数据目录
    options_parser.add_option('-d', '--debug', action='store_true', dest='debug', default=False,
                              help='enable debug mode')

    # 测试时代码压缩文件的解压目录
    options_parser.add_option('', '--target-dir', action='store', type='string', dest='target_dir',
                              help='the working directory of miniob database')


    options, args = options_parser.parse_args(sys.argv[1:])
    return options

def __init_test_suite(options, *, logger:logging.Logger) -> TestSuite:
    test_suite = TestSuite()
    test_suite.set_logger(logger)
    test_suite.set_test_case_base_dir(os.path.abspath(options.test_case_base_dir))
    if options.test_case_scores:
        test_suite.set_test_case_scores(os.path.abspath(options.test_case_scores))
    test_suite.set_test_result_base_dir(os.path.abspath(options.test_result_base_dir))
    test_suite.set_test_result_tmp_dir(os.path.abspath(options.test_result_tmp_dir))
    test_suite.set_test_user(options.test_user)

    if options.db_base_dir is not None:
        test_suite.set_db_server_base_dir(os.path.abspath(options.db_base_dir))
    if options.db_data_dir is not None:
        test_suite.set_db_data_dir(os.path.abspath(options.db_data_dir))

    test_suite.set_server_port(options.server_port)
    test_suite.set_use_unix_socket(options.use_unix_socket)
    test_suite.set_core_path(options.core_path)

    if options.server_started:
        test_suite.donot_need_start_server()

    if options.db_config is not None:
        test_suite.set_db_config(os.path.abspath(options.db_config))

    if options.test_cases is not None:
        test_suite.set_test_names(options.test_cases.split(','))

    if options.report_only:
        test_suite.set_report_only(True)

    return test_suite

def run(options, *, logger:logging.Logger=None) -> EvalResult:
    '''
    return result, reason
    result: True or False

    '''

    if not logger:
        logger = _logger

    logger.info("miniob test starting ...")

    # 由于miniob-test测试程序导致的失败，才认为是失败

    eval_result = EvalResult()

    try:
        test_suite:TestSuite = __init_test_suite(options, logger=logger)

        if test_suite != None:
            result = test_suite.run(eval_result)
        # result = True
    except Exception as ex:
        logger.exception(ex)
        eval_result.set_test_result(False)
        #eval_result.clear_message()
        eval_result.append_message(str(ex.args))

    return eval_result

if __name__ == '__main__':
    os.setpgrp()
    options = __init_options()

    result, evaluation = run(options)

    exit_code = 0
    if result is False:
        exit_code = 1
    else:
        _logger.info(evaluation)
    exit(exit_code)

