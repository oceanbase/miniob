import __init__
from typing import List, Dict
import difflib
from common import myrc as rc
from test_info import TestTask
from test_response import Response, DebugResponseMessage, NormalResponseMessage
from test_instruction import Instruction, InstructionGroup
from test_request import Request
from test_case import TestCase
from util import myutil

'''
class EvalResult:
  def __init__(self):
    self.__message:List[str] = []
    self.__test_result:bool = True  # 测试本身是否出现了异常
    self.__test_case_result: List[TestCaseResult] = []
    
  def set_test_result(self, result: bool):
    self.__test_result = result
  def test_result(self) -> bool :
    return self.__test_result

  def append(self, test_case_result: TestCaseResult):
    self.__test_case_result.append(test_case_result)

  def all_case_result(self) -> List[TestCaseResult]:
    return self.__test_case_result

  def to_string(self) -> str:
    return f'result={self.__test_result},message={self.__message},case_results={[str(r) for r in self.__test_case_result]}'
  def __str__(self):
    return self.to_string()

  def clear_message(self):
    self.__message = []
    
  def append_message(self, message):
    self.__message.append(message)
    
  def get_message(self):
    return "\n".join(self.__message)

'''


class InstructionResult:
    def __init__(self, *, instruction: Instruction, response: Response, score: float = 0.0):
        self.instruction: Instruction = instruction
        self.response: Response = response
        self.score = score

        self.expected_messages = [normal_message.message for normal_message in
                                  self.instruction.expected_response.messages]
        self.received_messages = [normal_message.message for normal_message in self.response.normal_messages()]
        self.__preprocess()

    def __str__(self) -> str:
        return f'instruction:{self.instruction},response:{self.response}'

    def __preprocess(self):
        self.expected_messages = myutil.strip_string_list(self.expected_messages)
        self.received_messages =  myutil.strip_string_list(self.received_messages)

    def check(self) -> bool:

        expected_messages = self.expected_messages
        received_messages = self.received_messages
        if len(expected_messages) != len(received_messages):
            return False

        for expected_message, received_message in zip(expected_messages, received_messages):
            if expected_message.upper() != received_message.upper():
                return False

        return True

    def clear_response(self):
        self.expected_messages = []
        self.received_messages = []
        self.response.messages = []
        # instruction 对象如果在多个地方引用就很尴尬，所以先不清理
        # self.instruction.expected_response.messages = []

    def show_message(self) -> List[str]:
        if self.check():
            return []

        result_strings = []
        if self.instruction.request:
            show_message = self.instruction.request.data
            if len(show_message) > 256:
                show_message = show_message[0:256] + '...'
            result_strings.append(show_message)

        debug_messages: List[str] = []
        for response_message in self.response.messages:
            if isinstance(response_message, DebugResponseMessage):
                message = response_message.message
                message = message if len(message) < 256 else message[0:256] + '...'
                debug_messages.append(message)
                if len(debug_messages) > 20:
                    debug_messages.append('...')
                    break

        one_message_max_size = 256
        max_diff_message_count = 12

        # 为了控制数据量，最多展示10行
        # 就展示一组有差异的数据
        diff_messages: List[str] = []
        matcher = difflib.SequenceMatcher(isjunk=lambda message: isinstance(message, DebugResponseMessage))
        matcher.set_seqs(self.instruction.expected_response.messages, self.response.messages)
        for group in matcher.get_grouped_opcodes(3):
            if len(diff_messages) > max_diff_message_count:
                break

            for tag, i1, i2, j1, j2 in group:
                if tag == 'equal':
                    for response_message in self.instruction.expected_response.messages[i1:i2]:
                        if len(diff_messages) > max_diff_message_count:
                            break
                        message = response_message.message
                        message = message if len(message) < one_message_max_size else message[
                                                                                      0:one_message_max_size] + '...'
                        diff_messages.append('  ' + message)

                    continue

                if tag in {'replace', 'delete'}:
                    for left_message in self.instruction.expected_response.messages[i1:i2]:
                        if len(diff_messages) > max_diff_message_count:
                            break

                        prefix = '- '
                        message = left_message.message if len(
                            left_message.message) < one_message_max_size else left_message.message[
                                                                              0:one_message_max_size] + '...'
                        diff_messages.append(prefix + message)

                if tag in {'replace', 'insert'}:
                    for right_message in self.response.messages[j1:j2]:
                        if len(diff_messages) > max_diff_message_count:
                            break
                        if isinstance(right_message, DebugResponseMessage):
                            # 调试信息在最开始输出了，不应该再输出一遍
                            continue

                        message = right_message.message if len(
                            right_message.message) < one_message_max_size else right_message.message[
                                                                               0:one_message_max_size] + '...'
                        diff_messages.append('+ ' + message)

        result_strings.extend(debug_messages)
        result_strings.extend(diff_messages)
        return result_strings


