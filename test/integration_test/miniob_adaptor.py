import os
import sys
import logging
import socket
import select
import traceback
from typing import Tuple
from miniob.miniob_client import MiniObClient
from miniob.miniob_server import MiniObServer

_logger = logging.getLogger('MiniOBAdaptor')

class MiniobClientDryrun(MiniObClient):
    def __init__(self, *,
                 server_port: int = None,
                 server_socket: str = None,
                 time_limit:float = 10.0,
                 charset='UTF-8',
                 logger:logging.Logger=None):
        pass

    def run_sql(self, sql: str, timeout=None, *, total_timeout_seconds:int=None) -> Tuple[bool, str]:
        return (True, 'dryrun')

    def close(self):
        pass

class MiniobServerDryrun(MiniObServer):
    def __init__(self):
        pass

    def stop_all_servers(self, wait_seconds=10.0):
        pass

    def start_server(self, test_user: str = '') -> bool:
        return True
    def stop_server(self) -> bool:
        return True

    def clean(self):
        pass
    def coredump_info(self) -> str:
        return None
  