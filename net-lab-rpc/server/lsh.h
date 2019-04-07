#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define OUT_PUT_BUFSIZE 65535
// 执行命令后的输出缓冲区
extern char* out_put;

void lsh_loop(void);
char *lsh_read_line(void);
char **lsh_split_line(char *);
int lsh_execute(char **);
int lsh_launch(char **);

/**
 * 程序内置命令的处理
*/
int lsh_num_builtins();
int lsh_cd(char **);
int lsh_help(char **);
int lsh_exit(char **);

extern int (*builtin_func[]) (char **);
extern const char *builtin_str[];