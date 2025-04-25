import os
import shutil
import subprocess
import re
from typing import List, Tuple
import logging
from enum import Enum

import __init__
from test_request import Request
from test_response import NormalResponseMessage, Response, ResponseMessage, DebugResponseMessage
from test_config import TestConfig
from test_exception import TestUserException
import util.myutil

_logger = logging.getLogger("Instruction")


class CommandName:
    ECHO = "echo"
    SORT = "sort"
    SQL = "sql"  # 静态SQL执行运行后就知道运行结果的SQL
    RUNTIME_SQL = "runtime_sql"  # 运行时生成的SQL，并且通过动态检测到执行结果的SQL
    CONNECT = "connect"  # 新建一个连接。connect `connection name`, 例如 connect user1
    CONNECTION = "connection"  # 切换连接。connection `connection name`, 例如 connection user1
    RESTART = "restart"  # 重启数据库
    CHUNK = "chunk"  # 一块指令，通常是从类似mysql test的文本文件中读取的test 文件加上result文件
    EXECUTABLE = "executable"  # 可以直接执行的指令


class ResultString:
    SUCCESS = "SUCCESS"
    FAILURE = "FAILURE"


class Instruction:
    """
    表示一个指令
    指令通常是由一个命令和一个预期的执行结果组成
    """

    def __init__(self):
        self.request: Request = None
        self.expected_response: Response = Response()
        self.timeout_seconds: int = 30  # 不精确的超时时间

    # virtual method
    def show_request(self) -> str:
        """
        返回结果时展示给用户看的信息
        """
        return self.request.data

    def __str__(self):
        return f'request={self.request},timeout_seconds={self.timeout_seconds},expected_response={self.expected_response}'


class EchoInstruction(Instruction):
    def __init__(self, message):
        super().__init__()
        self.request = Request(CommandName.ECHO, message)
        self.expected_response.messages.append(NormalResponseMessage(message))


class ConnectInstruction(Instruction):
    def __init__(self, message):
        super().__init__()
        self.request = Request(CommandName.CONNECT, f'connect {message}')
        # self.expected_response.messages.append(NormalResponseMessage(message)) # 执行只有成功失败，没有输出
        self.connection_name = message

    def show_request(self) -> str:
        return f'connect {self.connection_name}'

    def __str__(self):
        return super().__str__() + f',connection {self.connection_name}'


class ConnectionInstruction(Instruction):
    def __init__(self, message):
        super().__init__()
        self.request = Request(CommandName.CONNECTION, f'connection {message}')
        # self.expected_response.messages.append(NormalResponseMessage(message)) # 执行只有成功失败，没有输出
        self.connection_name = message

    def show_request(self) -> str:
        return f'connection {self.connection_name}'

    def __str__(self):
        return super().__str__() + f',connection {self.connection_name}'


class RestartInstruction(Instruction):
    """
    重启服务端
    所有的客户端连接都断开，重启后自动初始化默认连接，但是其他连接都不会恢复
    """

    def __init__(self, force_stop: bool = False):
        super().__init__()
        # force_stop not used currently
        self.force_stop: bool = force_stop
        self.request = Request(CommandName.RESTART, 'restart')

    # override
    def show_request(self) -> str:
        return 'restart'


class SortInstruction(Instruction):
    def __init__(self, instruction: Instruction):
        super().__init__()
        self.instruction = instruction
        self.request = instruction.request

        # 这里的expected_response 可能是空的，在运行时才生成
        # self.expected_response.messages = self.instruction.expected_response.messages.copy()
        # self.expected_response.messages.sort(key=lambda message:message.message)

    def show_request(self) -> str:
        return self.instruction.show_request()

    @staticmethod
    def sort_messages(messages: List[ResponseMessage]) -> List[ResponseMessage]:
        return_messages: List[ResponseMessage] = []
        normal_messages: List[NormalResponseMessage] = []
        for message in messages:
            if isinstance(message, NormalResponseMessage):
                normal_messages.append(message)
            else:
                return_messages.append(message)

        normal_messages.sort(key=lambda message: message.message.upper())
        return_messages.extend(normal_messages)
        return return_messages


