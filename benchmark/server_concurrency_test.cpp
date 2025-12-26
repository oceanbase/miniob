/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2023/04/28
//
// 优化说明：修复编译错误、提升安全性、完善基准测试逻辑、规范代码风格
// 核心目标：让客户端基准测试模块可正常运行，支持 TCP/Unix Socket 连接与 SQL 执行基准测试

#include <inttypes.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <cstring>
#include <limits>

#include <benchmark/benchmark.h>
#include <random>
#include <string>
#include <ostream>

#include "common/lang/stdexcept.h"
#include "common/log/log.h"
#include "common/net/io.h"  // 依赖 miniob 已封装的 writen 函数（若未存在需补充实现）
#include "rc.h"

using namespace std;
using namespace common;
using namespace benchmark;

/**
 * @brief 数据库客户端类，支持 TCP 和 Unix Socket 连接，提供 SQL 发送与结果接收功能
 */
class Client
{
public:
  Client() = default;
  virtual ~Client();

  /**
   * @brief 初始化 TCP 连接
   * @param host 服务器主机名或 IP 地址
   * @param port 服务器端口号
   * @return RC::SUCCESS 成功；其他错误码 失败
   */
  RC init_tcp(const string &host, int port);

  /**
   * @brief 初始化 Unix Socket 连接
   * @param unix_socket Unix Socket 文件路径
   * @return RC::SUCCESS 成功；其他错误码 失败
   */
  RC init_unix(const string &unix_socket);

  /**
   * @brief 关闭连接，释放套接字资源
   * @return RC::SUCCESS 成功
   */
  RC close();

  /**
   * @brief 发送 SQL 语句到服务器（包含终止符 '\0'）
   * @param sql 待发送的 SQL 字符串（非空）
   * @return RC::SUCCESS 成功；RC::IOERROR 发送失败
   */
  RC send_sql(const char *sql);

  /**
   * @brief 接收服务器返回的结果
   * @param result_stream 结果输出流（用于存储接收的结果）
   * @return RC::SUCCESS 成功；RC::IOERROR 接收失败
   */
  RC receive_result(ostream &result_stream);

  /**
   * @brief 执行 SQL 语句（发送+接收结果）
   * @param sql 待执行的 SQL 字符串
   * @param result_stream 结果输出流
   * @return RC::SUCCESS 成功；其他错误码 失败
   */
  RC execute(const char *sql, ostream &result_stream);

  /**
   * @brief 检查客户端是否已连接
   * @return true 已连接；false 未连接
   */
  bool is_connected() const { return socket_fd_ != -1; }

private:
  string server_addr_;  // 服务器地址描述（tcp://host:port 或 unix://path）
  int    socket_fd_ = -1;  // 套接字文件描述符（-1 表示未连接）
};

/**
 * @brief 基准测试基类，封装客户端初始化与资源清理逻辑
 */
class BenchmarkBase : public Fixture
{
public:
  BenchmarkBase() = default;
  virtual ~BenchmarkBase() = default;

  /**
   * @brief 获取基准测试名称（子类必须实现）
   * @return 测试名称字符串
   */
  string Name() const override = 0;

  /**
   * @brief 测试前初始化（每个测试用例执行前调用）
   * @param state 基准测试状态
   */
  void SetUp(const State &state) override;

  /**
   * @brief 测试后清理（每个测试用例执行后调用）
   * @param state 基准测试状态
   */
  void TearDown(const State &state) override;

protected:
  Client client_;  // 基准测试使用的客户端实例
  // 可配置参数（通过 benchmark 命令行传递或硬编码默认值）
  string host_ = "127.0.0.1";    // 服务器主机
  int    port_ = 1024;           // 服务器端口
  string unix_socket_path_ = "./miniob.sock";  // Unix Socket 路径
  bool   use_tcp_ = true;        // 是否使用 TCP 连接（默认 true）
};

// ------------------------------ Client 类实现 ------------------------------
Client::~Client()
{
  (void)this->close();  // 忽略返回值，确保析构时关闭连接
}

