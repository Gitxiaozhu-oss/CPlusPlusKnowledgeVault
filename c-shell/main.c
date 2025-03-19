#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
运行方法
使用gcc -o lsh src/main.c命令进行编译，
然后运行./lsh。如果你想使用基于标准库实现的lsh_read_line()函数，那么可以执行：gcc -DLSH_USE_STD_GETLINE -o lsh src/main.c*/
/*
  内置shell命令的函数声明：
 */
// 改变目录的函数声明
int lsh_cd(char **args);
// 打印帮助信息的函数声明
int lsh_help(char **args);
// 退出shell的函数声明
int lsh_exit(char **args);

/*
  内置命令列表，以及它们对应的函数。
 */
// 内置命令字符串数组
char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

// 指向内置命令对应函数的指针数组
int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit
};

// 计算内置命令的数量
int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  内置函数的实现。
*/

/**
   @brief 内置命令：改变目录。
   @param args 参数列表。 args[0] 是 "cd"。 args[1] 是要切换到的目录。
   @return 始终返回 1，以继续执行。
 */
int lsh_cd(char **args)
{
  // 检查是否提供了目录参数
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: \"cd\" 命令缺少参数\n");
  } else {
    // 尝试切换目录
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

/**
   @brief 内置命令：打印帮助信息。
   @param args 参数列表。 未使用。
   @return 始终返回 1，以继续执行。
 */
int lsh_help(char **args)
{
  int i;
  printf("Stephen Brennan 的 LSH\n");
  printf("输入程序名和参数，然后按回车键。\n");
  printf("以下是内置命令：\n");

  // 打印所有内置命令
  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("使用 man 命令获取其他程序的信息。\n");
  return 1;
}

/**
   @brief 内置命令：退出。
   @param args 参数列表。 未使用。
   @return 始终返回 0，以终止执行。
 */
int lsh_exit(char **args)
{
  return 0;
}

/**
  @brief 启动一个程序并等待它终止。
  @param args 以空指针结尾的参数列表（包括程序名）。
  @return 始终返回 1，以继续执行。
 */
int lsh_launch(char **args)
{
  pid_t pid;
  int status;

  // 创建子进程
  pid = fork();
  if (pid == 0) {
    // 子进程
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // 创建子进程出错
    perror("lsh");
  } else {
    // 父进程
    do {
      // 等待子进程状态改变
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/**
   @brief 执行 shell 内置命令或启动程序。
   @param args 以空指针结尾的参数列表。
   @return 如果 shell 应该继续运行返回 1，如果应该终止返回 0
 */
int lsh_execute(char **args)
{
  int i;

  // 检查是否输入了空命令
  if (args[0] == NULL) {
    // 输入了一个空命令。
    return 1;
  }

  // 检查是否是内置命令
  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  // 不是内置命令，启动外部程序
  return lsh_launch(args);
}

/**
   @brief 从标准输入读取一行输入。
   @return 从标准输入读取的行。
 */
char *lsh_read_line(void)
{
#ifdef LSH_USE_STD_GETLINE
  char *line = NULL;
  ssize_t bufsize = 0; // 让 getline 为我们分配缓冲区
  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);  // 接收到 EOF
    } else  {
      perror("lsh: getline\n");
      exit(EXIT_FAILURE);
    }
  }
  return line;
#else
#define LSH_RL_BUFSIZE 1024
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  // 检查内存分配是否成功
  if (!buffer) {
    fprintf(stderr, "lsh: 内存分配错误\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // 读取一个字符
    c = getchar();

    if (c == EOF) {
      exit(EXIT_SUCCESS);
    } else if (c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // 如果超出缓冲区大小，重新分配内存
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: 内存分配错误\n");
        exit(EXIT_FAILURE);
      }
    }
  }
#endif
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
   @brief 非常简单地将一行拆分为标记（token）。
   @param line 输入的行。
   @return 以空指针结尾的标记数组。
 */
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  // 检查内存分配是否成功
  if (!tokens) {
    fprintf(stderr, "lsh: 内存分配错误\n");
    exit(EXIT_FAILURE);
  }

  // 分割字符串
  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    // 如果超出缓冲区大小，重新分配内存
    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		free(tokens_backup);
        fprintf(stderr, "lsh: 内存分配错误\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

/**
   @brief 循环获取输入并执行。
 */
void lsh_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);

    // 释放内存
    free(line);
    free(args);
  } while (status);
}

/**
   @brief 主入口点。
   @param argc 参数数量。
   @param argv 参数向量。
   @return 状态码
 */
int main(int argc, char **argv)
{
  // 加载配置文件（如果有）。

  // 运行命令循环。
  lsh_loop();

  // 执行任何关闭/清理操作。

  return EXIT_SUCCESS;
}
