#!/bin/env python3

import os
import sys
import logging
import time
import subprocess
import socket
import traceback
import shutil
import psutil
import getpass
import pwd
import grp
import signal
from util import backtrace, myutil
from util import myshell

_logger = logging.getLogger('miniob_server')

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
                 observer_path: str,
                 data_dir: str,
                 config_file: str,
                 server_port: int,
                 server_socket: str,
                 clean_data_dir: bool,
                 logger:logging.Logger,
                 core_path:str=None,
                 trx_model:str=None,
                 protocol:str=None,
                 storage_engine:str=None):
        self.__check_data_dir(data_dir, clean_data_dir)

        self.__observer_path = observer_path
        self.__data_dir = data_dir

        self.__check_config(config_file)
        self.__config = config_file
        self.__server_port = server_port
        self.__server_socket = server_socket.strip()

        self.__process = None
        self.__core_path = core_path
        self.__logger:logging.Logger = logger

        self.__trx_model = trx_model
        self.__protocol = protocol
        self.__storage_engine = storage_engine

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, exc_tb):
        if self.__process is not None:
            self.stop_all_servers()
            self.clean()
            self.__process = None

    def __check_data_dir(self, data_dir: str, clean_data_dir: bool):
        if os.path.exists(data_dir) and clean_data_dir:
            shutil.rmtree(data_dir)

        os.makedirs(data_dir, exist_ok=True)
        if not(os.path.isdir(data_dir)):
            raise(Exception(data_dir + " is not a directory or failed to create"))

    def __check_config(self, config_file: str):
        if not config_file or not(os.path.isfile(config_file)):
            raise(Exception("config file does not exists: " + config_file))

    def init_server(self):
        self.__logger.info("miniob-server inited")
        # do nothing now

    def stop_all_servers(self, wait_seconds=10.0):
        process_name = 'observer'
        observer_path = self.__observer_path
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

                exitcode = 0
                try:
                    p.terminate()
                    exitcode = p.wait(wait_seconds)
                except psutil.TimeoutExpired as ex:
                    self.__logger.warn('failed to wait observer terminated, kill it now. process=%s', str(p))
                    p.kill()

                self.__logger.info('process has been killed: %s', str(p))
                if exitcode == -11: # -11: SIGSEGV
                    coreinfo = self.coredump_info()
                    if coreinfo:
                        self.__logger.warning('got core dump info: %s', coreinfo)

            except Exception as ex:
                self.__logger.warn('got exception while try to get process info. exception=%s', str(ex))
            finally:
                self.__process = None

    def start_server(self, test_user: str = '') -> bool:
        '''
        启动服务端程序，并使用探测端口的方式检测程序是否正常启动
        调试模式如果可以使用调试器启动程序就好了
        '''

        if self.__process != None:
            self.__logger.warn("Server has already been started")
            return False

        if self.__core_path:
            myutil.remove_sub_files(self.__core_path)

        self.__check_data_dir(self.__data_dir, False)

        time_begin = time.time()
        self.__logger.debug("use '%s' as observer work path", os.getcwd())
        observer_command = [self.__observer_path, '-f', self.__config]
        if len(self.__server_socket) > 0:
            observer_command.append('-s')
            observer_command.append(self.__server_socket)
        else:
            observer_command.append('-p')
            observer_command.append(str(self.__server_port))

        if self.__trx_model:
            observer_command.append('-t')
            observer_command.append(self.__trx_model)
        
        if self.__protocol:
            observer_command.append('-P')
            observer_command.append(self.__protocol)
        
        if self.__storage_engine:
            observer_command.append('-E')
            observer_command.append(self.__storage_engine)

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

    def stop_server(self) -> bool:
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
                self.__logger.warning("Failed to stop server: %s. process=%s", self.__observer_path, str(self.__process))
                return False
        except Exception as ex:
            # self.__process.kill()
            os.killpg(os.getpgid(self.__process.pid), signal.SIGKILL)
            self.__logger.warning("wait server exit timedout: %s. process=%s", self.__observer_path, str(self.__process))
            return False

        self.__logger.info("miniob-server exit with code %d. pid=%s", return_code, str(self.__process.pid))
        return True

    def clean(self):
        '''
        清理数据目录（如果没有配置调试模式）
        调试模式可能需要查看服务器程序运行的日志
        '''
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

    def __wait_server_started(self, timeout_seconds: float):
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
            self.__logger.info('cannot find core file. path=%s, pattern=%s', path, core_file_pattern)
            return None

        try:
            backtraces = backtrace.get_core_backtrace(exec_file=self.__observer_path, core_file=core_file)
            self.__logger.info('got backtrace of core file: %s', str(backtraces))
            return '\n'.join(backtraces[ : 15]) # 按照测试经验，10个长度太短了，可能只展示了`raise`等一些底层库的信息，应用的代码还没有展示
        except Exception as ex:
            self.__logger.info('failed to get core backtrace. core file=%s, ex=%s', core_file, str(ex))
            return None
    