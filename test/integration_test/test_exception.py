
class TestBaseException(Exception):
    def __init__(self, *args: object) -> None:
        super().__init__(*args)

class TestUserException(TestBaseException):
    '''
    由于选手的程序导致的异常，比如启动observer失败，连接服务端失败
    '''
    def __init__(self, *args: object) -> None:
        super().__init__(*args)

class TestPlatformException(TestBaseException):
    '''
    由于测试程序本身或者测试环境导致的异常，比如代码BUG、文件权限问题、测试用例本身有BUG
    '''
    def __init__(self, *args: object) -> None:
        super().__init__(*args)
