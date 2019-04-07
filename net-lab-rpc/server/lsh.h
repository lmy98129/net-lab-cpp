#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>
#include <errno.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define OUT_PUT_BUFSIZE 65535

void lsh_loop(void);
char *lsh_read_line(void);
char **lsh_split_line(char *);
char* lsh_execute(char **);
char* lsh_launch(char **);

/**
 * 程序内置命令的处理
*/
int lsh_num_builtins();
char* lsh_cd(char **);
char* lsh_help(char **);
int lsh_exit(char **);

extern char *(*builtin_func[]) (char **);
extern const char *builtin_str[];

size_t get_utf8_length(unsigned char byte);