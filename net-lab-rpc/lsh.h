#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

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