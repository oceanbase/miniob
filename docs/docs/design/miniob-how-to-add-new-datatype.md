---
title: 如何新增一种数据类型
---

# 概述

本文详细介绍如何在 MiniOB 中新增一种数据类型。MiniOB 采用分层架构设计，数据类型系统实现集中在 `src/observer/common` 目录下，通过统一的接口提供类型无关的数据操作。

# 核心架构

## 1. 数据类型系统架构

MiniOB 的数据类型系统采用分层设计，主要包含以下核心组件：

### Value 类 - 统一数据操作接口
- **路径**: `src/observer/common/value.h`
- **作用**: 封装实际数据值，提供类型无关的操作方法
- **特点**: 支持多种数据类型的统一操作接口

### DataType 基类 - 类型操作抽象
- **路径**: `src/observer/common/type/data_type.h`
- **作用**: 定义数据类型操作的抽象接口
- **功能**: 提供比较、算术运算、类型转换等操作的虚函数

### 具体类型实现类 - 特定类型操作
- **路径**: `src/observer/common/type/`
- **作用**: 每种数据类型对应一个实现类，实现具体的运算逻辑
- **示例**: `IntegerType`、`FloatType`、`CharType` 等

## 2. 现有数据类型

当前 MiniOB 支持以下数据类型：

```cpp
enum class AttrType
{
  UNDEFINED,
  CHARS,     ///< 字符串类型
  INTS,      ///< 整数类型(4字节)
  FLOATS,    ///< 浮点数类型(4字节)
  VECTORS,   ///< 向量类型
  BOOLEANS,  ///< boolean类型
  MAXTYPE,   ///< 请在 UNDEFINED 与 MAXTYPE 之间增加新类型
};
```

# 数据类型操作流程

## 示例：整数类型操作

以下代码展示了 MiniOB 如何处理整数类型数据：

```cpp
// 1. 创建整数 Value 对象
int val = 42;
Value value(val);  // 自动识别为 AttrType::INTS

// 2. 执行算术运算
Value left(10);
Value right(32);
Value result;
result.set_type(AttrType::INTS);  // 设置结果类型

// 3. 调用统一接口进行加法运算
RC rc = Value::add(left, right, result);
// 内部会调用 IntegerType::add() 方法
```

## 类型分发机制

`Value::add()` 方法通过以下机制实现类型分发：

```cpp
static RC add(const Value &left, const Value &right, Value &result)
{
    return DataType::type_instance(result.attr_type())->add(left, right, result);
}
```

# 新增数据类型步骤

## 以新增 DATE 类型为例

### 步骤 1: 添加类型枚举

在 `src/observer/common/type/attr_type.h` 中添加新类型：

```cpp
enum class AttrType
{
  UNDEFINED,
  CHARS,
  INTS,
  FLOATS,
  VECTORS,
  BOOLEANS,
  DATES,     ///< 新增：日期类型
  MAXTYPE,   ///< 请在 UNDEFINED 与 MAXTYPE 之间增加新类型
};
```

### 步骤 2: 注册类型实例

在 `src/observer/common/type/data_type.cpp` 中注册新类型：

```cpp
array<unique_ptr<DataType>, static_cast<int>(AttrType::MAXTYPE)> DataType::type_instances_ = {
    make_unique<DataType>(AttrType::UNDEFINED),
    make_unique<CharType>(),
    make_unique<IntegerType>(),
    make_unique<FloatType>(),
    make_unique<VectorType>(),
    make_unique<DataType>(AttrType::BOOLEANS),
    make_unique<DateType>(),  // 新增：注册 DateType 实例
};
```

### 步骤 3: 实现具体类型类

创建 `src/observer/common/type/date_type.h`：

```cpp
#pragma once
#include "common/type/data_type.h"

class DateType : public DataType
{
public:
  DateType() : DataType(AttrType::DATES) {}
  virtual ~DateType() {}

  int compare(const Value &left, const Value &right) const override;
  RC add(const Value &left, const Value &right, Value &result) const override;
  RC subtract(const Value &left, const Value &right, Value &result) const override;
  RC cast_to(const Value &val, AttrType type, Value &result) const override;
  RC to_string(const Value &val, string &result) const override;
  RC set_value_from_str(Value &val, const string &data) const override;
  
  int cast_cost(const AttrType type) override;
};
```

### 步骤 4: 实现类型操作逻辑

创建 `src/observer/common/type/date_type.cpp`：

```cpp
#include "common/type/date_type.h"
#include "common/value.h"

int DateType::compare(const Value &left, const Value &right) const
{
  // 实现日期比较逻辑
  // 返回 -1 (left < right), 0 (left == right), 1 (left > right)
}

RC DateType::add(const Value &left, const Value &right, Value &result) const
{
  // 实现日期加法运算（如加天数）
  // 设置 result 的值和类型
  return RC::SUCCESS;
}

RC DateType::to_string(const Value &val, string &result) const
{
  // 将日期值转换为字符串格式
  return RC::SUCCESS;
}

RC DateType::set_value_from_str(Value &val, const string &data) const
{
  // 从字符串解析日期值
  return RC::SUCCESS;
}
```

### 步骤 5: 扩展 Value 类支持

在 `src/observer/common/value.h` 中添加日期类型的构造函数：

```cpp
class Value final
{
public:
  // 现有构造函数...
  explicit Value(const Date &date);  // 新增：日期构造函数
  
  // 现有方法...
  Date get_date() const;             // 新增：获取日期值
  void set_date(const Date &date);   // 新增：设置日期值
};
```

### 步骤 6: 更新词法和语法规则（可选）

如果需要支持 SQL 中的 DATE 关键字，需要修改：

- **词法规则**: `src/observer/sql/parser/lex_sql.l`
- **语法规则**: `src/observer/sql/parser/yacc_sql.y`

# 实现要点

## 1. 类型兼容性

实现 `cast_cost()` 方法定义类型转换成本：

```cpp
int DateType::cast_cost(const AttrType type) override
{
  if (type == AttrType::DATES) {
    return 0;  // 相同类型，无转换成本
  } else if (type == AttrType::CHARS) {
    return 1;  // 可转换为字符串，成本为1
  }
  return INT32_MAX;  // 不支持转换
}
```

## 2. 错误处理

所有操作都应返回适当的 `RC` 状态码：

```cpp
RC DateType::add(const Value &left, const Value &right, Value &result) const
{
  if (left.attr_type() != AttrType::DATES || right.attr_type() != AttrType::INTS) {
    return RC::SCHEMA_FIELD_TYPE_MISMATCH;
  }
  // 执行加法操作...
  return RC::SUCCESS;
}
```

## 3. 内存管理

对于复杂数据类型，注意内存管理：

```cpp
void Value::set_date(const Date &date)
{
  reset();  // 清理之前的数据
  attr_type_ = AttrType::DATES;
  // 设置日期值...
}
```

# 测试验证

新增数据类型后，建议编写单元测试验证功能：

```cpp
TEST(DateTypeTest, BasicOperations)
{
  Value date1("2023-01-01");
  Value date2("2023-01-02");
  Value result;
  result.set_type(AttrType::DATES);
  
  // 测试比较操作
  EXPECT_EQ(DateType().compare(date1, date2), -1);
  
  // 测试字符串转换
  string str;
  EXPECT_EQ(DateType().to_string(date1, str), RC::SUCCESS);
  EXPECT_EQ(str, "2023-01-01");
}
```

# 总结

新增数据类型需要按照上述步骤系统性地修改多个文件，确保类型系统的一致性和完整性。建议在实现过程中参考现有的 `IntegerType` 和 `FloatType` 实现，保持代码风格的一致性。