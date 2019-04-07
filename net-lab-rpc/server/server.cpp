#include <stdio.h>  
#include <stdlib.h>  
#include <netinet/in.h>  
#include <sys/socket.h>  
#include <arpa/inet.h>  
#include <string.h>  
#include <unistd.h>  

#include "lsh.h"

// 完成三次握手但没有accept的队列的长度
#define BACKLOG 5
// 服务器端口
#define SERVER_PORT 11332  
// 接收和发送缓冲区大小
#define BUFFER_SIZE 1024  
// 退出命令
#define QUIT_CMD ".exit"  

// 应用层同时可以处理的连接  
#define CONCURRENT_MAX 8   
int client_fds[CONCURRENT_MAX];

int main(int argc, const char *argv[]) {
  printf("> ");
  fflush(stdout);

  // 接收recv和发送input缓冲区
  char input_msg[BUFFER_SIZE];
  char recv_msg[BUFFER_SIZE];

  // 服务器地址
  struct sockaddr_in server_addr;
  // IPv4地址族
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);
  server_addr.sin_addr.s_addr = INADDR_ANY;
  // server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  // sin_zero是为了让sockaddr与sockaddr_in两个数据结构保持大小相同而保留的空字节
  bzero(&(server_addr.sin_zero), 8);

  //创建socket
  int server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock_fd == -1) {
    perror("\r错误: 创建socket出错");
    return 1;
  }

  //绑定socket
  int bind_result = bind(server_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (bind_result == -1) {
    perror("\r错误: 绑定socket出错");
    return 1;
  }

  // 监听listen
  if (listen(server_sock_fd, BACKLOG) == -1) {
    perror("\r错误: 监听socket出错");
    return 1;
  }
  
  // 文件描述符表，每一位上的0表示该位对应文件描述符空闲，1表示被占用
  // fd_set事实上就是int32，即4个字节、32位长度的整型数
  fd_set server_fd_set;
  // 记录当前最大的文件描述符，初始时一个连接都没有，因此设为-1
  int max_fd = -1;

  // 超时时间设置
  struct timeval tv; 
  tv.tv_sec = 20;
  tv.tv_usec = 0;

  while (1) {
    // 将所有位都设为0
    FD_ZERO(&server_fd_set);

    // STDIN_FILENO为文件描述输入句柄，值为0
    // FD_SET将这个句柄放入到文件描述符表
    FD_SET(STDIN_FILENO, &server_fd_set);
    if (max_fd < STDIN_FILENO) {
      max_fd = STDIN_FILENO;
    }

    // 将服务器端socket对应的文件描述放入
    FD_SET(server_sock_fd, &server_fd_set);
    if (max_fd < server_sock_fd) {
      max_fd = server_sock_fd;
    }

    //客户端连接
    for (int i = 0; i < CONCURRENT_MAX; i++) {
      // 将当前客户端连接更新到文件描述符表
      if (client_fds[i] != 0) {
        FD_SET(client_fds[i], &server_fd_set);
        if (max_fd < client_fds[i]) {
          max_fd = client_fds[i];
        }
      }
    }

    // 使用select模型对连接状态进行监控，tv为超时时间设置
    int ret = select(max_fd + 1, &server_fd_set, NULL, NULL, &tv);
    //ret 为未状态发生变化的文件描述符的个数
    if (ret < 0) {
      perror("\r错误: select 出错");
      continue;
    }
    else if (ret == 0) {
      // printf("提示: select 超时\n");
      continue;
    }
    else {

      // 若文件描述符输入句柄STDIN_FILENO在FD_ISSET检查后正常，则可以对客户端发送消息
      if (FD_ISSET(STDIN_FILENO, &server_fd_set)) {
        printf("\r> ");
        fflush(stdout);
        // 先清空发送缓冲区
        bzero(input_msg, BUFFER_SIZE);
        // 输入要发送给客户端的消息到发送缓冲区
        fgets(input_msg, BUFFER_SIZE, stdin);
        // 输入“.exit"则退出服务器
        if (strncmp(input_msg, QUIT_CMD, strlen(QUIT_CMD)) == 0) {
          printf("\r提示: 退出服务器\n");
          exit(0);
        }

        // 注意：这是对所有已连接的客户端进行群发操作
        for (int i = 0; i < CONCURRENT_MAX; i++) {
          if (client_fds[i] != 0) {
            printf("\r提示：发送消息到客户端(%d) 文件描述符: %d\n", i, client_fds[i]);
            send(client_fds[i], input_msg, BUFFER_SIZE, 0);
          }
        }
        printf("\r> ");
        fflush(stdout);
      }

      // 有新的连接请求
      if (FD_ISSET(server_sock_fd, &server_fd_set)) {
        // 先建立一个地址结构体来存储这个新的连接地址
        struct sockaddr_in client_address;
        socklen_t address_len;
        // accept函数用来与其建立连接
        int client_sock_fd = accept(server_sock_fd, (struct sockaddr *)&client_address, &address_len);
        printf("\r提示: 新连接的客户端 文件描述符: %d\n", client_sock_fd);
        if (client_sock_fd > 0) {
        // 生成文件描述符成功，向文件描述符表中对应位置添加
          int index = -1;
          for (int i = 0; i < CONCURRENT_MAX; i++) {
            if (client_fds[i] == 0) {
              index = i;
              client_fds[i] = client_sock_fd;
              break;
            }
          }
          if (index >= 0) {
            // 文件描述符添加成功
            printf("\r提示: 新客户端(%d)加入成功 %s:%d\n", index, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
          }
          else {
            // 文件描述符数量达到上限，也即连接数量达到上限
            bzero(input_msg, BUFFER_SIZE);
            strcpy(input_msg, "提示: 服务器加入的客户端数达到最大值，无法加入!\n");
            send(client_sock_fd, input_msg, BUFFER_SIZE, 0);
            printf("\r提示: 客户端连接数达到最大值，新客户端加入失败 %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
          }
          printf("\r> ");
          fflush(stdout);
        }
      }

      // 遍历全部连接，查看状态是否改变
      for (int i = 0; i < CONCURRENT_MAX; i++) {
        if (client_fds[i] != 0) {
          if (FD_ISSET(client_fds[i], &server_fd_set)) {
            //处理某个客户端过来的消息
            bzero(recv_msg, BUFFER_SIZE);
            // 接收消息
            long byte_num = recv(client_fds[i], recv_msg, BUFFER_SIZE, 0);
            if (byte_num > 0) {
              // 接收消息成功
              if (byte_num > BUFFER_SIZE) {
                byte_num = BUFFER_SIZE;
              }
              recv_msg[byte_num] = '\0';
              printf("\r客户端(%d)> %s", i, recv_msg);
              // TODO: 就在这里调用shell
              char **args;
              int status = 1;
              // 切分成参数数组
              args = lsh_split_line(recv_msg);
              if (args != NULL) {
                // 执行命令
                status = lsh_execute(args);
                if (NULL != out_put) {
                  printf("%s", out_put);
                  send(client_fds[i], out_put, strlen(out_put), 0);
                  out_put = NULL;
                }
              }
            }
            else if (byte_num < 0) {
              // 接收消息出错
              printf("\r错误: 从客户端(%d)接受消息出错.\n", i);
            } else {
              // 客户端退出了
              FD_CLR(client_fds[i], &server_fd_set);
              client_fds[i] = 0;
              printf("\r提示: 客户端(%d)退出了\n", i);
            }
            printf("\r> ");
            fflush(stdout);
          }
        }
      }

    }
  }

  return 0;
}