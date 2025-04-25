from typing import List, Iterable


class ResponseMessage:
    """
    表示一个响应消息
    响应消息可以用来比较
    """

    def __init__(self, message: str):
        self.message: str = message

    def __str__(self) -> str:
        return self.message

    def __hash__(self) -> int:
        if not self.message.islower():
            return hash(self.message)
        return hash(self.message.upper())

    def __eq__(self, __value: object) -> bool:
        if __value is None:
            return False

        if not type(self) is type(__value):
            return False

        other: ResponseMessage = __value
        self_message: str = self.message
        other_message: str = other.message
        if len(self_message) != len(other_message):
            return False

        if self_message.islower():
            self_message = self_message.upper()
        if other_message.islower():
            other_message = other_message.upper()
        return self_message == other_message


class NormalResponseMessage(ResponseMessage):
    """
    表示一个普通的响应消息
    """

    def __init__(self, message: str):
        super().__init__(message)

    def __str__(self) -> str:
        return f'Normal:{super().__str__()}'


class DebugResponseMessage(ResponseMessage):
    """
    表示一个调试的响应消息
    调试信息仅用来帮助选手查看调试信息，不用来做结果对比
    """

    def __init__(self, message: str):
        super().__init__(message)

    def __str__(self) -> str:
        return f'Debug:{super().__str__()}'


class Response:
    """
    执行一个命令后收到的结果
    包含一系列的响应消息
    """

    def __init__(self):
        self.messages: List[ResponseMessage] = []

    def __str__(self) -> str:
        # 这里只取前 30 行是为了防止输出 message 过多。
        messages = self.messages[0:30]
        return f'messages={str(list(map(lambda message: message.message, messages)))}'

    def normal_messages(self) -> Iterable[ResponseMessage]:
        return filter(lambda message: isinstance(message, NormalResponseMessage), self.messages)