class InstructionResultGroup:
    def __init__(self, instruction_group: InstructionGroup) -> None:
        '''
        与 InstructionGroup 对应
        '''
        self.instruction_group: InstructionGroup = instruction_group
        self.results: List[InstructionResult] = []

    def __str__(self):
        result_string = '\n'.join(self.show_message())
        if result_string:
            result_string = ', results:' + result_string
        return f'name:{self.instruction_group.name}{result_string}'

    def check(self) -> bool:
        return all(map(InstructionResult.check, self.results))

    def show_message(self) -> List[str]:
        bool_result = True
        for result in self.results:
            if not result.check():
                bool_result = False
                break

        if bool_result:
            return []

        # 打印出问题的消息之前最多打印多少条之前的消息
        max_before_message_tips = 5
        before_message_count = 0
        result_strings = []
        for result in self.results:
            if result.check():
                before_message_count += 1
                if before_message_count < max_before_message_tips:
                    if result.instruction.request:
                        message = result.instruction.request.data
                        if len(message) > 256:
                            message = message[0:256] + '...'
                        result_strings.append(message)
                elif before_message_count == max_before_message_tips:
                    result_strings.append('...')

            else:
                result_strings.extend(result.show_message())
                return result_strings

        return []


class TestCaseResult:
    def __init__(self, test_case: TestCase) -> None:
        self.test_case: TestCase = test_case
        self.case_name = test_case.name
        self.result_groups: List[InstructionResultGroup] = []
        self.score = 0.0
        # 用户测试程序运行时出现问题，比如无法启动observer
        self.user_exception: str = None
        # 测试平台出现了问题
        self.test_exception: str = None

        # 因为有些命令是运行时生成的，所以在执行过程中需要将这些生成后的命令记录下来。
        # 最后返回给客户端的时候，需要展示本次运行后生成的命令。
        # key 是instruction_group的id，value是InstructionResultGroup
        self.__associate_result_group_map: Dict[int, InstructionResultGroup] = {}

    def __str__(self) -> str:
        user_exception_string = f', user_exception:{self.user_exception}' if self.user_exception else ''
        test_exception_string = f', test_exception:{self.test_exception}' if self.test_exception else ''
        return f'case:{self.case_name}{user_exception_string}{test_exception_string}, passed: {self.check()}' \
               f', result_groups:[{",".join(map(lambda result_group: "{" + str(result_group) + "}", self.result_groups))}]'

    def check(self) -> bool:
        '''
        检查当前case是否执行通过
        '''
        if self.user_exception or self.test_exception:
            return False

        return all(map(InstructionResultGroup.check, self.result_groups))

    def __init_associate_result_groups(self):
        for result_group in self.result_groups:
            self.__associate_result_group_map[id(result_group.instruction_group)] = result_group

    def show_message(self) -> List[str]:
        '''
        返回测试结果的展示信息
        '''
        self.__init_associate_result_groups()

        if self.test_exception:
            return [self.test_exception[0:4096]]

        for result_group in self.result_groups:
            if not result_group.check():
                result_strings = result_group.show_message()
                if self.user_exception:
                    result_strings.append(self.user_exception[0:4096])

                associate_groups = result_group.instruction_group.associate_groups
                if associate_groups:
                    result_strings.append('-- below are some requests executed before(partial) --')
                    for instruction_group in associate_groups:
                        # result_strings.extend(map(lambda instruction:instruction.request.data, instruction_group.instructions))
                        associate_result_group = self.__associate_result_group_map.get(id(instruction_group))
                        if associate_result_group:
                            result_strings.extend(
                                self.__get_result_group_instruction_show_message(associate_result_group))
                return result_strings

        if self.user_exception:
            return [self.user_exception[0:4096]]
        return []

    def __get_result_group_instruction_show_message(self, result_group: InstructionResultGroup) -> List[str]:
        '''
        获取InstructionResultGroup中所有InstructionResult的关联的指令展示信息
        因为有些指令是动态生成的，所以不能直接展示原始指令，而是展示运行后生成的指令。
        '''
        max_instruction_len = 256
        max_len = 5
        result_strings: List[str] = []
        result_strings.append(f'-- {result_group.instruction_group.name}')
        for instruction_result in result_group.results:
            instruction_message = instruction_result.instruction.show_request()
            instruction_message = instruction_message if len(
                instruction_message) < max_instruction_len else instruction_message[0:max_instruction_len] + '...'
            result_strings.append(instruction_message)
            if len(result_strings) > max_len:
                result_strings.append('...')
                break

        return result_strings


