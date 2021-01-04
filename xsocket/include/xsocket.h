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
// 继承基类构造函数的语法糖
#define USUPER(CLASS) \
 public:              \
  using CLASS::CLASS;
// 异常处理语法糖
#define throwif(x, exception) \
  do {                        \
    if (x) throw exception;   \
  } while (0)
#define throwifminus(x, exception) throwif(x < 0, exception)

// 停止符号，遇到这个FLAG就表明应该断开连接，不能再有写入操作
const int XSOCKET_FLAG_END = -1;

/*
 * @brief 异常基类
 * 能把继承了这个类的类的what()直接打印出类名称。
 */
class XSocketExceptionBase : public std::exception {
 public:
  std::string message;
  XSocketExceptionBase() {}
  XSocketExceptionBase(std::string msg) : message(msg) {}
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
 * @brief 异常类
 */
class XSocketExceptionTemplateUnallowed : public XSocketExceptionBase {
  USUPER(XSocketExceptionBase)
};
class XSocketExceptionPrepareSocket : public XSocketExceptionBase {
  USUPER(XSocketExceptionBase)
};

/*
 * @brief 地址类
 * 包含对地址的储存、套接字储存和生成套接字，
 * 提供socket的生成和删除函数，
 * 提供socket操作封装
 */
class XSocketAddress {
 public:
  std::string ip;
  int port = 0;
  struct sockaddr_in addr;
  // server模式使用：储存Client信息
  struct sockaddr_in addr_client;
  socklen_t addr_size = sizeof(struct sockaddr_in);
  int sock = 0;
  int sock_client = 0;

  // 一些配置
  // 最大服务器同时链接数量
  int max_connect = 20;
  XSocketAddress(std::string ip_, int port_) : ip(ip_), port(port_) {
    this->sock_open();
  }
  ~XSocketAddress() { this->sock_close(); }
  int sock_open() {
    // 参数错误了
    if (this->ip.length() == 0 || this->port <= 0) return -1;
    this->sock = socket(AF_INET, SOCK_STREAM, 0);  // TCP
    // this->sock = socket(AF_INET, SOCK_DGRAM, 0);    // UDP
    return this->sock;
  }
  int sock_close() {
    if (this->sock > 0) close(this->sock);
    this->sock = 0;
    if (this->sock_client > 0) close(this->sock_client);
    this->sock_client = 0;
    return 0;
  }
  int sock_bind() {
    return bind(this->sock, (struct sockaddr*)&this->addr,
                sizeof(struct sockaddr));
  }
  int sock_listen() { return listen(this->sock, this->max_connect); }
  // Server使用，返回客户端sock
  int sock_accept() {
    return this->sock_client =
               accept(this->sock, (struct sockaddr*)&this->addr_client,
                      &this->addr_size);
  }
  // Client使用，在初始化addr之后、bind之前使用。
  void mode_client() { this->sock_client = this->sock; }
  // 按照单个字符读取，读取sock_client
  int sock_read_byte() {
    int data = 0;
    recv(this->sock_client, 1, &data, 0);
  }
  // 按照字符串读取，遇到'\0'或者-1停止
  std::string sock_read_string() {
    static std::stringstream ss;
    ss.clear();
    int c = this->sock_read_byte();
  }
};

// /*
//  * @brief 返回数据包的类
//  * 在触发接收到数据的时候在message中包含这个类。
//  * 如果是P2P、Json/string中断调用模式，里面应该只有一个元素。
//  * ~~如果是P2P、char中断调用模式，里面应该是用vector构成的字符串。~~
//  * 如果是被动调用模式，就直接返回vector数据了
//  */
// /*
// Json Response 格式：
// {
//   code: 0,              // 0表示正常，必选
//   message: "Success",   // 仅用于显示，可选
//   data: {               // 必选
//     ...                 // 传输的数据
//   }
// }
// */
/*
template <class T>
class XSocketResponse {
 public:
  std::vector<T> content;
  // 如果是Json格式而且是中断调用模式，数据会解析到下面。
  // 未初始化
  static const int not_inited = -1;
  int code = XSocketResponse<T>::not_inited;
  std::string message;
  Json::Value data;

  void parse() {
    if (!std::is_same<T, Json::Value>::value) {
      this->code = XSocketResponse<T>::not_inited;
      return;
    }
    if (!this->content.size() == 1)
      throw ExceptionResponseParse(
          "You can only parse response using interrupt mode.");
    Json::Value top = this->content.get(0);
    if ((!top.isMember("code")) || (!top.isMember("data")))
      throw ExceptionResponseParse("Some arguments are missing.");
    this->data = top["data"];
    this->code = top["code"];
    if (top.isMember("message")) this->message = top["message"];
  }

  class ExceptionResponseParse : public XSocketExceptionBase {};
};
*/

/*
 * @brief 回调使用的参数类
 * 调用回调的时候把数据放里头做参数吧...
 */
template <class T>
class XSocketCallingMessage {
 public:
  // XSocketResponse<T>* response = NULL;
  // 缓冲区指针
  std::queue<T>* data;
  int code = 0;
  XSocketCallingMessage(std::queue<T>* data_, int code_)
      : data(data_), code(code_) {}
};

/*
 * @brief 事件处理类
 * 能进行基本的事件处理。
 */
template <class T>
class XEvents {
 public:
  // 记录函数指针，注意要强制转换调用。
  std::map<std::string, std::vector<size_t>> callers;
  void listener_add(std::string name,
                    void (*listener)(XSocketCallingMessage<T>)) {
    if (!listener) return;
    if (this->callers.find(name) == this->callers.end())
      this->callers[name] = std::vector<size_t>();
    this->callers[name].push_back(listener);
  }
  void listener_remove(std::string name,
                       void (*listener)(XSocketCallingMessage<T>)) {
    if (!listener) return;
    if (this->callers.find(name) == this->callers.end()) return;
    auto ptr = std::find(this->callers[name].begin(), this->callers[name].end(),
                         listener);
    if (ptr == this->callers[name].end()) return;
    this->callers[name].erase(ptr);
  }
  void call(std::string name, XSocketCallingMessage<T> message) {
    if (this->callers.find(name) == this->callers.end()) return;
    for (auto ptr : this->callers[name]) {
      // 启动线程，然后...不管他。
      std::future<void> f =
          std::async((void (*)(XSocketCallingMessage<T>))ptr, message);
    }
  }
};

/*
 * @brief XSocket的基类
 * 提供Json、std::string类型做缓冲区的模板。
 * 包含地址、缓冲区、缓冲区管理。
 */
template <class T>
class XSocket {
 public:
  XSocketAddress* addr;
  // 缓冲区
  std::queue<T> data;
  XSocket(std::string ip, int port) {
    // 检查模板类型是否正确
    if (!(std::is_same<T, Json::Value>::value ||
          std::is_same<T, std::string>::value))
      throw XSocketExceptionTemplateUnallowed();
    this->addr = new XSocketAddress(ip, port);
  }

