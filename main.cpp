#include <glog/logging.h>
#include <xsocket.h>

#include <csignal>
#include <cstdlib>
#include <ctime>
#include <iostream>

// std::future<void> server_loop(int port);
// std::future<void> client_loop(int port);
std::thread server_loop(int port);
std::thread client_loop(int port);

void on_init(const char *cmd) {
  FLAGS_alsologtostderr = 1;
  google::InitGoogleLogging(cmd);
}

int main(int argc, char **argv) {
  on_init(argv[0]);
  LOG(INFO) << "Hey my friend! Wish you happy to use XSocket!";
  srand(time(NULL));
  int port = rand() % 3000 + 5000;
  std::thread thread_server = server_loop(port);
  usleep(100000);
  std::thread thread_client = client_loop(port);
  thread_server.join();
  return 0;
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
  int count = -1;
  while (count) {
    usleep(utime);
    timer(arg);
    if (count > 0) count--;
  }
}

void server_sender(XSocketServerP2P<std::string> *self) {
  LOG(INFO) << ">";
  try {
    self->send_data("DATA FROM SERVER!!!\n");
  } catch (XSocketExceptionWriting e) {
    LOG(WARNING) << "Server: XSocketExceptionWriting " << e.what();
  }
  LOG(INFO) << "->";
}
void client_sender(XSocketClientP2P<std::string> *self) {
  LOG(INFO) << "<";
  try {
    self->send_data("Data from Client...\n");
  } catch (XSocketExceptionWriting e) {
    LOG(WARNING) << "Client: XSocketExceptionWriting " << e.what();
  }
  LOG(INFO) << "<-";
}

std::thread server_loop(int port) {
  XSocketServerP2P<std::string> *xss =
      new XSocketServerP2P<std::string>("127.0.0.1", port);
  new std::thread(looper_server, 1000000, server_sender, xss);
  return xss->server_loop_start();
}

std::thread client_loop(int port) {
  XSocketClientP2P<std::string> *xsc =
      new XSocketClientP2P<std::string>("127.0.0.1", port);
  new std::thread(looper_client, 100000, client_sender, xsc);
  return xsc->client_loop_start();
}