#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>

#define MAX_ARGS_NUM 10

typedef struct param
{
    unsigned char arg_buf[256];
}PARAM;

typedef struct tag_param_info
{
    int count;
    char func_name[256];
    PARAM args_re[MAX_ARGS_NUM];
}PARAM_MGR;

PARAM_MGR args;
#define   MAX_ARG_NUM    (8)
typedef  int64_t (*func_type)(long para1, ...);

typedef void *(*handle)()

typedef enum sym_type
{
    SIMPLE_SYM = 0,
    FUNC_SYM    
}SYM_TYPE;

SYM_TYPE get_symbol_type(char *cmd_str)
{
    char *func_judge = index(cmd_str,'(');
    if(func_judge == NULL)
    {
        return SIMPLE_SYM;
    }
    else
    {
        return FUNC_SYM;
    }
}

 int function_param_parse(char *cmd_str)
 {
    int ii = 0;
    memset(&args,0,sizeof(PARAM_MGR));
    SYM_TYPE sym_type = get_symbol_type(cmd_str);
    if(sym_type == SIMPLE_SYM)
    {
        args.count = 1;
        strcpy(args.args_re[args.count].arg_buf,cmd_str);
        return 1;
    }

    char *param_begin = strchr(cmd_str,'(');
    int func_name_len = param_begin - cmd_str;
    snprintf(args.func_name,func_name_len + 1,"%s",cmd_str);

    *param_begin++;

    char * param_end = strchr(cmd_str,')');
    int param_len = param_end - param_begin;
	
	if(param_begin == param_end)
	{
		return 1;
	}
    int count = 0;
	
	char real_param[256] = {'\0'};

	int temp = 0;
	for(ii == 0; ii < param_len;ii++)
	{
		if(param_begin[ii] == ' ' || param_begin[ii] == '\t' || \
            param_begin[ii] == '\n' || param_begin[ii] == '\0')
		{
			continue;
		}
        real_param[temp++] = param_begin[ii];
		
		if(param_begin[ii] == ',')
		{
			count++;
		}
	}
	
	args.count = count + 1;
	
	char *ptr = NULL;
	ptr = strtok(real_param,",");
	strcpy(args.args_re[0].arg_buf,ptr);
	
	
	for(ii = 1 ; ii <= count; ii++)
	{
		ptr = strtok(NULL,",");
		if(ptr)
		{
			strcpy(args.args_re[ii].arg_buf,ptr);

		}
	}
	
 }
 
 
 static int parse_function_args(long *args, char *para_str_begin, char *para_str_end)

{
    int arg_nr = 0;
    char *p_tmp;
    char *cur_arg = para_str_begin;

    if (para_str_begin==para_str_end) 
        return 0;

    p_tmp = para_str_begin-1;
    do 
    {
        cur_arg = p_tmp+1;
        if (cur_arg==para_str_end) 
            return -1;
        if (!isdigit(cur_arg[0]) && cur_arg[0]!='-' && cur_arg[0]!='"') 
            return -1;

        if (isdigit(cur_arg[0]) || '-'==cur_arg[0])
        {
            args[arg_nr] = strtol(cur_arg,NULL,0);
        }
        else if (cur_arg[0]=='"')
        {
            cur_arg++;
            args[arg_nr] = (long)(unsigned long)(void *)cur_arg;
            
            cur_arg = strchr(cur_arg, '"');
            if (!cur_arg)
                return -1;
            *cur_arg = 0;
            cur_arg++;
        }
        else
            return -1;
        
        arg_nr++;
        p_tmp = strchr(cur_arg, ',');
    } while (p_tmp && arg_nr<MAX_ARG_NUM);

    return 0;
}


static int parse_func_call(char *buf, func_type *fun_addr, long *args)
{
    char *func_name, *para_str_begin, *para_str_end;
    int ret;

    func_name = buf;
    para_str_begin = strchr(buf, '(');
    *para_str_begin = 0;
    para_str_begin++;
    
    
    para_str_end = strrchr(para_str_begin, ')');
    if (!para_str_end)
    {
        printf("invalid function call syntax");
        return -1;
    }
    *para_str_end = 0;

    ret = parse_function_args(args, para_str_begin, para_str_end);
    if (ret == 0)
    {
        printf("invalid function call syntax");
    }

    return ret;
}


int main(int argc ,char *argv[])
{
	char *cmd_str = argv[1];
	printf("cmd  %s\n",cmd_str);
	function_param_parse(cmd_str);
	
	int ii = 0;
	for(ii = 0;ii < args.count;ii++)
	{
		printf("buf: [%s]\n",args.args_re[ii].arg_buf);
	}

	for(ii = 0;ii < MAX_ARGS_NUM; ii++)
	{
		printf("%lu\n",args[ii]);
	}
	return 1;
}	