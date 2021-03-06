#include <arpa/inet.h>
#include <json/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

int main() {
  //创建套接字
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  //向服务器（特定的IP和端口）发起请求
  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));  //每个字节都用0填充
  serv_addr.sin_family = AF_INET;            //使用IPv4地址
  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  //具体的IP地址
  serv_addr.sin_port = htons(8001);                   //端口
  // serv_addr.sin_port = htons(1234);                    //端口
  int conn = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
  if (conn < 0) {
    std:: cout << "Cannot create connection!" << std::endl;
    return 0;
  }

  // 给服务器发送数据
  std::string str =
      "{\n\
  \"type\": \"client\",\n\
  \"message\": \"...\",\n\
  \"code\": 0,\n\
  \"data\": \n\
  {\"hd\": 1}\n\
}";
  write(sock, str.c_str(), str.length());
  //读取服务器传回的数据
  char buffer[4096];
  read(sock, buffer, sizeof(buffer));

  printf("Message form server: %s\n", buffer);

  //关闭套接字
  close(sock);

  return 0;
}
