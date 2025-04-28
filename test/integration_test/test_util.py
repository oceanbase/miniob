import __init__

import sys
from typing import List, Dict, Union
import logging
import importlib
import importlib.util
import os
import os.path
from test_case import TestCase
from test_suite import TestSuite
from test_initiator import DefaultTestInitiator, TestInitiator
from test_exception import TestPlatformException
from test_instruction import InstructionGroup, Instruction, RuntimeSqlInstruction, SortInstruction

_logger = logging.getLogger('TestUtil')


class TestCaseFetcher:
    """
    获取测试用例基类
    """

    def __init__(self):
        self._test_initiator: TestInitiator = None
        self._test_cases: Dict[str, TestCase] = {}

    @staticmethod
    def create(name: str) -> 'TestCaseFetcher':
        """
        创建一个测试用例获取器
        """
        if name.lower() == 'python':
            return PythonTestCaseFetcher()
        if name.lower() == 'text':
            return TextFileTestCaseFetcher()

        raise TestPlatformException(f'unknown test case fetcher: {name}')

    def name(self) -> str:
        return 'no-name'

    def list_test_cases_from_directory(self, path: str) -> None:
        """
        从指定目录中加载测试用例和测试初始化接口
        """
        pass

    def test_suite(self) -> TestSuite:
        if self._test_initiator is None:
            self._test_initiator = DefaultTestInitiator()

        test_suite = TestSuite()
        test_suite.test_cases = self._test_cases.values()
        test_suite.test_initiator = self._test_initiator
        return test_suite

    def test_initiator(self) -> TestInitiator:
        if not self._test_initiator:
            self._test_initiator = DefaultTestInitiator()
        return self._test_initiator

    def test_cases(self) -> List[TestCase]:
        """
        获取所有的测试用例
        """
        return self._test_cases.values()

    def get_test_cases_by_name(self, cases: List[str]) -> List[TestCase]:
        """
        根据名称获取测试用例
        """
        test_cases: List[TestCase] = []
        for case in cases:
            if case not in self._test_cases.keys():
                raise ValueError(f'no such case: {case}')

            test_cases.append(self._test_cases[case])

        return test_cases

    def add_test_case(self, test_case: TestCase) -> None:
        _logger.debug(f'got a test case: {test_case.name}')

        if not test_case.name:
            raise ValueError('cannot add test case without name')

        if test_case.name in self._test_cases.keys():
            raise ValueError(f'got a duplicate test case. name={test_case.name}')

        self._test_cases[test_case.name] = test_case
        self._test_cases = dict(sorted(self._test_cases.items(), key=lambda item: item[0]))


class PythonTestCaseFetcher(TestCaseFetcher):
    """
    从Python文件中加载测试用例
    """

    def __init__(self):
        super().__init__()
        self._test_suite_init_file = 'init.py'  # init.py 作为初始文件

    # override
    def name(self) -> str:
        return 'python'

    # override
    def list_test_cases_from_directory(self, path: str) -> None:
        """
        从指定目录加载所有的测试用例和初始化文件
        """
        if not os.path.isdir(path):
            raise ValueError(f'path is not a directory: {path}')

        files = os.listdir(path)

        # list all .py files
        files = filter(lambda filename: filename.endswith('.py') and len(filename) > 3, files)
        files = filter(lambda filename: not filename.startswith('.'), files)
        files = filter(lambda filename: filename != self._test_suite_init_file, files)
        # changes to fullpath name
        files = map(lambda filename, filepath=path: os.path.join(filepath, filename), files)
        # real file and not hidden
        files = filter(lambda full_filename: os.path.isfile(full_filename), files)

        for file in files:
            test_cases = self.load_file(file)
            for test_case in test_cases:
                self.add_test_case(test_case)

        init_file = os.path.join(path, self._test_suite_init_file)
        if not os.path.isfile(init_file):
            self._test_initiator = DefaultTestInitiator()
        else:
            self._test_initiator = self.load_initiator_from_file(init_file)

    def load_file(self, file: str) -> List[TestCase]:
        _logger.info('load test cases from file: %s', file)

        if not os.path.isfile(file):
            raise ValueError(f'the file given is not a regular file: {file}')

        base_name = os.path.splitext(os.path.basename(file))[0]  # 获取去掉后缀的
        module = self.get_exec_module('test_case_', file)
        test_cases: Union[TestCase, List[TestCase]] = module.create_test_cases()

        if isinstance(test_cases, TestCase):
            if not test_cases.name:
                test_cases.name = base_name
            return [test_cases]

        for test_case in test_cases:
            if not test_case.name:
                test_case.name = base_name

        return test_cases

    def load_initiator_from_file(self, init_file: str) -> TestInitiator:
        _logger.info('load test initiator from file: %s', init_file)

        if not os.path.isfile(init_file):
            raise ValueError(f'the file given is not a regular file: {init_file}')

        module = self.get_exec_module('test_init_', init_file)
        test_initiator = module.create_initiator()
        return test_initiator

    def get_exec_module(self, module_prefix: str, file: str):
        try:
            base_name = os.path.splitext(os.path.basename(file))[0]  # 获取去掉后缀的
            module_name = f'{module_prefix}{base_name}'
            spec = importlib.util.spec_from_file_location(module_name, file)
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            return module
        except Exception as ex:
            _logger.error(f'failed to load test case from file: {file}. exception={str(ex)}')
            raise ex


