#include "lsh.h"

#define LSH_TOK_BUFSIZE 64
// 分隔符集合
#define LSH_TOK_DELIM " \t\r\n\a"

// 管道的读写句柄
#define PIPE_READ 0
#define PIPE_WRITE 1

const char *builtin_str[] = {
  "cd",
  "help",
  // "exit"
};

// 程序内置命令的函数数组
char* (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  // &lsh_exit
};

/**
 * 读取用户输入的命令字符串
*/
char *lsh_read_line(void) {
  char *line = NULL;
  // 使用getline函数为我们分配内存缓冲区，所以这里的缓冲区大小为0
  size_t bufsize = 0;
  getline(&line, &bufsize, stdin);
  return line;
}

/**
 * 将用户输入的命令字符串转化为参数数组
*/
char **lsh_split_line(char *line) {
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = (char **)malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "netto: 内存空间分配错误\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while(token != NULL) {
    tokens[position] = token;
    position++;

    // 空间溢出处理
    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens = (char **)realloc(tokens, bufsize * sizeof(char *));
      if (!tokens) {
        fprintf(stderr, "netto: 内存空间分配错误\n");
        exit(EXIT_FAILURE);
      }   
    }

    // 反复分割，直到分割结束
    token = strtok(NULL, LSH_TOK_DELIM);
  }

  tokens[position] = NULL;
  return tokens;
}

/**
 * 父子进程调度
*/
char *lsh_launch(char **args) {
  int bufsize = OUT_PUT_BUFSIZE, position = 0;
  // 函数内部分配的内存空间，在函数执行结束后即会被自动销毁，不存在free的说法
  char* out_put = NULL;

  pid_t pid, wpid;
  int status;

  // 创建输入输出管道
  int std_in_pipe[2];
  int std_out_pipe[2];

  /**
   * 管道的构成如下
   * child                         parent
   * PIPE_READ <--std_in_pipe-- PIPE_WRITE
   * PIPE_WRITE --std_out_pipe--> PIPE_READ
   * 
   * 所以对于父进程来说有用的只有: 
   * std_in_pipe[PIPE_WRITE]
   * std_out_pipe[PIPE_READ]
  */
  
  // 分配输入管道
  if (pipe(std_in_pipe) < 0) {
    perror("lsh - 为子进程分配输入管道失败");
    return NULL;
  }

  // 分配输出管道
  if (pipe(std_out_pipe) < 0) {
    close(std_in_pipe[PIPE_READ]);
    close(std_in_pipe[PIPE_WRITE]);
    perror("lsh - 为子进程分配输出管道失败");
    return NULL;
  }

  pid = fork();
  if (pid == 0) {
    // 不能继续fork了，说明是子进程
    // 使用exec进行父子进程替换

    // 将标准输入定向到输入管道的读取端
    if (dup2(std_in_pipe[PIPE_READ], STDIN_FILENO) == -1) {
      exit(EXIT_FAILURE);
    }

    // 将标准输出定向到输出管道的写入端
    if (dup2(std_out_pipe[PIPE_WRITE], STDOUT_FILENO) == -1) {
      exit(EXIT_FAILURE);
    }

    // 将标准错误输出定向到输出管道的写入端
    if (dup2(std_out_pipe[PIPE_WRITE], STDERR_FILENO) == -1) {
      exit(EXIT_FAILURE);
    }

    // 关闭这些管道句柄，类似于文件写入完成后的close()函数
    close(std_in_pipe[PIPE_READ]);
    close(std_in_pipe[PIPE_WRITE]);
    close(std_out_pipe[PIPE_READ]);
    close(std_out_pipe[PIPE_WRITE]);

    if (execvp(args[0], args) == -1) {
      // 抛出错误，例如出现了不存在的命令
      perror("lsh");
    }
    // 子进程退出
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // 分配子进程出错，直接关闭管道句柄
    close(std_in_pipe[PIPE_READ]);
    close(std_in_pipe[PIPE_WRITE]);
    close(std_out_pipe[PIPE_READ]);
    close(std_out_pipe[PIPE_WRITE]);
    // fork错误
    perror("lsh");
  } else {
    // 能继续fork，说明是父进程
    out_put = (char *)malloc(bufsize * sizeof(char));

    // 关闭这些父进程用不到的管道
    close(std_in_pipe[PIPE_READ]);
    close(std_out_pipe[PIPE_WRITE]);
    
    char result;
    char chinese[4];
    char other[2];
    // 毕竟不知道究竟输出了多少，只好逐个字符读取
    while(read(std_out_pipe[PIPE_READ], &result, 1) == 1) {

      if (get_utf8_length(result) == 3) {
        // 遇到中文时直接连取三个，因为中文在utf-8中是三个字节的
        chinese[0] = result;
        read(std_out_pipe[PIPE_READ], &result, 1);
        chinese[1] = result;
        read(std_out_pipe[PIPE_READ], &result, 1);
        chinese[2] = result;
        chinese[3] = '\0';
        position += 3;
        snprintf(out_put + strlen(out_put), bufsize * sizeof(char), "%s", chinese);
      } else {
        other[0] = result;
        other[1] = '\0';
        snprintf(out_put + strlen(out_put), bufsize * sizeof(char), "%s", other);
        position++;
      }

      if (position >= bufsize) {
        bufsize += OUT_PUT_BUFSIZE;
        out_put = (char *)realloc(out_put, bufsize * sizeof(char));
        if (!out_put) {
          fprintf(stderr, "netto: 内存空间分配错误\n");
          exit(EXIT_FAILURE);
        }
      }
    }

    do {
      // waitpid处理子进程退出后的回收
      wpid = waitpid(pid, &status, WUNTRACED);
      // 当子进程退出或者因为信号而结束，退出回收循环
    } while(!WIFEXITED(status) && !WIFSIGNALED(status));

    // 读取到了信息之后再关闭对于父进程有用的管道句柄
    close(std_in_pipe[PIPE_WRITE]);
    close(std_out_pipe[PIPE_READ]);

  }

  return out_put;
}

