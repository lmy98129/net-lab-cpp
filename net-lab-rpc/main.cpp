#include "lsh.h"

int main(int argc, char **argv) {
  lsh_loop();
  return EXIT_SUCCESS;
}

void lsh_loop(void) {
  char *line;
  char **args;
  int status = 1;

  do {
    // 输入提示
    printf("> ");
    // 读取输入的命令字符串
    line = lsh_read_line();
    // 切分成参数数组
    args = lsh_split_line(line);
    // 执行命令
    status = lsh_execute(args);

    free(line);
    free(args);
    
  } while(status);
}