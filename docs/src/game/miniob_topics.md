miniob 题目

# 背景

这里的题目是2021年OceanBase数据库大赛初赛时提供的赛题。这些赛题的入门门槛较低, 适合所有参赛选手。 面向的对象主要是在校学生，数据库爱好者, 或者对基础技术有一定兴趣的爱好者, 并且考题对诸多模块做了简化，比如不考虑并发操作, 事务比较简单。
初赛的目标是让不熟悉数据库设计和实现的同学能够快速的了解与深入学习数据库内核，期望通过miniob相关训练之后，能够对各个数据库内核模块的功能与它们之间的关联有所了解，并能够在使用时，设计出高效的SQL,  并帮助降低学习OceanBase 内核的学习门槛。

# 题目介绍

预选赛，题目分为两类，一类必做题，一类选做题。选做题按照实现的功能计分。

计分规则：必做题和选做题都有分数。但是必做题做完后，选做题的分数才会进行累计。

作为练习，这些题目在单个实现时，是比较简单的。因为除了必做题，题目之间都是单独测试的。希望你能够在实现多个功能时，考虑一下多个功能之间的关联。比如实现了多字段索引，可以考虑下多字段索引的唯一索引、根据索引查询数据等功能。另外，可以给自己提高一点难度。比如在实现表查询时，可以想象内存中无法容纳这么多数据，那么如何创建临时文件，以及如何分批次发送结果到客户端。

## 必做题

| 名称                          | 分值 | 描述                                                         | 测试用例示例                                                 |
| ----------------------------- | ---- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 优化buffer pool               | 10   | 必做。实现LRU淘汰算法或其它淘汰算法。<br/> 题目没有明确的测试方法。同学可以通过这个简单的题目学习disk_buffer_pool的工作原理。 |                                                              |
| 查询元数据校验<br>select-meta | 10   | 必做。查询语句中存在不存在的列名、表名等，需要返回失败。需要检查代码，判断是否需要返回错误的地方都返回错误了。 | create table t(id int, age int);<br/>select * from t where name='a'; <br/>select address from t where id=1;<br/>select * from t_1000;<br/>select * from t where not_exists_col=1; |
| drop table<br>drop-table      | 10   | 必做。删除表。清除表相关的资源。<br/>注意：要删除所有与表关联的数据，不仅仅是在create table时创建的资源，还包括索引等数据。 | create table t(id int, age int);<br/>create table t(id int, name char);<br/>drop table t;<br/>create table t(id int, name char); |
| 实现update功能<br>update      | 10   | 必做。update单个字段即可。<br/>可以参考insert_record和delete_record的实现。目前能支持update的语法解析，但是不能执行。需要考虑带条件查询的更新，和不带条件的更新。 | update t set age =100 where id=2;<br/>update set age=20 where id>100; |
| 增加date字段<br>date          | 10   | 必做。要求实现日期类型字段。date测试不会超过2038年2月，不会小于1970年1月1号。注意处理非法的date输入，需要返回FAILURE。<br/>当前已经支持了int、char、float类型，在此基础上实现date类型的字段。<br/>这道题目需要从词法解析开始，一直调整代码到执行阶段，还需要考虑DATE类型数据的存储。<br/>注意：<br/>- 需要考虑date字段作为索引时的处理，以及如何比较大小;<br/>- 这里限制了日期的范围，所以简化了溢出处理的逻辑，测试数据中也删除了溢出日期，比如没有 2040-01-02；<br/>- 需要考虑闰年。 | create table t(id int, birthday date);<br/>insert into t values(1, '2020-09-10');<br/>insert into t values(2, '2021-1-2');<br/>select * from t; |
| 多表查询<br>select-tables     | 10   | 必做。当前系统支持单表查询的功能，需要在此基础上支持多张表的笛卡尔积关联查询。需要实现select * from t1,t2; select t1.\*,t2.\* from t1,t2;以及select t1.id,t2.id from t1,t2;查询可能会带条件。查询结果展示格式参考单表查询。每一列必须带有表信息，比如:<br/>t1.id \|  t2.id <br/>1 \| 1 | select * from t1,t2;<br/>select * from t1,t2 where t1.id=t2.id and t1.age > 10;<br/>select * from t1,t2,t3; |
| 聚合运算<br>aggregation-func  | 10   | 实现聚合函数 max/min/count/avg.<br/>包含聚合字段时，只会出现聚合字段，不会出现如select id, count(age) from t;这样的测试语句。聚合函数中的参数不会是表达式，比如age +1。 | select max(age) from t1; <br/>select count(*) from t1; <br/>select count(1) from t1; <br/>select count(id) from t1; |



