---
title: 如何新增一种数据类型
---

> 本文介绍如何新增一种数据类型。
MiniOB 的数据类型系统采用分层设计，实现集中在[path](../../../src/observer/common)文件夹下，核心组件包括：
1. Value 类：统一数据操作接口
路径：src/observer/common/value.h
作用：封装实际数据值，提供类型无关的操作方法
2. Type 工具类：特定类型的操作实现
路径：src/observer/common/type/
作用：每种数据类型对应一个工具类，实现具体运算逻辑

以下示例展示 MiniOB 如何处理整数类型数据：
```cpp
// 假设解析器识别到整数 "1"
int val = 1;
Value value(val);  // 封装为 Value 对象
// 执行加法运算
Value result;
Value::add(value, value, result);  // 调用加法接口
// Value::add 方法内部会根据类型调用对应工具类
// 对于 INT 类型，实际调用代码位于：
// src/observer/common/type/integer_type.cpp
```

# 若要新增一种数据类型（如 DATE），建议按以下步骤开发：
1. 在 src/observer/common/type/attr_type.h 中添加新的类型枚举以及对应类型名
2. 在 src/observer/common/type/data_type.cpp 中添加新的类型实例
3. 在 src/observer/common/type/ 文件夹下，参照现有工具类，实现 DateType 工具类
4. 在 Value 类中增加类型处理逻辑，支持date类型的分发，储存date类型值
5. 必要情况下还需要增加新的词法规则(lex_sql.l)以及语法规则(yacc_sql.y)，支持新类型关键字