class SqlInstruction(Instruction):
    """
    表示一个SQL命令
    SQL命令通常是由一个SQL语句和单条或多条期望的结果
    """

    def __init__(self, sql: str, *, expected: str = None, expected_messages: List[str] = None):
        super().__init__()
        if not isinstance(sql, str):
            raise ValueError(f'sql should be a string. sql={sql}')

        self.request = Request(CommandName.SQL, sql)
        if (expected is None and expected_messages is None) or (expected and expected_messages):
            raise ValueError(
                f'use either expected or expected_messages, but not both. expected={str(expected)}, '
                f'expected_messages={str(expected_messages)}')

        if expected:
            self.expected_response.messages.append(NormalResponseMessage(expected))

        if expected_messages:
            self.expected_response.messages.extend(
                map(lambda message: NormalResponseMessage(message), expected_messages))

    def show_request(self) -> str:
        return super().show_request()

    def __str__(self):
        return super().__str__() + f',sql={self.request.data},expected_response={self.expected_response}'

class EnsureSqlInstruction(Instruction):
    """
    表示一个ensure SQL命令
    用来检查SQL 执行计划是否符合预期
    """

    def __init__(self, sql: str, *, ensure: str = None):
        super().__init__()
        if not isinstance(sql, str):
            raise ValueError(f'sql should be a string. sql={sql}')

        self.request = Request(CommandName.SQL, "EXPLAIN " + sql)
        if (ensure is None):
            raise ValueError(
                f'ensure is None ')
        self.ensure = ensure

    def show_request(self) -> str:
        return super().show_request()

    def __str__(self):
        return super().__str__() + f',sql={self.request.data},ensure={self.ensure}'



class RuntimeSqlInstruction(Instruction):
    """
    RuntimeSql 是一种可以在运行时生成SQL语句的指令。并且它的期望结果会在运行时动态获取。
    NOTE: 使用RuntimeSqlInstruction时，由于期望的结果是动态获取的，对应的SQL语句会在MySQL中也运行一次。
    所以不要将RuntimeSQL与SqlInstruction放在同一个测试用例文件中。
    """

    class ResultType(Enum):
        """
        指示当前SQL语句执行结果的类型。
        比如DDL语句与DQL语句返回的结果类型差别很大，不能直接通过比较字符串来判断是否相等
        """
        BOOLEAN = 1  # 返回一个布尔值，就是只返回成功或失败。当前miniob不会考虑返回失败的原因
        RESULT_SET = 2  # 返回一个结果集，比如select语句

    def __init__(self, *, sql: str, result_type: ResultType, remove_header: bool = False):
        super().__init__()
        if not isinstance(sql, str):
            raise ValueError(f'sql should be a string. sql={sql}')

        self.request = Request(CommandName.RUNTIME_SQL, sql)
        self.mysql_request: Request = None
        self.result_type: RuntimeSqlInstruction.ResultType = result_type

        # 是否删除结果集的头，就是表头信息
        # 删除表头后可以更容易的跟mysql获取到的结果做对比
        self.remove_header: bool = remove_header

    def show_request(self) -> str:
        """
        返回结果时展示给用户看的信息。
        因为这里的语句可能运行时动态生成，所以这个指令通常会在运行时被记录下来，并在执行结果中记录生成后的指令
        所以这个方法不应该被调用到
        """
        raise ValueError(f"I'm runtime sql instruction. You should not call this method. request={self.request}")

    def next_sql(self) -> str:
        return self.request.data  # 用于后续扩展，支持动态生成SQL语句
    
    def next_mysql_sql(self) -> str:
        if self.mysql_request:
            return self.mysql_request.data
        else:
            return self.request.data


class RequestIncludingInstruction(Instruction):
    """
    有些命令的输出结果需要包含请求信息，这里做一个wrap。
    这个指令是为了适配mysql test格式的测试用例。有些命令，比如sql命令，同时会在输出文件中，将sql命令
    也输出到结果文件中。这个指令就是为了适配这种情况。
    NOTE: 这个指令是之前做文本格式的测试用例时写的适配，后面不再使用。找机会删掉
    """

    def __init__(self, instruction: Instruction, *, command: str = None):
        super().__init__()
        self.instruction = instruction
        self.request = instruction.request
        if not command:
            self.expected_response.messages = [NormalResponseMessage(self.request.command)]
        else:
            self.expected_response.messages = [NormalResponseMessage(command)]

        self.expected_response.messages.extend(self.instruction.expected_response.messages.copy())


