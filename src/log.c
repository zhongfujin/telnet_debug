#include <stdio.h>
#include <sys/stat.h>
#include "log.h"



/*This file achieve the log that accept reg and control log file size 
you can output more than one file base your reg*/

/*注册链表*/
TEL_LOG_REG_BASE *g_tellog_reglist; 

/*telnet log int,gloab var and log path*/
unsigned int tel_log_init()
{
    if(TEL_LOG_SWITCH == 0)
    {
        printf("unable tel log!\n");
        return RET_SUCCESS;
    }
    g_tellog_reglist = malloc(sizeof(TEL_LOG_REG_BASE));
    if(g_tellog_reglist == NULL)
    {
        printf("tel_log malloc failed!\n");
        assert(0);
        return RET_ERROR;
    }
    memset(g_tellog_reglist,0,sizeof(TEL_LOG_REG_BASE));

    g_tellog_reglist->reg_list = malloc(sizeof(SLL_LINK_LIST_BASE));
    if(NULL == g_tellog_reglist->reg_list)
    {
        printf("malloc failed!\n");
        assert(0);
        return RET_ERROR;
    }
    g_tellog_reglist->reg_list->head = NULL;
    g_tellog_reglist->reg_list->tail = NULL;
   
    g_tellog_reglist->list_num = 0;


    char tel_log_path[256] = {'\0'};
    sprintf(tel_log_path,"%s","/home/zhong");
    strcat(tel_log_path,"/log");
    if(access(tel_log_path,F_OK) == -1)
    {
        if(!mkdir(tel_log_path,0666))
        {
            printf("create %s failed!\n",tel_log_path);
        }
        return RET_ERROR;
    }

    return RET_SUCCESS;
}


/*get the gloab reg list*/
TEL_LOG_REG_BASE *get_tel_log_reg_list()
{
    return g_tellog_reglist;
}


/*extern function,log reg*/
unsigned int tel_log_reg(TEL_LOG_REG_INFO *reg_info,unsigned int cnt)
{
    if(NULL == reg_info || cnt <= 0)
    {
        printf("reg_info is NULL!\n");
        assert(0);
        return RET_ERROR;
    } 
    unsigned int ii = 0;
    TEL_LOG_REG_INFO *reg = NULL;
    TEL_LOG_REG_NODE *search_node = NULL;
    for(ii = 0; ii < cnt; ii++)
    {
        search_node = tel_log_search_node(reg_info[ii].module_id);
        if(search_node != NULL)
        {
            printf("node has reg! ID:%d\n",search_node->reg_info.module_id);
            assert(0);
            return RET_ERROR;
        }
        reg = reg_info + ii;
        tel_add_log_reg_node(reg);
    
    }
    return RET_SUCCESS;
}


/*add reg node*/
TEL_LOG_REG_NODE *tel_add_log_reg_node(TEL_LOG_REG_INFO *reg_info)
{
    TEL_LOG_REG_BASE *tel_log_reg_list = get_tel_log_reg_list();
    TEL_LOG_REG_NODE *reg_node = malloc(sizeof(TEL_LOG_REG_NODE));
    if(NULL == reg_node)
    {
        printf("malloc failed!\n");
        assert(0);
        return NULL;
    }
    memset(reg_node,0,sizeof(TEL_LOG_REG_NODE));
    memcpy(&reg_node->reg_info,reg_info,sizeof(TEL_LOG_REG_INFO));
    sll_link_list_insert_at_tail(tel_log_reg_list->reg_list,reg_node);
    tel_log_reg_list->list_num++; 
    return reg_node;
}

/*search node by modlue_id*/
TEL_LOG_REG_NODE *tel_log_search_node(unsigned int module_id)
{
    TEL_LOG_REG_NODE *node = NULL;
    int i = 0;
    for( i = 1; i <= g_tellog_reglist->list_num;i++)
    {
        node = sll_link_list_get_index(g_tellog_reglist->reg_list,i);
        if(node->reg_info.module_id == module_id)
        {
            break;
        }
        node = NULL;
    }
   
    return node;
}


/*create and open log file*/
FILE *tel_log_file_open(char *file,unsigned int level)
{
   
    FILE *fp = NULL;
    if(access(file,F_OK) < 0)
    {
       fp = fopen(file,"at+");
       if(fp == NULL)
       {
            printf("create tel thread log file failed!with error:%s\n",strerror(errno));
            return NULL;
       }
       fclose(fp);
    }
    
    fp = fopen(file,"rt+");
    if(fp == NULL)
    {
        printf("tel_log_file_open open or create file failed!\n");
        return NULL;
    }
    return fp;
}

