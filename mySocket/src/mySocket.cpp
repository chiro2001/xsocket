// #include <mySocket.h>
#include "../includes/mySocket.h"
// mySocket.cpp
// 实现两个功能：服务端和终端

class mySocket {
 private:
  std::string to_string() { return this->ip + ":" + std::to_string(this->port); }
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
    this->addr.sin_addr.s_addr = inet_addr(ip.c_str());
    this->addr.sin_port = htons(port);
    LOG(INFO) << "Created socket address at " << ip <<
  }

 public:
  // 共用一个地址信息
  struct sockaddr_in addr;
  std::string ip;
  int port;

  mySocket(std::string ip = "127.0.0.1", int port = 14514) {}
  class Server {};
  class Client {};
};