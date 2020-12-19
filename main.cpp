#include <iostream>
// #include <mySocket.h>
#include <glog/logging.h>

#include "./mysocket/includes/mysocket.h"
// #include "mysocket/src/mysocket.cpp"

void OnInit(const char* cmd) {
  // 调整输出等级
  FLAGS_alsologtostderr = 1;
  google::InitGoogleLogging(cmd);
}

int main(int argc, char** argv) {
  OnInit(argv[0]);
  LOG(INFO) << "This is socket test main module!";
  // 建立mySocket实例，直接用默认参数好了
  MyServer* server = (new MyServer())->start();
}