/*check the log file size,if it's larger than the log size that you reg,it will write at start*/
unsigned int tel_log_size_set(TEL_LOG_REG_NODE *log_node,unsigned int level,char *file,FILE *fp)
{
    if(TEL_LOG_SWITCH == 0)
    {
        return RET_SUCCESS;
    }
    struct stat info;
    memset(&info,0,sizeof(info));
    if(access(file,F_OK) < 0 || fp == NULL)
    {
        printf("log file open or create failed!\n");
        return -1;
    }
   
    if(stat(file,&info) < 0)
    {
        fclose(fp);
        fp = NULL;
        printf("tel_log_size_set stat file failed!\n");
        return -1;
    }

    /*设置日志文件大小在超过设定大小后，从头开始写*/
    if(log_node->reg_info.mulitifile == FALSE)
    {
        goto SINGLE;
    }
    else
    {
        switch(level)
        {
            case TEL_LOG_SECURITY_NOTSET:
                fseek(fp,log_node->reg_info.cur_size.cur_notset_log_size,SEEK_SET);
                if(log_node->reg_info.max_log_size <= log_node->reg_info.cur_size.cur_notset_log_size)
                {
                    
                    rewind(fp);
                    log_node->reg_info.cur_size.cur_notset_log_size = 0;
                }
                break;
            case TEL_LOG_SECURITY_DEBUG:
                fseek(fp,log_node->reg_info.cur_size.cur_debug_log_size,SEEK_SET);
                if(log_node->reg_info.max_log_size <= log_node->reg_info.cur_size.cur_debug_log_size)
                {
                    
                    rewind(fp);
                    log_node->reg_info.cur_size.cur_debug_log_size = 0;
                }
                break;
            case TEL_LOG_SECURITY_INFO:
                fseek(fp,log_node->reg_info.cur_size.cur_info_log_size,SEEK_SET);
                if(log_node->reg_info.max_log_size <= log_node->reg_info.cur_size.cur_info_log_size)
                {
                    
                    rewind(fp);
                    log_node->reg_info.cur_size.cur_info_log_size = 0;
                }
                break;
            case TEL_LOG_SECURITY_WARNING:
                fseek(fp,log_node->reg_info.cur_size.cur_warning_log_size,SEEK_SET);
                if(log_node->reg_info.max_log_size <= log_node->reg_info.cur_size.cur_warning_log_size)
                {
                    
                    rewind(fp);
                    log_node->reg_info.cur_size.cur_warning_log_size = 0;
                }
                break;
            case TEL_LOG_SECURITY_ERROR:
                fseek(fp,log_node->reg_info.cur_size.cur_error_log_size,SEEK_SET);
                if(log_node->reg_info.max_log_size <= log_node->reg_info.cur_size.cur_error_log_size)
                {
                    
                    rewind(fp);
                    log_node->reg_info.cur_size.cur_error_log_size = 0;
                }
                break;
            case TEL_LOG_SECURITY_CRITICAL:
                fseek(fp,log_node->reg_info.cur_size.cur_critical_log_size,SEEK_SET);
                if(log_node->reg_info.max_log_size <= log_node->reg_info.cur_size.cur_critical_log_size)
                {
                    
                    rewind(fp);
                    log_node->reg_info.cur_size.cur_critical_log_size = 0;
                }
                break;
            default:
                printf("No Such log level %d!\n",level);
                return RET_ERROR;
                break;   
        }
        return 0;
    }
SINGLE:
    fseek(fp,log_node->reg_info.cur_size.cur_log_size,SEEK_SET);
    if(log_node->reg_info.max_log_size <= log_node->reg_info.cur_size.cur_log_size)
    {
        
        rewind(fp);
        log_node->reg_info.cur_size.cur_log_size = 0;
    }
    return 0;
}

