#ifndef __TELNET_COM__H
#define __TELNET_COM__H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>

#include <sys/socket.h>
#include <errno.h>
#include <stdarg.h>

#include "sll_linklist.h"

#include "log.h"

#define BUFSIZE 1024
#define PORT  20000
#define MAX_CMD_HISTORY_NUM 500

#define   MAX_CMD_LEN    (256)
#define   MAX_ARG_NUM    (8)


/*telnet cmd结构体*/
typedef struct cmd
{
    char cmd[MAX_CMD_LEN];
    unsigned int size;
}IN_CMD;

/*历史命令数组队列*/
typedef struct cmd_history
{
    unsigned int all_cmd_num;
    IN_CMD queue_cmd[MAX_CMD_HISTORY_NUM];
}CMD_QUEUE;

/*telnet shell及服务端的mgr结构体*/
typedef struct tag_telnet_info
{
    CMD_QUEUE *history_cmd;         /*历史命令*/
    unsigned int has_input_cmd_num;       /*已经输入的命令数*/
    unsigned int cmd_index;               /*当前的命令索引(用于翻历史命令)*/
    unsigned int fd_conn;                 /*客户端连接fd*/

    unsigned int move_left_num;           /*光标左移数量*/

    char g_cmd[MAX_CMD_LEN];        /*cmd*/
    pid_t pid;                      /*当前进程号*/
}TELNET_MGR;

typedef  intptr_t (*func_type)(long para1, ...);

unsigned int parse_function_args(long *args, char *para_str_begin, char *para_str_end);
unsigned int parse_func_call(char *buf, func_type *fun_addr, long *args);
void do_call_func(func_type pfunc, long *args);
unsigned int show_var_info(const char *var_name);
void exec_function(char *call_func_str);
void proccess_cmd(char *cmd_line);
unsigned int telnet_reg_log();

void tel_printf(const char *fmt,...);

#define TELNET_LINE_MAX_LEN 1024


#define TEL_DEBUG(FMT,...) \
do \
{\
    TEL_LOG(TEL_LOG_MODULE_ID_TELNET,TEL_LOG_SECURITY_DEBUG,FMT,##__VA_ARGS__); \
}while(0)\


#define TEL_INFO(FMT,...) \
do \
{\
    TEL_LOG(TEL_LOG_MODULE_ID_TELNET,TEL_LOG_SECURITY_INFO,FMT,##__VA_ARGS__); \
}while(0)\

#define TEL_ERROR(FMT,...) \
do \
{\
    TEL_LOG(TEL_LOG_MODULE_ID_TELNET,TEL_LOG_SECURITY_ERROR,FMT,##__VA_ARGS__); \
}while(0)\


#define TELD_PRINTF(FMT,...)  tel_printf(FMT,##__VA_ARGS__)


#endif
