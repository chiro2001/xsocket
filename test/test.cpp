#include <iostream>
using namespace std;

#ifndef __unix__
#include <Windows.h>
#include <stdio.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")  //加载 ws2_32.dll

long unsigned int server(void *) {
  //初始化 DLL
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);

  //创建套接字
  SOCKET servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  //绑定套接字
  sockaddr_in sockAddr;
  memset(&sockAddr, 0, sizeof(sockAddr));  //每个字节都用0填充
  sockAddr.sin_family = PF_INET;           //使用IPv4地址
  sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  //具体的IP地址
  sockAddr.sin_port = htons(1234);                    //端口
  bind(servSock, (SOCKADDR *)&sockAddr, sizeof(SOCKADDR));

  //进入监听状态
  listen(servSock, 20);

  //接收客户端请求
  SOCKADDR clntAddr;
  int nSize = sizeof(SOCKADDR);
  SOCKET clntSock = accept(servSock, (SOCKADDR *)&clntAddr, &nSize);

  //向客户端发送数据
  char *str = "Hello World!";
  send(clntSock, str, strlen(str) + sizeof(char), NULL);

  //关闭套接字
  closesocket(clntSock);
  closesocket(servSock);

  //终止 DLL 的使用
  WSACleanup();

  return 0;
}

long unsigned int client(void *) {
  //初始化DLL
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
  //创建套接字
  SOCKET sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  //向服务器发起请求
  sockaddr_in sockAddr;
  memset(&sockAddr, 0, sizeof(sockAddr));  //每个字节都用0填充
  sockAddr.sin_family = PF_INET;
  sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  sockAddr.sin_port = htons(1234);
  connect(sock, (SOCKADDR *)&sockAddr, sizeof(SOCKADDR));
  //接收服务器传回的数据
  char szBuffer[MAXBYTE] = {0};
  recv(sock, szBuffer, MAXBYTE, NULL);
  //输出接收到的数据
  printf("Message form server: %s\n", szBuffer);
  //关闭套接字
  closesocket(sock);
  //终止使用 DLL
  WSACleanup();
  system("pause");
  return 0;
}

int main() {
  HANDLE thread_server, thread_client;
  DWORD id;
  CreateThread(NULL, 0, server, 0, 0, &id);
  Sleep(500);
  CreateThread(NULL, 0, client, 0, 0, &id);
  return 0;
}

#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <glog/logging.h>
#include <json/json.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  // 测试：创建socket之后不连接直接关闭
  // 创建套接字
  struct sockaddr_in addr;
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_UDP);
  // int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  // 设置地址
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  addr.sin_port = htons(1234);
  // bind(sock_server, (struct sockaddr *)&addr, sizeof(struct sockaddr));
  close(sock);
  puts("DONE");
  // 测试结果：成功，可以关闭
}
#endif