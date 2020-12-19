#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <json/json.h>

#include <iostream>


int main() {
  //创建套接字
  int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  //将套接字和IP、端口绑定
  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));  //每个字节都用0填充
  serv_addr.sin_family = AF_INET;            //使用IPv4地址
  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  //具体的IP地址
  serv_addr.sin_port = htons(1234);                    //端口
  bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

  //进入监听状态，等待用户发起请求
  listen(serv_sock, 20);

  //接收客户端请求
  struct sockaddr_in clnt_addr;
  socklen_t clnt_addr_size = sizeof(clnt_addr);
  int clnt_sock;
  char buf[4096] = "";
  while (true) {
    clnt_sock =
        accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    if (clnt_sock < 0) {
      std::cout << "Err." << std::endl;
      break;
    }
    // 接收客户端的信息
    int recv_n = recv(clnt_sock, buf, 4096, 0);
    if (recv_n < 0) {
      std::cout << "Err." << std::endl;
      break;
    }
    //向客户端发送数据
    std::string str_to_send =
        std::string("Recevied ") + std::to_string(recv_n) + "Bytes!";
    write(clnt_sock, str_to_send.c_str(), str_to_send.length());
  }

  //关闭套接字
  close(clnt_sock);
  close(serv_sock);

  return 0;
}
