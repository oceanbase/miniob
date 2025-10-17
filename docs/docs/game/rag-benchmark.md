---
title: 2025 初赛 RAG Benchmark 题目
---

# RAG Benchmark

## 题目背景

### RAG 是什么

RAG（检索增强生成）是一种结合了 信息检索 和 生成式 AI 的混合技术。它通过从外部数据源（如数据库或文档）检索相关内容,将其输入到大型语言模型（LLM）中,使得生成的结果更加准确、有针对性和上下文相关。传统生成式 AI（如纯 LLM）可能会产生"幻觉"（hallucination）,即输出并不准确或无根据的回答。RAG 技术利用检索的真实数据来提供事实依据,从而减少错误。

### RAG 实现原理

RAG 的实现通常包含以下几个关键步骤:

#### 文档拆分 （Chunk）

文档拆分是一种将大规模文档拆分为语义连贯、结构清晰的独立单元（即分段,Chunk）的技术。在 RAG（检索增强生成）系统中,分段通过平衡上下文保留与检索效率,优化大模型对海量知识库的语义理解与信息召回能力。

- 提升检索效率: 通过将长文本切分为适配嵌入模型输入长度的分段（如 256-512 个 Token）,减少向量化与相似度计算的开销,使语义检索速度提升 3-5 倍
- 优化语义完整性: 采用递归分块、语义分块等策略,依据自然段落或主题边界切分文档,避免关键信息截断。例如,在技术文档中按章节划分,可保持“函数定义 → 参数说明 → 示例代码”的完整逻辑链
- 适配多模态输入限制: 大模型对输入长度有严格限制,分段技术可将超长文档压缩为符合上下文窗口的语义单元,确保生成内容的连贯性与准确性
- 降低计算成本: 细粒度分段减少冗余数据处理,在实时场景中,能有效降低检索延迟,同时减少 GPU 内存占用。

#### 向量化嵌入（Embedding）

Embedding（嵌入向量） 是通过深度学习神经网络提取内容和语义,并生成的高维稠密向量。在 RAG 应用中,它是将非结构化数据（如文本、图片等）转换为机器可处理的数值形式的关键技术。

- 数学意义: 一个稠密浮点数组,每个元素表示数据的某个维度。
- 降维和语义化: 可以从稀疏的原始数据（如文本词频矩阵）映射到稠密的低维空间,同时保留语义信息。
- 作用: 将文档、问题等转化为向量,便于向量检索功能处理。相似内容的向量距离更近。

#### 向量检索

向量检索 是一种通过数学距离来查找和匹配高维向量的技术,常用于语义相似性和推荐系统。

- 语义理解: 不仅支持传统关键字搜索,更擅长语义相似性匹配（例如寻找含义类似的两个问题）。
- 高性能: 向量数据库提供了向量检索能力,即使是大规模向量数据也能快速查找。
- 应用场景: 帮助实现聊天机器人、推荐系统、智能搜索。
- 开发者可以将文档内容生成的向量（Embedding）存储在向量数据库中,后续通过向量检索 API 查找和匹配最相关的内容。更多内容参见向量检索。

### Langflow 介绍

Langflow 是一个开源的、可视化的 LangChain 工作流构建平台。它提供了一个直观的图形界面,让用户可以通过拖拽组件的方式来构建复杂的 AI 应用流程,而无需编写复杂的代码。

在本次比赛中,Langflow 作为工作流编排平台,帮助选手快速构建 RAG 系统,专注于核心的数据库检索逻辑实现。

## 题目介绍

我们设计了以 miniob 为向量数据库,langflow 为工作流的 RAG 流程。选手需要完善自己的工作流,在工作流中调用选手自己的 miniob 数据库来完成此题目。

### 测评流程

- 上传代码: 选手需要将自己的工作流导出为 json 格式,放在个人 miniob 仓库下,位置为项目根目录下的 `./rag/model.json` (注意文件名必须为 `model.json`)
- 模型导入: 将选手的 model.json 上传到测评的 langflow 平台
- 启动 miniob: 启动选手的 miniob 数据库
- 测评: 通过 HTTP API,运行选手的工作流并获得测评结果
- 评分: 通过测评结果计算召回率

