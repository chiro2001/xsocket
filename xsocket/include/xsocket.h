#pragma once

// 做点兼容工作
#ifdef _unix
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#define SOCKET int
#else
// 防止自动格式化把windows.h放到了上面
#if 1

#include <winsock2.h>

#endif

#include <windows.h>

#pragma comment(lib, "ws2_32.lib")
#define socklen_t int
#endif

#include <glog/logging.h>
#include <json/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
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
#define XSOCKET_BUFSIZ_EXCEPTION 1 << 10
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
const uint8_t XSOCKET_FLAG_END = 0xFF;

// 结束函数，在程序结束的时候调用，用来释放资源
void XSocketExit(void) {
#ifndef _unix
  WSACleanup();
#endif
}

// 初始化函数
void XSocketInit() {
  static bool inited = false;
  if (inited) return;
  // 进行初始化操作
#ifndef _unix
  //初始化WSA
  WORD sockVersion = MAKEWORD(2, 2);
  WSADATA wsaData;
  if (WSAStartup(sockVersion, &wsaData) != 0) {
    LOG(ERROR) << "XSocket: WSAStartup() error!!";
    exit(1);
  }
#endif
  // 注册结束函数
  atexit(XSocketExit);
  inited = true;
}

/*
 * @brief 异常基类
 * 能把继承了这个类的类的what()直接打印出类名称。
 */
class XSocketExceptionBase : public std::exception {
public:
  std::string message;

  XSocketExceptionBase() {}

  XSocketExceptionBase(std::string msg) : message(msg) {}

  XSocketExceptionBase *see_message(std::string message_) {
    this->message = message_;
    return this;
  }