RC Client::init_tcp(const string &host, int port)
{
  // 校验参数合法性
  if (host.empty() || port < 1 || port > std::numeric_limits<uint16_t>::max()) {
    LOG_WARN("invalid tcp connection params: host=%s, port=%d", host.c_str(), port);
    return RC::INVALID_ARGUMENT;
  }

  // 关闭已有连接
  if (is_connected()) {
    LOG_WARN("already connected to %s, close first", server_addr_.c_str());
    (void)this->close();
  }

  // 使用 getaddrinfo（线程安全、支持 IPv6）替代 gethostbyname
  struct addrinfo hints;
  struct addrinfo *serv_info = nullptr;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;    // 自动适配 IPv4/IPv6
  hints.ai_socktype = SOCK_STREAM;// TCP 流套接字

  char port_str[16];
  snprintf(port_str, sizeof(port_str), "%d", port);
  int ret = getaddrinfo(host.c_str(), port_str, &hints, &serv_info);
  if (ret != 0) {
    LOG_WARN("failed to resolve host %s:%d. error=%s", host.c_str(), port, gai_strerror(ret));
    return RC::IOERROR;
  }

  // 尝试连接第一个可用地址
  int sock_fd = -1;
  for (struct addrinfo *p = serv_info; p != nullptr; p = p->ai_next) {
    sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sock_fd < 0) {
      LOG_WARN("failed to create tcp socket. error=%s", strerror(errno));
      continue;
    }

    if (connect(sock_fd, p->ai_addr, p->ai_addrlen) == 0) {
      break;  // 连接成功
    }

    // 连接失败，关闭当前套接字
    ::close(sock_fd);
    sock_fd = -1;
    LOG_WARN("failed to connect to %s:%d. error=%s", host.c_str(), port, strerror(errno));
  }

  freeaddrinfo(serv_info);  // 释放地址信息资源
  if (sock_fd < 0) {
    LOG_ERROR("failed to establish tcp connection to %s:%d", host.c_str(), port);
    return RC::IOERROR;
  }

  // 保存连接信息
  socket_fd_ = sock_fd;
  server_addr_ = string("tcp://") + host + ":" + to_string(port);
  LOG_INFO("successfully connected to %s", server_addr_.c_str());
  return RC::SUCCESS;
}

RC Client::init_unix(const string &unix_socket)
{
  // 校验参数合法性（UNIX_PATH_MAX 为系统定义的最大路径长度）
  if (unix_socket.empty() || unix_socket.size() >= UNIX_PATH_MAX) {
    LOG_WARN("invalid unix socket path: %s (length=%zu, max=%d)",
             unix_socket.c_str(), unix_socket.size(), UNIX_PATH_MAX);
    return RC::INVALID_ARGUMENT;
  }

  // 关闭已有连接
  if (is_connected()) {
    LOG_WARN("already connected to %s, close first", server_addr_.c_str());
    (void)this->close();
  }

  // 创建 Unix Socket
  int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    LOG_WARN("failed to create unix socket. error=%s", strerror(errno));
    return RC::IOERROR;
  }

  // 初始化地址结构
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, unix_socket.c_str(), sizeof(addr.sun_path) - 1);  // 避免缓冲区溢出

  // 连接服务器
  if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    LOG_WARN("failed to connect to unix socket %s. error=%s", unix_socket.c_str(), strerror(errno));
    ::close(sock_fd);
    return RC::IOERROR;
  }

  // 保存连接信息
  socket_fd_ = sock_fd;
  server_addr_ = string("unix://") + unix_socket;
  LOG_INFO("successfully connected to %s", server_addr_.c_str());
  return RC::SUCCESS;
}

RC Client::close()
{
  if (socket_fd_ >= 0) {
    LOG_INFO("closing connection to %s", server_addr_.c_str());
    ::close(socket_fd_);
    socket_fd_ = -1;
    server_addr_.clear();
  }
  return RC::SUCCESS;
}

RC Client::send_sql(const char *sql)
{
  if (!is_connected()) {
    LOG_WARN("send_sql failed: not connected to server");
    return RC::IOERROR;
  }
  if (sql == nullptr || strlen(sql) == 0) {
    LOG_WARN("send_sql failed: sql is empty");
    return RC::INVALID_ARGUMENT;
  }

  // 调用 miniob 封装的 writen 函数（保证数据完整发送）
  int ret = writen(socket_fd_, sql, strlen(sql) + 1);  // 包含 '\0' 终止符
  if (ret != 0) {
    LOG_WARN("failed to send sql to %s. sql=%s, error=%s",
             server_addr_.c_str(), sql, strerror(ret));
    return RC::IOERROR;
  }
  LOG_DEBUG("successfully sent sql to %s: %s", server_addr_.c_str(), sql);
  return RC::SUCCESS;
}

