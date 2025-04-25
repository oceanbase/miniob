#!/bin/env python3

import os
import sys
import logging

import sys
import logging
import threading

class LoggingInitHelper:
  DEFAULT = None
  __default_lock = threading.Lock()

  def __init__(self):
    self.__lock = threading.Lock()
    self.__handlers = []

  def get_handler(self, logfile: str = None, level=logging.INFO):
    if not level:
      level = logging.INFO

    log_level = level
    log_format = "%(asctime)s.%(msecs)03d [%(levelname)-5s] >>> %(message)s " \
                 "<<< %(name)s [%(funcName)s@%(filename)s:%(lineno)s] [%(threadName)s] P:%(process)d T:%(thread)d"
    log_date_format = "%Y-%m-%d %H:%M:%S"

    if logfile is None:
      handler = logging.StreamHandler(stream=sys.stdout)
      handler.setFormatter(logging.Formatter(fmt=log_format, datefmt=log_date_format))
    else:
      handler = logging.FileHandler(logfile)
      handler.setFormatter(logging.Formatter(fmt=log_format, datefmt=log_date_format))
      
    return handler

  def init_log(self, logfile: str = None, *, level=logging.INFO, force=False):
    with self.__lock:
      if self.__handlers and (not force):
        return

      handler = self.get_handler(logfile, level)
      
      root_logger = logging.getLogger()
      if self.__handlers:
        for old_handler in self.__handlers:
          root_logger.removeHandler(old_handler)
        root_logger.addHandler(handler)
      else:
        logging.basicConfig(level=level, handlers=[handler])

      self.__handlers = [handler]

  def add_log(self, logfile: str = None, *, level=logging.INFO):
    if not self.__handlers:
      self.init_log(logfile, level=level, force=True)
      return

    with self.__lock:
      handler = self.get_handler(logfile, level)
      
      root_logger = logging.getLogger()
      root_logger.addHandler(handler)

      self.__handlers.append(handler)

  @staticmethod
  def default():
    if LoggingInitHelper.DEFAULT:
      return LoggingInitHelper.DEFAULT

    with LoggingInitHelper.__default_lock:
      LoggingInitHelper.DEFAULT = LoggingInitHelper()
      return LoggingInitHelper.DEFAULT

def init_log(logfile: str = None, *, level=logging.INFO, force=False):
  return LoggingInitHelper.default().init_log(logfile, level=level, force=force)
def add_log(logfile: str = None, *, level=logging.INFO):
  return LoggingInitHelper.default().add_log(logfile, level=level)

if __name__ == '__main__':
  #init_log()
  #logging.info('this is a text')
  #init_log('1.txt')
  #logging.info('output to a file')

  #olog = logging.getLogger('o')
  #olog.info('output to file')

  LoggingInitHelper.default().init_log()
  logging.info('output to console')
  LoggingInitHelper.default().init_log('1.txt', force=False)
  logging.info('output to console and file')
  LoggingInitHelper.default().init_log('1.txt', force=True)
  logging.info('output to file only')
