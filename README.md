### 计划接口使用方法：

1. 初始化地址
2. 添加回调
3. 运行start()直接开始监听
4. 运行stop()结束监听

### Server使用方法：

0. 初始化地址
1. 设置数据格式：bytes/Json（使用同一个模板类）
2. （回调方式可共用）
3. 回调方式1：中断调用（P2P/P2M）
   1. 添加回调函数
   2. 运行start()开始监听
4. 回调方式2：被动调用（P2P/P2M）
   1. 运行start()开始监听
   2. 运行data_empty()了解是否有数据传输到缓冲区
   3. 运行data_get()按个获取缓冲区数据
   4. 运行data_get_count(count)获取N个字节的数据/N个消息的数据
5. 设置连接方式：P2P/P2M/AR
   1. P2M模式在监听时不主动关闭连接，等Client发送完成之后自行断开
   2. P2P模式一旦连接上就不自动断开
   3. AR模式类似HTTP服务器
6. 运行stop()结束监听

### Client使用方法：

0. 初始化地址
1. 设置数据格式：bytes/Json（使用同一个模板类）
2. 设置连接、回调方式（请使用不同的继承类）
3. 回调方式1：中断调用（P2P）
   1. 添加回调函数
   2. 运行start()开始监听
   3. 运行stop()结束监听
4. 回调方式2：被动调用（P2P）
   1. 运行start()开始监听
   2. 运行data_empty()了解是否有数据传输到缓冲区
   3. 运行data_get()获取缓冲区数据
   4. 运行data_get_count(count)获取N个字节的数据/N个消息的数据
5. 回调方式3：P2M模式，只发送
   1. 设置运行模式：阻塞/非阻塞，非阻塞模式只能通过回调获取错误信息
   2. 阻塞模式
          1. 等待连接建立
          2. 连接建立之后直接发送消息
          3. 发送完成消息之后主动关闭连接
   3. 非阻塞模式
          1. 专门开启一个线程来运行上面的阻塞模式

### 运行逻辑分析

**关于网络协议**：使用TCP吧。

**关于被动/中断模式**：两种模式可以混合共存，不互相影响。

1. 接收到数据照样存入缓冲区，也发送中断信息
2. 但是中断信息不包含数据，需要数据还得从缓冲区拿
3. 在中断之外也能查询/pop缓冲区内容
4. 注意线程加锁

**关于逻辑继承**：代码在逻辑上有重合部分，可以继承

1. **Server**
   1. Server做抽象基类，逻辑：
      1. `bind`
      2. `listen`
      3. `while`
         1. `accept`
   2. ServerP2P继承Server
   3. ServerP2M可以继承ServerP2M，通过更改开关实现逻辑
   4. ServerAR继承Server
2. **Client**
   1. Client做抽象基类（仅包含`connect`逻辑）
   2. ClientP2P、ClientP2M、ClientAR均继承Client

**P2P**：

1. 初始化地址信息
2. `socket`生成一个套接字
3. **Server**
   1. `bind`地址
   2. `lisent`开始监听
   3. `while`
      1. `accept`接收到连接
      2. `while`
         1. `recv`接收数据 :arrow_right: 写入缓冲区 :arrow_right: 触发中断发送中断消息
            1. 如果是表示结束的标志就主动`close`断开连接
      3. (*其他线程*)`send`发送数据
4. **Client**
   1. `connect`请求连接
   2. `while`
      1. `recv`接收数据 :arrow_right: 写入缓冲区 :arrow_right: 触发中断发送中断消息
         1. 如果是表示结束的标志就主动`close`断开连接
   3. (*其他线程*)`send`发送消息

**P2M**：==此时数据流只能从 Client => Server！==

1. 初始化地址信息
2. `socket`生成一个套接字
3. **Server**
   1. `bind`地址
   2. `lisent`开始监听
   3. `while`
      1. `accept`接收到连接
      2. `while`
         1. `recv`接收数据 :arrow_right: 写入缓冲区 :arrow_right: 触发中断发送中断消息
            1. 如果是表示结束的标志就主动`close`断开连接
4. **Client**
   1. `connect`请求连接
   2. `send`发送消息
   3. `close`断开连接

**AR**：Ask-Response，==此时类似网络HTTP服务==

1. 初始化地址信息
2. `socket`生成一个套接字
3. **Server**
   1. `bind`地址
   2. `lisent`开始监听
   3. `while`
      1. `accept`接收到连接
      2. `while`
         1. `recv`接收数据 :arrow_right: 写入缓冲区 :arrow_right: 触发中断发送中断消息
            1. 如果是表示结束的标志就**跳出`while`**
      3. 处理数据
      4. `send`发送数据
      5. `close`断开连接
4. **Client**
   1. `connect`请求连接
   2. `send`发送消息
   3. `recv`接收数据 :arrow_right: 写入缓冲区 :arrow_right: 触发中断发送中断消息
   4. `close`断开连接



