import os
import sys

'''
'''

__this_file = os.path.abspath(__file__)
__root_dir = os.path.dirname(os.path.dirname(__this_file))

if not __root_dir in sys.path:
    sys.path.insert(0, __root_dir)
