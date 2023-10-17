# Date实现解析

> 此实现解析有往届选手提供。具体代码实现已经有所变更，因此仅供参考。

- by caizj

## DATE的存储


一种实现方式：date以int类型的YYYYMMDD格式保存，比如2021-10-21，保存为整数就是2021\*1000 + 10\*100 + 21，在select展示时转成字符串YYYY-MM-DD格式，注意月份和天数要使用0填充。

在parse.cpp中，参考

```c++
int value_init_date(Value* value, const char* v) {
    value->type = DATES;
    int y,m,d;
    sscanf(v, "%d-%d-%d", &y, &m, &d);//not check return value eq 3, lex guarantee
    bool b = check_date(y,m,d);
    if(!b) return -1;
    int dv = y*10000+m*100+d;
    value->data = malloc(sizeof(dv));//TODO:check malloc failure
    memcpy(value->data, &dv, sizeof(dv));
    return 0;
}
```

## 修改点

### 语法上修改支持

需要可匹配date的token词和DATE_STR值（一定要先于SSS，因为date的输入DATE_STR是SSS的子集)

语法(yacc文件)上增加type，value里增加DATE_STR值

```c++
[Dd][Aa][Tt][Ee]                     RETURN_TOKEN(DATE_T);  // 增加DATE的token，需要在yacc文件中增加DATE_T的token

{QUOTE}[0-9]{4}\-(0?[1-9]|1[012])\-(0?[1-9]|[12][0-9]|3[01]){QUOTE} yylval->string=strdup(yytext); RETURN_TOKEN(DATE_STR);  // 使用正则表达式过滤DATE。需要在yacc文件中增加 %token <string> DATE_STR
```

同时，需要增加一个DATE类型，与INTS,FLOATS等含义相同：

```c++
// in parse_defs.h
typedef enum { UNDEFINED, CHARS, INTS, FLOATS, DATES, TEXTS, NULLS } AttrType;
```

### Date的合法性判断

输入日期的格式可以在词法分析时正则表达式里过滤掉。润年，大小月日期的合法性在普通代码中再做进一步判断。

在parse阶段，对date做校验，并格式化成int值保存（参考最前面的代码），同时对日期的合法性做校验，参考：

```c++
bool check_date(int y, int m, int d)
{
    static int mon[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool leap = (y%400==0 || (y%100 && y%4==0));
    return y > 0
        && (m > 0)&&(m <= 12)
        && (d > 0)&&(d <= ((m==2 && leap)?1:0) + mon[m]);
}
```

### 增加新的类型date枚举

代码里有多处和类型耦合地方（增加一个类型，要动很多处离散的代码，基础代码在这方面的可维护性不好)

包括不限于，以下几处：

- DefaultConditionFilter
需要增加DATES类型的数据对比。因为这里将date作为整数存储，那么可以直接当做INTS来对比，比如：

```c++
    case INTS: 
    case DATES: {
      // 没有考虑大小端问题
      // 对int和float，要考虑字节对齐问题,有些平台下直接转换可能会跪
                               int left = *(int *)left_value;
                               int right = *(int *)right_value;
                               cmp_result = left - right;
    } break;
```

- BplusTreeScanner
与 `DefaultConditionFilter` 类似，也需要支持DATE类型的对比，可以直接当做整数比较。参考其中一块代码：

```c++
case INTS:case DATES: {
      i1 = *(int *) pdata;
      i2 = *(int *) pkey;
      if (i1 > i2)
        return 1;
      if (i1 < i2)
        return -1;
      if (i1 == i2)
        return 0;
    }
      break;
```

- ATTR_TYPE_NAME(storage/common/field_meta.cpp)
保存元数据时，需要这里的信息，比较简单，参考：

```c++
const char *ATTR_TYPE_NAME[] = {
  "undefined",
  "chars",
  "ints",
  "floats",
  "dates"
};
```

- insert_record_from_file（storage/default/default_storage_stage.cpp）

这个接口主要是为了支持从文件导入数据的，同样，实现可以与int类型保持一致。

```c++
switch (field->type()) {
        case INTS: case DATES:{
        deserialize_stream.clear(); // 清理stream的状态，防止多次解析出现异常
        deserialize_stream.str(file_value);

        int int_value;
        deserialize_stream >> int_value;
        if (!deserialize_stream || !deserialize_stream.eof()) {
          errmsg << "need an integer but got '" << file_values[i]
                 << "' (field index:" << i << ")";

          rc = RC::SCHEMA_FIELD_TYPE_MISMATCH;
        } else {
          value_init_integer(&record_values[i], int_value);
        }
      }
```

### Date select展示

TupleRecordConverter::add_record时做格式转换，需要按照输出要求，将日期类型数据，转换成合适的字符串。参考：

```c++
case DATES: {
  int value = *(int*)(record + field_meta->offset());
  char buf[16] = {0};
  snprintf(buf,sizeof(buf),"%04d-%02d-%02d",value/10000,    (value%10000)/100,value%100); // 注意这里月份和天数，不足两位时需要填充0
  tuple.add(buf,strlen(buf));
}
break;
```

### 异常失败处理

只要输入的日期不合法，输出都是FAILURE\n。包括查询的where条件、插入的日期值、更新的值等。这里在解析时(parse.cpp)中就可以直接返回错误。

## 自测覆盖点

1. 日期输入，包括合法和非法日期格式。非法日期可以写单元测试做。
2. 日期值比较=、 >、 <、 >=、 <=
3. 日期字段当索引。很多同学漏掉了这个点。
4. 日期展示格式，注意月份和天数补充0