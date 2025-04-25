#!/usr/bin/env python3
# -*- coding: utf-8 -*-#

import json
import logging

_logger = logging.getLogger('case_name_mapper')

class CaseNameMapper:
    '''
    因为本地上使用的测试名称，与前端配置的可能会不同，前端展示配置的名字也可能会不太容易操作，
    所以这里做一个后端case name到前端case name的映射。
    '''

    def __init__(self):
        self.__mapper = {}

    def use_json_file(self, filename:str):
        try:
            f = open(filename, 'r')
            s = f.read()
            self.use_json_string(s)
        except Exception as ex:
            _logger.error('failed to use json file %s. ex=%s', filename, str(ex))

        _logger.info('case name mapper use file: %s', filename)

    def use_json_string(self, json_string:str):
        try:
            json_data = json.loads(json_string)
            if type(json_data) != dict:
                _logger.error('invalid json. should be dict data: %s', json_string)

            self.__mapper = json_data
        except Exception as ex:
            _logger.error('failed to parse json data: %s', json_string)

    def map_name(self, case_name: str) -> str:
        value: str = self.__mapper.get(case_name)
        if not value:
            return case_name
        return value

if __name__ == '__main__':
    filename = '/root/miniob_test/etc/case_name_map.json'
    mapper = CaseNameMapper()
    mapper.use_json_file(filename)
    print(mapper.map_name('primary-complex-sub-query'))
