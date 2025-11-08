全文索引（Full-Text Search）是一种用于在大量文本数据中进行高效搜索的技术。它通过基于相似度的查询，而不是精确数值比较，来查找文本中的相关信息。相比于使用 LIKE + % 的模糊匹配，全文索引在处理大量数据时速度更快。

简化流程如下:

#### **文本预处理**

- **分词（Tokenization）**: 将文本数据拆分为单个的词语或短语，这些词语成为索引的基本单位。例如，“全文索引的原理”可能会被拆分为“全文”、“索引”、“原理”等词条。
- **去除停用词（Stop Words Removal）**: 停用词是指在搜索中不太有意义的常用词汇，如“的”、“是”等。去除这些词可以减少索引的规模，并提高搜索效率。

#### **倒排索引（Inverted Index）**

倒排索引是全文索引的核心数据结构。它通过记录每个词条在哪些文档中出现来实现快速查询。

- **词典（Dictionary）**: 保存所有出现过的词条，以及这些词条的文档频率。
- **倒排列表（Posting List）**: 对于每个词条，倒排列表保存了包含该词条的文档 ID，甚至可能包含词条在文档中出现的位置和频率等信息。

#### **查询处理**

- **排名和排序**: 全文索引系统通常会根据词频、文档长度、词条的逆文档频率（IDF）等因素对查询结果进行评分和排序，返回最相关的文档。

#### 功能要求:

- 支持使用 jieba 进行中文分词
- 支持创建全文索引，支持全文索引查询
- 支持使用 BM25 评分
- SQL 示例:

```sql
SELECT TOKENIZE('information_schema.SCHEMATA表的主要功能是什么？', 'jieba') as text_tokens;
["information", "schema", "SCHEMATA", "表", "主要", "功能"] // 结果之间存在空格

ALTER TABLE texts ADD FULLTEXT INDEX idx_texts_jieba (content) WITH PARSER jieba;
SELECT id, content, MATCH(content) AGAINST('你好') AS score FROM texts WHERE MATCH(content) AGAINST('你好') > 0 ORDER BY MATCH(content) AGAINST('你好') DESC, id ASC;
```

#### 注意:

- 本题只需实现使用 [cppjieba](https://github.com/yanyiwu/cppjieba) 库进行 jieba 分词。初始代码已经实现 cppjieba 的对接，提测能正常通过编译。
- 本题只需使用 BM25 评分进行排序，`MATCH(content) AGAINST` 返回结果为该文档的 BM25 评分。
- BM25 分数计算公式如下:

$$
\operatorname{score}(D,Q)=\sum_{i=1}^{n}\operatorname{IDF}(q_i)\cdot
\frac{\operatorname{TF}(q_i,D)\,(k_1+1)}
{\operatorname{TF}(q_i,D)+k_1\Bigl(1-b+b\cdot\frac{|D|}{\mathrm{avgdl}}\Bigr)}.
$$

其中 $k_1$ = 1.5, $b$ = 0.75。

- 本题只会对单列建全文索引，不考虑多列索引。
- 测评端代码标准参考:
  - 测评端 jieba 分词: [https://github.com/HuXin0817/cppjieba/](https://github.com/HuXin0817/cppjieba/)
  - BM25: [https://github.com/dorianbrown/rank_bm25/tree/0.2.2/](https://github.com/dorianbrown/rank_bm25/tree/0.2.2)