/**
 * 运行命令
*/
char* lsh_execute(char **args) {
  if (args[0] == NULL) {
    // 输入了一个空的命令
    return NULL;
  }

  // 查看是否是程序内置的命令
  for (int i = 0; i<lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  // 非内置命令，开启进程调度
  return lsh_launch(args);
}

/**
 * 以下是对一些程序内置的命令进行的特殊处理
*/

/**
 * 内置命令的个数，由内置命令字符串数组的长度求得
*/
int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/**
 * cd命令
*/
char* lsh_cd(char **args) {
  char* out_put = NULL;
  if (args[1] == NULL) {
    // fprintf(stderr, "netto: cd命令应当携带一个参数\n");
    out_put = (char* )malloc(OUT_PUT_BUFSIZE * sizeof(char));
    sprintf(out_put + strlen(out_put), "%s", "netto: cd命令应当携带一个参数\n");
  } else {
    if (chdir(args[1]) != 0) {
      out_put = (char* )malloc(OUT_PUT_BUFSIZE * sizeof(char));
      // perror("lsh");
      sprintf(out_put + strlen(out_put), "netto: 文件夹跳转出错: %s\n", strerror(errno));
    }
  }
  return out_put;
}

/**
 * help命令
*/
char* lsh_help(char **args) {
  char* out_put = (char* )malloc(OUT_PUT_BUFSIZE * sizeof(char));
  sprintf(out_put + strlen(out_put), "\nnetto, 一款基于socket、brenns10/lsh项目的，类telnet远程程序调用\n");
  sprintf(out_put + strlen(out_put), "命令用法: 输入<命令> [参数]之后回车即可运行\n");
  sprintf(out_put + strlen(out_put), "以下是程序内置的命令\n\n");

  for(int i=0; i<lsh_num_builtins(); i++) {
    sprintf(out_put + strlen(out_put), "  %s\n", builtin_str[i]);
  }

  sprintf(out_put + strlen(out_put), " .exit\n");
  sprintf(out_put + strlen(out_put), "\n请使用man <命令>获取其他程序的更多信息\n");
  return out_put;
}

/**
 * exit命令
*/
// int lsh_exit(char **args) {
//   return 0;
// }

size_t get_utf8_length(unsigned char byte) {
    size_t length = 0;
    if (byte >= 0xFC) // lenght 6
      length = 6;  
    else if (byte >= 0xF8)
      length = 5;
    else if (byte >= 0xF0)
      length = 4;
    else if (byte >= 0xE0)
      length = 3;
    else if (byte >= 0xC0)
      length = 2;
    else
      length = 1;
    return length;
  }     