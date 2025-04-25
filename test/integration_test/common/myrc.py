#!/bin/env python3

import os
import sys
import logging


class ReturnCode:
    SUCCESS = 0
    FAILED = 1
    NOENT = 2
    INVALID_ARGUMENT = 3
    EOF = 4
    UNSUPPORTED = 5
    EEXIST = 17
    IOERROR = 18
    TIMEOUT = 19

    GIT_CLONE_FAILED = 100
    GIT_CLONE_TIMEOUT = 101
    GIT_RESET_ERROR = 102

    COMPILE_CMAKE_ERROR = 200
    COMPILE_COMPILE_ERROR = 201

    RUN_CREATE_TABLE_ERROR = 300
    RUN_CREATE_INDEX_ERROR = 301
    RUN_LOAD_DATA_ERROR = 302
    RUN_LOAD_DATA_TIMEOUT = 303
    RUN_SELECT_ERROR = 304
    RUN_SELECT_TIMEOUT = 305
    RUN_START_SERVER_FAILED = 306
    RUN_START_CLIENT_FAILED = 307
    RUN_EXCEPITON = 308

    VALIDATION_ERROR = 400

    @staticmethod
    def to_string(rc: int):
        if rc == ReturnCode.SUCCESS:
            return "success"
        if rc == ReturnCode.FAILED:
            return "failed"
        if rc == ReturnCode.NOENT:
            return "noent"
        if rc == ReturnCode.COMPILE_CMAKE_ERROR:
            return "cmake error"
        if rc == ReturnCode.COMPILE_COMPILE_ERROR:
            return "make error"
        return "unknown"


class Result:
    def __init__(self):
        self.__rc = ReturnCode.SUCCESS
        self.__outputs = []
        self.__errors = []
        self.__body = None  # 返回成功时才有意思

    def __str__(self):
        return self.to_string()

    def to_string(self, pretty=False):
        delimeter = '\n' if pretty else '\\n'
        return f'rc={self.__rc}:{ReturnCode.to_string(self.__rc)},' \
               f'outputs={delimeter.join(self.__outputs)},errors={delimeter.join(self.__errors)},body={str(self.__body)}'

    def outputs(self):
        return self.__outputs

    def errors(self):
        return self.__errors

    def error_message(self):
        return ','.join(self.__errors)

    def append_output(self, line):
        self.__outputs.append(line)
        return self

    def append_error(self, fmt, *args):
        self.__errors.append(fmt % args)
        if self.__rc == ReturnCode.SUCCESS:
            self.__rc = ReturnCode.FAILED
        return self

    def append_errors(self, lines):
        for line in lines:
            self.__errors.append(line)
        return self

    def set_rc(self, rc: ReturnCode):
        self.__rc = rc

    def with_rc(self, rc: ReturnCode):
        self.__rc = rc
        return self

    def with_body(self, body):
        self.__body = body
        return self

    def rc(self):
        return self.__rc

    def body(self):
        return self.__body

    def ok(self):
        return self.__rc == ReturnCode.SUCCESS

    def __bool__(self):
        return self.ok()

    @staticmethod
    def failed(fmt, *args):
        result = Result()
        result.set_rc(ReturnCode.FAILED)
        result.append_error(fmt % args)
        return result

    @staticmethod
    def success(body=None):
        result = Result()
        return result.with_body(body)


class Error(Exception):
    def __init__(self, rc: ReturnCode, *, msg: str = None, ex: Exception = None):
        self.__rc = rc
        self.__message = msg
        self.__base_exception = ex

    def __str__(self):
        s = f'rc={self.__rc}, msg={self.__message}'
        if self.__base_exception:
            s += f'ex={str(self.__base_exception)}'
        return s

    def rc(self):
        return self.__rc

    def message(self):
        return self.__message

    def exception(self):
        return self.__base_exception

    @staticmethod
    def of(rc: ReturnCode, *, msg: str = None, ex: Exception = None):
        return Error(rc, msg=msg, ex=ex)


if __name__ == '__main__':
    a = 100
    result = Result.failed('failed=%d, b=%s', a, 'test').with_rc(1).append_errors(['a', 'b'])
    print(str(result))
    result = Result.failed('failed to test suite %s, case %s', 'abc', 'cde')
    print(str(result))
