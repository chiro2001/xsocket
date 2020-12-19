#pragma once
#include <arpa/inet.h>
#include <glog/logging.h>
#include <json/json.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <future>

#define MY_SOCKET_BUFSIZE (1 << 16)

class MySocket {
 private:
  void config_addr() {
    if (this->ip.length() == 0 || this->port <= 0) {
      LOG(ERROR) << "Unable to create socket address at " << this->to_string();
      return;
    }
    // 创建套接字
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // 设置地址
    memset(&this->addr, 0, sizeof(this->addr));
    this->addr.sin_family = AF_INET;
    this->addr.sin_addr.s_addr = inet_addr(this->ip.c_str());
    this->addr.sin_port = htons(port);
    LOG(INFO) << "Created socket address at " << this->to_string();
  }

 public:
  // 共用一个地址信息
  struct sockaddr_in addr;
  std::string ip;
  int port;

  // 创建好的套接字
  int sock_server = -1;
  int sock_client = -1;

  // 回调部分
  void (*onopen)(void) = NULL;
  void (*onerror)(void) = NULL;
  // 在数据发送和接收端检查、修改一下数据
  void (*message)(Json::Value) = NULL;
  void (*onsend)(Json::Value) = NULL;
  // send，Server和Client实现方法不一样
  void send(Json::Value);

  std::string to_string() {
    return this->ip + ":" + std::to_string(this->port);
  }

  void do_close() {
    if (this->sock_client > 0) close(this->sock_client);
    if (this->sock_server > 0) close(this->sock_server);
  }
  MySocket(std::string ip = "127.0.0.1", int port = 14514) {
    this->ip = ip, this->port = port;
  }
  ~MySocket() {
    // 关闭连接
    this->do_close();
  }
};

class MyServer : public MySocket {
 public:
  // 标记：是否已经开始监听
  bool mark_started = false;
  bool mark_running = false;
  std::future<int> future_mainloop;

  MyServer* start();
  void stop();
  void send(Json::Value);
};
class MyClient : public MySocket {
 public:
  MyClient* start();
  void send(Json::Value);
};