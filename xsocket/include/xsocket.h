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
#include <map>
#include <queue>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

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
 * 1. 设置数据格式：bytes/Json（使用同一个模板类）
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
 * 1. 设置数据格式：bytes/Json（使用同一个模板类）
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

/*
 * @brief 异常基类
 * 能把继承了这个类的类的what()直接打印出类名称。
 */
class XSocketExceptionBase : public std::exception {
 public:
  std::string message;
  XSocketExceptionBase() {}
  XSocketExceptionBase* see_message(std::string message_) {
    this->message = message_;
    return this;
  }
  const char* what() const throw() {
    std::stringstream ss;
    ss << "[" << typeid(*this).name() << "] : " << this->message;
    return (const char*)(ss.str().c_str());
  }
};

/*
 * @brief 地址类
 * 包含对地址的储存、套接字储存和生成套接字，
 * 提供socket的生成和删除函数
 */
class XSocketAddress {
 public:
  std::string ip;
  int port = 0;
  struct sockaddr_in addr;
  int sock = 0;
  XSocketAddress(std::string ip_, int port_) : ip(ip_), port(port_) {
    this->sock_open();
  }
  int sock_open() {
    // 参数错误了
    if (this->ip.length() == 0 || this->port <= 0) return -1;
    this->sock = socket(AF_INET, SOCK_STREAM, 0);  // TCP
    // this->sock = socket(AF_INET, SOCK_DGRAM, 0);    // UDP
  }
  int sock_close() {
    close(this->sock);
    this->sock = 0;
    return 0;
  }
};

/*
 * @brief 返回数据包的类
 * 在触发接收到数据的时候在message中包含这个类。
 */
template <class T>
class XSocketResponse {
 public:
  std::vector<T> data;
};

/*
 * @brief 回调使用的参数类
 * 调用回调的时候把数据放里头做参数吧...
 */
template <class T>
class XSocketCallingMessage {
 public:
  XSocketResponse<T>* response = NULL;
};

/*
 * @brief 事件处理类
 * 能进行基本的事件处理。
 */
class XEvents {
 public:
  // 记录函数指针，注意要强制转换调用。
  std::map<std::string, vector<size_t>> callers;
};

/*
 * @brief XSocket的基类
 * 提供char、Json、string类型做缓冲区的模板。
 * 包含地址、缓冲区、缓冲区管理。
 */
template <class T>
class XSocket {
 public:
  XSocketAddress addr;
  // 缓冲区
  std::queue<T> data;
  XSocket() {
    // 检查模板类型是否正确
    if (!(std::is_same<T, char>::value || std::is_same<T, Json::Value>::value ||
          std::is_same<T, std::string>::value))
      throw ExceptionTemplateUnallowed();
  }
  // 缓冲区管理
  bool data_empty() { return this->data.empty(); }
  std::vector<T> data_get_count(int size) {
    std::vector<T> tmp;
    while (!this->data_empty() && size) {
      tmp.emplace_back(this->data.front());
      this->data.pop();
      size--;
    }
    return tmp;
  }
  std::vector<T> data_get(int size) {
    return this->data_get_count(this->data.size());
  }
  class ExceptionTemplateUnallowed : public XSocketExceptionBase {};
};

/*
 * @brief XSocket的P2P连接部分
 * 包含两种回调方式：中断调用和被动调用。
 */
template <class T>
class XSocketP2P : public XSocket<T> {};

/*
 * @brief XScoket的P2M连接部分
 * 直接继承P2P部分，其实就是打开了个切换开关。
 * 但是Client不能继承这个。
 */
template <class T>
class XSocketP2M : public XSocketP2P<T> {};

/*
 * @brief
 *
 */