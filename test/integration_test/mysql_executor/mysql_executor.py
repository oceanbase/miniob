import pymysql
import logging
import traceback
from enum import Enum
from typing import List, Dict, Tuple, Any

_logger = logging.getLogger('mysql_executor')

class MysqlConnectConfig:
    def __init__(self, *,
                 host:str=None,
                 port:int=None,
                 unix_socket:str=None,
                 user:str='root',
                 password:str=None,
                 database:str='test'):
        self.host = host
        self.port = port
        self.unix_socket = unix_socket # if unix socket is not None, host and port will be ignored
        self.user = user
        self.password = password
        self.database = database

    def __str__(self) -> str:
        return f'host:{self.host}, port:{self.port}, unix_socket:{self.unix_socket}, ' \
               f'user:{self.user}, password:{self.password}, database:{self.database}'

class MysqlErrorEnum(Enum):
    SUCCESSED = 0
    CANT_CREATE_FILE = 1004
    CANT_CREATE_TABLE = 1005
    CANT_CREATE_DB = 1006
    DB_CREATE_EXISTS = 1007
    DB_DROP_EXISTS = 1008
    CANT_DELETE_FILE = 1009
    CANT_OPEN_FILE = 1016
    NO_DB_ERROR = 1046
    TABLE_EXISTS_ERROR = 1050
    NON_UNIQ_ERROR = 1052
    NONUNIQ_TABLE = 1066
    UNKNOWN_ERROR = 1105
    UNKNOWN_TABLE = 1109
    NO_SUCH_TABLE = 1146
    SYNTAX_ERROR = 1149

class MysqlError:
    def __init__(self, *, code:int, message:str):
        self.code = code
        self.message = message

    def __str__(self):
        code_string = f'({MysqlErrorEnum(self.code).name})' if self.code in MysqlErrorEnum.__members__ else ''
        return f'code:{self.code}{code_string}, message:{self.message}'

class MysqlResult:
    def __init__(self, *, header:List[str]=None, rows:List[Tuple[Any]]=None, error:MysqlError=None):
        self.header = header
        self.rows   = rows
        self.error  = error

    def __str__(self):
        error = f', error:{self.error}' if self.error else ''
        return f'header:{self.header}, rows:{self.rows}{error}'

    def has_error(self) -> bool:
        return self.error and (self.error.code != MysqlErrorEnum.SUCCESSED.value)

class MysqlClient:
    def __init__(self) -> None:
        self.config:MysqlConnectConfig = None
        self.connection:pymysql.Connection = None

    def connect(self, config:MysqlConnectConfig, *, autocommit=True) -> None:
        self.config = config
        self.connection = pymysql.connect(host=config.host, port=config.port, user=config.user, password=config.password, autocommit=autocommit)
        _logger.info('connect to mysql server success: %s', str(config))

    def execute(self, sql:str) -> MysqlResult:
        try:
            with self.connection.cursor() as cursor:
                cursor.execute(sql)
                rows = cursor.fetchall()

                header = []
                if cursor.description:
                    header = [ desc[0] for desc in cursor.description ]

                _logger.debug('execute mysql: %s, rows=%s', sql, str(rows))
                return MysqlResult(header=header, rows=rows)
        except pymysql.Error as ex:
            _logger.info('execute sql(%s) error: %s', sql, str(ex))
            return MysqlResult(error=MysqlError(code=ex.args[0], message=ex.args[1]))
