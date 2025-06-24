---
title: LAB#4 性能测试
---

> 请不要将代码提交到公开仓库（包括提交带有题解的 Pull Request），同时也请不要抄袭其他同学或网络上可能存在的代码。

# LAB#4 性能测试

在数据库系统实现原理与实践课程的前三个实验中，你已经完成了数据库系统中的存储引擎，查询引擎，事务引擎等各个组件的实现。本课程的最后一个实验为 TPC-C 测试。你需要根据文档的指导，首先完成 TPC-C 测试所必须的功能，预计在完成这些功能后，就可以直接通过 TPC-C 测试。你可以在此基础上继续优化 MiniOB 的性能以获得更高的 TPC-C 测试性能。

注意：TPC-C 性能测试的成绩**不计入**到本实验的成绩中。

## 通过 TPC-C 测试所需的前置功能

* 实现聚合函数（Min/Max/Count/Sum）。支持如下类型的语法：
```sql
SELECT min(NO_O_ID) FROM NEW_ORDER WHERE NO_D_ID = %s AND NO_W_ID = %s
```

* 支持 update/delete 语句。

**提示** 当前 MiniOB 已基本支持 delete 语句的语法解析/执行，只需要适配 LSM-Tree 存储引擎即可。
**提示** update 语句的实现可参考现有的 insert/delete 语句的实现。需要支持如下类型的 update 语法

```sql
UPDATE CUSTOMER SET C_BALANCE = C_BALANCE + %s WHERE C_ID = %s AND C_D_ID = %s AND C_W_ID = %s;
```

## 如何在 MiniOB 上运行 TPCC 测试

## 准备环境
**安装 Python 2.x**
目前只支持 Python 2.7 或 Python 2.6，因此需要先安装 Python 2.x

**安装相关依赖**
需要安装 pymysql 0.9.3，例如在 centos 环境下，可以执行以下命令安装：
```
sudo pip2 install pymysql==0.9.3 --trusted-host pypi.python.org --trusted-host pypi.org --trusted-host files.pythonhosted.org
```

**如何在 Ubuntu24.04 中安装python2/pip2**

由于 python2/pip2 已经停止维护，这里提供一种在 Ubuntu 24.04（目前 Docker 镜像使用 Ubuntu 24.04）从源码编译安装 python2/pip2 的方法，仅供参考：
```
# 安装依赖
sudo apt install build-essential zlib1g-dev libncurses5-dev libgdbm-dev libnss3-dev libssl-dev libreadline-dev libffi-dev wget

# 下载 python2 源码并编译安装
wget https://mirrors.aliyun.com/python-release/source/Python-2.7.18.tgz && tar xzf Python-2.7.18.tgz && cd Python-2.7.18 && ./configure --enable-optimizations --with-openssl &&  make altinstall && ln -sfn '/usr/local/bin/python2.7' '/usr/bin/python2'

# 安装pip2
git clone https://github.com/pypa/get-pip && cd get-pip && python2 public/2.7/get-pip.py
```

## 运行测试

* 准备 config 文件，例如:
```
[miniob]

# The path to the unix socket
unix_socket          = /tmp/miniob.sock

```

* 启动 MiniOB，例如：
```
./bin/observer -E lsm -t lsm -P mysql -s /tmp/miniob.sock -f ../etc/observer.ini
```

* 运行 tpcc 测试，例如：
```
python ./tpcc.py --config=miniob.config  miniob  --debug --clients 4
```

注意：为了便于在资源受限的测试环境中运行 TPCC 测试，同时受限于 MiniOB 当前的性能，此版本简化了某些测试参数。

## TPC-C 性能优化提示
* 利用主键索引。
例如，对于如下的查询语句，就可以直接通过主键索引查找到对应的记录(避免全表扫描)，且主键索引保证最多只有一行记录。
```
SELECT W_TAX FROM WAREHOUSE WHERE W_ID = %s;
```

* LSM-Tree 优化。
例如，可以合理调整 LSM-Tree 的参数设置，优化 Block Cache，使用 BloomFilter，优化合并策略等等。

* 利用 perf 等工具观察性能瓶颈，进行针对性优化。

可参考：https://www.brendangregg.com/FlameGraphs/cpuflamegraphs.html

**提示** 对于存在主键索引的表，表中不能有重复主键的数据。

**注意**：线上测试环境为 1c1g，因此并发测试性能可能与本地环境存在一些出入，线上测试成绩仅供参考，不作为课程成绩的一部分。


## TPC-C 测试表结构

```
create table warehouse (w_id int, w_name char(10), w_street_1 char(20), w_street_2 char(20), w_city char(20), w_state char(2), w_zip char(9), w_tax float, w_ytd float, primary key(w_id));

create table district (d_id int, d_w_id int, d_name char(10), d_street_1 char(20), d_street_2 char(20), d_city char(20), d_state char(2), d_zip char(9), d_tax float, d_ytd float, d_next_o_id int, primary key(d_w_id, d_id));

create table customer (c_id int, c_d_id int, c_w_id int, c_first char(16), c_middle char(2), c_last char(16), c_street_1 char(20), c_street_2 char(20), c_city char(20), c_state char(2), c_zip char(9), c_phone char(16), c_since char(30), c_credit char(2), c_credit_lim int, c_discount float, c_balance float, c_ytd_payment float, c_payment_cnt int, c_delivery_cnt int, c_data char(50), primary key(c_w_id, c_d_id, c_id));

create table history (h_c_id int, h_c_d_id int, h_c_w_id int, h_d_id int, h_w_id int, h_date datetime, h_amount float, h_data char(24), primary key(h_c_id, h_c_d_id, h_c_w_id, h_d_id, h_w_id));

create table new_orders (no_o_id int, no_d_id int, no_w_id int, primary key(no_w_id, no_d_id, no_o_id));

create table orders (o_id int, o_d_id int, o_w_id int, o_c_id int, o_entry_d datetime, o_carrier_id int, o_ol_cnt int, o_all_local int, primary key(o_w_id, o_d_id, o_id));

create table order_line ( ol_o_id int, ol_d_id int, ol_w_id int, ol_number int, ol_i_id int, ol_supply_w_id int, ol_delivery_d char(30), ol_quantity int, ol_amount float, ol_dist_info char(24), primary key(ol_w_id, ol_d_id, ol_o_id, ol_number));

create table item (i_id int, i_im_id int, i_name char(24), i_price float, i_data char(50), primary key(i_id));

create table stock (s_i_id int, s_w_id int, s_quantity int, s_dist_01 char(24), s_dist_02 char(24), s_dist_03 char(24), s_dist_04 char(24), s_dist_05 char(24), s_dist_06 char(24), s_dist_07 char(24), s_dist_08 char(24), s_dist_09 char(24), s_dist_10 char(24), s_ytd float, s_order_cnt int, s_remote_cnt int, s_data char(50), primary key(s_w_id, s_i_id));
```

## 参考资料

TPC-C 测试：https://www.tpc.org/tpcc/

TPC-C 测试脚本：https://github.com/oceanbase/py-tpcc/tree/miniob
