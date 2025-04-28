from typing import Dict, List, Any
from mysql_executor.mysql_executor import MysqlClient, MysqlConnectConfig, MysqlResult, MysqlErrorEnum, MysqlError
import datetime, decimal
from decimal import Decimal, ROUND_HALF_UP

class MysqlAdaptor:
    def __init__(self, *, config:MysqlConnectConfig):
        self.config = config
        self.__default_client_name = 'default'
        self.__default_client:MysqlClient = None
        self.__clients:Dict[str, MysqlClient] = {}
        self.__current_client:MysqlClient = None
        self.__database:str = None

    def init(self, defult_client_name:str):
        self.__default_client_name = defult_client_name
        self.__default_client = MysqlClient()
        self.__default_client.connect(self.config)

        db = self.config.database
        if not db.startswith('`'):
            db = f'`{db}`'

        self.__default_client.execute(f'drop database if exists {db}')
        self.__default_client.execute(f'create database {db}')
        self.__default_client.execute(f'use {db}')

        self.__database = db

        self.__clients[self.__default_client_name] = self.__default_client

        self.__current_client = self.__default_client

    def new_connect(self, client_name:str):
        if client_name in self.__clients.keys():
            raise ValueError(f'client {client_name} already exists')

        client = MysqlClient()
        client.connect(self.config)
        if self.__database:
            client.execute(f'use {self.__database}')

        self.__clients[client_name] = client

    def execute(self, sql:str) -> MysqlResult:
        return self.__current_client.execute(sql)

    def set_current_client(self, client_name:str):

        if not client_name in self.__clients.keys():
            raise ValueError(f'no such client: {client_name}')

        self.__current_client = self.__clients[client_name]

    @staticmethod
    def _field_string(value:Any) -> str:
        if value is None:
            return 'NULL'

        if isinstance(value, str):
            return value

        if isinstance(value, int):
            return str(value)
        #f'{value:.2f}' 默认不是四舍五入。对于 0.625 会保留成 0.62
        if isinstance(value, float) or isinstance(value, decimal.Decimal):
            real_value = Decimal(value).quantize(Decimal('0.00'), rounding=ROUND_HALF_UP)
            s = f'{real_value}'
            s = s.rstrip('0').rstrip('.')
            return s

        if isinstance(value, datetime.date):
            return f'{value.year:04}-{value.month:02}-{value.day:02}'

        raise ValueError(f'unknown mysql type: {type(value)}')

    @staticmethod
    def result_to_miniob(mysql_result:MysqlResult) -> List[str]:
        field_sep = ' | '
        header = mysql_result.header
        rows = mysql_result.rows

        result_list : List[str] = []
        if header:
            result_list.append(field_sep.join(header))

        if rows:
            for row in rows:
                result_list.append(field_sep.join(map(lambda x: MysqlAdaptor._field_string(x), row)))

        return result_list
