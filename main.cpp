#include <iostream>
#include <glog/logging.h>
// #include <mySocket.h>
#include "./mySocket/includes/mySocket.h"


void OnInit(const char* cmd) {
  google::InitGoogleLogging(cmd);
}

int main(int argc, char** argv) {
    std::cout << "Hello, world!\n";
}