class TestResult:
    def __init__(self, *, test_task: TestTask, case_results: List[TestCaseResult]):
        self.task_id = test_task.task_id

        if case_results:
            self.return_code: rc.ReturnCode = rc.ReturnCode.SUCCESS
            self.test_case_result: List[TestCaseResult] = case_results
            self.message: str = ''
        else:
            self.return_code: rc.ReturnCode = rc.ReturnCode.FAILED
            self.test_case_result: List[TestCaseResult] = []
            self.message = 'Unknown'

        if test_task:
            self.branch = test_task.branch
            self.commit_id = test_task.commit_id

    def to_string(self):
        return f'''task_id={self.task_id},return_code={self.return_code},
               branch={self.branch},commit_id={self.commit_id},
               message={self.message},case_result={','.join([str(c) for c in self.test_case_result])}
            '''

    def __str__(self):
        return self.to_string()

    def set_branch(self, branch: str):
        self.branch = branch

    def set_commit_id(self, commit_id: str):
        self.commit_id = commit_id

    def set_message(self, message: str):
        self.message = message

    def set_result_code(self, returncode: rc.ReturnCode):
        self.return_code = returncode


def __test_show_message():
    print(__test_show_message.__name__ + " starting...")
    instruction: Instruction = Instruction()
    instruction.request = Request('echo', "hello")
    expected_strings = '''
  1 | 1 | A | 1
  2 | 2 | B | 2
  3 | 3 | D | 4
  4 | 4 | C | 3
  5 | 5 | E | 5.5
  6 | 6 | F | 6.6
  7 | 7 | G | 7.7
  8 | 8 | H | 8.8
  9 | 9 | I | 9.9
  10 | 10 | J | 10.1
  11 | 11 | K | 11.11
  ID | AGE | NAME | SCORE
  '''.splitlines()
    expected_strings = filter(lambda s: len(s) != 0, map(str.strip, expected_strings))
    instruction.expected_response.messages = list(map(lambda message: NormalResponseMessage(message), expected_strings))

    received_strings = '''
  # this is the first debug string
  1 | 1 | a | 1
  2 | 3 | c | 1
  3 | 3 | d | 4
  5 | 5 | e | 5.5
  # this is the second debug string
  6 | 6 | f | 6.6
  6 | 6 | f | 6.6
  7 | 7 | g | 7.7
  8 | 8 | H | 8.8
  9 | 9 | I | 9.9
  10 | 10 | J | 10.1
  ID | AGE | NAME | SCORE
  # this is the last debug string
  '''.splitlines()
    received_strings = filter(lambda s: len(s) != 0, map(str.strip, received_strings))
    response_messages = list(
        map(lambda message: DebugResponseMessage(message) if message.startswith('#') else NormalResponseMessage(
            message),
            received_strings))

    response: Response = Response()
    response.messages = response_messages
    result: InstructionResult = InstructionResult(instruction=instruction, response=response)
    print(f"result.check={result.check()}")
    results = result.show_message()
    print('\n'.join(results))


def __test_show_message_equal():
    print(__test_show_message_equal.__name__ + " starting...")

    instruction: Instruction = Instruction()
    instruction.request = Request('echo', "hello")
    expected_strings = '''
  1 | 1 | A | 1
  2 | 2 | B | 2
  3 | 3 | D | 4
  4 | 4 | C | 3
  5 | 5 | E | 5.5
  6 | 6 | F | 6.6
  7 | 7 | G | 7.7
  8 | 8 | H | 8.8
  9 | 9 | I | 9.9
  10 | 10 | J | 10.1
  11 | 11 | K | 11.11
  ID | AGE | NAME | SCORE
  '''.splitlines()
    expected_strings = filter(lambda s: len(s) != 0, map(str.strip, expected_strings))
    instruction.expected_response.messages = list(map(lambda message: NormalResponseMessage(message), expected_strings))

    received_strings = '''
  # this is the first debug message
  1 | 1 | A | 1
  2 | 2 | B | 2
  3 | 3 | D | 4
  4 | 4 | C | 3
  5 | 5 | E | 5.5
  # this is the second debug message
  6 | 6 | F | 6.6
  7 | 7 | G | 7.7
  8 | 8 | H | 8.8
  9 | 9 | I | 9.9
  10 | 10 | J | 10.1
  11 | 11 | K | 11.11
  ID | AGE | NAME | SCORE
  # this is the last debug message
  '''.splitlines()
    received_strings = filter(lambda s: len(s) != 0, map(str.strip, received_strings))
    response_messages = list(
        map(lambda message: DebugResponseMessage(message) if message.startswith('#') else NormalResponseMessage(
            message),
            received_strings))

    response: Response = Response()
    response.messages = response_messages
    result: InstructionResult = InstructionResult(instruction=instruction, response=response)
    print(f"result.check={result.check()}")
    results = result.show_message()
    print('\n'.join(results))


if __name__ == '__main__':
    __test_show_message()
    __test_show_message_equal()
