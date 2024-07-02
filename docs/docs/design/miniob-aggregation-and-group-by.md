---
title: MiniOB 向量化执行 aggregation 和 group by 实现
---

# MiniOB 向量化执行 aggregation 和 group by 实现

本篇文档介绍 MiniOB 向量化执行中的简单聚合（ungrouped aggregation）和分组聚合（grouped aggregation）实现。

## 执行模型（processing model）

首先，介绍下执行模型的概念，数据库执行模型定义了在数据库中如何执行一个查询计划。这里简单介绍下两种执行模型：火山模型(Volcano iteration model) 和向量化模型(Vectorization model)。

```
  Volcano iteration model     Vectorization model
                                                  
    ┌───────────┐                ┌───────────┐    
    │           │                │           │    
    │ operator1 │                │ operator1 │    
    │           │                │           │    
    └─┬─────▲───┘                └─┬─────▲───┘    
      │     │                      │     │        
next()│     │ tuple    next_chunk()│     │ chunk/batch 
    ┌─▼─────┴───┐                ┌─▼─────┴───┐    
    │           │                │           │    
    │ operator2 │                │ operator2 │    
    │           │                │           │    
    └─┬─────▲───┘                └─┬─────▲───┘    
      │     │                      │     │        
next()│     │ tuple    next_chunk()│     │ chunk/batch
    ┌─▼─────┴───┐                ┌─▼─────┴───┐    
    │           │                │           │    
    │ operator3 │                │ operator3 │    
    │           │                │           │    
    └───────────┘                └───────────┘    
```

### 火山模型

在火山模型中，每个查询计划操作符（operator）都实现了一个 `next()` 函数。在每次调用时，操作符要么返回一个元组(tuple)，要么在没有更多元组时返回一个空结果。每个操作符在它的子运算符调用 `next()` 来获取元组，然后对它们进行处理，并向上返回结果。

### 向量化执行模型

向量化执行模型与火山模型类似，其中每个运算符都有一个生成元组的 `next_chunk()` 方法。然而不同的是，`next_chunk()`方法的每次调用都会获取一组元组而不是仅仅一个元组，这会分摊迭代调用开销。

### MiniOB 中的实现

在 MiniOB 中已经实现了上述的两种执行模型，可以通过配置项 `execution_mode` 用于区分这两种执行模型。默认使用火山模型，按 `tuple` 迭代。可通过下述 SQL 语句设置执行模型。

将执行模型设置为按 `chunk` 迭代的向量化模型。

```sql
SET execution_mode = 'chunk_iterator';
```

将执行模型设置为按 `tuple` 迭代的火山模型。

```sql
SET execution_mode = 'tuple_iterator';
```

## 向量化执行模型中 aggregation 和 group by 实现

### aggregation 实现

aggregation 实现主要位于`src/sql/operator/aggregate_vec_physical_operator.cpp` 中，这里需要实现不带分组的聚合，即类似 `select sum(a),sum(b) from t` 的SQL语句。
在 aggregation 实现中，主要涉及到以下几个步骤：
1. 在 `open()` 函数中，通过调用下层算子的 `next(Chunk &chunk)` 来不断获得下层算子的输出结果（Chunk）。对从下层算子获得的 Chunk 进行表达式计算，聚合计算，并将聚合结果暂存在聚合算子中。
2. 在 `next(Chunk &chunk)` 函数中，将暂存的聚合结果按 Chunk 格式向上返回。

### group by 实现

group by 实现主要位于`src/sql/operator/group_by_vec_physical_operator.cpp`中，这里需要实现分组聚合，即类似`select a, sum(b) from t group by a`的SQL语句。
group by 实现采用了基于哈希的分组方式，主要涉及到以下几个步骤：
1. 在 `open()` 函数中，通过调用下层算子的`next(Chunk &chunk)` 来不断获得下层算子的输出结果（Chunk）。对从下层算子获得的 Chunk 进行表达式计算，根据分组列计算出分组位置，并将聚合结果暂存在相应的分组位置中。
2. 在 `next(Chunk &chunk)` 函数中，将暂存在哈希表中的聚合结果按 Chunk 格式向上返回。

### 实验

1. 需要补充 `src/observer/sql/parser/yacc_sql.y` 中 标注`// your code here` 位置的 `aggregation` 和 `group by` 相关的语法。并通过 `src/sql/parser/gen_parser.sh` 生成正确的语法分析代码。
2. 需要实现 `src/observer/sql/operator/aggregate_vec_physical_operator.cpp` 中标注 `// your code here` 位置的代码。
3. 需要完整实现位于 `src/observer/sql/operator/group_by_vec_physical_operator.h` 的group by 算子。
4. 需要实现 `src/observer/sql/expr/aggregate_hash_table.cpp::StandardAggregateHashTable` 中标注 `// your code here` 位置的代码。

### 测试

