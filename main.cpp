#include <glog/logging.h>
#include <xsocket.h>

#include <iostream>

void on_init(const char *cmd) {
  FLAGS_alsologtostderr = 1;
  google::InitGoogleLogging(cmd);
}

int main(int argc, char **argv) {
  on_init(argv[0]);
  LOG(INFO) << "Hey my friend! Wish you happy to use Xsocket!";
  return 0;
}

/*
 * 计划接口使用方法：
 * 1. 初始化地址
 * 2. 添加回调
 * 3. 运行start()直接开始监听
 * 4. 运行stop()结束监听
 * 
 * Server使用方法：
 * 0. 初始化地址
 * 1. 设置数据格式：bytes/Json（即使用不同的继承类）
 * 2. 设置回调方式
 * 3. 回调方式1：中断调用（P2P/P2M）
 *   1. 添加回调函数
 *   2. 运行start()开始监听
 * 4. 回调方式2：被动调用（P2P/P2M）
 *   1. 运行start()开始监听
 *   2. 运行data_empty()了解是否有数据传输到缓冲区
 *   3. 运行data_get()按个获取缓冲区数据
 *   4. 运行data_get_count(count)获取N个字节的数据/N个消息的数据
 * 5. 设置连接方式：P2P/P2M（只能运行一个长链接，一对一/允许多个短链接，一对多）
 *   1. P2M模式在监听时不主动关闭连接，等Client发送完成之后自行断开
 *   2. P2P模式一旦连接上就不自动断开
 *   3. Err...所以是都一样的处理方法
 * 6. 运行stop()结束监听
 * 
 * 
 * Client使用方法：
 * 0. 初始化地址
 * 1. 设置数据格式：bytes/Json（即使用不同的继承类）
 * 2. 设置连接、回调方式（请使用不同的继承类）
 * 3. 回调方式1：中断调用（P2P）
 *   1. 添加回调函数
 *   2. 运行start()开始监听
 *   3. 运行stop()结束监听
 * 4. 回调方式2：被动调用（P2P）
 *   1. 运行start()开始监听
 *   2. 运行data_empty()了解是否有数据传输到缓冲区
 *   3. 运行data_get()获取缓冲区数据
 *   4. 运行data_get_count(count)获取N个字节的数据/N个消息的数据
 * 5. 回调方式3：P2M模式，只发送
 *   1. 设置运行模式：阻塞/非阻塞，非阻塞模式只能通过回调获取错误信息
 *   2. 阻塞模式
 *     1. 等待连接建立
 *     2. 连接建立之后直接发送消息
 *     3. 发送完成消息之后主动关闭连接
 *   3. 非阻塞模式
 *     1. 专门开启一个线程来运行上面的阻塞模式
 * 
 */