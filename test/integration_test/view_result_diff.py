import os
import sys
import logging
import shutil

import __init__
from util import myshell

class ViewResultDiff:
    def __init__(self, max_line:int = 10):
        self.__max_line = max_line

    def get_diff_view(self, src_file: str, dst_file: str) -> str:
        '''
        compare two files
        return empty string if they are same
        or return the error info or diff info
        '''
        if not src_file or not os.path.isfile(src_file):
            return 'DiffView:source file is not exists'
        if not dst_file or not os.path.isfile(dst_file):
            return 'DiffView:dest file is not exists'

        '''
        去掉所有空格、空行然后做比较
        先将源文件复制到一个临时文件中
        NOTE: 这里假设每次只有一个程序在处理这些文件
        '''
        from_file = src_file + '.cmp'
        to_file = dst_file + '.cmp'
        shutil.copyfile(src_file, from_file)
        shutil.copyfile(dst_file, to_file)
        myshell.run_shell_command(["sed -i '/^[  ]*$/d' " + from_file], shell=True)
        myshell.run_shell_command(["sed -i '/^[  ]*$/d' " + to_file], shell=True)

        # 不区分大小写
        commands = ['diff', '-B', '-w', '-b', '-E', '-i', '-U', '3', from_file, to_file]
        returncode, outputs, errors = myshell.run_shell_command(commands)
        if returncode == 0:
            return '' # the source file is idetified to dest file

        '''
        --- /root/miniob_test/etc/result/primary-complex-sub-query.result 2022-06-13 01:42:23.000000000 +0000
        +++ ./primary-complex-sub-query.result.tmp  2022-06-21 03:11:57.924965835 +0000
        @@ -29,85 +29,3 @@
    
         1. SELECT
          select * from csq_1 where id in (select csq_2.id from csq_2 where csq_2.id in (select csq_3.id from csq_3));
          -1 | 4 | 11.2
          -ID | COL1 | FEAT1
        '''

        if returncode == 2: # diff return error
            return '\n'.join(errors)

        if len(outputs) < 3:
            return '\n'.join(outputs)

        results = []
        for line in outputs[3:]:
            line = line.strip()
            if not line:
                continue

            if line.startswith('@@ -'):
                break

            results.append(line)
            if len(results) >= self.__max_line:
                break

        return '\n'.join(results)

if __name__ == '__main__':
    v = ViewResultDiff()
    result = v.get_diff_view('/root/miniob_test/etc/result/basic.result', '/root/miniob_test/etc/result/basic.result')
    print(result)

