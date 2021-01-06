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
                   void (*timer)(XSocketServerP2P<std::string> *),
                   XSocketServerP2P<std::string> *arg) {
  int count = -1;
  while (count) {
    std::this_thread::sleep_for(std::chrono::milliseconds(utime));
    timer(arg);
    if (count > 0) count--;
  }
}

void looper_client(unsigned int utime,
                   void (*timer)(XSocketClientP2P<std::string> *),
                   XSocketClientP2P<std::string> *arg) {
  int count = 10;
  while (count) {
    std::this_thread::sleep_for(std::chrono::milliseconds(utime));
    timer(arg);
    if (count > 0) count--;
  }
  // 由Client结束
  arg->stop();
  LOG(INFO) << "Client: I will end this connection!";
}

void server_sender(XSocketServerP2P<std::string> *self) {
  static int cnt = 1;
  try {
    self->send_data(std::string("DATA FROM SERVER!!!") + std::to_string(cnt++) + std::string("\n"));
  } catch (XSocketExceptionWriting e) {
    LOG(WARNING) << "Server: " << e.what();
  } catch (XSocketExceptionConnectionClosed e) {
    // LOG(WARNING) << "Server: " << e.what();
  }
}
void client_sender(XSocketClientP2P<std::string> *self) {
  static int cnt = 1;
  try {
    self->send_data(std::string("\tData from Client...") + std::to_string(cnt++) + std::string("\n"));
  } catch (XSocketExceptionWriting e) {
    LOG(WARNING) << "Client: " << e.what();
  } catch (XSocketExceptionConnectionClosed e) {
    // LOG(WARNING) << "Client: " << e.what();
  }
}

void onclose(XSocketCallingMessage<std::string> *msg) {
  // 再启动
  LOG(INFO) << "XEvent: onclose";
  delete xsc;
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
  std::thread(looper_server, 0, server_sender, xss).detach();
  xsc->xevents->listener_add("onmessage", onmessage);
  xsc->xevents->listener_add("onclose", onclose);
  std::thread(looper_client, 0, client_sender, xsc).detach();
  // std::this_thread::sleep_for(std::chrono::milliseconds(10));
  std::this_thread::sleep_for(std::chrono::milliseconds(10000));
  exit(0);
  return 0;
}
