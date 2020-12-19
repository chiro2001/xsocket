// #include <mySocket.h>
#include "../includes/mysocket.h"
// mySocket.cpp
// 实现两个功能：服务端和终端

// Server
int mainloop(MyServer* ms) {
  // 进入监听状态
  int ret = listen(ms->sock_server, 20);
  if (ret < 0) {
    LOG(ERROR) << "Can not listen on " << ms->to_string();
    if (ms->onerror) ms->onerror();
    return -1;
  }
  struct sockaddr_in addr;
  socklen_t addr_size = sizeof(addr);
  char *buf = (char*)malloc(sizeof(char) * MY_SOCKET_BUFSIZE);
  if (!buf) {
    LOG(ERROR) << "Can not malloc memory of " << std::to_string(MY_SOCKET_BUFSIZE);
    if (ms->onerror) ms->onerror();
    return -1;
  }
  while (ms->mark_running) {
    if (ms->sock_client > 0) close(ms->sock_client);
    ms->sock_client = accept(ms->sock_server, (struct sockaddr_in*)&addr, &addr_size);
  }
  ms->stop();
}

MyServer* MyServer::start() {
  if (this->mark_started) return this;
  if (this->sock_server > 0) close(this->sock_server);
  // 创建套接字
  this->sock_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  // 绑定对应地址
  bind(this->sock_server, (struct sockaddr*)&this->addr,
       sizeof(struct sockaddr));
  // 开启主线程
  // 线程对应函数必须是静态的方法...
  this->future_mainloop = std::async(mainloop, this);
  this->mark_started = true;
  this->mark_running = true;
  return this;
}

// 会等待mainloop来调用
void MyServer::stop() {
  if (this->mark_running) {
    this->mark_running = false;
    return;
  }
  this->do_close();
}
