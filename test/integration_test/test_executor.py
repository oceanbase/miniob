from typing import Any, List, Dict
import logging
import traceback
import socket
import os.path
import time
import socket

import __init__
from test_instruction import EnsureSqlInstruction, ResultString
from test_instruction import Instruction, SortInstruction, EchoInstruction
from test_instruction import SqlInstruction, RuntimeSqlInstruction
from test_instruction import ChunkInstruction, RequestIncludingInstruction
from test_instruction import ConnectInstruction, ConnectionInstruction, RestartInstruction
from test_instruction import ExecutableInstruction
from test_response import NormalResponseMessage, DebugResponseMessage, Response
from test_case import TestCase
from test_suite import TestSuite
from miniob.miniob_client import MiniObClient
from miniob.miniob_server import MiniObServer
from test_exception import TestPlatformException, TestUserException
from test_util import TestCaseFetcher
from test_result import InstructionResult, TestCaseResult, InstructionResultGroup
from mysql_executor.mysql_executor import MysqlResult, MysqlConnectConfig
from mysql_adaptor import MysqlAdaptor
from miniob_adaptor import MiniobClientDryrun, MiniobServerDryrun
from test_config import TestConfig
from util import mycompile

_logger = logging.getLogger('Executor')


class ExecuteContext:
    """
    执行测试时的上下文信息
    当前有一个实现类是 TestCaseExecutor
    """

    def __init__(self):
        pass

    def current_client(self) -> MiniObClient:
        pass

    def set_current_client(self, client: MiniObClient) -> None:
        pass

    def create_client(self, client_name: str) -> MiniObClient:
        pass

    def restart_server(self, force_stop: bool) -> None:
        pass

    def execute_instruction(self, instruction: Instruction) -> InstructionResult:
        pass

    def get_mysql_adaptor(self) -> MysqlAdaptor:
        pass

    def get_test_config(self):
        pass


class InstructionExecutor:
    """
    执行某个指令
    """

    def __init__(self):
        pass

    def execute(self, instruction: Instruction, context: ExecuteContext) -> InstructionResult:
        pass


class EchoExecutor(InstructionExecutor):
    def __init__(self):
        super().__init__()

    def execute(self, instruction: Instruction, context: ExecuteContext) -> InstructionResult:
        request = instruction.request
        data = request.data
        response = Response()
        response.messages.append(NormalResponseMessage(data))
        return InstructionResult(instruction=instruction, response=response)


class ConnectExecutor(InstructionExecutor):
    def __init__(self):
        super().__init__()

    def execute(self, instruction: Instruction, context: ExecuteContext) -> InstructionResult:
        if not isinstance(instruction, ConnectInstruction):
            raise ValueError(f'The instruction is not a connect instruction. type={str(type(instruction))}')

        connect_instruction: ConnectInstruction = instruction
        client_name = connect_instruction.connection_name
        client = context.create_client(client_name)
        if not client:
            raise TestUserException(f'failed to create client: {client_name}')

        return InstructionResult(instruction=instruction, response=Response())


class ConnectionExecutor(InstructionExecutor):
    def __init__(self):
        super().__init__()

    def execute(self, instruction: Instruction, context: ExecuteContext) -> InstructionResult:
        if not isinstance(instruction, ConnectionInstruction):
            raise ValueError(f'The instruction is not a connection instruction. type={str(type(instruction))}')

        connection_instruction: ConnectionInstruction = instruction
        client_name = connection_instruction.connection_name
        context.set_current_client(client_name)
        time.sleep(2)
        return InstructionResult(instruction=instruction, response=Response())


class RestartExecutor(InstructionExecutor):
    def __init__(self):
        super().__init__()

    def execute(self, instruction: Instruction, context: ExecuteContext) -> InstructionResult:
        restart_instruction: RestartInstruction = instruction
        context.restart_server(restart_instruction.force_stop)
        return InstructionResult(instruction=instruction, response=Response())


