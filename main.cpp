#include <glog/logging.h>
#include <xsocket.h>

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <iostream>

XSocketServerP2P<std::string> *xss;
XSocketClientP2P<std::string> *xsc;
int port;
std::string ip = "0.0.0.0";

void on_init(const char *cmd) {
  FLAGS_alsologtostderr = 1;
  google::InitGoogleLogging(cmd);
}

void onmessage(XSocketCallingMessage<std::string> *msg) {
  std::string str = msg->response->data;
  LOG(INFO) << "[" << msg->data << "](" << msg->code << ")" << str;
}

void looper_server(unsigned int utime,
                   int (*timer)(XSocketServerP2P<std::string> *),
                   XSocketServerP2P<std::string> *arg) {
  int count = -1;
  while (count) {
    std::this_thread::sleep_for(std::chrono::milliseconds(utime));
    timer(arg);
    if (count > 0) count--;
  }
}

void looper_client(unsigned int utime,
                   int (*timer)(XSocketClientP2P<std::string> *),
                   XSocketClientP2P<std::string> *arg) {
  int count = 10;
  while (count) {
    std::this_thread::sleep_for(std::chrono::milliseconds(utime));
    LOG(INFO) << "looper_client(): count = " << count << ", ret = " << timer(arg);
    if (count > 0) count--;
  }
  // 由Client结束
  LOG(INFO) << "Client: I will end this connection!";
  try {
    arg->stop();
  } catch (XSocketExceptionConnectionClosed e) {
    LOG(INFO) << "stop()ing..." << e.what();
  }
}

int server_sender(XSocketServerP2P<std::string> *self) {
  static int cnt = 1;
  try {
    self->send_data(std::string("DATA FROM SERVER!!!") + std::to_string(cnt++) +
                    std::string("\n"));
  } catch (XSocketExceptionWriting e) {
    LOG(WARNING) << "Server: " << e.what();
    return 1;
  } catch (XSocketExceptionConnectionClosed e) {
    LOG(WARNING) << "Server: " << e.what();
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
    return 1;
  } catch (XSocketExceptionConnectionClosed e) {
    LOG(WARNING) << "Client: " << e.what();
    return 2;
  }
  return 0;
}

void onclose(XSocketCallingMessage<std::string> *msg) {
  // 再启动
  LOG(INFO) << "XEvent: onclose";
  if (xsc) {
    std::thread(onclose, msg).detach();
    auto tmp = xsc;
    xsc = NULL;
    delete tmp;
  }
  LOG(INFO) << "Client: starting new...";
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  xsc = new XSocketClientP2P<std::string>(ip, port);
  xsc->start();
  std::thread(looper_client, 0, client_sender, xsc).detach();
  xsc->xevents->listener_add("onmessage", onmessage);
  xsc->xevents->listener_add("onclose", onclose);
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
  std::thread(looper_server, 500, server_sender, xss).detach();
  xsc->xevents->listener_add("onmessage", onmessage);
  xsc->xevents->listener_add("onclose", onclose);
  std::thread(looper_client, 100, client_sender, xsc).detach();
  // std::this_thread::sleep_for(std::chrono::milliseconds(10));
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  exit(0);
  return 0;
}
