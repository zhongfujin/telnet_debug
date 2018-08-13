#include "telnet_getsymbol_addr.h"
#include "telnet_elfdetails.h"




SO_NAME_LIST *g_so_name_list = NULL;


unsigned int debug_init_list()
{
    unsigned int ret;
    g_so_name_list = malloc(sizeof(SO_NAME_LIST));
    if(NULL == g_so_name_list)
    {
        assert(0);
        ret = -1 ;
        goto EXIT_LABEL;
    }
    memset(g_so_name_list,0,sizeof(SO_NAME_LIST));

    return 0;
EXIT_LABEL:
    return ret;
    
}



uintptr_t get_sym_addr(const char *filename,const char *sym_name)
{
    unsigned int idx = 0;
    size_t *symbols_num = (size_t *)malloc(sizeof(size_t));
    struct elf_interp *interp = NULL;
    struct elf_symbol *symbols = exe_load_symbols(filename,symbols_num,interp);
    for(idx = 0; idx < *symbols_num; idx++)
    {
        if(symbols[idx].name != NULL)
        {
            if(strcmp(symbols[idx].name,sym_name) == 0)
            {
                return symbols[idx].address;
            }
        }
    }
    return -1;
}


uintptr_t ld_find_address(const char *sym_name)
{
    SO_NODE *node = NULL;
    uintptr_t sym_addr;
    unsigned int flag = 0;
   
    SLL_LINK_LIST_NODE *sllnode = g_so_name_list->so_name_list.head;
    while(sllnode)
    {
        node = (SO_NODE*)sllnode;
        sym_addr = get_sym_addr(node->map.pathname,sym_name);
        if(sym_addr != -1)
        {
            flag = 1;
            
            break;
        }
        sllnode = sllnode->next;
    }

    if(flag == 0)
    {
        return -1;
    }

    /*保证获取的符号在so的虚拟地址范围空间内*/
    if(sym_addr > node->map.addr_start)
    {
        TEL_INFO("find symbol in %s:0x%lx\n",node->map.pathname,sym_addr);
        return sym_addr;
    }
    else
    {
        sym_addr = node->map.addr_start + sym_addr;
        if(sym_addr < node->map.addr_end)
        {
            TEL_INFO("find symbol in %s:0x%lx\n",node->map.pathname,sym_addr);
            return sym_addr;
        }
    }
    return -1;
   
}



unsigned int so_name_add_node(SO_NAME_LIST *list,struct ld_maps *map)
{
    SO_NODE *so_node;
    so_node = malloc(sizeof(SO_NODE));
    if(NULL == so_node)
    {
        TEL_ERROR("os malloc failed!\n");
        return -1;
    }
    memcpy(&so_node->map,map,sizeof(struct ld_maps));
    if(!sll_link_list_insert_at_tail(&list->so_name_list,so_node))
    {
        printf("insert failed !\n");
    }
    list->num++;
    return 0;
}


unsigned int get_maps_so_list(pid_t pid)
{
    unsigned int bufsz = 4096;
    char *buf = malloc(bufsz);
    if(buf == NULL)
    {
        return -1;
    }
    memset(buf,0,bufsz);

    struct ld_maps *map = malloc(sizeof(struct ld_maps));
    memset(map,0,sizeof(struct ld_maps));
    
    char filename[128] = {0};

    /*maps文件路径*/
    sprintf(filename,"/proc/%d/maps",pid);
    if(access(filename,F_OK) != 0)
    {
        TEL_ERROR("No such file:%s\n",filename);
        return -1;
    }
    char self[256] = {0};

    /*获取可执行文件的文件名*/
    readlink("/proc/self/exe", self, 256);
    
    FILE *fp = fopen(filename,"r");
    if (NULL == fp)
    {
        TEL_ERROR("open failed!\n");
        return -1;
    }
    char *maps  = malloc(bufsz);
    memset(maps,0,bufsz);
    fseek(fp,0,SEEK_SET);

    /*初始化链表*/
    debug_init_list();
    SO_NODE *temp_node = NULL;

    /*获取maps文件的一行*/
    while(fgets(maps,bufsz,fp) != NULL)
    {
        ld_maps_parse(maps,map);
        if(map == NULL)
        {
            TEL_ERROR("maps parse failed!\n");
            return -1;
        }

        /*so name为空*/
        if (map->pathname == NULL)
        {
            continue;
        }

        /*[stack]等*/
        if(strchr(map->pathname,'['))
        {
            continue;
        } 

        /*过滤掉非so文件与程序本身*/
        if(strstr(map->pathname,".so") == NULL)
        {
            if(strstr(map->pathname,self) == NULL)
            {
                continue;
            }
        }
        
        if(g_so_name_list->num > 0)
        {
            temp_node = (SO_NODE *)sll_link_list_get_tail(&g_so_name_list->so_name_list);
            if(temp_node != NULL)
            {
                if(strcmp(temp_node->map.pathname,map->pathname) == 0)
                {
                
                    if(temp_node->map.addr_start < map->addr_start)
                    {
                        if(temp_node->map.addr_end < map->addr_end)
                        {
                           
                            temp_node->map.addr_end = map->addr_end;
                            
                        }
                        continue;
                    }
                    else
                    {
                        
                        temp_node->map.addr_start = map->addr_start;
                        if(temp_node->map.addr_end < map->addr_end)
                        {
                            temp_node->map.addr_end = map->addr_end;
                            
                        }
                        continue;
                    }
                }
            }
            else
            {
                printf("error!\n");
            }
        }
        if(!so_name_add_node(g_so_name_list,map))
        {
            TEL_ERROR("add node failed %s!\n",map->pathname);
        }
    }
    return RET_SUCCESS;
}


