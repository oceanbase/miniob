import os

prefix = "vscode_dev_with_local_virtual_env_"

for filename in os.listdir("."):
    if os.path.isfile(filename):
        new_name = prefix + filename
        os.rename(filename, new_name)