## 选做题

| 名称                            | 分值 | 描述                                                         | 测试用例示例                                                 |
| ------------------------------- | ---- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 多表join操作<br>join-tables     | 20   | INNER JOIN。需要支持join多张表。主要工作是语法扩展。注意带有多条on条件的join操作。 | select * from t1 inner join t2 on t1.id=t2.id;<br/>select * from t1 inner join t2 on t1.id=t2.id inner join t3 on t1.id=t3.id;<br/>selec * from t1 inner join t2 on t1.id=t2.id and t2.age>10 where t1.name >='a'; |
| 一次插入多条数据<br>insert      | 10   | 单条插入语句插入多行数据。一次插入的数据要同时成功或失败。   | insert into t1 values(1,1),(2,2),(3,3);                      |
| 唯一索引<br>unique              | 10   | 唯一索引：create unique index。                              | create unique index i_id on t1(id);<br/>insert into t1 values(1,1);<br/>insert into t1 values(1,2); -- failed |
| 支持NULL类型<br>null            | 10   | 字段支持NULL值。包括但不限于建表、查询和插入。默认情况不允许为NULL，使用nullable关键字表示字段允许为NULL。<br/>Null不区分大小写。<br/>注意NULL字段的对比规则是NULL与**任何** 数据对比，都是FALSE。<br/>如果实现了NULL，需要调整聚合函数的实现。 | create table t1 (id int not null, age int not null, address nullable); <br/>create table t1 (id int, age int, address char nullable);<br/> insert into t1 values(1,1, null); |
| 简单子查询<br>simple-sub-query  | 10   | 支持简单的IN(NOT IN)语句；<br/>支持与子查询结果做比较运算；<br/>支持子查询中带聚合函数。<br/>子查询中不会与主查询做关联。 | select * from t1 where name in(select name from t2);<br/>select * from t1 where t1.age >(select max(t2.age) from t2);<br/>select * from t1 where t1.age > (select avg(t2.age) from t2) and t1.age > 20.0; <br/>NOTE: 表达式中可能存在不同类型值比较 |
| 多列索引<br>multi-index         | 20   | 多个字段关联起来称为单个索引。<br/>                          | create index i_id on t1(id, age);                            |
| 超长字段<br>text                | 20   | 超长字段的长度可能超出一页，比如常见的text,blob等。这里仅要求实现text（text 长度固定4096字节），可以当做字符串实现。<br/>注意：当前的查询，只能支持一次返回少量数据，需要扩展<br/>如果输入的字符串长度，超过4096，那么应该保存4096字节，剩余的数据截断。<br/>需要调整record_manager的实现。当前record_manager是按照定长长度来管理页面的。 | create table t(id int, age int, info text);<br/>insert into t values(1,1, 'a very very long string');<br/>select * from t where id=1; |
| 查询支持表达式<br>expression    | 20   | 查询中支持运算表达式，这里的运算表达式包括 +-*/。<br/>仅支持基本数据的运算即可，不对date字段做考察。<br/>运算出现异常，按照NULL规则处理。<br/>只需要考虑select。 | select * from t1,t2 where t1.age +10 > t2.age *2 + 3-(t1.age +10)/3;<br/>select t1.col1+t2.col2 from t1,t2 where t1.age +10 > t2.age *2 + 3-(t1.age +10)/3; |
| 复杂子查询<br>complex-sub-query | 20   | 子查询在WHERE条件中，子查询语句支持多张表与AND条件表达式，查询条件支持max/min等。<br>注意考虑一下子查询与父表相关联的情况。 | select * from t1 where age in (select id from t2 where t2.name in (select name from t3)) |
| 排序<br>order-by                | 10   | 支持oder by功能。不指定排序顺序默认为升序(asc)。<br/>不需要支持oder by字段为数字的情况，比如select * from t order by 1; | select * from t,t1 where t.id=t1.id order by t.id asc,t1.score desc; |
| 分组<br>group-by                | 20   | 支持group by功能。group by中的聚合函数也不要求支持表达式     | select t.id, t.name, avg(t.score),avg(t2.age) from t,t2 where t.id=t2.id group by t.id,t.name; |