需通过 `unittest/observer/parser_test.cpp` 和 `test/case/test/vectorized-aggregation-and-group-by.test`。

注意1：训练营中的测试采用对MiniOB/MySQL 输入相同的测试 SQL（MiniOB 中建表语句包含 storage format 选项，MySQL 中不包含），对比 MiniOB 执行结果与 MySQL 执行结果的方式进行。训练营中的测试 case 相比 `test/case/test/vectorized-aggregation-and-group-by.test` 更加多样（包含一些随机生成的 SQL），但不存在更加复杂的 SQL（不需要实现除 `sum` 之外的聚合函数）。

注意2：如果需要运行 `parser_test` 请移除`DISABLED_` 前缀。

## 实现 SIMD 指令优化的 aggregation 和 group by

### SIMD 介绍

如下图所示，Single Instruction Multiple Data(SIMD)，单指令多数据流，可以使用一条指令同时完成多个数据的运算操作。传统的指令架构是SISD就是单指令单数据流，每条指令只能对一个数据执行操作。因此，通过合理使用 SIMD 指令，可以减少运算执行过程中的 CPU 指令数，从而优化计算开销。[参考资料](#参考资料)中包含了 SIMD 介绍文章及指令手册。

```
          SISD                                 SIMD               
┌────┐      ┌────┐     ┌────┐        ┌────┐      ┌────┐     ┌────┐
│ a0 │  +   │ b0 │  =  │ c0 │        │ a0 │  +   │ b0 │  =  │ c0 │
└────┘      └────┘     └────┘        │    │      │    │     │    │
                                     │    │      │    │     │    │
┌────┐      ┌────┐     ┌────┐        │    │      │    │     │    │
│ a1 │  +   │ b1 │  =  │ c1 │        │ a1 │  +   │ b1 │  =  │ c1 │
└────┘      └────┘     └────┘        │    │      │    │     │    │
                                     │    │      │    │     │    │
┌────┐      ┌────┐     ┌────┐        │    │      │    │     │    │
│ a2 │  +   │ b2 │  =  │ c2 │        │ a2 │  +   │ b2 │  =  │ c2 │
└────┘      └────┘     └────┘        │    │      │    │     │    │
                                     │    │      │    │     │    │
┌────┐      ┌────┐     ┌────┐        │    │      │    │     │    │
│ a3 │  +   │ b3 │  =  │ c3 │        │ a3 │  +   │ b3 │  =  │ c3 │
└────┘      └────┘     └────┘        └────┘      └────┘     └────┘
```

### SIMD 指令在 MiniOB 中的应用

通过 SIMD 指令，我们可以优化 MiniOB 向量化执行引擎中的部分批量运算操作，如表达式计算，聚合计算，hash group by等。在 `src/observer/sql/expr/arithmetic_operator.hpp` 中需要实现基于 SIMD 指令的算术运算；在`deps/common/math/simd_util.cpp` 中需要实现基于 SIMD 指令的数组求和函数 `mm256_sum_epi32` 和`_mm256_storeu_si256`； 使用 SIMD 指令优化 hash group by 的关键在于实按批操作的哈希表，MiniOB 的实现参考了论文：`Rethinking SIMD Vectorization for In-Memory Databases` 中的线性探测哈希表（Algorithm 5），更多细节可以参考`src/observer/sql/expr/aggregate_hash_table.cpp`中的注释。

注意：在编译时，只有使用 `-DUSE_SIMD=ON` 时（默认关闭），才会编译 SIMD 指令相关代码。

### 实验

1. 需要使用 SIMD 指令实现 `src/observer/sql/expr/arithmetic_operator.hpp` 中标注 `// your code here` 位置的代码。
2. 需要使用 SIMD 指令实现 `deps/common/math/simd_util.cpp` 中标注 `// your code here` 位置的代码。
3. 需要使用 SIMD 指令实现 `src/observer/sql/expr/aggregate_hash_table.cpp::LinearProbingAggregateHashTable` 中标注 `// your code here` 位置的代码。

### 测试

需要通过 `arithmetic_operator_test` 和 `aggregate_hash_table_test` 单元测试以及 `arithmetic_operator_performance_test` 和 `aggregate_hash_table_performance_test` 性能测试。

注意：如果需要运行 `arithmetic_operator_test`、`aggregate_hash_table_test`、`arithmetic_operator_performance_test`、`aggregate_hash_table_performance_test` ，请移除`DISABLED_` 前缀。

### 参考资料

1. [Rethinking SIMD Vectorization for In-Memory Databases](https://15721.courses.cs.cmu.edu/spring2016/papers/p1493-polychroniou.pdf)
2. [vectorized hash table](https://15721.courses.cs.cmu.edu/spring2024/slides/06-vectorization.pdf)
3. [SIMD 入门文章](https://zhuanlan.zhihu.com/p/94649418)
4. [SIMD 指令手册](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html)