unsigned int ld_maps_parse(char *buf,struct ld_maps *mymap)
{
    struct ld_maps *map;
    char *ptr = NULL;
    char *token = NULL;
    if(buf == NULL)
    {
        TEL_ERROR("No input map\n");
        return -1;
    }
    
    token = strtok(buf,"-");
    map = malloc(sizeof(struct ld_maps));
    memset(map,0,sizeof(struct ld_maps));
    if(token != NULL)
    {
        map->addr_start = (uintptr_t)strtoul(token, NULL, 16);/*获取so在进程空间的起始地址*/
        mymap->addr_start = map->addr_start;
        token = strtok(NULL," ");
        if(NULL != token)
        {
            map->addr_end = (uintptr_t)strtoul(token, NULL, 16);/*获取so在进程空间的结束地址*/
            mymap->addr_end = map->addr_end;
        }   
    }
    if(token != NULL)
            token = strtok(NULL," ");
    map->permission = PROCMAPS_PERMS_NONE;/*获取该maps行对应so的权限*/
    int idx = 0;
    for (idx = strlen(token) - 1; idx >= 0; --idx) 
    {
        switch (token[idx]) 
        {
            case 'r':
                map->permission |= PROCMAPS_PERMS_READ;
                break;
            case 'w':
                map->permission |= PROCMAPS_PERMS_WRITE;
                break;
            case 'x':
                map->permission |= PROCMAPS_PERMS_EXEC;
                break;
            case 'p':
                map->permission |= PROCMAPS_PERMS_PRIVATE;
                break;
            case 's':
                map->permission |= PROCMAPS_PERMS_SHARED;
                break;
            case '-':
                break;
            default:
                printf("invaild\n");
                break;
        }
    }
    mymap->permission = map->permission;
    if(token != NULL)
    {
        token = strtok(NULL," ");
    }
    
    map->offset = (off_t)strtoul(token, NULL, 16);/*获取偏移大小*/
    mymap->offset = map->offset;
    if(token != NULL)
    {
        token = strtok(NULL, ":");
    }
    
    map->device_major = (int)strtol(token, NULL, 10);/*获取主设备号*/
    mymap->device_major = map->device_major;
    if(token != NULL)
    {
        token = strtok(NULL, " ");
    }
    
    map->device_minor = (int)strtol(token, NULL, 10);
    mymap->device_minor = map->device_minor;
    if(token != NULL)
    {
        token = strtok(NULL, " ");
    }
    
    map->inode = (ino_t)strtoul(token, NULL, 10);/*获取文件inode*/
    mymap->inode = map->inode;
    if(token != NULL)
    {
        token = strtok(NULL, "\n");
    }
    if(token != NULL)
    {
        if((ptr = strchr(token,'/')) != NULL )/*获取so名称*/
        {
            map->pathname_sz = strlen(ptr);
            strncpy(map->pathname ,ptr,map->pathname_sz);
            mymap->pathname_sz = map->pathname_sz;
        }
        if((ptr = strchr(token,'[')) != NULL )/*是否为[stack] [vsdo]等*/
        {
            map->pathname_sz = strlen(ptr);
            strncpy(map->pathname ,ptr,map->pathname_sz);
        }

    }
    strcpy(mymap->pathname,map->pathname);
    free(map);
    map = NULL;
    return 0;   
}