class TextFileTestCaseFetcher(TestCaseFetcher):
    """
    文本文件测试模式的测试用例获取工具
    """

    def __init__(self):
        super().__init__()
        self.__test_suffix = '.test'
        self.__result_suffix = '.result'

    # override
    def name(self) -> str:
        return 'text'

    def list_test_cases_from_directory(self, path: str) -> None:
        """
        从指定目录加载测试用例
        测试用例在path/test 测试结果文件在path/result 下
        """
        if not os.path.isdir(path):
            raise ValueError(f'cannot get test cases. path is not a directory: {path}')

        test_dir = os.path.join(path, 'test')
        result_dir = os.path.join(path, 'result')
        if not os.path.isdir(test_dir):
            raise ValueError(f'cannot get test cases. test directory not exists: {test_dir}')

        if not os.path.isdir(result_dir):
            raise ValueError(f'cannot get test cases. result directory not exists: {result_dir}')

        test_case_names: List[str] = []
        for test_file in os.listdir(test_dir):
            _logger.info(f'got a test file: {test_file}')
            full_filename = os.path.join(test_dir, test_file)
            if not os.path.isfile(full_filename):
                continue

            if not full_filename.endswith(self.__test_suffix):
                continue

            test_case_name = test_file[:-len(self.__test_suffix)]
            result_file = os.path.join(result_dir, test_case_name + self.__result_suffix)
            if not os.path.isfile(result_file):
                _logger.warning(f'no result file for test file: {test_file}')
                continue

            test_case_names.append(test_case_name)

            test_case = self.load_file(path, test_case_name)
            self.add_test_case(test_case)

    def load_file(self, basedir: str, test_case_name: str) -> TestCase:
        test_file = os.path.join(basedir, 'test', test_case_name + self.__test_suffix)
        result_file = os.path.join(basedir, 'result', test_case_name + self.__result_suffix)
        if not os.path.isfile(test_file) or not os.path.isfile(result_file):
            raise ValueError(f'failed to load text file test case. ' \
                             f'cannot find test file: {test_file} or result file: {result_file}')

        test_content: str = None
        result_content: str = None
        try:
            with open(test_file, 'r') as test_fp, open(result_file, 'r') as result_fp:
                test_content = test_fp.read()
                result_content = result_fp.read()
        except Exception as ex:
            raise ValueError(f'failed to load text file test case. ' \
                             f'exception={str(ex)}. test file = {test_file}, result file = {result_file}')

        test_case = TestCase()
        test_case.name = test_case_name
        instruction_group: InstructionGroup = test_case.add_execution_group('default')
        instruction_group.add_chunck_instruction(test_string=test_content, result_string=result_content)
        return test_case
