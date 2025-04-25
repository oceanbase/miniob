#!/bin/env python3

import os
import sys
import logging
import threading

from configparser import ConfigParser

from common import mylog

_logger = logging.getLogger('configuration_reader')

class ConfigurationReader:
  def __init__(self, filename: str):
    self.__lock = threading.Lock()
    self.__filename = filename

    self.__config_items = self.__parse()

  def __parse(self) -> ConfigParser:
    with self.__lock:
      cp = ConfigParser()
      try:
        cp.read(self.__filename)
        _logger.debug('read config from file: %s', self.__filename)
      except Exception as ex:
        _logger.error('failed to read configuration from file %s', self.__filename)
        raise

      return cp

  def get(self, section, key, default=None, reparse=False):
    config_items = self.__config_items
    if reparse:
      try:
        config_items = self.__parse()
      except Exception as ex:
        _logger.error('failed to read config')

    with self.__lock:
      value = config_items.get(section, key, fallback=default)
      if value is None:
        return default
      return value

  def get_int(self, section, key, default=None, reparse=False):
    value = self.get(section, key, default, reparse)
    try:
      return int(value)
    except Exception as ex:
      _logger.warn('failed to convert %s to integer', str(value))
      return default
    
  def get_bool(self, section, key, default=None, reparse=False):
    value = self.get(section, key, default, reparse)
    try:
      value = value.upper()
      if value == '0':
        return False
      if value == 'FALSE' or value == 'NO' or value == 'OFF':
        return False
      
      return True
    except Exception as ex:
      _logger.warn('failed to convert %s to boolean', str(value))
      return default

if __name__ == '__main__':
  pass
