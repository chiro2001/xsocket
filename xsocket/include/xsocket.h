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
class XSocketExceptionConnectionWillClose : public XSocketExceptionBase {
  USUPER(XSocketExceptionBase)
};
class XSocketExceptionConnectionError : public XSocketExceptionBase {
  USUPER(XSocketExceptionBase)
};
class XSocketExceptionReading : public XSocketExceptionBase {
  USUPER(XSocketExceptionBase)
};
class XSocketExceptionWriting : public XSocketExceptionBase {
  USUPER(XSocketExceptionBase)
};

/*
 * @brief 地址类、套接字操作类
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

  std::string to_string() {
    static std::stringstream ss;
    ss.str("");
    ss << this->ip << "[" << this->port << "]";
    return ss.str();
  }
  // 一些配置
  // 最大服务器同时链接数量
  int max_connect = 20;
  XSocketAddress(std::string ip_, int port_) : ip(ip_), port(port_) {
    this->sock_open();
  }
  ~XSocketAddress() { this->sock_close_all(); }
  int sock_open() {
    // 参数错误了
    if (this->ip.length() == 0 || this->port <= 0) return -1;
    this->sock = socket(AF_INET, SOCK_STREAM, 0);  // TCP
    // this->sock = socket(AF_INET, SOCK_DGRAM, 0);    // UDP
    LOG(INFO) << "Address: opened " << this->to_string();
    return this->sock;
  }
  int sock_close_all() {
    if (this->sock > 0) close(this->sock);
    this->sock = 0;
    if (this->sock_client > 0) close(this->sock_client);
    this->sock_client = 0;
    return 0;
  }
  int sock_close_client() {
    if (this->sock_client > 0) close(this->sock_client);
    this->sock_client = 0;
    return 0;
  }
  int sock_bind() {
    LOG(INFO) << "Address: binding " << this->to_string();
    return bind(this->sock, (struct sockaddr*)&this->addr,
                sizeof(struct sockaddr));
  }
  int sock_bind_client() {
    LOG(INFO) << "Address: binding client " << this->to_string();
    return bind(this->sock_client, (struct sockaddr*)&this->addr,
                sizeof(struct sockaddr));
  }
  int sock_listen() {
    LOG(INFO) << "Address: start listening on " << this->to_string();
    return listen(this->sock, this->max_connect);
  }
  // Server使用，返回客户端sock
  int sock_accept() {
    LOG(INFO) << "Address: accepting on " << this->to_string();
    return this->sock_client =
               accept(this->sock, (struct sockaddr*)&this->addr_client,
                      &this->addr_size);
  }

  void sock_connect() {
    throwifminus(connect(this->sock_client, (struct sockaddr*)&this->addr,
                         sizeof(this->addr)),
                 XSocketExceptionConnectionError());
  }
  // Client使用，在初始化addr之后、bind之前使用。
  // void mode_client() { this->sock_client = this->sock; }

  // 按照单个字符读取，读取sock_client
  int sock_read_byte() {
    int data = 0;
    throwif(read(this->sock_client, &data, 1) <= 0, XSocketExceptionReading());
    return data;
  }
  // 按照字符串读取，遇到'\0'或者<0停止
  std::string sock_read_string() {
    static std::stringstream ss;
    ss.str("");
    int c;
    while ((c = this->sock_read_byte()) > 0) {
      ss << (char)c;
    }
    throwifminus(c, XSocketExceptionConnectionWillClose());
    return ss.str();
  }

  // 写入字符
  void sock_write_byte(uint8_t d) {
    throwif(this->sock_client <= 0 || write(this->sock_client, &d, 1) <= 0, XSocketExceptionWriting());
  }
  // 写入字符串
  void sock_write_string(std::string s) {
    for (uint8_t c : s) this->sock_write_byte(c);
  }

  // 写入结束字符
  void sock_bye() { this->sock_write_byte((uint8_t)XSOCKET_FLAG_END); }
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
  XSocketCallingMessage(int code_, std::queue<T>* data_)
      : data(data_), code(code_) {}
};

/*
 * @brief 事件处理类
 * 能进行基本的事件处理。
 */