  ~XSocket() {
    // 包含断开socket操作
    delete this->addr;
  }
  // 缓冲区管理
  bool data_empty() { return this->data.empty(); }
  std::vector<T> data_get_count(size_t size) {
    std::vector<T> tmp;
    while (!this->data_empty() && size) {
      tmp.emplace_back(this->data.front());
      this->data.pop();
      size--;
    }
    return tmp;
  }
  std::vector<T> data_get(size_t size) {
    return this->data_get_count(this->data.size());
  }
};

/*
 * @brief XSocket的Server共用逻辑部分
 */
template <class T>
class XSocketServer : public XSocket<T> {
 public:
  // 工作模式枚举类
  enum class Stats { P2P = 1 << 0, P2M = 1 << 1, AR = 1 << 2 };
  Stats stats;
  XSocketServer(std::string ip, int port) : XSocket<T>(ip, port) {
    this->prepare();
  }
  void prepare() {
    // throwif(this->addr->sock_bind() < 0,
    //         XSocket<T>::ExceptionPrepareSocket("Error binding."));
    if (this->addr->sock_bind() < 0)
      throw XSocketExceptionPrepareSocket("Error binding.");
    // throw XSocket::ExceptionPrepareSocket();
    // throw XSocket<T>::ExceptionPrepareSocket;
    // throwifminus(this->addr->sock_listen(),
    //              XSocket<T>::ExceptionPrepareSocket("Error listening."));
  }
  // 一个必须由子类实现的纯虚函数
  virtual void server_loop() = 0;
};

/*
 * @brief P2P模式下的XSocketServer
 * 好像可以简称XSS？
 */
template <class T>
class XSocketServerP2P : public XSocketServer<T> {
 public:
  XSocketServerP2P(std::string ip, int port) : XSocketServer<T>(ip, port) {}
  void server_loop() {
    while (true) {
      ;
    }
  }
};

// XSocketServerP2P<std::string> xss("120.0.0.1", 1234);