---
title: MiniOB 向量数据库 - 2025 版
---

# MiniOB 向量数据库

## 向量搜索

向量搜索技术是非结构化数据检索的关键技术之一，通常通过近似最近邻搜索（Approximate Nearest Neighbor Search, ANN）的方式来在高维空间中进行检索，以此来找到满足要求的数据。向量搜索在检索相似的图片、音频和文本等方面发挥着关键作用。例如，在图像检索中，我们可以通过计算图像的特征向量，然后使用向量检索技术来找到与查询图像最相似的图像；在推荐系统中，我们可以通过计算用户和物品的特征向量，然后使用向量检索技术来找到与用户兴趣最相似的其他用户或物品。下面针对向量搜索技术的基本概念进行简要介绍。

### 向量（Vector）是什么

向量是一种表示多维特征的数据结构。每个向量由一组数值组成，这些数值通常对应于某种特定的特征或属性。例如，在图像处理中，一个向量可以表示图像的颜色、纹理等特征。为了更有效地管理非结构化数据，常见的做法是将其转换为向量表示，并存储在向量数据库中，这种转换过程通常被称为 Embedding。通过将文本、图像或其他非结构化数据映射到高维向量空间，我们可以捕捉数据的语义特征和潜在关系。单词、短语或整个文档以及图像、音频和其他类型的数据都可以表示成向量，例如，我们可以将下面的文本表示成向量：

```
"West Highland White Terrier": [0.0296700,0.0231020,0.0166550,0.0642470,-0.0110980, ... ,0.0253750]
```

### 向量数据库是什么

向量数据库是可以高效存储/检索向量的数据库。向量数据库用专门的数据结构和算法来处理向量之间的相似性计算和查询。通过构建索引结构，向量数据库可以快速找到最相似的向量。

### 向量搜索的应用

向量在检索相似的图片、音频和文本等方面发挥着关键作用，这源于其数据属性和特征表示能力。在机器学习和数据科学领域，向量被广泛用于描述数据特征。以图片数据为例，我们可以将其表示为向量。在计算机中，图片本质上是由像素构成的二维矩阵。每个像素的亮度值可视为图片的一个特征，因此，我们可以将这些亮度值串联成一个高维向量，从而实现图片的向量化表示。这种向量化表示使我们能够利用向量空间中的距离和相似度度量方法来比较不同图片之间的相似程度。例如，欧氏距离可用于衡量两个图片向量间的像素差异，而余弦相似度则可测量它们的方向差异。通过计算向量间的距离或相似度，我们可以量化评估不同图片之间的相似程度。

除此之外，检索增强生成（Retrieval-Augmented Generation）已经成为大语言模型应用的范式之一。而向量数据库是 RAG 应用中重要组成部分，向量数据库的存储容量、召回精度和召回速度在很大程度上影响了大语言模型应用的服务质量。

## MiniOB 向量数据库赛题

本次赛题，需要选手在 MiniOB 的基础上实现向量数据库的基本功能，向量数据库的功能被拆解为如下几个题目。

注意：在实现向量数据库相关题目时，不限制向量检索算法的实现方式，可以基于开源的第三方库实现，也可以自行实现。

### MiniOB 向量类型

VECTOR 是一种结构，最多可以容纳指定数量的条目 N，定义如下：

```sql
VECTOR(N)
```

每个条目是一个 4 字节（单精度）浮点数值。

默认长度为 2048；最大条目数为 16383。要声明一个使用默认长度的 VECTOR 列，应定义为 `VECTOR` 而不带括号；尝试以 `VECTOR()`（空括号）方式定义列将引发语法错误。

VECTOR 不能与任何其他类型进行比较。它可以与其他 VECTOR 进行相等性比较，但不支持其他比较操作。

- 支持创建包含向量类型的表：

```sql
CREATE TABLE items (id int, embedding vector(3));
```

- 支持插入向量类型的记录（注意：这里需要支持 `STRING_TO_VECTOR` 函数将字符串类型的值转换为向量类型存储）：