template <class T>
class XEvents {
 public:
  // 缓冲区指针
  std::queue<T>* data;
  XEvents(std::queue<T>* data_) : data(data_) {}
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
  // 空消息
  void call(std::string name, int code = 0) {
    this->call(name, XSocketCallingMessage<T>(code, this->data));
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
  // 事件管理
  XEvents<T>* xevents;
  XSocket(std::string ip, int port) {
    // 检查模板类型是否正确
    if (!(std::is_same<T, Json::Value>::value ||
          std::is_same<T, std::string>::value))
      throw XSocketExceptionTemplateUnallowed();
    this->addr = new XSocketAddress(ip, port);
    this->xevents = new XEvents<T>(&this->data);
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

  void send_data(T d) {
    if (std::is_same<T, std::string>::value) {
      this->addr->sock_write_string(d);
    } else {
    }
  }
};

// 工作模式枚举
uint8_t P2P = 1 << 0, P2M = 1 << 1, AR = 1 << 2;

/*
 * @brief XSocket的Server共用逻辑部分
 */
template <class T>
class XSocketServer : public XSocket<T> {
 public:
  uint8_t stats;
  XSocketServer(std::string ip, int port) : XSocket<T>(ip, port) {
    this->prepare();
  }
  void prepare() {
    throwifminus(this->addr->sock_bind(),
                 XSocketExceptionPrepareSocket("Error binding."));
    throwifminus(this->addr->sock_listen(),
                 XSocketExceptionPrepareSocket("Error listening."));
  }
  // 一个必须由子类实现的函数
  // static void server_loop();
};

/*
 * @brief P2P模式下的XSocketServer
 * 好像可以简称XSS？
 */
template <class T>
class XSocketServerP2P : public XSocketServer<T> {
 public:
  XSocketServerP2P(std::string ip, int port) : XSocketServer<T>(ip, port) {
    this->stats |= P2P;
  }
  static void server_loop(XSocketServerP2P<T>* self) {
    while (true) {
      self->addr->sock_accept();
      while (true) {
        try {
          std::string data_str = self->addr->sock_read_string();
          // 写入缓冲区
          if (std::is_same<T, std::string>::value) {
            self->data.push(data_str);
          } else {
          }
          LOG(INFO) << "ServerP2P: got string " << data_str;
          // 触发onmessage事件
          self->xevents->call("onmessage");
        } catch (XSocketExceptionConnectionWillClose e) {
          LOG(ERROR) << e.what();
          break;
        } catch (XSocketExceptionReading e) {
          LOG(ERROR) << e.what();
          break;
        }
      }
      self->addr->sock_close_client();
    }
  }
  std::future<void> server_loop_start() {
    return std::async(XSocketServerP2P::server_loop, this);
  }
};

/*
 * @brief XSocket的Client共用逻辑部分
 */
template <class T>
class XSocketClient : public XSocket<T> {
 public:
  uint8_t stats;
  XSocketClient(std::string ip, int port) : XSocket<T>(ip, port) {
    this->prepare();
  }
  void prepare() {
    throwifminus(this->addr->sock_bind_client(),
                 XSocketExceptionPrepareSocket("Error binding."));
  }
  // 一个必须由子类实现的函数
  // static void client_loop();
};

/*
 * @brief P2P模式下的XSocketClient
 * 就叫他XSC好了。
 */
template <class T>
class XSocketClientP2P : public XSocketClient<T> {
 public:
  XSocketClientP2P(std::string ip, int port) : XSocketClient<T>(ip, port) {
    this->stats |= P2P;
  }
  static void client_loop(XSocketClientP2P<T>* self) {
    while (true) {
      self->addr->sock_connect();
      while (true) {
        try {
          std::string data_str = self->addr->sock_read_string();
          // 写入缓冲区
          if (std::is_same<T, std::string>::value) {
            self->data.push(data_str);
          } else {
          }
          LOG(INFO) << "ClientP2P: got string " << data_str;
          // 触发onmessage事件
          self->xevents->call("onmessage");
        } catch (XSocketExceptionConnectionWillClose e) {
          LOG(ERROR) << e.what();
          break;
        } catch (XSocketExceptionReading e) {
          LOG(ERROR) << e.what();
          break;
        }
      }
      self->addr->sock_close_client();
    }
  }
  std::future<void> client_loop_start() {
    return std::async(XSocketClientP2P::client_loop, this);
  }
};