  const char *what() const throw() {
    static std::stringstream ss;
    static char buf[XSOCKET_BUFSIZ_EXCEPTION];
    ss.str("");
    ss << "[" << typeid(*this).name() << "] : " << this->message;
    strcpy(buf, ss.str().c_str());
    return (const char *) buf;
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

class XSocketExceptionConnectionClosed : public XSocketExceptionBase {
USUPER(XSocketExceptionBase)
};

class XSocketExceptionResponseParse : public XSocketExceptionBase {
USUPER(XSocketExceptionBase)
};

class XSocketExceptionData : public XSocketExceptionBase {
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
  SOCKET sock = 0;
  SOCKET sock_client = 0;
  // 控制标志
  bool opened = false;
  // 线程锁：用于控制写入和读取不能同时进行，即只有在读取缓冲区为空的时候可以send数据
  std::mutex lock;
  // 线程锁：控制sock打开关闭标志的锁
  std::mutex lock_sock;

  std::string to_string() {
    static std::stringstream ss;
    static std::string result;
    ss.str("");
    // ss << this->ip << "[" << this->port << "]";
    ss << this->ip << ":" << this->port;
    result = ss.str();
    return result;
  }

  // 一些配置
  // 最大服务器同时链接数量
  int max_connect = 20;

  XSocketAddress(std::string ip_, int port_) : ip(ip_), port(port_) {
    XSocketInit();
    this->sock_open();
  }

  ~XSocketAddress() { this->sock_close_all(); }

  int sock_open() {
    // 参数错误了
    if (this->ip.length() == 0 || this->port <= 0) return -1;
    // 设置成非阻塞socket
    this->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // this->sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    // 设置地址
    memset(&this->addr, 0, sizeof(this->addr));
    this->addr.sin_family = AF_INET;
    this->addr.sin_addr.s_addr = inet_addr(this->ip.c_str());
    this->addr.sin_port = htons(port);
    LOG(INFO) << "Address: opened " << this->to_string();
    return this->sock;
  }

  inline bool sock_is_open() { return this->opened; }

  int sock_close_all() {
    if (this->sock_close_client()) {
      this->sock = 0;
      return 0;
    }
    this->lock_sock.lock();
    if (this->sock > 0) {
      LOG(INFO) << "Address: closing server";
      close(this->sock);
    }
    this->sock = 0;
    this->lock_sock.unlock();
    return 0;
  }

  bool sock_close_client() {
    this->lock_sock.lock();
    bool flag = false;
    if (this->sock_client > 0) {
      LOG(INFO) << "Address: closing client";
      close(this->sock_client);
      this->opened = false;
      flag = true;
    }
    this->sock_client = 0;
    this->lock_sock.unlock();
    return flag;
  }

  int sock_bind() {
    int ret = bind(this->sock, (struct sockaddr *) &this->addr,
                   sizeof(struct sockaddr));
    LOG(INFO) << "Address: binding " << this->to_string() << "(" << ret << ")";
    return ret;
  }

  int sock_bind_client() {
    // LOG(INFO) << "Address: binding client " << this->ip;
    return bind(this->sock_client, (struct sockaddr *) &this->addr,
                sizeof(struct sockaddr));
  }

  int sock_listen() {
    LOG(INFO) << "Address: start listening";
    return listen(this->sock, this->max_connect);
  }

  // Server使用，返回客户端sock
  int sock_accept() {
    LOG(INFO) << "Address: accepting";
    this->sock_client = accept(this->sock, (struct sockaddr *) &this->addr_client,
                               &this->addr_size);
    if (this->sock_client >= 0) this->opened = true;
    return this->sock_client;
  }

  void sock_connect() {
    throwifminus(connect(this->sock_client, (struct sockaddr *) &this->addr,
                         sizeof(this->addr)),
                 XSocketExceptionConnectionError());
    if (this->sock_client >= 0) this->opened = true;
    LOG(INFO) << "Address: connected";
  }

  // Client使用，在初始化addr之后、bind之前使用。
  void mode_client() { this->sock_client = this->sock; }

  // 按照单个字符读取，读取sock_client
  inline uint8_t sock_read_byte(int flag = 0) {
    throwif(this->sock_client <= 0 || !this->sock_is_open(),
            XSocketExceptionConnectionClosed(
                    "Trying to read before connection establish."));

    uint8_t data = 0;
    // 同时接收带外数据
    // 先设置成非阻塞的socket然后恢复
    int ret = recv(this->sock_client, (char *) &data, 1, flag);
    if (ret == 0) {
      this->opened = false;
      throw XSocketExceptionConnectionWillClose("shuting down now!");
    }
    throwif(ret < 0, XSocketExceptionReading("sock_read_byte"));
    throwif(data == XSOCKET_FLAG_END, XSocketExceptionConnectionWillClose());
    return data;
  }

  // 按照字符串读取，遇到'\0'或者<0停止
  std::string sock_read_string() {
    std::stringstream ss;
    uint8_t c;
    bool flag = true;
    while (flag) {
      try {
        c = this->sock_read_byte();
      } catch (XSocketExceptionReading) {
        continue;
      }
      flag = false;
    }
    ss << (char) c;
    this->lock.lock();
    // 读取完成一个字符串之前不准发送数据
    while ((c = this->sock_read_byte()) > 0) {
      ss << (char) c;
    }
    this->lock.unlock();
    throwif(c == XSOCKET_FLAG_END, XSocketExceptionConnectionWillClose());
    return ss.str();
  }

  // 写入字符
  inline void sock_write_byte(uint8_t d, int flag = 0) {
    throwif(this->sock_client <= 0 || (!this->sock_is_open() && !flag),
            XSocketExceptionConnectionClosed(
                    "Trying to write before connection establish."));
    // LOG(INFO) << "locking writing socket...";
    if (!flag) this->lock.lock();
    int ret;
    ret = send(this->sock_client, (const char *) &d, 1, flag);
    if (!flag) this->lock.unlock();
    // LOG(INFO) << "released writing socket";
    throwif(ret <= 0, XSocketExceptionWriting("Error when writing..."));
  }

  // 写入字符串
  void sock_write_string(std::string s) {
    for (uint8_t c : s) this->sock_write_byte(c);
    this->sock_write_byte(0);
  }

  // 写入结束字符，使用带外数发送
  void sock_bye() {
    LOG(INFO) << "AddrSock: sending exit code!";
    try {
      //  this->sock_write_byte((uint8_t) XSOCKET_FLAG_END);
      this->sock_write_byte((uint8_t) XSOCKET_FLAG_END, MSG_OOB);
    } catch (XSocketExceptionConnectionClosed) {}
  }

  // 放弃后续数据，等待结束信号
  void sock_wait_exit() {
    LOG(INFO) << "waiting exit...";
    try {
      // 设置为仅仅读取OOB数据
      this->sock_read_byte(MSG_OOB);
    } catch (XSocketExceptionConnectionWillClose) {
      LOG(INFO) << "wait exit done!";
    }
  }

  // 中断传输
  void sock_shutdown() {
    // 终止传送
    shutdown(this->sock_client, 1);
  }
};

/*
 * @brief 返回数据包的类
 * 在触发接收到数据的时候在message中包含这个类。
 * content里面应该只有一个元素。
 */
/*
Json Response 格式：
{
  code: 0,              // 0表示正常，必选
  message: "Success",   // 仅用于显示，可选
  data: {               // 必选
    ...                 // 传输的数据
  }
}
*/
template<class T>
class XSocketResponse {
public:
  XSocketResponse() {}
};

// 特化两个类型
template<>
class XSocketResponse<std::string> {
public:
  std::vector<std::string> content;
  // 如果是Json格式而且是中断调用模式，数据会解析到下面。
  // 未初始化
  static const int not_inited = -1;
  int code = XSocketResponse<std::string>::not_inited;
  std::string message;
  std::string data;

  XSocketResponse() {}

  XSocketResponse(const XSocketResponse<std::string> &x)
          : content(x.content), code(x.code), message(x.message), data(x.data) {}

  XSocketResponse(std::vector<std::string> content_) : content(content_) {
    this->parse();
  }

  void parse() {
    if (!this->content.size() == 1)
      throw XSocketExceptionResponseParse("Data in content incorrect.");
    this->code = XSocketResponse<std::string>::not_inited;
    this->data = this->content[0];
  }
};

template<>
class XSocketResponse<Json::Value> {
public:
  std::vector<Json::Value> content;
  // 如果是Json格式而且是中断调用模式，数据会解析到下面。
  // 未初始化
  static const int not_inited = -1;
  int code = XSocketResponse<Json::Value>::not_inited;
  std::string message;
  Json::Value data;

  XSocketResponse() {}

  XSocketResponse(const XSocketResponse<Json::Value> &x)
          : content(x.content), code(x.code), message(x.message), data(x.data) {}

  XSocketResponse(std::vector<Json::Value> content_) : content(content_) {
    this->parse();
  }

  void parse() {
    if (!this->content.size() == 1)
      throw XSocketExceptionResponseParse("Data in content incorrect.");
    Json::Value top = this->content[0];
    if ((!top.isMember("code")) || (!top.isMember("data")))
      throw XSocketExceptionResponseParse("Some arguments are missing.");
    this->data = top["data"];
    this->code = top["code"].asInt();
    if (top.isMember("message")) this->message = top["message"].asString();
  }
};

template<class T>
class XSocket;

/*
 * @brief 回调使用的参数类
 * 调用回调的时候把数据放里头做参数吧...
 */
template<class T>
class XSocketCallingMessage {
public:
  XSocketResponse<T> response;
  // 缓冲区指针
  XSocket<T> *data = NULL;
  int code = 0;

  XSocketCallingMessage(int code_, XSocket<T> *data_,
                        XSocketResponse<T> response_)
          : data(data_), code(code_), response(response_) {}

  XSocketCallingMessage(int code_, XSocket<T> *data_)
          : data(data_), code(code_) {}
};

/*
 * @brief 事件处理类
 * 能进行基本的事件处理。
 */
template<class T>
class XEvents {
public:
  // 缓冲区指针
  XSocket<T> *data;

  XEvents(XSocket<T> *data_) : data(data_) {}

  // 记录函数指针，注意要强制转换调用。
  std::map<std::string, std::vector<void *>> callers;

  void listener_add(std::string name,
                    void (*listener)(XSocketCallingMessage<T>)) {
    if (!listener) return;
    if (!this->is_hooked(name)) this->callers[name] = std::vector<void *>();
    this->callers[name].push_back((void *) listener);
  }

  bool is_hooked(std::string on) {
    return this->callers.find(on) != this->callers.end();
  }

  void listener_remove(std::string name,
                       void (*listener)(XSocketCallingMessage<T>)) {
    if (!listener) return;
    if (!this->is_hooked(name)) return;
    auto ptr = std::find(this->callers[name].begin(), this->callers[name].end(),
                         listener);
    if (ptr == this->callers[name].end()) return;
    this->callers[name].erase(ptr);
  }

  void call(std::string name, XSocketCallingMessage<T> message) {
    if (this->callers.find(name) == this->callers.end()) return;
    for (auto ptr : this->callers[name]) {
      // 启动线程，然后分离
      std::thread((void (*)(XSocketCallingMessage<T>)) ptr, message).detach();
    }
  }

  // 空消息
  void call(std::string name, int code = 0) {
    this->call(name, XSocketCallingMessage<T>(code, this->data));
  }

  // 单条response消息
  void call(std::string name, T d, int code = 0) {
    std::vector<T> v = {d};
    XSocketCallingMessage<T> msg =
            XSocketCallingMessage<T>(code, this->data, XSocketResponse<T>(v));
    this->call(name, msg);
  }
};

/*
 * @brief XSocket的基类
 * 提供Json、std::string类型做缓冲区的模板。
 * 包含地址、缓冲区、缓冲区管理。
 */
template<class T>
class XSocket {
public:
  XSocketAddress *addr;
  // 缓冲区
  std::queue<T> data;
  // 缓冲区要加锁
  std::mutex lock;
  // 事件管理
  XEvents<T> *xevents;
  // 启动线程管理
  std::future<void> future;
  // 控制标志
  // 监听线程是否在运行
  bool running = false;

  XSocket(std::string ip, int port) {
    // 检查模板类型是否正确
    if (!(std::is_same<T, Json::Value>::value ||
          std::is_same<T, std::string>::value))
      throw XSocketExceptionTemplateUnallowed();
    this->addr = new XSocketAddress(ip, port);
    this->xevents = new XEvents<T>(this);
  }

  ~XSocket() {
    // 包含断开socket操作
    delete this->addr;
  }

  // 重启连接
  void reset() {
    this->running = false;
    // 注销事件
    delete this->xevents;
    this->xevents = new XEvents<T>(this);
    // 清空缓冲区
    while (!this->data_empty()) this->data_pop();
    // 断开连接
    std::string ip = this->addr->ip;
    int port = this->addr->port;
    delete this->addr;
    this->addr = new XSocketAddress(ip, port);
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

  void data_push(T d) {
    // LOG(INFO) << "locking writing data";
    this->lock.lock();
    this->data.push(d);
    this->lock.unlock();
    // LOG(INFO) << "locking writing data";
  }

  T data_pop() {
    // LOG(INFO) << "locking writing data";
    this->lock.lock();
    if (this->data_empty())
      throw XSocketExceptionData("Data is empty when poping.");
    T d = this->data.front();
    this->data.pop();
    this->lock.unlock();
    // LOG(INFO) << "locking writing data";
    return d;
  }

  void send_data(T d) {
    // this->lock.lock();
    if (std::is_same<T, std::string>::value) {
      this->addr->sock_write_string(d);
    } else {
    }
    // this->lock.unlock();
  }

  inline bool is_open() { return this->addr->sock_is_open(); }

  void stop() {
    if (!this->addr->sock_is_open()) {
      LOG(WARNING) << "unexcepted stop()!";
      return;
    }
    // 禁止写入
    // this->addr->lock.lock();
    // 用flag终止进程
    this->running = false;
    // 发送停止消息
    this->addr->sock_bye();
    // 等待对方的停止消息到达
    this->addr->sock_wait_exit();
    // this->addr->lock.unlock();
    // 然后中断传输，但是保留这边的缓冲区内容。
    this->addr->sock_shutdown();
    // this->addr->sock_close_all();
    // 再然后触发onclose事件
    this->xevents->call("onclose");
  }
};

// 工作模式枚举
uint8_t P2P = 1 << 0, P2M = 1 << 1, AR = 1 << 2;

/*
 * @brief XSocket的Server共用逻辑部分
 */
template<class T>
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
template<class T>
class XSocketServerP2P : public XSocketServer<T> {
public:
  XSocketServerP2P(std::string ip, int port) : XSocketServer<T>(ip, port) {
    this->stats |= P2P;
  }

  static void loop(XSocketServerP2P<T> *self) {
    self->running = true;
    while (self->running) {
      self->addr->sock_accept();
      self->xevents->call("onopen");
      while (true) {
        try {
          std::string data_str = self->addr->sock_read_string();
          // 在onmessage没有设置hook的时候写入缓冲区
          if (self->xevents->is_hooked("onmessage"))
            if (std::is_same<T, std::string>::value) {
              self->data_push(data_str);
            } else {
            }
          // LOG(INFO) << "ServerP2P: got string " << data_str;
          // 触发onmessage事件
          self->xevents->call("onmessage", data_str);
        } catch (XSocketExceptionConnectionWillClose e) {
          LOG(WARNING) << e.what();
          self->addr->opened = false;
          self->xevents->call("onclose");
          LOG(INFO) << "ServerLoop: sending exit code!";
          self->addr->sock_bye();
          break;
        } catch (XSocketExceptionReading e) {
          LOG(ERROR) << e.what();
          break;
        }
      }
      self->addr->sock_close_client();
      // usleep(1000000);
      LOG(INFO) << "Server connect restarting...";
    }
  }

  std::future<void> *start() {
    this->future = std::async(XSocketServerP2P::loop, this);
    return &this->future;
  }

  std::future<void> *restart() {
    this->reset();
    return this->start();
  }
};

/*
 * @brief XSocket的Client共用逻辑部分
 */
template<class T>
class XSocketClient : public XSocket<T> {
public:
  uint8_t stats;

  XSocketClient(std::string ip, int port) : XSocket<T>(ip, port) {
    this->prepare();
  }

  void prepare() {
    // throwifminus(this->addr->sock_bind_client(),
    //  XSocketExceptionPrepareSocket("Error binding."));
  }
  // 一个必须由子类实现的函数
  // static void client_loop();
};

/*
 * @brief P2P模式下的XSocketClient
 * 就叫他XSC好了。
 */
template<class T>
class XSocketClientP2P : public XSocketClient<T> {
public:
  XSocketClientP2P(std::string ip, int port) : XSocketClient<T>(ip, port) {
    this->stats |= P2P;
  }

  static void loop(XSocketClientP2P<T> *self) {
    self->running = true;
    while (self->running) {
      self->addr->mode_client();
      self->addr->sock_connect();
      self->xevents->call("onopen");
      while (true) {
        try {
          std::string data_str = self->addr->sock_read_string();
          // 写入缓冲区
          if (std::is_same<T, std::string>::value) {
            self->data_push(data_str);
          } else {
          }
          // LOG(INFO) << "ClientP2P: got string " << data_str;
          // 触发onmessage事件
          self->xevents->call("onmessage", data_str);
        } catch (XSocketExceptionConnectionWillClose e) {
          self->addr->opened = false;
          self->xevents->call("onclose");
          LOG(WARNING) << e.what();
          break;
        } catch (XSocketExceptionReading e) {
          LOG(ERROR) << e.what();
          break;
        }
      }
      self->addr->sock_close_client();
      // usleep(1000000);
      LOG(WARNING) << "Client restarting...";
    }
    LOG(WARNING) << "Client listening thread exiting...";
  }

  std::future<void> *start() {
    this->future = std::async(XSocketClientP2P::loop, this);
    return &this->future;
  }

  std::future<void> *restart() {
    this->reset();
    // 等待线程loop线程关闭然后再重启这个线程
    while (this->future.wait_for(std::chrono::milliseconds(0)) !=
           std::future_status::ready)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return this->start();
  }
};