class SortExecutor(InstructionExecutor):
    def __init__(self):
        super().__init__()

    def execute(self, instruction: Instruction, context: ExecuteContext) -> InstructionResult:
        if not isinstance(instruction, SortInstruction):
            raise ValueError(f'The instruction is not a sort instruction. type={str(type(instruction))}')

        sort_instruction: SortInstruction = instruction
        parent_instruction = sort_instruction.instruction
        parent_result: InstructionResult = context.execute_instruction(parent_instruction)
        response: Response = parent_result.response

        # 在收到的回复中，带有正常的消息和调试消息
        # 调试消息放在最上面，其它的消息排序后放在下面。调试消息不排序
        sorted_response = Response()
        sorted_response.messages.extend(SortInstruction.sort_messages(response.messages))

        # 排序instruction的expected_messages
        expected_messages = parent_result.instruction.expected_response.messages.copy()
        expected_messages = SortInstruction.sort_messages(expected_messages)

        result_instruction = Instruction()
        result_instruction.request = parent_instruction.request
        result_instruction.expected_response.messages = expected_messages

        return InstructionResult(instruction=result_instruction, response=sorted_response)


class SqlExecutor(InstructionExecutor):
    DEFAULT_ENCODING = 'UTF-8'

    def __init__(self):
        super().__init__()

    def execute(self, instruction: Instruction, context: ExecuteContext) -> InstructionResult:
        lines = self.run_sql(context, instruction.request.data)
        response = Response()
        for line in lines:
            line = line.strip()
            if line.startswith('#'):
                response.messages.append(DebugResponseMessage(line))
            else:
                response.messages.append(NormalResponseMessage(line))

        return InstructionResult(instruction=instruction, response=response)

    def run_sql(self, context: ExecuteContext, sql: str) -> List[str]:
        result, data = context.current_client().run_sql(sql)
        if not result:
            raise TestUserException(f'failed to receive response from observer')

        if data:
            lines = data.splitlines()
        else:
            lines = []
        return lines

class EnsureSqlExecutor(InstructionExecutor):
    DEFAULT_ENCODING = 'UTF-8'

    def __init__(self):
        super().__init__()

    def execute(self, instruction: Instruction, context: ExecuteContext) -> InstructionResult:
        if not isinstance(instruction, EnsureSqlInstruction):
            raise ValueError(f'The instruction is not a ensure sql instruction. type={str(type(instruction))}')
        ensure_sql_instruction: EnsureSqlInstruction = instruction

        lines = self.run_ensure_sql(context, instruction.request.data, ensure_sql_instruction.ensure)
        response = Response()
        for line in lines:
            line = line.strip()
            if line.startswith('#'):
                response.messages.append(DebugResponseMessage(line))
            else:
                response.messages.append(NormalResponseMessage(line))
        

        return InstructionResult(instruction=instruction, response=response)

    def run_ensure_sql(self, context: ExecuteContext, sql: str, ensure: str) -> List[str]:
        result, data = context.current_client().run_sql(sql)
        if not result:
            raise TestUserException(f'failed to receive response from observer')

        if not self.check_result(result, data, ensure):
            raise TestUserException(f'failed to ensure sql {str(sql)}, ensure {str(ensure)}')
        return []
    
    def check_result(self, result, data, command) -> bool:
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