class ChunkInstruction(Instruction):
    """
    表示一块指令
    通常是从类似mysql test的文本文件中读取的test 文件加上result文件。
    测试命令一行一个，会忽略掉空行，但是空行也会输出一个空行到结果中。
    NOTE: 这个指令是之前做文本格式的测试用例时写的适配，后面不再使用。找机会删掉
    """

    def __init__(self, *, test_string: str, result_string: str):
        super().__init__()
        self.test_string = test_string
        self.result_string = result_string

        self.request = Request(CommandName.CHUNK, test_string)
        result_lines = result_string.splitlines()
        response_messages: List[NormalResponseMessage] = [NormalResponseMessage(result_line) for result_line in
                                                          result_lines]
        self.expected_response.messages.extend(response_messages)


class ExecutableInstruction(Instruction):
    """
    可直接运行的指令
    比如单元测试。编译完成后，直接运行指定文件。
    """

    def __init__(self):
        super().__init__()
        self._test_config: TestConfig = None
        self._logger = _logger

    def run(self, *, test_config: TestConfig, logger: logging.Logger = None) -> Tuple[List[str], float]:
        """
        运行指令
        运行的结果仅用于展示，当前不需要动态的判断展示的结果。
        运行执行失败，通过抛异常的方式提出。
        """
        self._test_config = test_config
        if logger:
            self._logger = logger

        return self._run()

    def _run(self) -> Tuple[List[str], float]:
        return [], 0.0


class ExecuteFileInstruction(ExecutableInstruction):
    def __init__(self, *, file: str, timeout: float = -1):
        super().__init__()
        self.file = file
        self.timeout_seconds = timeout

    def _run(self) -> Tuple[List[str], float]:
        try:
            process = subprocess.run(self.file,
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.STDOUT,
                                     timeout=self.timeout_seconds,
                                     close_fds=True,
                                     universal_newlines=True)

            messages = f'the output of {self.file}\n{process.stdout}'
            if process.returncode != 0:
                raise TestUserException(f'the result of {self.file} is {process.returncode}\n{messages}')

            return messages.splitlines(), 0.0
        except subprocess.TimeoutExpired as te:
            raise TestUserException(f'[timeout] file is "{self.file}" timeout={self.timeout_seconds}')


class UnittestInstruction(ExecuteFileInstruction):
    def __init__(self, *, name: str, timeout: float = -1):
        super().__init__(file='', timeout=timeout)
        self.name = name
        self.request = Request(CommandName.EXECUTABLE, name)

    def _run(self) -> Tuple[List[str], float]:
        build_dir = self._test_config.build_dir
        self.file = os.path.join(build_dir, 'bin', self.name)
        return super()._run()


class AnnBmInstruction(ExecuteFileInstruction):
    """
    ann benchmark 指令
    """
    def __init__(self, ann_benchmark_dir: str, timeout: float = -1):
        super().__init__(file='', timeout=timeout)
        self._ann_benchmark_dir: str = ann_benchmark_dir

    def _run(self) -> Tuple[List[str], float]:
        messages = ""
        try:
            ann_run_command: str = f"cd {self._ann_benchmark_dir} && rm -rf results/fashion-mnist-784-euclidean && python3 run.py --dataset fashion-mnist-784-euclidean --docker-tag ann-benchmarks-miniob --local --timeout 100 --runs 1"
            process = subprocess.run(ann_run_command,
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.STDOUT,
                                     timeout=self.timeout_seconds,
                                     cwd=self._ann_benchmark_dir,
                                     close_fds=True,
                                     universal_newlines=True,
                                     shell=True)

            if process.returncode != 0:
                raise TestUserException(f'ann benchmark run failed, the result is {process.returncode}\n{messages}')
            
            # return messages.splitlines()
        except subprocess.TimeoutExpired as te:
            raise TestUserException(f'[timeout] ann timeout={self.timeout_seconds}')
        
        # check result
        ann_plot_command :str = f"cd {self._ann_benchmark_dir} && python3 plot.py --dataset fashion-mnist-784-euclidean"
        process = subprocess.run(ann_plot_command,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT,
                                timeout=60,
                                cwd=self._ann_benchmark_dir,
                                close_fds=True,
                                universal_newlines=True,
                                shell=True)

        # 使用正则表达式匹配包含 "MiniOBVector" 的输出结果，输出结果示例：
        #   0:                                                     MiniOBVector(query_probes=1)        0.965      178.338
        pattern = r"^\s*\d+:\s+MiniOBVector\(\)\s+(\d+\.\d+)\s+(\d+\.\d+)"
        match = re.search(pattern, process.stdout, re.MULTILINE)
        
        if match:
            recall = float(match.group(1))
            qps = float(match.group(2))
            
            if recall >= 0.9 and qps >=100:
                messages = f"recall: {recall}, qps: {qps}"
            elif recall < 0.9:
                raise TestUserException(f'recall is under 0.9, recall = {str(recall)}')
            elif qps < 100:
                raise TestUserException(f'qps is under 100, qps = {str(qps)}')

        else:
            print(process.stdout)
            raise TestUserException("ann benchmark failed")
        
        return messages.splitlines(), 0.0

