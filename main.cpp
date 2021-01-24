#include <glog/logging.h>
#include <xsocket.h>

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <iostream>

#define MODE_SLOW

#ifdef MODE_SLOW
const int time_loop_server = 100;
const int time_loop_client = 100;
const int time_loop_main = 1000;
#else
const int time_loop_server = 0;
const int time_loop_client = 0;
const int time_loop_main = 1000;
#endif

XSocketServerP2P<std::string> *xss;
XSocketClientP2P<std::string> *xsc;
int port;
// std::string ip = "0.0.0.0";
std::string ip = "127.0.0.1";
std::future<void> future_server, future_client;

void on_init(const char *cmd) {
  FLAGS_alsologtostderr = 1;
  google::InitGoogleLogging(cmd);
}

void onmessage(XSocketCallingMessage<std::string> msg) {
  std::string str = msg.response.data;
  LOG(INFO) << "[" << msg.data << "](" << msg.code << ")" << str;
}

bool running_client = false;
bool running_server = false;

void looper_server(unsigned int utime,
                   int (*timer)(XSocketServerP2P<std::string> *),
                   XSocketServerP2P<std::string> *arg) {
  int count = -1;
  running_server = true;
  while (count && running_server) {
    std::this_thread::sleep_for(std::chrono::milliseconds(utime));
    int ret = timer(arg);
    if (ret != 2)
      LOG(INFO) << "looper_server(): count = " << count
                << ", ret = " << ret;
    if (count > 0) count--;
  }
  running_server = false;
}

void looper_client(unsigned int utime,
                   int (*timer)(XSocketClientP2P<std::string> *),
                   XSocketClientP2P<std::string> *arg) {
//  int count = 10;
  int count = -1;
  running_client = true;
  while (count && running_client) {
    std::this_thread::sleep_for(std::chrono::milliseconds(utime));
    int ret = timer(arg);
    LOG(INFO) << "looper_client(): count = " << count
              << ", ret = " << ret;
    if (count > 0) count--;
  }
//  // 由Client结束
//  LOG(INFO) << "Client: I will end this connection!";
//  try {
//    arg->stop();
//  } catch (XSocketExceptionConnectionClosed e) {
//    LOG(INFO) << "stop()ing..." << e.what();
//  }
  running_client = false;
}

int server_sender(XSocketServerP2P<std::string> *self) {
  static int cnt = 1;
  try {
    self->send_data(std::string("DATA FROM SERVER!!!") + std::to_string(cnt++) +
                    std::string("\n"));
  } catch (XSocketExceptionWriting e) {
    LOG(WARNING) << "Server: " << e.what();
    cnt--;
    return 1;
  } catch (XSocketExceptionConnectionClosed e) {
    // LOG(WARNING) << "Server: " << e.what();
    cnt--;
    return 2;
  }
  return 0;
}

int client_sender(XSocketClientP2P<std::string> *self) {
  static int cnt = 1;
  try {
    self->send_data(std::string("\tData from Client...") +
                    std::to_string(cnt++) + std::string("\n"));
  } catch (XSocketExceptionWriting e) {
    LOG(WARNING) << "Client: " << e.what();
    cnt--;
    return 1;
  } catch (XSocketExceptionConnectionClosed e) {
    // LOG(WARNING) << "Client: " << e.what();
    cnt--;
    return 2;
  }
  return 0;
}

void onclose(XSocketCallingMessage<std::string> msg) {
//  // 再启动
  LOG(INFO) << "XEvent: onclose";
//  LOG(INFO) << "Client: starting new...";
//  std::this_thread::sleep_for(std::chrono::milliseconds(50));
//  xsc->restart();
//  LOG(INFO) << "Client: restarted!";
//  // 等待sender进程退出
//  while (future_client.wait_for(std::chrono::milliseconds(0)) !=
//         std::future_status::ready) {
//    std::this_thread::sleep_for(std::chrono::milliseconds(1));
//  }
//  future_client =
//      std::async(looper_client, time_loop_client, client_sender, xsc);
//  xsc->xevents->listener_add("onmessage", onmessage);
//  xsc->xevents->listener_add("onclose", onclose);
}

int main(int argc, char **argv) {
  on_init(argv[0]);
  LOG(INFO) << "Hey my friend! Wish you happy to use XSocket!";
  srand(time(NULL));
  port = rand() % 3000 + 5000;
  xss = new XSocketServerP2P<std::string>(ip, port);
  xss->start();
  xsc = new XSocketClientP2P<std::string>(ip, port);
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  xsc->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  xss->xevents->listener_add("onmessage", onmessage);
  future_server =
          std::async(looper_server, time_loop_server, server_sender, xss);
  xsc->xevents->listener_add("onmessage", onmessage);
  xsc->xevents->listener_add("onclose", onclose);
  future_client =
          std::async(looper_client, time_loop_client, client_sender, xsc);
  uint64_t running_time = 0;
  while (running_time++ < 0xFFFFFFFF) {
    std::this_thread::sleep_for(std::chrono::milliseconds(time_loop_main));
    // 等待looper停止
    running_client = false;
    // 等待sender进程退出
    while (future_client.wait_for(std::chrono::milliseconds(0)) !=
           std::future_status::ready) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    LOG(INFO) << "Client: I will end this connection!";
    try {
      LOG(INFO) << "Client: trying to stop...";
      xsc->stop();
    } catch (XSocketExceptionConnectionClosed e) {
      LOG(INFO) << "stop()ing..." << e.what();
    }
    LOG(INFO) << "Client: trying to restart...";
    xsc->restart();
    future_client =
            std::async(looper_client, time_loop_client, client_sender, xsc);
    LOG(INFO) << "Client: looper started ok.";
    xsc->xevents->listener_add("onmessage", onmessage);
    xsc->xevents->listener_add("onclose", onclose);
    LOG(INFO) << "Client: restarted ok.";
  }
//  exit(0);
//  return 0;
}
