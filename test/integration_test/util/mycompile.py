#!/bin/env python3

import os
import shutil
import logging
import traceback
from typing import List

from common import myrc as rc
from util import myshell
from common.mytime_meter import TimeMeter

_logger = logging.getLogger('mycompile')

class CompileRunner:
    def __init__(self, source_dir: str, build_dir: str = None,
                 rebuild: bool = True):
        self.__source_dir = source_dir
        self.__build_dir = build_dir
        self.__rebuild = rebuild

        if build_dir is None:
            self.__build_dir = source_dir + '/build'

    def init(self) -> rc.Result:
        if self.__rebuild:
            _logger.info('clean the build path while rebuild is need: %s', self.__build_dir)
            if os.path.exists(self.__build_dir):
                try:
                    shutil.rmtree(self.__build_dir)
                except Exception as ex:
                    pass

        try:
            os.makedirs(self.__build_dir, exist_ok=True)
        except Exception as ex:
            return rc.Result.failed('failed to create build dir: %s. exception=%s', self.__build_dir, traceback.format_exc())
        return rc.Result.success()

    def run_cmake(self, cmake_args: [str] = [], test_user: str = "root") -> rc.Result:
        commands = ["cmake", "-B", self.__build_dir, "--log-level=WARNING"]

        for arg in cmake_args:
            commands.append(arg)

        commands.append(self.__source_dir)
        _logger.info('cmake commands: %s', "[" + test_user + "] " + ' '.join(commands))
        returncode, outputs, errors = myshell.run_shell_command(commands, user=test_user)

        if returncode != 0:
            _logger.info('cmake error, %s', '\n'.join(errors))
            return rc.Result.failed('cmake failed').append_errors(errors).with_rc(rc.ReturnCode.COMPILE_CMAKE_ERROR)

        _logger.info('cmake done: builddir=%s', self.__build_dir)
        return rc.Result.success()

    def run_make(self, make_args: List[str] = None, test_user: str = "root") -> rc.Result:
        if make_args is None:
            make_args = []

        commands = ['make', '--silent', '-C', self.__build_dir]
        for arg in make_args:
            commands.append(arg)

        time_meter = TimeMeter()
        _logger.info('make commands: %s', "[" + test_user + "] " + ' '.join(commands))
        returncode, _, errors = myshell.run_shell_command(commands, record_stdout=True, user=test_user)
        time_meter.stop()

        result = rc.Result.success()
        if returncode != 0:
            _logger.info('make return error: %s', '\n'.join(errors))
            result = rc.Result.failed('compile failed').append_errors(errors).with_rc(rc.ReturnCode.COMPILE_COMPILE_ERROR)
        else:
            _logger.info('compile done')
        _logger.info('compile cost %.3f ms', time_meter.passed_ms())
        return result
   
