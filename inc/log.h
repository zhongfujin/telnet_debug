#ifndef __LOG__H
#define __LOG__H
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <assert.h>
#include <unistd.h>

#include <errno.h>
#include <time.h>
#include <stdarg.h>

#include "sll_linklist.h"



#define TEL_LOG_LINE_SIZE 1024
#define FTPS_MAX_PATH_LEN  255

#define TEL_LOG_SWITCH  1

/*MODULE*/

#define TEL_LOG_MODULE_ID_TELNET            0x01


typedef enum log_level
{
    TEL_LOG_SECURITY_NOTSET =   0x01,
    TEL_LOG_SECURITY_DEBUG =    0x02,
    TEL_LOG_SECURITY_INFO =     0x03,
    TEL_LOG_SECURITY_WARNING =  0x04,
    TEL_LOG_SECURITY_ERROR =    0x05,
    TEL_LOG_SECURITY_CRITICAL = 0x06
}LOG_SECURITY_LEVEL;

#define SECURITY_NOTSET         "   NOTSET      "
#define SECURITY_DEBUG          "   DEBUG       "
#define SECURITY_INFO           "   INFO        "
#define SECURITY_WARNING        "   WARNING     "
#define SECURITY_ERROR          "   ERROR       "
#define SECURITY_CRITICAL       "   CRITICAL    "

typedef struct tag_cur_size_init
{
    unsigned int cur_notset_log_size;
    unsigned int cur_debug_log_size;
    unsigned int cur_info_log_size;
    unsigned int cur_warning_log_size;
    unsigned int cur_error_log_size;
    unsigned int cur_critical_log_size;
    unsigned int cur_log_size;
}TEL_LOG_CUR_SIZE;


typedef struct tag_log_reg_info
{
    int module_id;
    int level;
    int mulitifile;
    char file_name[64];
    TEL_LOG_CUR_SIZE cur_size;
    unsigned int max_log_size;
}TEL_LOG_REG_INFO;


typedef struct tag_tel_log_reg_base
{
    SLL_LINK_LIST_BASE *reg_list;
    unsigned int list_num;
}TEL_LOG_REG_BASE;


typedef struct tag_tel_log_reg_node
{
    SLL_LINK_LIST_NODE tel_log_reg_node;
    TEL_LOG_REG_INFO reg_info;
}TEL_LOG_REG_NODE;

unsigned int tel_log_init();
unsigned int tel_log_reg(TEL_LOG_REG_INFO *reg_info,unsigned int cnt);
TEL_LOG_REG_BASE *get_tel_log_reg_list();
TEL_LOG_REG_NODE *tel_add_log_reg_node(TEL_LOG_REG_INFO *reg_info);
TEL_LOG_REG_NODE *tel_log_search_node(unsigned int id);

FILE *tel_log_file_open(char *file,unsigned int level);

unsigned int tel_log_size_set(TEL_LOG_REG_NODE *log_node,unsigned int level,char *file,FILE *fp);

void tel_write_log(unsigned int id,unsigned int level,const char *fmt,...);

#define TEL_LOG(MODULE_ID,LEVEL,FMT,...) \
do \
{\
    tel_write_log(MODULE_ID,LEVEL," <%s:%d>"FMT,__func__,__LINE__, ##__VA_ARGS__); \
}while(0)\

#ifdef __cplusplus
}
#endif
#endif