class TpccInstruction(ExecuteFileInstruction):
    """
    tpcc benchmark 指令
    """
    def __init__(self, tpcc_benchmark_dir: str, timeout: float = -1):
        super().__init__(file='', timeout=timeout)
        self._tpcc_benchmark_dir: str = tpcc_benchmark_dir

    def _run(self) -> Tuple[List[str], float]:
        messages = ""
        try:
            ann_run_command: str = f"cd {self._tpcc_benchmark_dir} && python2 ./tpcc.py --config=miniob.config  miniob --ddl ./tpcc-miniob.sql --clients 2"
            process = subprocess.run(ann_run_command,
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.STDOUT,
                                     timeout=self.timeout_seconds,
                                     cwd=self._tpcc_benchmark_dir,
                                     close_fds=True,
                                     universal_newlines=True,
                                     shell=True)

            if process.returncode != 0:
                raise TestUserException(f'tpcc benchmark run failed, the result is {process.returncode}\n{messages}')
            
        except subprocess.TimeoutExpired as te:
            raise TestUserException(f'[timeout] tpcc timeout={self.timeout_seconds}')
        
        all_stdout = process.stdout

        # 分割输出内容并读取最后一行
        last_line = all_stdout.strip().split("\n")[-1]
        match = re.search(r"([\d.]+)\s+txn/s", last_line)
        txn_per_second = 0.0
        if match:
            txn_per_second = float(match.group(1))  # 将数字转换为浮点数
        else:
            raise TestUserException(f'parse tpcc result failed')
        

        return messages.splitlines(), txn_per_second


