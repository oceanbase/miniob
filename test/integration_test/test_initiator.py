import logging
from test_exception import TestUserException, TestPlatformException
from test_config import TestConfig
from util import mycompile

_logger = logging.getLogger("TestInitiator")


class TestInitiator:
    """
    在一个TestSuite所有的测试开始之前执行一些动作
    """

    def __init__(self):
        self.name: str = ''
        self.description: str = ''

        self._logger = _logger
        self._test_config = None

    def init(self, *, test_config: TestConfig, logger: logging.Logger = None):
        self._test_config = test_config
        if logger:
            self._logger = logger

        self._logger.info('call the test init routine')
        self._init()
        self._logger.info('test init done')

    def _init(self):
        raise TestPlatformException('don\'t call the default init routine')


class DefaultTestInitiator(TestInitiator):
    def __init__(self):
        super().__init__()

    def _init(self):
        self.compile()

    def compile(self):
        self._logger.info('compile begin')
        source_dir: str = self._test_config.source_dir
        build_dir: str = self._test_config.build_dir
        test_user: str = self._test_config.test_user
        cmake_args = self._test_config.cmake_args
        make_args = self._test_config.make_args
        use_ccache = self._test_config.use_ccache

        if not cmake_args:
            cmake_args = []

        if not make_args:
            make_args = []

        if use_ccache:
            cmake_args.append('-DCMAKE_C_COMPILER_LAUNCHER=ccache')
            cmake_args.append('-DCMAKE_CXX_COMPILER_LAUNCHER=ccache')

            # 这里通过环境变量设置ccache的配置，就会在全局生效，因此miniob-test不能支持一次运行多个选手的任务
            # 现在使用同一个源码目录，不再设置basedir
            # os.environ['CCACHE_BASEDIR'] = source_dir

            # 从最终的代码来看，使用ccache的话，只需要改配置文件即可，不需要修改这些东西。
            # cmake的参数是可以配置的。
            self._logger.info('user ccache')
        else:
            self._logger.info('does not use ccache')

        compiler = mycompile.CompileRunner(source_dir=source_dir, build_dir=build_dir)
        result = compiler.run_cmake(cmake_args, test_user)
        if not result:
            raise TestUserException('failed to run cmake\n' + result.error_message())

        self._logger.info('make begin')
        result = compiler.run_make(make_args, test_user)
        if not result:
            raise TestUserException('failed to run make\n' + result.error_message())

        self._logger.info('compile done')
