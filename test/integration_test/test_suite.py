import logging
from typing import List
from test_case import TestCase
from test_initiator import TestInitiator, DefaultTestInitiator

_logger = logging.getLogger('TestSuite')


class TestSuite:
    """
    描述一个测试集合
    一个测试集合有一个测试初始化工作器，一系列测试用例。
    每次执行这一套测试的时候，先执行初始化，再执行一系列的测试用例。
    初始化，比如对测试源码做编译。
    """
    def __init__(self):
        self.test_initiator: TestInitiator = DefaultTestInitiator()
        self.test_cases: List[TestCase] = []

    def name(self) -> str:
        return self.test_initiator.name

    def description(self) -> str:
        return self.test_initiator.description
