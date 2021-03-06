#include <glog/logging.h>
#include <mysocket.h>

#include <csignal>
#include <cstdlib>
#include <ctime>
#include <iostream>

// #include "./mysocket/includes/mysocket.h"
// #include "mysocket/src/mysocket.cpp"

void OnInit(const char* cmd) {
  // 调整输出等级
  // FLAGS_alsologtostderr = 4;
  FLAGS_alsologtostderr = 1;
  google::InitGoogleLogging(cmd);
}

// 放在全局是因为让匿名函数好操作
// 当然可以放在局部
MyServer* server;
MyClient* client;

void server_loop(int port) {
  // 建立MyServer实例
  server = new MyServer(std::string("127.0.0.1"), port);
  server->onmessage = [](Json::Value message) {
    // LOG(INFO) << "Server got: " << message;
    // 复读
    // server->send(message);
    // LOG(INFO) << "Server got new message";
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
  std::future<void> fu = std::async([]() {
    while (true) {
      int t = rand() % 3 + 1;
      sleep(t);
      Json::Value val;
      val["message"] = std::string("I slept for ") + std::to_string(t) +
                       std::string(" second(s)!");
      server->send(val);
    }
  });
  int ret = server->future_mainloop.get();
  LOG(INFO) << "Got result: " << ret;
}

int cnt = 0;
int cnt_err = 0;

void client_loop(int port) {
  // 同样建立实例
  sleep(1);
  client = new MyClient(std::string("127.0.0.1"), port);
  // Client改成同步状态
  client->onmessage = [](Json::Value message) {
    LOG(INFO) << "Cilent got: " << message;
  };
  Json::Value val = message_parser("{\"value\": 0}");
  while (true) {
    while (client->sock_client < 0) {
      try {
        client->start();
        LOG(INFO) << "Client started";
      } catch (MySocket::ExceptionCreateConnect) {
        LOG(WARNING) << "Cilent can not create connection, retrying";
        sleep(1);
        continue;
      }
      break;
    }

    try {
      // val[(std::string("value") + std::to_string(cnt)).c_str()] = cnt++;
      val["value"] = cnt++;
      client->send(val);
      // LOG(INFO) << "Cilent sent: " << val;
      usleep(10000);
      // sleep(1);
    } catch (MySocket::ExceptionReading) {
      LOG(ERROR) << "Client meets exception reading";
      cnt_err++;
      LOG(ERROR) << "request#" << cnt
                 << "\terror rate: " << (double)cnt_err / cnt;
      // usleep(1000000);
      // usleep(10000);
      sleep(2);
      continue;
    } catch (MySocket::ExceptionCreateConnect) {
      // Never reach here... Maybe
      sleep(1);
      continue;
    }
  }
}

void sig_handler(int sig) {
  if (sig == SIGINT) {
    // delete client;
    // delete server;
  }
}

int main(int argc, char** argv) {
  // signal(SIGINT, sig_handler);
  OnInit(argv[0]);
  LOG(INFO) << "This is socket test main module!";
  srand(time(NULL));
  int port = rand() % 3000 + 5000;
  std::future<void> future_server = std::async(server_loop, port);
  usleep(1000000);
  std::future<void> future_client = std::async(client_loop, port);
  future_server.get();
  future_client.get();
  return 0;
}