class RuntimeSqlExecutor(InstructionExecutor):
    """
    执行一个运行时才能检查的SQL指令。
    该SQL的语句也是运行时获取的。执行后，同时也将相同的语句在MySQL中运行一次，然后比较结果
    """

    def __init__(self):
        super().__init__()

    def execute(self, instruction: Instruction, context: ExecuteContext) -> InstructionResult:
        if not isinstance(instruction, RuntimeSqlInstruction):
            raise ValueError(f'The instruction is not a runtime sql instruction. type={str(type(instruction))}')

        runtime_sql_instruction: RuntimeSqlInstruction = instruction
        sql = runtime_sql_instruction.next_sql()  # 运行时获取SQL语句

        # 先在MiniOB中运行这个SQL
        result, data = context.current_client().run_sql(sql, total_timeout_seconds=instruction.timeout_seconds)

        if not result:
            show_sql = sql if len(sql) < 256 else sql[:256] + '...'
            raise TestUserException(f'failed to receive response from observer. reason={str(data)}')

        _logger.debug('execute miniob sql [%s] with result [%s]', sql, str(data))

        # 获取miniob返回的结果
        miniob_lines: List[str] = []
        if data:
            miniob_lines = data.splitlines()

        # 计算miniob的response
        debug_lines: List[str] = []
        normal_lines: List[str] = []

        for line in miniob_lines:
            line = line.strip()
            if line.startswith('#'):
                debug_lines.append(line)
            else:
                normal_lines.append(line)

        # 删除表头
        if runtime_sql_instruction.remove_header and \
                runtime_sql_instruction.result_type == RuntimeSqlInstruction.ResultType.RESULT_SET and \
                not (len(normal_lines) == 1 and normal_lines[
                    0].upper() == ResultString.FAILURE):  # 如果是失败的结果，就一行数据，假设不会有表头叫FAILURE
            normal_lines = normal_lines[1:]  # 删除表头

        response = Response()
        for line in debug_lines:
            response.messages.append(DebugResponseMessage(line))
        for line in normal_lines:
            response.messages.append(NormalResponseMessage(line))

        # 在mysql中执行一下，获取mysql的执行结果。mysql的执行结果就是我们期望sql执行后的结果
        sql = runtime_sql_instruction.next_mysql_sql()
        mysql_lines = []
        mysql_adaptor: MysqlAdaptor = context.get_mysql_adaptor()
        mysql_result: MysqlResult = mysql_adaptor.execute(sql)
        if runtime_sql_instruction.result_type == RuntimeSqlInstruction.ResultType.BOOLEAN:
            result_string: str = ResultString.SUCCESS if not mysql_result.has_error() else ResultString.FAILURE
            mysql_lines.append(result_string)

        else:
            if mysql_result.has_error():
                mysql_lines.append(ResultString.FAILURE)
            else:
                mysql_lines = MysqlAdaptor.result_to_miniob(mysql_result)

                if runtime_sql_instruction.remove_header:
                    mysql_lines = mysql_lines[1:]  # 删除表头

        # 因为runtime sql instruction的sql语句是动态的，这里创建一个静态的SQL指令
        # 其中mysql运行后的执行结果就是我们的预期执行结果
        sql_instruction: SqlInstruction = SqlInstruction(sql, expected_messages=mysql_lines)

        return InstructionResult(instruction=sql_instruction, response=response)


class ExecutableExecutor(InstructionExecutor):
    DEFAULT_ENCODING = 'UTF-8'

    def __init__(self):
        super().__init__()

    def execute(self, instruction: Instruction, context: ExecuteContext) -> InstructionResult:
        if not isinstance(instruction, ExecutableInstruction):
            raise TestPlatformException(f'invalid instruction. type:{type(instruction)}, need:ExecutableInstruction')

        lines,score = instruction.run(test_config=context.get_test_config())
        response = Response()
        for line in lines:
            line = line.strip()
            response.messages.append(DebugResponseMessage(line))

        return InstructionResult(instruction=instruction, response=response, score = score)


