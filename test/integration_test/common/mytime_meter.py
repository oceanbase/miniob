#!/bin/env python3
import logging
import time

_logger = logging.getLogger('time-meter')

class TimeMeter:
  def __init__(self, started=True):
    self.__start_ts = 0
    self.__stop_ts = 0

    if started:
      self.start()

  def start(self):
    self.__start_ts = time.monotonic()

  def stop(self):
    self.__stop_ts = time.monotonic()

  def passed_ms(self) -> float:
    return (self.__stop_ts - self.__start_ts) * 1000
