#include <unistd.h>
#include <string.h>
#include <pthread.h>
/* 仅用于调试输出 */
#include <stdio.h>

// 定义一个长度为16的字符数组，用于强制头部按16字节对齐
typedef char ALIGN[16];

// 定义一个联合体，用于表示内存块的头部信息
union header {
    struct {
        size_t size;         // 内存块的大小
        unsigned is_free;    // 标记该内存块是否空闲
        union header *next;  // 指向下一个内存块头部的指针
    } s;
    // 强制头部按16字节对齐
    ALIGN stub;
};
// 为联合体类型定义别名
typedef union header header_t;

// 定义链表的头指针和尾指针，初始化为NULL
header_t *head = NULL, *tail = NULL;
// 定义全局互斥锁，用于保证线程安全
pthread_mutex_t global_malloc_lock;

// 查找空闲且能容纳指定大小的内存块
header_t *get_free_block(size_t size)
{
    header_t *curr = head;
    // 遍历链表
    while(curr) {
        // 查看是否有空闲且能容纳请求大小的内存块
        if (curr->s.is_free && curr->s.size >= size)
            return curr;
        curr = curr->s.next;
    }
    return NULL;
}

// 释放指定的内存块
void free(void *block)
{
    header_t *header, *tmp;
    // 程序断点是进程数据段的末尾
    void *programbreak;

    // 如果传入的指针为空，直接返回
    if (!block)
        return;
    // 加锁，保证线程安全
    pthread_mutex_lock(&global_malloc_lock);
    // 获取内存块的头部信息
    header = (header_t*)block - 1;
    // sbrk(0) 获取当前程序断点的地址
    programbreak = sbrk(0);

    /*
       检查要释放的内存块是否是链表中的最后一个。
       如果是，则缩小堆的大小并将内存释放给操作系统。
       否则，将该内存块标记为空闲。
     */
    if ((char*)block + header->s.size == programbreak) {
        if (head == tail) {
            // 如果链表中只有一个节点，将头指针和尾指针置为NULL
            head = tail = NULL;
        } else {
            tmp = head;
            // 遍历链表找到倒数第二个节点
            while (tmp) {
                if(tmp->s.next == tail) {
                    tmp->s.next = NULL;
                    tail = tmp;
                }
                tmp = tmp->s.next;
            }
        }
        /*
           sbrk() 传入负数参数会减小程序断点，
           从而将内存释放给操作系统。
        */
        sbrk(0 - header->s.size - sizeof(header_t));
        /* 注意：这个锁并不能真正保证线程安全，
           因为 sbrk() 本身不是线程安全的。
           假设在我们获取程序断点之后、减小断点之前，
           发生了一个外部的 sbrk(N) 调用，
           那么我们最终会释放外部 sbrk() 获取的内存。
        */
        pthread_mutex_unlock(&global_malloc_lock);
        return;
    }
    // 将该内存块标记为空闲
    header->s.is_free = 1;
    // 解锁
    pthread_mutex_unlock(&global_malloc_lock);
}

// 分配指定大小的内存
void *malloc(size_t size)
{
    size_t total_size;
    void *block;
    header_t *header;
    // 如果请求的大小为0，直接返回NULL
    if (!size)
        return NULL;
    // 加锁，保证线程安全
    pthread_mutex_lock(&global_malloc_lock);
    // 查找空闲且能容纳指定大小的内存块
    header = get_free_block(size);
    if (header) {
        // 找到合适的空闲块，将其标记为已使用
        header->s.is_free = 0;
        // 解锁
        pthread_mutex_unlock(&global_malloc_lock);
        // 返回内存块的起始地址
        return (void*)(header + 1);
    }
    // 需要从操作系统获取内存，以容纳请求的内存块和头部信息
    total_size = sizeof(header_t) + size;
    // 调用 sbrk() 分配内存
    block = sbrk(total_size);
    if (block == (void*) -1) {
        // 分配失败，解锁并返回NULL
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }
    header = block;
    // 设置内存块的大小
    header->s.size = size;
    // 标记该内存块为已使用
    header->s.is_free = 0;
    // 下一个节点指针置为NULL
    header->s.next = NULL;
    if (!head)
        // 如果链表为空，将头指针指向该节点
        head = header;
    if (tail)
        // 如果链表不为空，将尾节点的下一个指针指向该节点
        tail->s.next = header;
    // 更新尾指针
    tail = header;
    // 解锁
    pthread_mutex_unlock(&global_malloc_lock);
    // 返回内存块的起始地址
    return (void*)(header + 1);
}

// 分配指定数量和大小的内存，并初始化为0
void *calloc(size_t num, size_t nsize)
{
    size_t size;
    void *block;
    // 如果数量或大小为0，直接返回NULL
    if (!num || !nsize)
        return NULL;
    // 计算总大小
    size = num * nsize;
    // 检查乘法是否溢出
    if (nsize != size / num)
        return NULL;
    // 调用 malloc() 分配内存
    block = malloc(size);
    if (!block)
        return NULL;
    // 将分配的内存初始化为0
    memset(block, 0, size);
    return block;
}

// 重新分配指定内存块的大小
void *realloc(void *block, size_t size)
{
    header_t *header;
    void *ret;
    // 如果传入的指针为空或请求的大小为0，调用 malloc() 分配内存
    if (!block || !size)
        return malloc(size);
    // 获取内存块的头部信息
    header = (header_t*)block - 1;
    // 如果当前内存块的大小足够，直接返回该内存块
    if (header->s.size >= size)
        return block;
    // 调用 malloc() 分配新的内存块
    ret = malloc(size);
    if (ret) {
        // 将原内存块的内容复制到新的内存块
        memcpy(ret, block, header->s.size);
        // 释放原内存块
        free(block);
    }
    return ret;
}

// 调试函数，用于打印整个链表信息
void print_mem_list()
{
    header_t *curr = head;
    printf("head = %p, tail = %p \n", (void*)head, (void*)tail);
    while(curr) {
        printf("addr = %p, size = %zu, is_free=%u, next=%p\n",
               (void*)curr, curr->s.size, curr->s.is_free, (void*)curr->s.next);
        curr = curr->s.next;
    }
}    