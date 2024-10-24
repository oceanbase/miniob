# miniob-test
miniob自动化功能测试

运行所有测试用例：
```bash
python3 miniob_test.py
```

运行 basic 测试用例
```bash
python3 miniob_test.py --test-cases=basic --report-only
```

> 如果要运行多个测试用例，则在 --test-cases 参数中使用 ',' 分隔写多个即可

注意：MiniOB 的题目要求是运行结果与 MySQL8.0 的结果一致，因此删除了过时的结果文件。如果不清楚某个测试用例的预期结果，参考 MySQL8.0 的运行结果即可。

更多运行方法和参数可以参考 miniob_test.py

