
class Request:
    def __init__(self, command:str, data:str):
        self.command:str = command
        self.data:str = data

    def __str__(self):
        return f'command:{self.command},data:{self.data}'