```sql
INSERT INTO items VALUES (1, STRING_TO_VECTOR('[1,2,3]'));
```

- 支持用于处理 VECTOR 值的 SQL 函数。

| 名称               | 描述                                             |
| ------------------ | ------------------------------------------------ |
| DISTANCE()         | 根据指定的方法计算两个向量之间的距离             |
| STRING_TO_VECTOR() | 获取由符合格式的字符串表示的 VECTOR 列的二进制值 |
| VECTOR_TO_STRING() | 获取 VECTOR 列的字符串表示，给定其二进制值       |

**DISTANCE(vector, vector, string)**

计算两个向量之间的距离，根据指定的计算方法。它接受以下参数：

- 一个 VECTOR 数据类型的列。
- 一个 VECTOR 数据类型的输入查询。
- 一个字符串，指定了距离度量方式。支持的值有 COSINE、DOT 和 EUCLIDEAN。由于该参数是字符串，因此必须加引号。

  - l2_distance

    - 语法：l2_distance(vector A, vector B)
    - 计算公式：$[ D = \sqrt{\sum_{i=1}^{n} (A_{i} - B_{i})^2} ]$

  - cosine_distance：

    - 语法：cosine_distance(vector A, vector B)
    - 计算公式：$[ D = 1 - \frac{\mathbf{A} \cdot \mathbf{B}}{|\mathbf{A}| |\mathbf{B}|} = 1 - \frac{\sum_{i=1}^{n} A_i B_i}{\sqrt{\sum_{i=1}^{n} A_i^2} \sqrt{\sum_{i=1}^{n} B_i^2}} ]$

  - inner_product：
    - 语法：inner_product(vector A, vector B)
    - 计算公式：$[ D = \mathbf{A} \cdot \mathbf{B} = a_1 b_1 + a_2 b_2 + ... + a_n b_n = \sum_{i=1}^{n} a_i b_i ]$

**VECTOR_DISTANCE 是此函数的同义词。**

```sql
SELECT DISTANCE(STRING_TO_VECTOR("[1.01231, 2.0123123, 3.0123123, 4.01231231]"), STRING_TO_VECTOR("[1, 2, 3, 4]"), "COSINE");
```

---

**STRING_TO_VECTOR(string)**

将向量的字符串表示转换为二进制形式。字符串的预期格式是一个或多个逗号分隔的浮点数值列表，用方括号（[ ]）包围。值可以用十进制或科学记数法表示。由于该参数是字符串，因此必须加引号。

**TO_VECTOR() 是此函数的同义词。**

VECTOR_TO_STRING() 是此函数的逆操作：

```sql
SELECT VECTOR_TO_STRING(STRING_TO_VECTOR("[1.05, -17.8, 32]"));
```

此类值中的所有空白字符（数字后、方括号前或后，或两者的任意组合）在使用时都会被修剪。

---

**VECTOR_TO_STRING(vector)**

给定一个 VECTOR 列值的二进制表示，此函数返回其字符串表示，该格式与 STRING_TO_VECTOR() 函数的参数格式相同。

**FROM_VECTOR() 被接受为此函数的同义词。**

无法解析为向量值的参数会引发错误。

此函数的输出最大大小为 262128（16 \* 16383）字节。

## 参考资料

[向量数据库](https://en.wikipedia.org/wiki/Vector_database)

[MySQL 向量类型介绍](https://dev.mysql.com/doc/refman/9.4/en/vector.html)

[MySQL 向量函数介绍](https://dev.mysql.com/doc/refman/9.4/en/vector-functions.html)

[ivfflat 原理介绍](https://www.timescale.com/blog/nearest-neighbor-indexes-what-are-ivfflat-indexes-in-pgvector-and-how-do-they-work/)

[支持 MiniOB 的 ann-benchmarks fork 仓库](https://github.com/oceanbase/ann-benchmarks/tree/miniob)