# 测试常见问题
## 测试Case

### 优化buffer pool



题目中要求实现一个LRU算法。但是LRU算法有很多种，所以大家可以按照自己的想法来实现。因为不具备统一性，所以不做统一测试。

另外，作为练习，除了实现LRU算法之外，还可以考虑对buffer pool做进一步的优化。比如支持更多的页面或无限多的页面（当前buffer pool的实现只能支持固定个数的页面）、支持快速的查找页面（当前的页面查找算法复杂度是O(N)的）等。



### basic 测试

基础测试是隐藏的测试case，是代码本身就有的功能，比如创建表、插入数据等。如果选手把原生仓库代码提交上去，就能够测试通过basic。做其它题目时，可能会影响到basic测试用例，需要注意。

### select-meta 测试

这个测试对应了“元数据校验”。选手们应该先做这个case。

常见测试失败场景有一个是 where 条件校验时 server core了。

注意，训练营和此处关于 select-meta 题目的描述是过时的，错误地沿用了2021 select-meta 题目的描述。2022 select-meta 赛题的要求为：为 select 查询实现类似 **SELECT \*, attrbutes FROM RELATIONS [WHERE CONDITIONS]** 的功能。

### drop-table case测试

目前遇到最多的失败情况是没有校验元数据，比如表删除后，再执行select，按照“元数据校验”规则，应该返回"FAILURE"。

### date 测试

date测试需要注意校验日期有效性。比如输入"2021-2-31"，一个非法的日期，应该返回"FAILURE"。

date不需要考虑和string(char)做对比。比如 select * from t where d > '123'; select * from t where d < 'abc'; 不会测试这种场景。但是需要考虑日期与日期的比较，比如select * from t where d > '2021-01-21';。

date也不会用来计算平均值。

select * form t where d=’2021-02-30‘； 这种场景在mysql下面是返回空数据集，但是我们现在约定都返回 FAILURE。

> 温馨提示：date 可以使用整数存储，简化处理

### 浮点数展示问题

按照输出要求，浮点数最多保留两位小数，并且去掉多余的0。目前没有直接的接口能够输出这种格式。比如 printf("%.2f", f); 会输出 1.00，printf("%g", f); 虽然会删除多余的0，但是数据比较大或者小数位比较多时展示结果也不符合要求。



### 浮点数与整数转换问题

比如 create table t(a int, b float); 在当前的实现代码中，是不支持insert into t values(1,1); 这种做法的，因为1是整数，而字段`b`是浮点数。那么，我们在比赛中，也不需要考虑这两种转换。

但是有一种例外情况，比如聚合函数运算：`select avg(a) from t;`,需要考虑整数运算出来结果，是一个浮点数。

### update 测试

update 也要考虑元数据校验，比如更新不存在的表、更新不存在的字段等。

需要考虑不能转换的数据类型更新，比如用字符串更新整型字段。

对于整数与浮点数之间的转换，不做考察。学有余力的同学，可以做一下。

更新需要考虑的几个场景，如果这个case没有过，可以对比一下：

假设存在这个表：

create table t (id int, name char, col1 int, col2 int);

表上有个索引

create index i_id on t (id);

-- 单行更新

update t set name='abc' where id=1;

-- 多行更新

update t set name='a' where col1>2; -- 假设where条件能查出来多条数据

-- 更新索引

update t set id=4 where name='c';

-- 全表更新

update t set col1=100;

-- where 条件有多个

update t set name='abc' where col1=0 and col2=0;

一些异常场景：

- 更新不存在的表
- 更新不存在的字段
- 查询条件中包含不合法的字段
- 查询条件查出来的数据集合是空（应该什么都不做，返回成功）
- 使用无法转换的类型更新某个字段，比如使用字符串更新整型字段