class ChunkExecutor(InstructionExecutor):
    """
    这里是为了适配文本模式的测试用例。后面不要了 TODO 删除
    """

    def __init__(self):
        super().__init__()

        self.__command_prefix = '--'

    def get_instruction_from_line(self, line: str) -> Instruction:
        line = line.strip()
        if not line:
            return None

        if line.startswith('#'):
            return EchoInstruction(line[4:].strip())

        if line.startswith(self.__command_prefix):
            command_line = line[len(self.__command_prefix):].strip()
            args = command_line.split(' ', 1)
            command = args[0].lower()
            command_arg = ''
            if len(args) > 1:
                command_arg = args[1]

            if 'echo' == command:
                return EchoInstruction(command_arg)
            if 'sort' == command:
                sql_instruction = SqlInstruction(command_arg, expected='fake')
                sort_instruction = SortInstruction(sql_instruction)
                request_including_instruction = RequestIncludingInstruction(sort_instruction, command=command_arg)
                return request_including_instruction
            if 'connect' == command:
                return ConnectInstruction(command_arg)
            if 'connection' == command:
                return ConnectionInstruction(command_arg)
            if 'restart' == command:
                return RestartInstruction(command_arg)

            raise ValueError(f'unknown command: {command}. command line is {line}')

        else:
            return RequestIncludingInstruction(SqlInstruction(line, expected='fake'))

    def execute(self, instruction: Instruction, context: ExecuteContext) -> InstructionResult:
        if not isinstance(instruction, ChunkInstruction):
            raise ValueError(f'The instruction is not a chunk instruction. type={str(type(instruction))}')

        chunk_instruction: ChunkInstruction = instruction

        command_lines: List[str] = chunk_instruction.test_string.splitlines()
        response: Response = Response()
        for command_line in command_lines:
            command_line = command_line.strip()
            if not command_line:
                response.messages.append(NormalResponseMessage(''))
                continue

            command_instruction = self.get_instruction_from_line(command_line)
            result = context.execute_instruction(command_instruction)
            response.messages.extend(result.response.messages.copy())

        return InstructionResult(instruction=instruction, response=response)


class MiniOBConfig:
    def __init__(self):
        self.observer_path: str = None  # observer二进制文件的路径
        self.data_dir: str = None  # miniob运行时的数据目录
        self.config_file: str = None  # miniob运行时的配置文件
        self.server_port: int = -1  # miniob运行时的端口
        self.server_socket: str = None  # miniob运行时的unix socket
        self.clean_data_dir: bool = True  # 是否清理数据目录
        self.core_path: str = None  # core文件的路径，通常需要修改系统配置来配合
        self.trx_model: str = None  # 事务模型
        self.client_time_limit: float = None  # 客户端的超时时间

    def __str__(self):
        return str(self.__dict__)