class InstructionGroup:
    """
    描述一个指令组
    一些有关联关系的指令放在一起，放到一个组里面
    """

    def __init__(self, name):
        # 指令组的名字，不要求唯一
        self.name = name

        # 描述依赖哪些测试用例
        # 可以用来统计测试用例之间的依赖关系
        self.dependiencies: List[str] = []

        # 指令组中的指令，运行时依次执行
        self.instructions: List[Instruction] = []

        # 有些命令运行时需要更多的信息来帮助用户查找问题。比如某个select或者update语句报错，
        # 给出这个测试用例上之前执行过的建表和建索引的语句，可以帮助查问题
        self.associate_groups: List[InstructionGroup] = []

    def add_associate_groups(self, associate_groups):
        for group in associate_groups:
            self.associate_groups.append(group)

    def add_instruction(self, instruction: Instruction):
        self.instructions.append(instruction)
        return self

    def add_echo_instruction(self, message: str):
        echo_instruction = EchoInstruction(message)
        return self.add_instruction(echo_instruction)

    def add_sort_instruction(self, instruction: Instruction):
        sort_instruction = SortInstruction(instruction)
        return self.add_instruction(sort_instruction)

    def add_sql_instruction(self, sql: str, *, expected: str = None, expected_messages: List[str] = None):
        sql_instruction = SqlInstruction(sql, expected=expected, expected_messages=expected_messages)
        return self.add_instruction(sql_instruction)

    def add_sort_sql_instruction(self, sql: str, *, expected: str = None, expected_messages: List[str] = None):
        sql_instruction = SqlInstruction(sql, expected=expected, expected_messages=expected_messages)
        return self.add_sort_instruction(sql_instruction)

    def add_runtime_ddl_instruction(self, sql: str, mysql_sql: str = None):
        runtime_sql_instruction = RuntimeSqlInstruction(sql=sql,
                                                        result_type=RuntimeSqlInstruction.ResultType.BOOLEAN,
                                                        remove_header=False)
        if mysql_sql and mysql_sql.strip():
            mysql_sql = mysql_sql.strip()
            runtime_sql_instruction.mysql_request = Request(CommandName.RUNTIME_SQL, mysql_sql)
        return self.add_instruction(runtime_sql_instruction)

    def add_runtime_dml_instruction(self, sql: str):
        return self.add_runtime_ddl_instruction(sql)

    def add_runtime_dql_instruction(self, sql: str, *, timeout_seconds: int = -1):
        runtime_sql_instruction = RuntimeSqlInstruction(sql=sql,
                                                        result_type=RuntimeSqlInstruction.ResultType.RESULT_SET,
                                                        remove_header=True)
        runtime_sql_instruction.timeout_seconds = timeout_seconds
        return self.add_instruction(runtime_sql_instruction)

    def add_sort_runtime_dql_instruction(self, sql: str):
        runtime_sql_instruction = RuntimeSqlInstruction(sql=sql,
                                                        result_type=RuntimeSqlInstruction.ResultType.RESULT_SET,
                                                        remove_header=True)
        return self.add_sort_instruction(runtime_sql_instruction)

    def __get_sql_and_expected_messages(self, block_string: str) -> Tuple[str, List[str]]:
        strings: List[str] = util.myutil.split_and_strip_string(block_string)

        if len(strings) < 2:
            raise ValueError(f'argument string should has 2 lines: {block_string}')

        sql = strings[0]
        expected_messages = strings[1:]
        return sql, expected_messages
    
    def __get_sql_and_ensure_messages(self, block_string: str) -> Tuple[str, str]:
        strings: List[str] = util.myutil.split_and_strip_string(block_string)

        if len(strings) != 2:
            raise ValueError(f'argument string should has at least one line: {block_string}')

        ensure = strings[0]
        sql = strings[1]
        return ensure, sql

    def add_block_sql_instruction(self, block_string: str):
        """
        为了方便编写case的帮助函数
        block_string 中包含多行文本，第一行是SQL，其余是SQL执行的结果。结果都会排序
        block_string 最少有两行
        会把字符串中前面和后面的空行都删掉。空行就是只包含空字符的行
        每行都会把字符串前面和后面的空字符给去掉
        """

        sql, expected_messages = self.__get_sql_and_expected_messages(block_string)
        return self.add_sql_instruction(sql, expected_messages=expected_messages)

    def add_sort_block_sql_instruction(self, block_string: str):
        """
        为了方便编写case的帮助函数
        block_string 中包含多行文本，第一行是SQL，其余是SQL执行的结果。结果都会排序
        block_string 最少有两行
        会把字符串中前面和后面的空行都删掉。空行就是只包含空字符的行
        每行都会把字符串前面和后面的空字符给去掉
        """

        sql, expected_messages = self.__get_sql_and_expected_messages(block_string)
        return self.add_sort_sql_instruction(sql, expected_messages=expected_messages)
    
    def add_ensure_sql_instruction(self, block_string: str):
        """
        为了方便编写case的帮助函数
        block_string 包含 ensure sql，例如 
        ensure:hashjoin
        select * from t inner join s on t.a=s.a;
        该语句会检查该sql 的执行计划是否满足 `ensure:hashjoin` 的要求。
        """
        ensure, sql = self.__get_sql_and_ensure_messages(block_string)
        ensure_sql_instruction = EnsureSqlInstruction(sql=sql, ensure=ensure)
        return self.add_instruction(ensure_sql_instruction)

    def add_unittest_instruction(self, name: str, timeout: float = -1):
        """
        单元测试
        """
        unittest_instruction = UnittestInstruction(name=name, timeout=timeout)
        return self.add_instruction(unittest_instruction)
    
    def add_annbm_instruction(self,ann_benchmark_dir: str, timeout: float = -1):
        """
        ann benchmark
        """
        annbm_instruction = AnnBmInstruction(ann_benchmark_dir, timeout)
        return self.add_instruction(annbm_instruction)
    
    def add_tpcc_instruction(self,tpcc_benchmark_dir: str, timeout: float = -1):
        """
        tpcc benchmark
        """
        tpcc_instruction = TpccInstruction(tpcc_benchmark_dir, timeout)
        return self.add_instruction(tpcc_instruction)


    def add_chunck_instruction(self, *, test_string: str, result_string: str):
        """
        这个功能是希望支持mysql test整个文本文件校验的功能
        但是后来看下来，不太容易展示错误信息，而且把文本文件转换成python的格式，并不需要多少时间
        """
        chunk_instruction = ChunkInstruction(test_string=test_string, result_string=result_string)
        return self.add_instruction(chunk_instruction)
