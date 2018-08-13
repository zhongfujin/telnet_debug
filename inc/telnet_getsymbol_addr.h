#ifndef __TELNET_GETSYMBOL_ADDR__H
#define __TELNET_GETSYMBOL_ADDR__H

#include <stdint.h>
#include "telnet_com.h"

/**
 *\brief maps文件解析存储结构体
 */
struct ld_maps
{
    uintptr_t addr_start;
    uintptr_t addr_end;
    unsigned int permission;
    off_t offset;
    unsigned int device_major;
    unsigned int device_minor;
    ino_t inode;
    char pathname[128];
    size_t pathname_sz;
    unsigned int filetype;
    
};

/**
 *\brief so权限枚举
 */
enum 
{
    PROCMAPS_PERMS_NONE     = 0x0,
    PROCMAPS_PERMS_READ     = 0x1,
    PROCMAPS_PERMS_EXEC     = 0x2,
    PROCMAPS_PERMS_WRITE    = 0x4,
    PROCMAPS_PERMS_PRIVATE  = 0x8,
    PROCMAPS_PERMS_SHARED   = 0x10
};

/**
 *\brief so_name链表
 */
typedef struct so_name_node
{
    SLL_LINK_LIST_BASE so_name_list;
    unsigned int num;
}SO_NAME_LIST;

/**
 *\brief so_name节点
 */
typedef struct so_node
{
    SLL_LINK_LIST_NODE node;
    struct ld_maps map;
}SO_NODE;


unsigned int debug_init_list();
uintptr_t get_sym_addr(const char *filename,const char *sym_name);                  
unsigned int ld_find_library(char *libname);                                  
uintptr_t ld_find_address(const char *sym_name);             
unsigned int ld_maps_parse(char *buf,struct ld_maps *mymap);
unsigned int so_name_add_node(SO_NAME_LIST *list,struct ld_maps *map);
unsigned int get_maps_so_list(pid_t pid);

#endif