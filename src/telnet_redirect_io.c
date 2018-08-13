#include "telnet_redirect_io.h"
#include "telnet_com.h"


unsigned int g_ori_std_input;


unsigned int g_ori_std_output;


unsigned int g_ori_std_err;


void save_ori_io()
{
    g_ori_std_input = dup(0);
    g_ori_std_output = dup(1);
    g_ori_std_err = dup(2);
}

void restore_ori_io()
{
    dup2(g_ori_std_input, 0);
    dup2(g_ori_std_output,1);
    dup2(g_ori_std_err,2);
}


void redirect_io(TELNET_MGR *tel_mgr)
{
    dup2(tel_mgr->fd_conn,0);
    dup2(tel_mgr->fd_conn,1);
    dup2(tel_mgr->fd_conn,2);
}



void tel_printf(const char *fmt,...)
{
    unsigned int ret = 0;
    va_list args;
    va_start(args, fmt);
    char buf[TELNET_LINE_MAX_LEN] = {'\0'};
    char buf_write[TELNET_LINE_MAX_LEN] = {'\0'};
    int buf_len = 0;
    buf_len = vsnprintf(buf,TELNET_LINE_MAX_LEN,fmt,args);
    va_end(args);

    out_n_to_rn(buf,buf_write);

AGAIN:
    ret = write(1, buf_write, TELNET_LINE_MAX_LEN);
    if (ret < 0 && EINTR == errno)
    {
        goto AGAIN;
    }
}



unsigned int out_n_to_rn(char *pszsrc, char *pszdst)
{
    char *cp1;
    char *cp2;
    for (cp1 = pszsrc, cp2 = pszdst; *cp1 != '\0'; cp1++)
    {
        if (*cp1 == '\n')
        {
            *cp2++ = '\r';
            *cp2++ = '\n';
        }
        else
        {
            *cp2++ = *cp1;
        }
    }
    *cp2 = '\0';

    return RET_SUCCESS;
}