### 多表查询

多表查询的输入SQL，只要是字段，都会带表名。比如不会存在 select id from t1,t2;

不带字段名称的场景（会测试）：select * from t1,t2;

带字段：select t1.id, t1.age, t2.name from t1,t2 where t1.id=t2.id;

或者：select t1.* ,  t2.name from t1,t2 where t1.id=t2.id;

多表查询，查询出来单个字段时，也需要加上表名字。原始代码中，会把表名给删除掉。比如select t1.id from t1,t2; 应该输出列名： t1.id。这里需要调整原始代码。输出列名的规则是：单表查询不带表名，多表查询带表名。

不要仅仅使用最简单的笛卡尔积运算，否则可能会内存不足。



### 聚合运算

不需要考虑聚合字段与普通字段同时出现的场景。比如： select id, count(1) from t1;

必做题中的聚合运算只需要考虑单张表的情况。

字符串可以不考虑AVG运算。

最少需要考虑的场景： 

假设有一张表 create table t(id int, name char, price float);

select count(*) from t;

select count(id) from t;

select min(id) from t;

select min(name) from t;  -- 字符串

select max(id) from t;  

select max(name) from t;

select avg(id) from t;  -- 整数做AVG运算，输出可能是浮点数，所以要注意浮点数输出格式

select avg(price) from t;

select avg(price), max(id), max(name) from t;

还需要考虑一些异常场景：

select count(*,id) from t;

select count() from t;

select count(not_exists_col) from t;



### 支持NULL类型

NULL的测试case描述的太过简单，这里做一下补充说明。
NULL的功能在设计时，参考了mariadb的做法。包括NULL的比较规则：`任何` 值与NULL做对比，结果都是FALSE。

因为miniob的特殊性，字段默认都是不能作为NULL的，所以这个测试用例中，要求增加关键字`nullable`，表示字段可以是NULL。

需要考虑的场景
- 建表
create table t(id int, num int nullable, birthday date nullable);
表示创建一个表t,字段num和birthday可以是NULL, 而id不能是NULL。

建索引
create index i_num on t(num);
支持在可以为NULL的字段上建索引

需要支持增删改查
```sql
insert into t values(1, 2, '2020-01-01');
insert into t values(1, null, null);
insert into t values(1, null, '2020-02-02'); -- 同学们自己多考虑几种场景
insert into t values(null, 1, '2020-01-02'); -- 应该返回FAILURE，因为ID不能是NULL
```

```sql
select * from t; -- 全表遍历
-- null 条件查询，同学们自己多测试几种场景

select * from t where id is null;
select * from t where id is not null;
select * from t where num is null; 
select * from t where num > null;
select * from t where num <> null;
select * from t where 1=null;
select * from t where 'a'=null;
select * from t where null = null;
select * from t where null is null; -- 注意 = 与 is 的区别
select * from t where '2020-01-31' is null;
```

> 不要忘记多表查询

聚合

```sql
select count(*) from t;
select count(num) from t;
select avg(num) from t;
```

> 字段值是NULL时，比较特殊，不需要统计在内。如果是AVG，不会增加统计行数，也不需要默认值。


### inner-join 

inner-join 与 多表查询类似，很多同学做完多表查询就开始做inner-join了。
inner-join出现非常多的一个问题就是下面的语句，返回了空数据，或者没有任何返回，可能是测试时程序coredump，或者长时间没有返回结果，比如死循环。测试语句是：

```sql
select * from join_table_large_1 inner join join_table_large_2 on join_table_large_1.id=join_table_large_2.id inner join join_table_large_3 on join_table_large_1.id=join_table_large_3.id inner join join_table_large_4 on join_table_large_3.id=join_table_large_4.id inner join join_table_large_5 on 1=1 inner join join_table_large_6 on join_table_large_5.id=join_table_large_6.id where join_table_large_3.num3 <10 and join_table_large_5.num5>90;
```

### 表达式
表达式需要考虑整数和浮点数的比较。比如 t.id > 1.1 或者 5/4 = 1等。