class TestCaseExecutor(ExecuteContext):
    """
    执行某个测试用例
    遍历所有的指令，然后执行。
    测试的流程：
    启动服务端
    创建默认客户端连接
    遍历所有的指令组并执行
    """
    DEFAULT_CLIENT_NAME = 'default'

    def __init__(self, *, test_config: TestConfig, miniob_config: MiniOBConfig, mysql_config: MysqlConnectConfig,
                 test_case: TestCase, logger: logging.Logger = None, dryrun: bool = False) -> None:
        super().__init__()
        self.__logger = _logger
        if logger:
            self.__logger = logger

        self.__test_config = test_config
        self.__miniob_config = miniob_config
        self.__mysql_config = mysql_config
        self.__test_case: TestCase = test_case
        self.__clients: Dict[str, MiniObClient] = {}
        self.__current_client: MiniObClient = None
        self.__miniob_server: MiniObServer = None
        self.__dryrun: bool = dryrun

        # 使用mysql来获取SQL的期望运行结果
        self.__mysql_adaptor: MysqlAdaptor = None

        # 每种类型的指令都有对应的执行器
        self.__instruction_executors: Dict[int, InstructionExecutor] = {
            id(EchoInstruction): EchoExecutor(),
            id(SortInstruction): SortExecutor(),
            id(SqlInstruction): SqlExecutor(),
            id(RuntimeSqlInstruction): RuntimeSqlExecutor(),
            id(ConnectInstruction): ConnectExecutor(),
            id(ConnectionInstruction): ConnectionExecutor(),
            id(RestartInstruction): RestartExecutor(),
            id(ExecutableInstruction): ExecutableExecutor(),
            id(ChunkInstruction): ChunkExecutor(),
            id(EnsureSqlInstruction): EnsureSqlExecutor()
        }

    def execute(self) -> TestCaseResult:
        time_begin = time.perf_counter()
        self.__logger.info(f'test case {self.__test_case.name} start')

        case_failed: bool = False
        test_case_result = TestCaseResult(self.__test_case)

        try:
            # 执行前初始化
            self.__close_clients()

            if self.__test_case.need_observer:
                self.__miniob_server = self.__start_server()
                self.__init_default_client()

            if self.__test_case.need_mysql:
                self.init_mysql_adaptor_if_need()

            for instruction_group in self.__test_case.instruction_groups:
                self.__logger.info(f'instruction group: {instruction_group.name} start ...')
                result_group = InstructionResultGroup(instruction_group)
                test_case_result.result_groups.append(result_group)

                for instruction in instruction_group.instructions:
                    self.__logger.info(f'instruction start: {str(instruction.request)}')
                    try:
                        result = self.execute_instruction(instruction)
                    except TestUserException as ex:
                        self.__logger.info(f'got user exception {str(ex)}.')
                        self.__logger.info(traceback.format_exc())
                        result_string = str(ex)
                        if self.__miniob_server:
                            coredump_info = self.__miniob_server.coredump_info()
                            if coredump_info:
                                self.__logger.info('got coredump info')
                                result_string += coredump_info

                        test_case_result.user_exception = result_string
                        response: Response = Response()
                        response.messages.append(NormalResponseMessage(result_string))
                        result = InstructionResult(instruction=instruction, response=response)

                        case_failed = True

                    self.__logger.info(
                        f'instruction end: {str(instruction)}, case_failed={case_failed}, result={str(result)}')
                    result_group.results.append(result)
                    if result.score != 0.0:
                        test_case_result.score += result.score

                    if case_failed:
                        self.__logger.info(f'instruction execute failed: {str(instruction)}, result={str(result)}')
                        break  # break in one instruction group

                    if not case_failed and not result.check():
                        self.__logger.info(f'instruction execute failed: {str(instruction)}, result={str(result)}')
                        case_failed = True
                        break

                    # 这个指令执行成功了，但是有些回复内容会特别多，这里清理掉可以省一些内存
                    result.clear_response()

                if case_failed:
                    break  # break in instruction groups

        except TestUserException as ex:
            self.__logger.info(f'got user exception {str(ex)}.')
            self.__logger.info(traceback.format_exc())
            test_case_result.user_exception = str(ex)
            # 执行的指令中可能有restart，但是observer 重启失败后，miniob_server可能就是None了，所以这里判断一下
            if self.__miniob_server:
                coredump_info = self.__miniob_server.coredump_info()
                if coredump_info:
                    self.__logger.info('got coredump info')
                    test_case_result.user_exception += coredump_info

        except Exception as ex:
            self.__logger.error(f'failed to test case: {self.__test_case.name}. ex={str(ex)}')
            self.__logger.error(traceback.format_exc())
            test_case_result.test_exception = str(ex)

        finally:
            self.__close_clients()

            if self.__miniob_server:
                self.__miniob_server.stop_all_servers()
                self.__miniob_server.clean()
                self.__miniob_server = None

        time_cost = time.perf_counter() - time_begin
        self.__logger.info(f'test case {self.__test_case.name} done, cost={time_cost:.3f}s')
        return test_case_result

    def dryrun(self) -> TestCaseResult:
        """
        看下指令是否都可以执行，但是不校验结果
        """
        time_begin = time.perf_counter()
        self.__logger.info(f'[dryrun] test case {self.__test_case.name} start')

        test_case_result = TestCaseResult(self.__test_case)

        try:
            # 执行前初始化
            self.__close_clients()

            if self.__test_case.need_observer:
                self.__miniob_server = self.__start_server()
                self.__init_default_client()

            if self.__test_case.need_mysql:
                self.init_mysql_adaptor_if_need()

            for instruction_group in self.__test_case.instruction_groups:
                self.__logger.info(f'instruction group: {instruction_group.name} start ...')
                result_group = InstructionResultGroup(instruction_group)
                test_case_result.result_groups.append(result_group)

                for instruction in instruction_group.instructions:
                    self.__logger.info(f'instruction start: {str(instruction.request)}')

                    result = self.execute_instruction(instruction)

                    self.__logger.info(f'instruction end: {str(instruction)}, result={str(result)}')
                    result_group.results.append(result)

        except Exception as ex:
            self.__logger.error(f'failed to test case: {self.__test_case.name}. ex={str(ex)}')
            self.__logger.error(traceback.format_exc())
            test_case_result.test_exception = str(ex)

        finally:
            self.__close_clients()

            if self.__miniob_server:
                self.__miniob_server.stop_all_servers()
                self.__miniob_server.clean()
                self.__miniob_server = None

        time_cost = time.perf_counter() - time_begin
        self.__logger.info(f'test case {self.__test_case.name} done. cost={time_cost:.3f}s')
        return test_case_result

    def __get_instruction_executor(self, instruction: Instruction) -> InstructionExecutor:
        if isinstance(instruction, ExecutableInstruction):
            instruction_class_id = id(ExecutableInstruction)
        else:
            instruction_class_id = id(type(instruction))
        executor = self.__instruction_executors.get(instruction_class_id)
        if not executor:
            raise TestPlatformException(
                f'cannot find instruction executor. instruction class name: {str(type(instruction))}.')
        return executor

    # override
    def execute_instruction(self, instruction: Instruction) -> InstructionResult:
        instruction_executor = self.__get_instruction_executor(instruction)
        return instruction_executor.execute(instruction, self)

    def __start_server(self) -> MiniObServer:
        if self.__dryrun:
            return MiniobServerDryrun()

        trx_model: str = self.__miniob_config.trx_model
        if self.__test_case.server_trx_model:
            trx_model = self.__test_case.server_trx_model
        protocol: str = "plain"
        storage_engine: str = ""
        if self.__test_case.miniob_protocol:
            protocol = self.__test_case.miniob_protocol
        
        if self.__test_case.storage_engine:
            storage_engine = self.__test_case.storage_engine


        self.__logger.info('start server with trx_model=%s', str(trx_model))

        miniob_server: MiniObServer = MiniObServer(observer_path=self.__miniob_config.observer_path,
                                                   data_dir=self.__miniob_config.data_dir,
                                                   config_file=self.__miniob_config.config_file,
                                                   server_port=self.__miniob_config.server_port,
                                                   server_socket=self.__miniob_config.server_socket,
                                                   clean_data_dir=self.__miniob_config.clean_data_dir,
                                                   logger=self.__logger,
                                                   core_path=self.__miniob_config.core_path,
                                                   trx_model=trx_model,
                                                   protocol=protocol,
                                                   storage_engine=storage_engine)
        miniob_server.init_server()
        miniob_server.clean()
        ret = miniob_server.start_server()
        if not ret:
            error_message = "Failed to start observer"
            coredump_info = miniob_server.coredump_info()
            if coredump_info:
                error_message = coredump_info
            raise TestUserException(error_message)
        return miniob_server

    def __create_client(self) -> MiniObClient:
        if self.__dryrun:
            return MiniobClientDryrun()

        try:
            miniob_client = MiniObClient(server_port=self.__miniob_config.server_port,
                                         server_socket=self.__miniob_config.server_socket,
                                         time_limit=self.__miniob_config.client_time_limit,
                                         logger=self.__logger)
            return miniob_client
        except socket.error as ex:
            raise TestUserException(str(socket.error)) from ex

    def __init_default_client(self):
        client = self.__create_client()
        self.__clients[TestCaseExecutor.DEFAULT_CLIENT_NAME] = client
        self.__current_client = client

    def __close_clients(self):
        for client in self.__clients.values():
            client.close()

        self.__clients.clear()
        self.__current_client = None

    # override
    def get_test_config(self):
        return self.__test_config

    # override
    def get_mysql_adaptor(self) -> MysqlAdaptor:
        return self.__mysql_adaptor

    # override
    def set_current_client(self, client_name: str) -> None:
        client = self.__clients.get(client_name)
        if not client:
            raise TestPlatformException(f'no such client: {client_name}')

        self.__current_client = client

        if self.__mysql_adaptor:
            self.__mysql_adaptor.set_current_client(client_name)

    # override
    def create_client(self, client_name: str) -> MiniObClient:
        client = self.__clients.get(client_name)
        if client:
            return client

        client = self.__create_client()
        self.__clients[client_name] = client

        if self.__mysql_adaptor:
            self.__mysql_adaptor.new_connect(client_name)
        return client

    # override
    def current_client(self) -> MiniObClient:
        return self.__current_client

    # override
    def restart_server(self, force_stop: bool) -> None:
        self.__logger.info('restart server')
        self.__close_clients()
        if not self.__miniob_server:
            raise TestPlatformException('server is not started')

        self.__miniob_server.stop_all_servers()
        ret = self.__miniob_server.start_server()
        if not ret:
            error_message = "Failed to restart observer"
            if self.__miniob_server:
                coredump_info = self.__miniob_server.coredump_info()
                if coredump_info:
                    error_message = coredump_info
            raise TestUserException(error_message)

        self.__init_default_client()

    def init_mysql_adaptor_if_need(self):
        self.__logger.info('init mysql adaptor. config=%s', str(self.__mysql_config))

        if not self.__mysql_adaptor and self.__mysql_config:
            self.__mysql_adaptor = MysqlAdaptor(config=self.__mysql_config)
            self.__mysql_adaptor.init(TestCaseExecutor.DEFAULT_CLIENT_NAME)


