#pragma once
#include <arpa/inet.h>
#include <fcntl.h>
#include <glog/logging.h>
#include <json/json.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <exception>
#include <future>
#include <iostream>
#include <sstream>
#include <string>

// #define MY_SOCKET_BUFSIZE (1 << 16)

/*
 * 计划接口使用方法：
 * 1. 初始化地址
 * 2. 添加回调
 * 3. 运行start()直接开始监听
 * 4. 运行stop()结束监听
 * 
 * Server使用方法：
 * 0. 初始化地址
 * 1. 设置数据格式：bytes/Json（即使用不同的继承类）
 * 2. 设置回调方式
 * 3. 回调方式1：中断调用（P2P/P2M）
 *   1. 添加回调函数
 *   2. 运行start()开始监听
 * 4. 回调方式2：被动调用（P2P/P2M）
 *   1. 运行start()开始监听
 *   2. 运行data_empty()了解是否有数据传输到缓冲区
 *   3. 运行data_get()按个获取缓冲区数据
 *   4. 运行data_get_count(count)获取N个字节的数据/N个消息的数据
 * 5. 设置连接方式：P2P/P2M（只能运行一个长链接，一对一/允许多个短链接，一对多）
 *   1. P2M模式在监听时不主动关闭连接，等Client发送完成之后自行断开
 *   2. P2P模式一旦连接上就不自动断开
 *   3. Err...所以是都一样的处理方法
 * 6. 运行stop()结束监听
 * 
 * 
 * Client使用方法：
 * 0. 初始化地址
 * 1. 设置数据格式：bytes/Json（即使用不同的继承类）
 * 2. 设置连接、回调方式（请使用不同的继承类）
 * 3. 回调方式1：中断调用（P2P）
 *   1. 添加回调函数
 *   2. 运行start()开始监听
 *   3. 运行stop()结束监听
 * 4. 回调方式2：被动调用（P2P）
 *   1. 运行start()开始监听
 *   2. 运行data_empty()了解是否有数据传输到缓冲区
 *   3. 运行data_get()获取缓冲区数据
 *   4. 运行data_get_count(count)获取N个字节的数据/N个消息的数据
 * 5. 回调方式3：P2M模式，只发送
 *   1. 设置运行模式：阻塞/非阻塞，非阻塞模式只能通过回调获取错误信息
 *   2. 阻塞模式
 *     1. 等待连接建立
 *     2. 连接建立之后直接发送消息
 *     3. 发送完成消息之后主动关闭连接
 *   3. 非阻塞模式
 *     1. 专门开启一个线程来运行上面的阻塞模式
 * 
 */

class XSocket {
 public:
  // 共用一个地址信息
  struct sockaddr_in addr;
  std::string ip;
  int port;
  
  // 同步模式：只支持单个Client连接上
  bool mode_async = true;

  // 标记：是否已经开始监听
  bool mark_started = false;
  bool mark_running = false;
  
  // 创建好的套接字
  int sock_server = -1;
  int sock_client = -1;

  // 回调部分
  void (*onopen)(void) = NULL;
  void (*onerror)(void) = NULL;
  void (*onclienterror)(void) = NULL;
  // 在数据发送和接收端检查、修改一下数据
  void (*onmessage)(Json::Value) = NULL;
  void (*onsend)(Json::Value) = NULL;
  // send，Server和Client实现方法一样
  void send(Json::Value);

  std::string to_string() {
    return this->ip + ":" + std::to_string(this->port);
  }
  void config_addr() {
    if (this->ip.length() == 0 || this->port <= 0) {
      LOG(ERROR) << "Unable to create socket address at " << this->to_string();
      return;
    }
    // 创建套接字
    // 使用UDP了。如果需要稳定性不需要高流量就使用TCP
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_UDP);
    // int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // 设置地址
    memset(&this->addr, 0, sizeof(this->addr));
    this->addr.sin_family = AF_INET;
    this->addr.sin_addr.s_addr = inet_addr(this->ip.c_str());
    this->addr.sin_port = htons(port);
    LOG(INFO) << "Created socket address at " << this->to_string();
  }

  void do_close() {
    if (this->sock_client > 0) {
      close(this->sock_client);
      this->sock_client = -1;
    };
    if (this->sock_server > 0) {
      close(this->sock_server);
      this->sock_server = -1;
    }
  }
  ~XSocket() {
    // 关闭连接
    this->do_close();
  }

  class ExceptionParseJson : public std::exception {};
  class ExceptionCreateConnect : public std::exception {};
  class ExceptionReading : public std::exception {};
};

class XServer : public XSocket {
 public:
  std::future<int> future_mainloop;

  XServer(std::string ip_, int port_, bool use_async = true) {
    this->ip = ip_, this->port = port_, this->mode_async = use_async;
    config_addr();
  }
  XServer* start();
  void stop();
};
class XClient : public XSocket {
 public:
  std::future<int> future_recvloop;
  XClient(std::string ip_, int port_, bool use_async = true) {
    this->ip = ip_, this->port = port_, this->mode_async = use_async;
    config_addr();
  }

  XClient* start();
  void stop();
  void recv();
};

Json::Value message_parser(std::string src);