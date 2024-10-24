# miniob-test
miniob自动化功能测试

运行所有测试用例：
```bash
python3 miniob_test.py
```

运行指定测试用例
```bash
python3 miniob_test.py --test-cases=`case-name` --report-only
```

> 把`case-name` 换成你想测试的用例名称。
> 如果要运行多个测试用例，则在 --test-cases 参数中使用 ',' 分隔写多个即可

**注意**: 由于当前绝大部分测试已经没有result文件，所以无法正常测试。示例命令中增加了参数 `--report-only` 就不会真正的运行测试。

你可以设计自己的测试用例，并填充 result 文件后，删除 `--report-only` 参数进行测试。你可以用这种方法进行日常回归测试，防止在实现新功能时破坏了之前编写的功能。

运行 basic 测试用例
```bash
python3 miniob_test.py --test-cases=basic
```

注意：MiniOB 的题目要求是运行结果与 MySQL8.0 的结果一致，因此删除了过时的结果文件。如果不清楚某个测试用例的预期结果，参考 MySQL8.0 的运行结果即可。

更多运行方法和参数可以参考 miniob_test.py