RC Client::receive_result(ostream &result_stream)
{
  if (!is_connected()) {
    LOG_WARN("receive_result failed: not connected to server");
    return RC::IOERROR;
  }

  char tmp_buf[4096];  // 增大缓冲区，提升接收效率
  ssize_t read_len = 0;
  bool msg_ended = false;

  while (!msg_ended) {
    read_len = ::read(socket_fd_, tmp_buf, sizeof(tmp_buf));
    if (read_len < 0) {
      if (errno == EAGAIN || errno == EINTR) {  // 非阻塞重试/中断重试
        continue;
      }
      LOG_WARN("failed to read result from %s. error=%s", server_addr_.c_str(), strerror(errno));
      return RC::IOERROR;
    }
    if (read_len == 0) {
      LOG_WARN("connection closed by server %s", server_addr_.c_str());
      return RC::IOERROR;
    }

    // 写入结果流
    result_stream.write(tmp_buf, read_len);

    // 检查是否遇到消息终止符 '\0'
    for (ssize_t i = 0; i < read_len; ++i) {
      if (tmp_buf[i] == '\0') {
        msg_ended = true;
        break;
      }
    }
  }

  LOG_DEBUG("successfully received result from %s", server_addr_.c_str());
  return RC::SUCCESS;
}

RC Client::execute(const char *sql, ostream &result_stream)
{
  RC rc = send_sql(sql);
  if (rc != RC::SUCCESS) {
    LOG_WARN("execute failed: send_sql failed. sql=%s", sql);
    return rc;
  }

  rc = receive_result(result_stream);
  if (rc != RC::SUCCESS) {
    LOG_WARN("execute failed: receive_result failed. sql=%s", sql);
    return rc;
  }

  return RC::SUCCESS;
}

// ------------------------------ BenchmarkBase 类实现 ------------------------------
void BenchmarkBase::SetUp(const State &state)
{
  Fixture::SetUp(state);
  RC rc = RC::FAIL;

  // 根据配置初始化连接（支持 TCP/Unix Socket 切换）
  if (use_tcp_) {
    rc = client_.init_tcp(host_, port_);
  } else {
    rc = client_.init_unix(unix_socket_path_);
  }

  // 基准测试初始化失败则终止
  if (rc != RC::SUCCESS) {
    LOG_FATAL("benchmark setup failed: init client failed. rc=%d", rc);
    throw std::runtime_error("client initialization failed");
  }
  LOG_INFO("benchmark setup success: %s connected", client_.is_connected() ? "yes" : "no");
}

void BenchmarkBase::TearDown(const State &state)
{
  Fixture::TearDown(state);
  (void)client_.close();  // 关闭客户端连接
  LOG_INFO("benchmark teardown success");
}

// ------------------------------ 示例基准测试用例 ------------------------------
/**
 * @brief 测试简单查询 SQL 的执行性能（SELECT 1）
 */
class SimpleQueryBenchmark : public BenchmarkBase
{
public:
  string Name() const override { return "simple_query_benchmark"; }
};

BENCHMARK_F(SimpleQueryBenchmark, Select1)(State &state)
{
  stringstream result;
  const char *sql = "SELECT 1;";

  // 循环执行基准测试（state.Range 可配置执行次数）
  for (auto _ : state) {
    RC rc = client_.execute(sql, result);
    if (rc != RC::SUCCESS) {
      state.SkipWithError("execute sql failed");
      break;
    }
    result.str("");  // 清空结果流，避免内存累积
  }
}

// 注册基准测试参数（可通过命令行覆盖）
BENCHMARK_REGISTER_F(SimpleQueryBenchmark, Select1)->Range(1, 10000);

// ------------------------------ 主函数 ------------------------------
int main(int argc, char **argv)
{
  // 初始化日志模块（确保基准测试日志可输出）
  LoggerFactory::init_default("./benchmark_client.log");

  // 解析 benchmark 命令行参数
  Initialize(&argc, argv);
  if (ReportUnrecognizedArguments(argc, argv)) {
    return 1;
  }

  // 运行所有基准测试用例
  RUN_ALL_BENCHMARKS();

  // 清理日志资源
  LoggerFactory::destroy();
  return 0;
}

// ------------------------------ 补充依赖：writen 函数实现（若 miniob 未提供） ------------------------------
// 若 common/net/io.h 中未实现 writen，需添加以下代码（可放在单独的 io.cpp 文件中）
#if !defined(HAVE_WRITEN)
namespace common {
int writen(int fd, const void *buf, size_t n)
{
  size_t nleft = n;
  ssize_t nwritten = 0;
  const char *ptr = static_cast<const char *>(buf);

  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) <= 0) {
      if (nwritten < 0 && errno == EINTR) {
        nwritten = 0;  // 被中断，重试
      } else {
        return -1;     // 真正错误
      }
    }
    nleft -= nwritten;
    ptr += nwritten;
  }
  return 0;
}
} // namespace common
#define HAVE_WRITEN
#endif