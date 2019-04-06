#include "lsh.h"

#define LSH_TOK_BUFSIZE 64
// 分隔符集合
#define LSH_TOK_DELIM " \t\r\n\a"

const char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

// 程序内置命令的函数数组
int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit
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
    fprintf(stderr, "lsh: 内存空间分配错误\n");
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
        fprintf(stderr, "lsh: 内存空间分配错误\n");
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
int lsh_launch(char **args) {
  pid_t pid, wpid;
  int status;
  
  pid = fork();
  if (pid == 0) {
    // 不能继续fork了，说明是子进程
    // 使用exec进行父子进程替换
    if (execvp(args[0], args) == -1) {
      // 抛出错误，例如出现了不存在的命令
      perror("lsh");
    }
    // 如果真的退出了那肯定程序得结束
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // fork错误
    perror("lsh");
  } else {
    // 能继续fork，说明是父进程
    do {
      // waitpid处理子进程退出后的回收
      wpid = waitpid(pid, &status, WUNTRACED);
      // 当子进程退出或者因为信号而结束，退出回收循环
    } while(!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/**
 * 运行命令
*/
int lsh_execute(char **args) {
  if (args[0] == NULL) {
    // 输入了一个空的命令
    return 1;
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
int lsh_cd(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: cd命令应当携带一个参数\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

/**
 * help命令
*/
int lsh_help(char **args) {
  printf("lsh, 基于brenns10/lsh项目改造\n");
  printf("用法: 输入lsh <命令> [参数]之后回车即可运行\n");
  printf("以下是程序内置的命令\n\n");

  for(int i=0; i<lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("\n请使用man <命令>获取其他程序的更多信息\n\n");
  return 1;
}

/**
 * exit命令
*/
int lsh_exit(char **args) {
  return 0;
}
