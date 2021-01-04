#include <glog/logging.h>
#include <xsocket.h>

#include <csignal>
#include <cstdlib>
#include <ctime>
#include <iostream>

void server_loop(int port);
void client_loop(int port);

void on_init(const char *cmd) {
  FLAGS_alsologtostderr = 1;
  google::InitGoogleLogging(cmd);
}

int main(int argc, char **argv) {
  on_init(argv[0]);
  LOG(INFO) << "Hey my friend! Wish you happy to use XSocket!";
  srand(time(NULL));
  int port = rand() % 3000 + 5000;
  std::future<void> future_server = std::async(server_loop, port);
  usleep(1000000);
  std::future<void> future_client = std::async(client_loop, port);
  future_server.get();
  future_client.get();
  return 0;
}

void looper_server(unsigned int utime,
                   void (*timer)(XSocketServerP2P<std::string> *),
                   XSocketServerP2P<std::string> *arg) {
  int count = -1;
  while (count) {
    timer(arg);
    usleep(utime);
    if (count > 0) count--;
  }
}

void looper_client(unsigned int utime,
                   void (*timer)(XSocketClientP2P<std::string> *),
                   XSocketClientP2P<std::string> *arg) {
  int count = -1;
  while (count) {
    timer(arg);
    usleep(utime);
    if (count > 0) count--;
  }
}

void server_sender(XSocketServerP2P<std::string> *self) {
  self->send_data("DATA FROM SERVER!!!\n");
}
void client_sender(XSocketClientP2P<std::string> *self) {
  self->send_data("Data from Client...\n");
}

void server_loop(int port) {
  XSocketServerP2P<std::string> xss("120.0.0.1", port);
  std::async(looper_server, 1000000, server_sender, &xss);
  xss.server_loop_start().get();
}

void client_loop(int port) {
  XSocketClientP2P<std::string> xsc("120.0.0.1", port);
  std::async(looper_client, 100000, client_sender, &xsc);
  xsc.client_loop_start().get();
}