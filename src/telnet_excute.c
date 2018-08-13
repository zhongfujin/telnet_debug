#include "telnet_getsymbol_addr.h"
#include "telnet_com.h"
#include <stdint.h>
#include <ctype.h>
#include <inttypes.h>

/*解析命令行*/
unsigned int parse_function_args(long *args, char *para_str_begin, char *para_str_end)

{
    int arg_nr = 0;
    char *tmp;
    char *cur_arg = para_str_begin;

    if (para_str_begin == para_str_end) 
        return 0;

    tmp = para_str_begin - 1;

    /*若参数为整数，便直接保存。为字符串就保存地址*/
    do 
    {
        cur_arg = tmp + 1;
        if (cur_arg == para_str_end) 
        {
            return -1;
        }
        if (!isdigit(cur_arg[0]) && cur_arg[0] != '-' && cur_arg[0] != '"') 
        {
            return -1;
        }
        
        if (isdigit(cur_arg[0]) || '-' == cur_arg[0])
        {
            args[arg_nr] = strtol(cur_arg,NULL,0);
        }
        else if (cur_arg[0] == '"')
        {
            cur_arg++;
            args[arg_nr] = (uintptr_t)(uintptr_t)(void *)cur_arg;
            
            cur_arg = strchr(cur_arg, '"');
            if (NULL == cur_arg)
            {
                return -1;
            }
            *cur_arg = 0;
            cur_arg++;
        }
        else
            return -1;
        
        arg_nr++;
        tmp = strchr(cur_arg, ',');
    }while (tmp && arg_nr < MAX_ARG_NUM);

    return 0;
}


unsigned int parse_func_call(char *buf, func_type *fun_addr, long *args)
{
    char *func_name;
    char *para_str_begin;
    char *para_str_end;
    unsigned int ret;

    func_name = buf;
    para_str_begin = strchr(buf, '(');
    *para_str_begin = 0;
    para_str_begin++;

    /*获取符号地址*/
    uintptr_t addr = ld_find_address(func_name);
    if(addr != -1)
    {
        *fun_addr = (void *)addr;
    }
    
    else
    {
        TELD_PRINTF("unknown symbol %s", func_name);
        return -1;
    }


    para_str_end = strrchr(para_str_begin, ')');
    if (NULL == para_str_end)
    {
        TELD_PRINTF("invalid function call syntax");
        return -1;
    }
    *para_str_end = 0;

    ret = parse_function_args(args, para_str_begin, para_str_end);
    if (ret)
    {
        TELD_PRINTF("invalid function call syntax");
    }

    return ret;
}


void do_call_func(func_type pfunc, long *args)
{
    int64_t ret = pfunc(args[0]
        ,args[1]
        ,args[2]
        ,args[3]
        ,args[4]
        ,args[5]
        ,args[6]
        ,args[7]);

#if __WORDSIZE == 32
    TELD_PRINTF("value:0x%-16"PRIx32" (%"PRIi32")\n",(uint32_t )ret, (int32_t )ret);
#else
    TELD_PRINTF("value:0x%-16"PRIx64" (%"PRIi64")\n",(uint64_t )ret, (int64_t )ret);
#endif



}



unsigned int show_var_info(const char *var_name)
{
    uintptr_t addr = ld_find_address(var_name);
    if(addr == -1)
    {
        TELD_PRINTF("unkown symbol:%s",var_name);
        return -1;
    }

    void *var_addr = NULL;
    var_addr = (void *)(addr);

#if __WORDSIZE == 32
    TELD_PRINTF("address:%p value:0x%-16"PRIx32" (%"PRIi32")\n",var_addr,(uint32_t *)var_addr, (int32_t *)var_addr);
#else
    TELD_PRINTF("address:%p value:0x%-16"PRIx64" (%"PRIi64")\n",var_addr,(uint64_t *)var_addr, (int64_t *)var_addr);
#endif
    

    return 1;

}




void exec_function(char *call_func_str)
{
    func_type pfunc;
    long  args[MAX_ARG_NUM];
    
    int ret = parse_func_call(call_func_str, &pfunc, args);


    if (!ret)
        do_call_func(pfunc, args);


}


void proccess_cmd(char *cmd_line)
{
    if (strchr(cmd_line, '(') == NULL)
    {
        show_var_info(cmd_line);
    }
    else
    {
        exec_function(cmd_line);
    }
    fflush(stdout);

}