### 数据集

测评的知识库来源于 [OceanBase 官方文档](https://github.com/oceanbase/oceanbase-doc),部分的测评数据如下:

```json
[
  {
    "question": "在MySQL模式和Oracle模式下,集群名、租户名和用户名的最大长度分别是多少？",
    "support_facts": ["标识符长度限制"]
  },
  {
    "question": "ODP连接数据库时有哪些限制？",
    "support_facts": ["ODP(OceanBase Database Proxy) 连接限制"]
  },
  {
    "question": "单个表的行长度、列数、索引个数的最大限制是多少？",
    "support_facts": ["单个表的限制"]
  },
  {
    "question": "在MySQL模式和Oracle模式下,`VARCHAR`类型的最大长度分别是多少？",
    "support_facts": ["字符串类型限制"]
  },
  {
    "question": "物理备库的使用有哪些具体限制？",
    "support_facts": ["功能使用限制"]
  }
]
```

### 评分规则

#### 召回率计算

通过问题,答案,以及数据集的 `support_facts` 获得召回率,计算公式:

```python
score = sum(1 for support_fact in support_facts if support_fact in answer) / len(support_facts)
```

#### 通过条件

我们会从数据集中选取 30 个问题,并计算平均召回率 (平均召回率 = 所有题目的召回率总和 / 题目数量),若平均召回率高于 70%,则通过此题目

### 测评环境变量介绍

- `OB_DOC_PATH`: OceanBase 官方文档目录地址,测评使用的是 https://github.com/oceanbase/oceanbase-doc/tree/V4.3.5/zh-CN 下的文档
- `EMBEDDING_NAME`: 词嵌入模型名,测评使用的模型是 bge-m3
- `EMBEDDING_BASE_URL`: 词嵌入模型地址
- `LLM_NAME`: 大语言模型名,
- `LLM_API_KEY`: 大语言模型 Api Key
- `LLM_BASE_URL`: 大语言模型地址
- `QA_SERVER_GET_QUESTION_URL`: 测评机生成问题地址
- `QA_SERVER_POST_ANSWER_URL`: 测评机发送答案地址
- `MINIOB_SERVER_SOCKET`: miniob socket 文件地址

### 初始工作流

为了方便大家更快上手,我们提供了一个初始的 langflow 工作流 json 文件,包含了与测评机器做答案交互,调用大模型等繁琐 IO 操作。同时也提供了一些必要的组件,内容如下 (从左到右,从上到下一次介绍):

- `Directory`: 负责知识库的导入,通过测评环境变量 `OB_DOC_PATH` 获得 OceanBase 官方文档目录地址,来加载 OceanBase 介绍文档
- `Split Text`: 文档的分块,通过制定字符/规则划分文档
- `Ollama Embeddings`: 词嵌入模型
- `Chat Input`: 调用 `QA_SERVER_GET_QUESTION_URL` 向测评机获取问题
- `MiniOB`: MiniOB 数据库交互
- `Parser`: 解析 MiniOB 的检索结果
- `Prompt`: 通过检索结果生成提示词
- `Qwen`: 通义千问模型
- `Chat Output`: 输出大模型结果
- `Test Output`: 投送测评结果到测评机

### 测评提示

- 初始工作流的组件默认未连接，请先完成连线后再运行
- 在 MiniOB 组件代码中标注有 TODO，请完成 TODO 并正确连接以通过此题
- 请勿修改如环境变量名等配置项名称以保证测评顺利进行

## 参考资料

### RAG 技术

- [RAG: Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks](https://arxiv.org/abs/2005.11401)
- [RAG 概述](https://www.oceanbase.com/docs/common-oceanbase-cloud-1000000002951473#0-title-%E6%A6%82%E5%BF%B5%E4%BB%8B%E7%BB%8D)

### Langflow 工作流

- [LangChain 官方文档](https://python.langchain.com/)
- [Langflow 官方文档](https://docs.langflow.org/)
- [Langflow 代码仓库](https://github.com/langflow-ai/langflow)

### 自测模型参考

- [阿里云百炼平台](https://bailian.console.aliyun.com/?tab=model#/model-market)
- [Ollama](https://ollama.com/)
