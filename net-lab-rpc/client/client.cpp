#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <curses.h>

// 缓冲区大小
#define BUFFER_SIZE 1024
// 服务器端口
#define SERVER_PORT 11332  
// 退出命令
#define QUIT_CMD ".exit"  
// 密码最大长度
#define PWD_SIZE 100
// 密码消息的前缀
#define PWD_PRE "PASSWD"
// 密码正确前缀
#define PWD_SUC "SUC"
// 密码错误前缀
#define PWD_ERR "ERR"

int main(int argc, const char *argv[]) {
  if (argc < 3) {
    printf("netto: 参数格式 <ip地址> <端口号>\n");
    exit(EXIT_SUCCESS);
  }


  // 密码输入缓冲区
  char passwd[PWD_SIZE];
  bool is_pwd_send = false, is_pwd_correct = false;

  printf("欢迎使用netto客户端\n请输入密码: ");
  fflush(stdout);

  // 隐藏密码输入
  initscr();
  scanf("%s", passwd);
  endwin();

  // 接收recv和发送input缓冲区
  char recv_msg[BUFFER_SIZE];
  char input_msg[BUFFER_SIZE];

  // 服务器地址
  struct sockaddr_in server_addr;
  // IPv4地址族
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(atoi(argv[2]));
  server_addr.sin_addr.s_addr = inet_addr(argv[1]);
  // server_addr.sin_addr.s_addr = inet_addr("198.181.38.49");
  // sin_zero是为了让sockaddr与sockaddr_in两个数据结构保持大小相同而保留的空字节
  bzero(&(server_addr.sin_zero), 8);

  //创建socket
  int server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock_fd == -1) {
    perror("\r错误: 创建socket出错");
    return 1;
  }

  // 连接服务器socket
  if (connect(server_sock_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) == 0) {
    // 文件描述符表，每一位上的0表示该位对应文件描述符空闲，1表示被占用
    // fd_set事实上就是int32，即4个字节、32位长度的整型数
    fd_set client_fd_set;

    // 超时时间设置
    struct timeval tv;
    tv.tv_sec = 20;
    tv.tv_usec = 0;

    // 发送密码
    bzero(input_msg, BUFFER_SIZE);
    strcpy(input_msg, PWD_PRE);
    strcpy(input_msg + strlen(PWD_PRE), passwd);
    is_pwd_send = true;

    if (send(server_sock_fd, input_msg, strlen(input_msg), 0) == -1) {
      perror("\r错误: 发送消息出错!");
    }

    while (1) {
      // 将所有位都设为0
      FD_ZERO(&client_fd_set);

      // STDIN_FILENO为文件描述输入句柄，值为0
      // FD_SET将这个句柄放入到文件描述符表
      FD_SET(STDIN_FILENO, &client_fd_set);

      // 将服务器端socket对应的文件描述放入
      FD_SET(server_sock_fd, &client_fd_set);

      // 使用select模型对连接状态进行监控，tv为超时时间设置
      select(server_sock_fd + 1, &client_fd_set, NULL, NULL, &tv);
      // 若文件描述符输入句柄STDIN_FILENO在FD_ISSET检查后正常，则可以对客户端发送消息
      if (FD_ISSET(STDIN_FILENO, &client_fd_set)) {
        if (is_pwd_send && is_pwd_correct) {
          printf("\r> ");
          fflush(stdout);
          bzero(input_msg, BUFFER_SIZE);
          fgets(input_msg, BUFFER_SIZE, stdin);
          // 输入“.exit"则退出客户端

          // 若出现了开头\r的奇怪现象则对此作出处理
          if (strncmp(input_msg, "\r", strlen("\r")) == 0) {
            strcpy(input_msg, input_msg + 1);
          }
          
          if (strncmp(input_msg, QUIT_CMD, strlen(QUIT_CMD)) == 0) {
            printf("\r提示: 退出客户端\n");
            exit(0);
          }
          if (send(server_sock_fd, input_msg, strlen(input_msg), 0) == -1) {
            perror("\r错误: 发送消息出错!");
          }
        }
      }

      // 处理服务器返回的消息
      if (FD_ISSET(server_sock_fd, &client_fd_set)) {
        bzero(recv_msg, BUFFER_SIZE);
        long byte_num = recv(server_sock_fd, recv_msg, BUFFER_SIZE, 0);
        if (byte_num > 0) {
          // 接收消息成功
          if (byte_num > BUFFER_SIZE) {
            byte_num = BUFFER_SIZE;
          }
          recv_msg[byte_num] = '\0';
          if (strncmp(recv_msg, PWD_SUC, strlen(PWD_SUC)) == 0) {
            // 密码正确
            is_pwd_correct = true;
            printf("\r%s", recv_msg + strlen(PWD_SUC));
          } else if (strncmp(recv_msg, PWD_ERR, strlen(PWD_ERR)) == 0) {
            // 密码不正确
            is_pwd_correct = false;
            printf("\r%s", recv_msg + strlen(PWD_ERR));
            exit(EXIT_SUCCESS);
          } else {
            printf("\r%s", recv_msg);
          }
        }
        else if (byte_num < 0) {
          // 接收消息出错
          printf("\r错误: 接受服务器消息出错!\n");
        }
        else {
          // 服务器端退出了
          printf("\r提示: 服务器退出!\n");
          exit(EXIT_SUCCESS);
        }
        printf("\r> ");
        fflush(stdout);
      }

    }
  } else {
    // 服务器端已关闭了
    printf("\r提示: 服务器已关闭!\n");
    exit(0);
  }
  return 0;
}