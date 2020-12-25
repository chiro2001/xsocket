#include <glog/logging.h>
#include <xsocket.h>

#include <iostream>

void on_init(const char *cmd) {
  FLAGS_alsologtostderr = 1;
  google::InitGoogleLogging(cmd);
}

int main(int argc, char **argv) {
  on_init(argv[0]);
  LOG(INFO) << "Hey my friend! Wish you happy to use Xsocket!";
  return 0;
}

/*
 * 计划接口使用：
 * 1. 初始化地址
 * 2. 添加回调
 * 3. 运行start()直接开始监听
 * 4. 运行stop()结束监听
 */