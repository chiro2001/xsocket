// #include <mySocket.h>
#include "../includes/mysocket.h"
// mySocket.cpp
// 实现两个功能：服务端和终端

// 按照格式解析字符串到Json
// 格式：
/*
{
  "type": "client",   // 指示了发出信息的地方
  "message": "...",
  "code": 0,
  "data": {...}
}

{
  "type": "server",
  "message": "...",
  "code": 0,
  "data": {...}
}
*/
Json::Value message_parser(std::string src) {
  Json::Reader reader;
  Json::Value root;
  if (!reader.parse(src, root)) throw MyServer::ExceptionParseJson();
}

// Server
int mainloop(MyServer* ms) {
  // 进入监听状态
  int ret = listen(ms->sock_server, 20);
  if (ret < 0) {
    LOG(ERROR) << "MyServer: Can not listen on " << ms->to_string();
    CALL_IF_EXIST(ms->onerror);
    return -1;
  }
  // 客户端地址
  struct sockaddr_in addr;
  socklen_t addr_size = sizeof(addr);
  // 输入缓冲区
  char* buf = (char*)malloc(sizeof(char) * MY_SOCKET_BUFSIZE);
  // 接收到的字节数
  int recv_n = 0;
  if (!buf) {
    LOG(ERROR) << "MyServer: Can not malloc memory of "
               << std::to_string(MY_SOCKET_BUFSIZE);
    CALL_IF_EXIST(ms->onerror);
    return -1;
  }
  while (ms->mark_running) {
    LOG(INFO) << "while...";
    if (ms->sock_client > 0) close(ms->sock_client);
    ms->sock_client =
        accept(ms->sock_server, (struct sockaddr*)&addr, &addr_size);
    // 在这里出错是客户端的锅，咱们不背。
    if (ms->sock_client < 0) {
      LOG(ERROR) << "MyServer: Can not accept client connnecttion";
      CALL_IF_EXIST(ms->onclienterror);
      continue;
    }
    // 接收信息
    recv_n = recv(ms->sock_client, buf, MY_SOCKET_BUFSIZE, 0);
    if (recv_n <= 0) {
      LOG(ERROR) << "MyServer: Can not receive data from client";
      CALL_IF_EXIST(ms->onclienterror);
      continue;
    }
    // 转换信息的格式到JSON
    Json::Value root;
    try {
      root = message_parser(std::string(buf));
    } catch (MyServer::ExceptionParseJson) {
      LOG(ERROR) << "MyServer: Data from client format error";
      CALL_IF_EXIST(ms->onclienterror);
      continue;
    }
    CALL_IF_EXIST_A(ms->onmessage, root);
  }
  ms->stop();
}

MyServer* MyServer::start() {
  if (this->mark_started) return this;
  if (this->sock_server > 0) close(this->sock_server);
  // 创建套接字
  this->sock_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (this->sock_server < 0 && this->onerror) {
    LOG(ERROR) << "MyServer: Can not create socket";
    if (this->onerror) this->onerror();
    return this;
  }
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