/*write log*/
void tel_write_log(unsigned int module_id,unsigned int level,const char *fmt,...)
{

	if(TEL_LOG_SWITCH == 0)
    {
        return;
    }
	
    /*判断ID是否注册*/
    TEL_LOG_REG_NODE *log_node = tel_log_search_node(module_id);
    if(NULL == log_node)
    {   
        printf("tel log is not reg,ID=%d\n",module_id);
        return;
    }

    if(level < log_node->reg_info.level)
    {
        printf("[tel_log] level %d is not set!the lowest level that you reg is %d\n",
                    level,log_node->reg_info.level);
        return;
    }
    char log_path[256] = {0};
    char buf[TEL_LOG_LINE_SIZE] = {'\0'};
    char time_buf[256] = {'\0'};
    char level_str[32] = {'\0'};
    int buf_len;
    FILE *fp = NULL;

    time_t timep;
    struct tm *tim = NULL;

    /*格式化打印*/
    va_list args;
    va_start(args, fmt);
    
    strcpy(log_path,"/home/zhong");
    strcat(log_path,"/log/");
    strcat(log_path,log_node->reg_info.file_name);
    
    switch(level)
    {
        case TEL_LOG_SECURITY_NOTSET:
            strcpy(level_str,SECURITY_NOTSET);
            break;
        case TEL_LOG_SECURITY_DEBUG:
            if(log_node->reg_info.mulitifile == TRUE)
            {
                strcat(log_path,"_debug.log");
            }
            strcpy(level_str,SECURITY_DEBUG);
            break;
        case TEL_LOG_SECURITY_INFO:
            if(log_node->reg_info.mulitifile == TRUE)
            {
                strcat(log_path,"_info.log");
            }
            strcpy(level_str,SECURITY_INFO);
            break;
        case TEL_LOG_SECURITY_WARNING:
            if(log_node->reg_info.mulitifile == TRUE)
            {
                strcat(log_path,"_warning.log");
            }
            strcpy(level_str,SECURITY_WARNING);
            break;
        case TEL_LOG_SECURITY_ERROR:
            if(log_node->reg_info.mulitifile == TRUE)
            {
                strcat(log_path,"_error.log");
            }
            strcpy(level_str,SECURITY_ERROR);
            break;
        case TEL_LOG_SECURITY_CRITICAL:
            if(log_node->reg_info.mulitifile == TRUE)
            {
                strcat(log_path,"_critical.log");
            }
            strcpy(level_str,SECURITY_CRITICAL);
            break;
        default:
            printf("No Such log level %d!\n",level);
            goto EXIT;
    }
    if(log_node->reg_info.mulitifile == FALSE)
    {
        strcat(log_path,".log");
    }
    fp  = tel_log_file_open(log_path,level);
    if(fp == NULL)
    {
        goto EXIT;
        return;
    }
    if(0 != tel_log_size_set(log_node,level,log_path,fp))
    {
        goto EXIT;
    }
    
    /*获取时间*/
    time(&timep);
    tim = localtime(&timep);
    if(tim == NULL)
    {
        printf("tel_write log get time failed!\n");
        return;
    }
    
    sprintf(time_buf,"%d/%02d/%02d-%02d:%02d:%02d %s",
                tim->tm_year + 1900,tim->tm_mon + 1,tim->tm_mday,tim->tm_hour,\
        tim->tm_min,tim->tm_sec,level_str);
    
    buf_len = vsnprintf(buf,TEL_LOG_LINE_SIZE,fmt,args);
    va_end(args);
    if(log_node->reg_info.mulitifile == FALSE)
    {
        goto SINGLE;
    }
    else
    {
        switch(level)
        {
            case TEL_LOG_SECURITY_NOTSET:
                log_node->reg_info.cur_size.cur_notset_log_size += strlen(time_buf);
                fwrite(time_buf,strlen(time_buf),1,fp);
                
                log_node->reg_info.cur_size.cur_notset_log_size += buf_len;
                fwrite(buf,buf_len,1,fp);
                break;
            case TEL_LOG_SECURITY_DEBUG:
                log_node->reg_info.cur_size.cur_debug_log_size += strlen(time_buf);
                fwrite(time_buf,strlen(time_buf),1,fp);

                log_node->reg_info.cur_size.cur_debug_log_size += buf_len;
                fwrite(buf,buf_len,1,fp);
                break;
            case TEL_LOG_SECURITY_INFO:
                log_node->reg_info.cur_size.cur_info_log_size += strlen(time_buf);
                fwrite(time_buf,strlen(time_buf),1,fp);
                
                log_node->reg_info.cur_size.cur_info_log_size += buf_len;
                fwrite(buf,buf_len,1,fp);
                break;
            case TEL_LOG_SECURITY_WARNING:
                log_node->reg_info.cur_size.cur_warning_log_size += strlen(time_buf);
                fwrite(time_buf,strlen(time_buf),1,fp);
                
                log_node->reg_info.cur_size.cur_warning_log_size += buf_len;
                fwrite(buf,buf_len,1,fp);
                break;
            case TEL_LOG_SECURITY_ERROR:
                log_node->reg_info.cur_size.cur_error_log_size += strlen(time_buf);
                fwrite(time_buf,strlen(time_buf),1,fp);
                
                log_node->reg_info.cur_size.cur_error_log_size += buf_len;
                fwrite(buf,buf_len,1,fp);
                break;
            case TEL_LOG_SECURITY_CRITICAL:
                log_node->reg_info.cur_size.cur_critical_log_size += strlen(time_buf);
                fwrite(time_buf,strlen(time_buf),1,fp);
                
                log_node->reg_info.cur_size.cur_critical_log_size += buf_len;
                fwrite(buf,buf_len,1,fp);
                break;
            default:
                printf("No Such log level %d!\n",level);
                break;   
        }
        goto EXIT;
     }
SINGLE:
    log_node->reg_info.cur_size.cur_log_size += strlen(time_buf);
    fwrite(time_buf,strlen(time_buf),1,fp);
                
    log_node->reg_info.cur_size.cur_log_size += buf_len;
    fwrite(buf,buf_len,1,fp);

EXIT:
    if(fp != NULL)
    {
        fclose(fp);
        fp = NULL;
    }
    return;
}


