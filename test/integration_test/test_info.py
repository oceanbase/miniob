import logging
from typing import List

class TestTask:
    def __init__(self, *, task_id = None, player: str = None, git_repo: str, branch: str = None, commit_id: str = None, test_suite:str=None):
        self.task_id = task_id
        self.player = player
        self.git_repo = git_repo
        self.branch = branch
        self.commit_id = commit_id
        self.cases:List[str] = None
        self.test_suite:str = test_suite

    def to_string(self):
        return f'task_id={str(self.task_id)},git_repo={str(self.git_repo)},branch={str(self.branch)},commit_id={str(self.commit_id)},test_suite={str(self.test_suite)}'

    def __str__(self):
        return self.to_string()

    @staticmethod
    def default_player() -> str:
        return "test"
