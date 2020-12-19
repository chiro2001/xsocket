#include <iostream>
// #include <mysocket.h>
#include <glog/logging.h>

#include "./mysocket/includes/mysocket.h"
// #include "mysocket/src/mysocket.cpp"

void OnInit(const char* cmd) {
  // 调整输出等级
  FLAGS_alsologtostderr = 1;
  google::InitGoogleLogging(cmd);
}

// 放在全局是因为让匿名函数好操作
// 当然可以放在局部
MyServer* server;
MyClient* client;

void server_loop() {
  // 建立MyServer实例，直接用默认参数好了
  server = new MyServer();
  server->onmessage = [](Json::Value message) {
    LOG(INFO) << "Server got: " << message;
    // 复读
    server->send(message);
  };
  while (true) {
    try {
      server->start();
      LOG(INFO) << "Server started";
    } catch (MySocket::ExceptionCreateConnect) {
      sleep(1);
      continue;
    }
    break;
  }
  int ret = server->future_mainloop.get();
  LOG(INFO) << "Got result: " << ret;
}

void client_loop() {
  // 同样建立实例
  sleep(1);
  client = new MyClient();
  // Client目前还是一发一收状态
  client->onmessage = [](Json::Value message) {
    LOG(INFO) << "Cilent got: " << message;
    client->do_close();
    client->start();
  };
  while (true) {
    try {
      client->start();
      LOG(INFO) << "Client started";
    } catch (MySocket::ExceptionCreateConnect) {
      sleep(1);
      continue;
    }
    break;
  }
  Json::Value val = message_parser("{\"value\": 1}");
  int cnt = 1;
  while (true) {
    try {
      val["value"] = cnt++;
      client->send(val);
      LOG(INFO) << "Cilent sent: " << val;
      client->recv();
      client->do_close();
      client->start();
      usleep(100000);
      // sleep(2);
    } catch (MySocket::ExceptionReading) {
      LOG(WARNING) << "Client meets exception reading";
      usleep(100000);
      continue;
    }
  }
}

int main(int argc, char** argv) {
  OnInit(argv[0]);
  LOG(INFO) << "This is socket test main module!";
  std::future<void> future_server = std::async(server_loop);
  for (unsigned long long i = 0; i < 1 << 25; i++)
    ;
  std::future<void> future_client = std::async(client_loop);
  future_server.get();
  future_client.get();
  return 0;
}
