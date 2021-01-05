#include <glog/logging.h>
#include <xsocket.h>

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <iostream>

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
    usleep(utime);
    timer(arg);
    if (count > 0) count--;
  }
}

void looper_client(unsigned int utime,
                   void (*timer)(XSocketClientP2P<std::string> *),
                   XSocketClientP2P<std::string> *arg) {
  int count = 10;
  while (count) {
    usleep(utime);
    timer(arg);
    if (count > 0) count--;
  }
  // 由Client结束
  arg->stop();
  LOG(INFO) << "Client: I will end this connection!";
}

void server_sender(XSocketServerP2P<std::string> *self) {
  try {
    self->send_data("DATA FROM SERVER!!!\n");
  } catch (XSocketExceptionWriting e) {
    LOG(WARNING) << "Server: " << e.what();
  }
}
void client_sender(XSocketClientP2P<std::string> *self) {
  try {
    self->send_data("Data from Client...\n");
  } catch (XSocketExceptionWriting e) {
    LOG(WARNING) << "Client: " << e.what();
  }
}

int main(int argc, char **argv) {
  on_init(argv[0]);
  LOG(INFO) << "Hey my friend! Wish you happy to use XSocket!";
  srand(time(NULL));
  int port = rand() % 3000 + 5000;
  std::string ip = "0.0.0.0";
  XSocketServerP2P<std::string> *xss =
      new XSocketServerP2P<std::string>(ip, port);
  xss->start();
  XSocketClientP2P<std::string> *xsc =
      new XSocketClientP2P<std::string>(ip, port);
  xsc->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  xss->xevents->listener_add("onmessage", onmessage);
  std::thread(looper_server, 0, server_sender, xss).detach();
  xsc->xevents->listener_add("onmessage", onmessage);
  std::thread(looper_client, 0, client_sender, xsc).detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  // 再启动
  delete xsc;
  xsc = new XSocketClientP2P<std::string>(ip, port);
  xsc->start();
  std::thread(looper_client, 0, client_sender, xsc).detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  exit(0);
  return 0;
}
