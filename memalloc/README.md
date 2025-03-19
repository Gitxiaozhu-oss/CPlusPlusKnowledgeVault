### memalloc
memalloc是一个简单的内存分配器。

它实现了`malloc()`、`calloc()`、`realloc()`和`free()`函数。

- **编译与运行**：使用`gcc -o memalloc.so -fPIC -shared memalloc.c`命令进行编译。其中，`-fPIC`和`-shared`选项确保编译输出的是位置无关代码，并指示链接器生成适合动态链接的共享对象。

在Linux系统上，如果将环境变量`LD_PRELOAD`设置为共享对象的路径，那么该文件将在其他任何库之前被加载。我们可以利用这个技巧先加载编译好的库文件，这样在 shell 中后续运行的命令就会使用我们的`malloc()`、`free()`、`calloc()`和`realloc()`函数。

执行`export LD_PRELOAD=$PWD/memalloc.so`命令。此时，如果你运行诸如`ls`的命令，它将使用我们的内存分配器。
```bash
ls
memalloc.c		memalloc.so		README.md
```
或者运行`vim memalloc.c`命令。你也可以使用这个内存分配器来运行自己的程序。

一旦你完成实验，可以执行`unset LD_PRELOAD`命令停止使用我们的分配器。 