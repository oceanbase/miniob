from typing import List, Tuple, Any
import logging
from test_instruction import InstructionGroup

_logger = logging.getLogger('TestCase')


class TestCase:
    """
    描述一个测试用例
    一个测试用例有一个名字，还有一系列的指令组。
    指令组就是把指令按照特性或者关联性放到某一个组里面。
    指令除了告诉我们需要运行什么命令，还会告诉我们运行这个命令之后期望得到什么结果。
    """

    def __init__(self):
        self.name = None
        self.description = ''  # 把测试用例的描述直接写在这里
        self.instruction_groups: List[InstructionGroup] = []

        # 这个测试是否需要启动 observer。有些case，比如unittest，不需要启动服务端。
        self.need_observer: bool = True
        # 有些case也不需要启动 MySQL。MySQL用于动态生成的SQL语句，同时在 observer 和 mysql中运行，然后对比结果
        self.need_mysql: bool = True
        # observer 启动时的一些参数，这个是 trx 模型
        self.server_trx_model: str = None
        # miniob 使用的通讯协议，不指定时使用 plain 协议，可选字段 plain/mysql
        self.miniob_protocol: str = None
        # miniob 使用的存储引擎，不指定时使用默认的堆表存储引擎。
        self.storage_engine: str = None
        # miniob 使用的事务模型，不指定时使用默认的Vacuous 事务
        self.transaction_model: str = None

    def __str__(self):
        return f'name:{self.name}'

    def add_execution_group(self, name: str,
                            associate_instruction_groups: List[InstructionGroup] = None) -> InstructionGroup:
        group = InstructionGroup(name)
        if associate_instruction_groups:
            for index, associate_group in enumerate(associate_instruction_groups):
                if not isinstance(associate_group, InstructionGroup):
                    raise ValueError(
                        f'got one object is not instance of InstructionGroup. type is {str(type(associate_group))}. ' \
                        f'test case name: {self.name}, execute group name: {name}, associate index: {index}')
            group.add_associate_groups(associate_instruction_groups)

        self.instruction_groups.append(group)
        return group
