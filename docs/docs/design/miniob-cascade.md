---
title: Cascade Optimizer(查询优化器)
---

# Cascade Optimizer(查询优化器)

优化器（Optimizer）是数据库中用于把逻辑计划转换成物理执行计划的核心组件，很大程度上决定了一个系统的性能。查询优化的本质就是对于给定的查询，找到该查询对应的正确物理执行计划并且是最低的"代价(cost)"。

在 MiniOB 中，查询优化器目前将优化分为两个阶段（基于规则的优化和基于代价的优化）。本文主要介绍基于代价的查询优化器实现（Cascade Optimizer），其代码实现主要位于 `src/observer/optimizer/cascade`。

## 基本概念

### Operator（算子）
描述某个特定的代数运算，例如join，aggregation 等，operator 分为 logical operator/physical operator。logical operator描述算子的逻辑定义，与具体实现无关。physical operator描述算子的具体实现算法。例如对 join 来说，logical operator 就是 join operator；physical operator 有hash join operator/nested loop join operator等。
在 MiniOB 中，operator 对应的抽象类为 `OperatorNode`。

### Expression（表达式）
这里的Expression（表达式）表示一个带有零或多个Input Expression（输入表达式）的Operator，同样可以分为Logical Expression（逻辑表达式）和 Physical Expression（物理表达式）。
例如对于 TableScan 算子来说，TableScan 算子本身就可以理解为是一个 Expression（不带输入表达式）；对于 Join 算子来说，Join 算子以及其子节点一起构成一个 Expression。

MiniOB 当前的实现中为每个 `OperatorNode` 增加了一个成员变量 `vector<OperatorNode*> general_children_;`，可以认为这里的 Expression 也对应到 MiniOB 中的 `OperatorNode`。

### Group
Group 是一组逻辑等效的逻辑和物理表达式，产生相同的输出。
例如对于 `SELECT * FROM A JOIN B ON A.id = B.id JOIN C ON C.id = A.id;` 来说，其输出结果为 A，B，C 三个表根据 Join 条件进行 Join 的结果。逻辑表达式就可能包括 `(A Join B) Join C`，`A Join (B Join C)`；物理表达式就可能包括`(A Hash Join B) Nested Loop Join C`，`(A Hash Join B) Hash Join C`等。
在 MiniOB 中，Group 对应的实现位于 `src/observer/sql/optimizer/cascade/group.h`。

### M_EXPR
M_EXPR 是 Expression（表达式） 的一种紧凑形式。 输入是 Group 而不是 Expression。因此可以理解为， M_EXPR 包含多个 Expression。在 Cascade 中，所有搜索都是通过 M_EXPR 完成的。
在 MiniOB 中，M_EXPR 对应的实现位于 `src/observer/sql/optimizer/cascade/group_expr.h`。

### Rule（规则）
规则是将 Expression 转换为逻辑等价表达式。包含了Transformation Rule 和 Implementation Rule。其中，Transformation Rule 是将逻辑表达式转换为逻辑表达式；Implementation Rule 是将一个逻辑表达式转换为物理表达式。
在 MiniOB 中，Rule 对应的实现位于 `src/observer/sql/optimizer/cascade/rules.h`，目前 MiniOB 只实现了 Implementation Rule。

### Memo（在 Columbia 中也叫做 Search Space）
Memo 用于存放查询优化过程中所有枚举的计划的数据结构，利用 Memo 来避免生成重复的计划生成。
在 MiniOB 中，Memo 对应的实现位于 `src/observer/sql/optimizer/cascade/memo.h`。


## 搜索策略

在 Cascade 中，优化算法类似于DFS。我们有一个 task stack，每次循环都会从 task stack 中取一个 task 执行，每次执行可能导致新的任务被压入栈。这样不停循环直到栈为空，这样我们从根节点开始就能找到一个最优的物理计划。

其整体流程如下：
```
while task_list is not empty
  pop task
  perform task
```

在cascade（columbia）中，task共分为5种， 其整体调度的流程图如下：
```
       optimize()                         
           │                              
           │                              
           ▼                              
    ┌─────────────┐        ┌─────────────┐
    │             │ ─────► │             │
    │   O_GROUP   │        │  O_INPUTS   │
    │             │ ◄────  │             │
    └──────┬──────┘        └─────────────┘
           │                      ▲       
           ▼                      │       
    ┌─────────────┐        ┌──────┴──────┐
    │             │ ─────► │             │
    │   O_EXPR    │        │ APPLY_RULE  │
    │             │ ◄───── │             │
    └─────┬───────┘        └─────────────┘
          │ ▲                             
          ▼ │                             
    ┌───────┴─────┐                       
    │             │                       
    │   E_GROUP   │                       
    │             │                       
    └─────────────┘    
```
* O_GROUP：根据优化目标，对一个 Group 进行优化，即遍历 Group 中的每个 M_EXPR，为logical M_EXPR 生成一个 O_EXPR task，为 physical M_EXPR 生成一个 O_INPUTS task。

* O_EXPR：对一个logical M_EXPR 进行优化，对于该 logical M_EXPR，如果存在可以应用的规则，则生成一个 APPLY_RULE task。

* E_GROUP：为 Group 中的每一个 logical M_EXPR 生成一个 O_EXPR task。

* APPLY_RULE：应用具体的优化规则，从逻辑表达式转换到逻辑表达式，或者从逻辑表达式转换为物理表达式。如果产生逻辑表达式，则生成一个 O_EXPR task；如果产生物理表达式，则生成一个 O_INPUTS task。

* O_INPUTS：对物理表达式的代价进行计算，在此过程中需要递归地计算子节点的代价。

在 MiniOB 中，5 种类型的 task 位于 `src/observer/sql/optimizer/cascade/tasks` 目录下，
Cascade Optimizer 的入口函数为 `src/observer/sql/optimizer/cascade/optimizer.h::Optimizer::optimize`。

## 如何为 Cascade 添加新的算子转换规则

1. 添加逻辑算子和物理算子的定义，可参考`src/observer/sql/operator/table_get_logical_operator.h` 和 `src/observer/sql/operator/table_scan_physical_operator.h`
2. 添加逻辑算子到物理算子的转换规则，可参考`src/observer/sql/optimizer/implementation_rules.h::LogicalGetToPhysicalSeqScan`
3. 在 `src/observer/sql/optimizer/rules.h` 中的 `RuleSet` 中注册相应的转换规则。

## WIP
1. 将现有的基于规则的逻辑计划到逻辑计划的转换加入到 cascade optimizer 中。
2. 实现 Apply Rule 中的 Expr binding。
3. 实现 property enforce。
4. 统计信息收集更多信息。
