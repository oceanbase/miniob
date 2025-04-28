#!/bin/env python3

import os
import subprocess
import sys
import logging
import socket
import select
import traceback
import time
from typing import Tuple
import psutil

_logger = logging.getLogger('miniob_client')

class MiniObClient:
    '''
    测试客户端。使用TCP连接，向服务器发送命令并反馈结果
    '''

    def __init__(self, *,
                 server_port: int = None,
                 server_socket: str = None,
                 time_limit:float = 10.0,
                 charset='UTF-8',
                 logger:logging.Logger=None):
        if (server_port != None and (server_port < 0 or server_port > 65535)) and server_socket is None:
            raise ValueError(f"Invalid server port: {server_port}")

        if logger:
            self.__logger = logger.getChild('miniob-client')
        else:
            self.__logger = _logger

        self.__server_port = server_port
        self.__server_socket = server_socket.strip()
        self.__time_limit = time_limit
        self.__socket = None
        self.__buffer_size = 8192
        self.__charset=charset

        if not self.__time_limit:
            self.__time_limit = 10.0
        self.__logger.info("time limit: %f", self.__time_limit)

        sock = None
        if len(self.__server_socket) > 0:
            sock = self.__init_unix_socket(self.__server_socket)
        else:
            sock = self.__init_tcp_socket(self.__server_port)

        self.__socket = sock
        if sock != None:
            self.__socket.setblocking(False)

            self.__poller = select.poll()
            self.__poller.register(self.__socket, select.POLLIN | select.POLLPRI | select.POLLHUP | select.POLLERR)

    def __init_tcp_socket(self, server_port:int):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(('127.0.0.1', server_port))
        return s

    def __init_unix_socket(self, server_socket: str):
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.connect(server_socket)
        return sock

    def is_valid(self) -> bool:
        return self.__socket is not None

    def __recv_response(self, timeout:float = None, *, total_timeout_seconds=None):
        result = ''

        if not timeout:
            timeout = self.__time_limit

        if timeout != None:
            timeout *= 1000

        deadline = time.time() + 3600*24
        if total_timeout_seconds != None and total_timeout_seconds > 0:
            deadline = time.time() + total_timeout_seconds

        while time.time() < deadline:
            events = self.__poller.poll(timeout) # timeout is in millisecond
            if len(events) == 0:
                raise TimeoutError(f'Poll timeout after {timeout/1000} second(s)')

            (_, event) = events[0]
            if event & (select.POLLHUP | select.POLLERR):
                msg = f"Failed to receive from server. poll return " \
                      f"POLLHUP={str(event & select.POLLHUP)} or POLLERR=str(event & select.POLLERR)"
                self.__logger.info(msg)
                raise socket.error(msg)

            data = self.__socket.recv(self.__buffer_size)
            if len(data) > 0:
                try:
                    if data[0] == 0 and len(result) == 0:
                        self.__logger.error('hnwyllmm receive from server \\0 byte')
                        ps_ef = subprocess.run(['ps', '-ef'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)

                        self.__logger.error(str(ps_ef.stdout))
                        self.__logger.error(str(ps_ef.stderr))
                except BaseException as ex:
                    self.__logger.error('hnwyllmm exception=%s', str(ex))

                result_tmp = data.decode(encoding= self.__charset)
                self.__logger.debug("receive from server[size=%d]: '%s'", len(data), result_tmp)
                if data[len(data) - 1] == 0:
                    result += result_tmp[0:-2]
                    return result.strip() + '\n'
                else:
                    result += result_tmp # TODO 返回数据量比较大的时候，python可能会hang住
                    # 可以考虑返回列表
            else:
                self.__logger.info("receive from server error. result len=%d", len(data))
                raise socket.error("receive return zero length data. the connection may be closed")

        if time.time() >= deadline:
            self.__logger.info('receive from server timeout. timeout=%f', total_timeout_seconds)

        raise TimeoutError(f'timeout {total_timeout_seconds} second(s)')


    def run_sql(self, sql: str, timeout=None, *, total_timeout_seconds=None) -> Tuple[bool, str]:
        '''
        向服务端发起SQL请求并接收结果
        如果发生异常或者超时，就返回False作为第一个参数。否则认为执行成功，并使用第二个参数返回结果
        '''
        if not sql:
            raise ValueError(f'invalid sql value: "{str(sql)}"')

        try:
            data = str.encode(sql, self.__charset)
            self.__socket.sendall(data)
            self.__socket.sendall(b'\0')
            self.__logger.debug("send command to server(size=%d) '%s'", len(data) + 1, sql)
            result = self.__recv_response(timeout=timeout, total_timeout_seconds=total_timeout_seconds)
            self.__logger.debug("receive result from server '%s'", result)
            return True, result
        except Exception as ex:
            self.__logger.error("Failed to send message to server: '%s'", str(ex))
            return False, str(ex)

    def close(self):
        if self.__socket:
            self.__socket.close()
            self.__socket = None