class TestExecutor:
    """
    运行一次测试，会执行所有测试用例
    """

    def __init__(self, *, test_config: TestConfig, logger: logging.Logger = None) -> None:
        self.__test_config = test_config
        if logger:
            self.__logger = logger
        else:
            self.__logger = _logger

    def execute(self) -> List[TestCaseResult]:
        self.__logger.info(f'test config is {str(self.__test_config)}')

        test_suite = self.list_cases()
        test_suite.test_initiator.init(test_config=self.__test_config, logger=self.__logger)
        case_names = [test_case.name for test_case in test_suite.test_cases]
        self.__logger.info(f'got {len(test_suite.test_cases)} cases to test: {",".join(case_names)}')

        return [self.test(test_case) for test_case in test_suite.test_cases]

    def list_cases(self) -> TestSuite:
        test_suite = TestSuite()
        test_case_fetcher: TestCaseFetcher = TestCaseFetcher.create(self.__test_config.test_case_type)
        # 从指定路径拉取所有测试用例
        test_case_fetcher.list_test_cases_from_directory(self.__test_config.test_case_dir)
        if self.__test_config.test_cases:
            # 指定了某些测试用例，就只测试这些测试用例
            test_cases = [test_case_fetcher.get_test_cases_by_name(case_name) for case_name in
                          self.__test_config.test_cases]
        else:
            test_cases = test_case_fetcher.test_cases()

        test_suite.test_initiator = test_case_fetcher.test_initiator()
        test_suite.test_cases = test_cases
        return test_suite

    def test(self, test_case: TestCase) -> TestCaseResult:
        # 不要抛异常
        miniob_config = MiniOBConfig()
        miniob_config.observer_path = self.__test_config.observer_path
        miniob_config.data_dir = self.__test_config.db_data_dir
        miniob_config.server_port = self.__test_config.server_port
        if self.__test_config.use_unix_socket:
            miniob_config.server_socket = os.path.join(self.__test_config.db_data_dir, 'miniob.sock')

        miniob_config.config_file = self.__test_config.db_config
        miniob_config.core_path = self.__test_config.core_path

        mysql_config: MysqlConnectConfig = self.__test_config.get_mysql_config()

        test_case_executor = TestCaseExecutor(logger=self.__logger,
                                              test_config=self.__test_config,
                                              miniob_config=miniob_config,
                                              mysql_config=mysql_config,
                                              test_case=test_case)
        return test_case_executor.execute()
