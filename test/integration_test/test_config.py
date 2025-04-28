from typing import List
import os

import __init__
from mysql_executor.mysql_executor import MysqlConnectConfig


class TestConfig:
    def __init__(self):
        self.test_case_dir: str  = ''  # 存放test case文件的目录
        self.test_case_type: str = 'python' # 当前仅支持python，不支持text
        self.game_test_dir = os.path.dirname(os.path.abspath(__file__)) # 存放本仓库代码的地址
        self.test_user: str      = None # 执行测试的用户

        # 编译相关的参数
        self.source_dir: str       = None # 源码放置的路径
        self.build_dir: str        = None # 编译输出的路径
        self.use_ccache: bool      = False # 是否使用CCACHE
        self.cmake_args: [str]     = []
        self.make_args: [str]      = []

        self.test_cases:List[str]  = None
        self.observer_path: str    = None
        self.db_data_dir: str      = ''
        self.db_config: str        = ''
        self.server_port           = -1
        self.use_unix_socket: bool = True
        self.debug: bool           = False

        self.core_path:str        = None

        self.mysql_enabled:bool   = False
        self.mysql_host:str       = None
        self.mysql_port:int       = -1
        self.mysql_unix_socket:str= None
        self.mysql_user:str       = None
        self.mysql_password:str   = None
        self.mysql_db:str         = None

    def get_mysql_config(self) -> MysqlConnectConfig:
        if not self.mysql_enabled:
            return None

        config = MysqlConnectConfig()
        config.host        = self.mysql_host
        config.port        = self.mysql_port
        config.unix_socket = self.mysql_unix_socket
        config.user        = self.mysql_user
        config.password    = self.mysql_password
        config.database    = self.mysql_db
        return config

    def __str__(self) -> str:
        return str(self.__dict__)
