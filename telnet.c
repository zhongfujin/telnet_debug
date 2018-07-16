#include <stdio.h>  
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <pty.h>
#include <arpa/telnet.h>

#include <errno.h>


#define TELNETD_TC_IAC     255        /* interpret as command: */
#define TELNETD_TC_DONT    254        /* you are not to use option */
#define TELNETD_TC_DO      253        /* please, you use option */
#define TELNETD_TC_WONT    252        /* I won't use option */
#define TELNETD_TC_WILL    251        /* I will use option */
#define TELNETD_TC_SB      250        /* interpret as subnegotiation */
#define TELNETD_TC_GA      249        /* you may reverse the line */
#define TELNETD_TC_EL      248        /* erase the current line */
#define TELNETD_TC_EC      247        /* erase the current character */
#define TELNETD_TC_AYT     246        /* are you there */
#define TELNETD_TC_AO      245        /* abort output--but let prog finish */
#define TELNETD_TC_IP      244        /* interrupt process--permanently */
#define TELNETD_TC_BREAK   243        /* break */
#define TELNETD_TC_DM      242        /* data mark--for connect. cleaning */
#define TELNETD_TC_NOP     241        /* nop */
#define TELNETD_TC_SE      240        /* end sub negotiation */
#define TELNETD_TC_EOR     239        /* end of record (transparent mode) */
#define TELNETD_TC_ABORT   238        /* Abort process */
#define TELNETD_TC_SUSP    237        /* Suspend process */
#define TELNETD_TC_EOF     236        /* End of file */
#define TELNETD_TC_SYNCH   242        /* for telfunc calls */

#define MOVE_UP     "\033[A"
#define MOVE_DOWN   "\033[B"
#define MOVE_RIGHT  "\033[C"
#define MOVE_LEFT   "\033[D"



#define BUFSIZE 1024
#define PORT  12000
void process_thread(void *arg);
int telnet_ser();

void proccess_cmd(char *cmd_line);
void *shell_thread_func(void *arg);
void keyboard_move_up(int fd);
void keyboard_move_down(int fd);
void keyboard_backsapce(char *cmd,int len,int fd);
void keyboard_move_left(int fd,int len);
void keyboard_move_right(int fd);
void insert_char(char *cmd,int len,char c,int fd);

#define MAX_CMD_HISTORY_NUM  500

typedef struct cmd
{
    char cmd[512];
    int size;
}IN_CMD;

typedef struct cmd_history
{
    int all_cmd_num;
    IN_CMD queue_cmd[MAX_CMD_HISTORY_NUM];
}CMD_QUEUE;

CMD_QUEUE *history_cmd;
int has_input_cmd_num = 0;
int cmd_index = 0;;

int fd_server;
int fd_conn;

int move_left_num = 0;
int move_right_num = 0;

char g_cmd[512] = {0};

int telnet_ser()
{
    int ret = 0;
    int cli_size;
    pthread_t tid;
    int on = 1;
    
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    memset(&server_addr,0,sizeof(server_addr));
    memset(&client_addr,0,sizeof(client_addr));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr  = htonl(INADDR_ANY);
    server_addr.sin_port = htons(12000);
    
    fd_server = socket(AF_INET,SOCK_STREAM,0);
    if(fd_server < 0)
    {
        perror("socket");
        return -1;
    }
    
    setsockopt(fd_server,SOL_SOCKET,SO_REUSEADDR,(const char *)&on, sizeof(on));
    ret = bind(fd_server,(struct sockaddr *)&server_addr,sizeof(struct sockaddr));
    if(ret < 0)
    {
        perror("bind");
        return -1;
    }
    
    listen(fd_server,5);
    while(1)
    {
        fd_conn = accept(fd_server,(struct sockaddr *)&client_addr,&cli_size);
        if(fd_conn < 0)
        {
            perror("accept");
        }
        printf("accept a connecttion!\n");
        ret = pthread_create(&tid,NULL,(void *)process_thread,(void *)fd_conn);
        if(ret < 0)
        {
            perror("pthread_create");
            return -1;
        }
        pthread_join(tid,NULL);
    }
    
}

int write_reliable(int fd, const void *buf, size_t count)
{
    int ret;

TRY_AGAIN:
    ret = write(fd, buf, count);
    if (ret<0 && EINTR==errno)
        goto TRY_AGAIN;

    return ret;
}

int write_certain_bytes(int fd, const void *buf, size_t count)
{
    int ret, left=count, finished=0;

    while (left>0)
    {
        ret = write_reliable(fd, buf+finished, left);
        if (ret<0)
            return ret;

        finished+=ret;
        left-=ret;
    }

    return 0;
}

int read_reliable(int fd, void *buf, size_t count)
{
    int ret;

TRY_AGAIN:
    ret = read(fd, buf, count);
    if (ret<0 && EINTR==errno)
        goto TRY_AGAIN;

    return ret;
}

int read_certain_bytes(int fd, void *buf, size_t count)
{
    int ret, left=count, finished=0;

    while (left>0)
    {
        ret = read_reliable(fd, buf+finished, left);
        if (ret<0)
            return ret;

        finished+=ret;
        left-=ret;
    }

    return 0;
}
void process_thread(void *arg)
{
    int fd = (int)arg;
    make_new_session(fd);
    
}

void send_iac(int sockfd, char cmd, char opt)
{
    char data[3] = {IAC, cmd, opt};
    write_certain_bytes(sockfd, data, sizeof(data));
}

void set_useful_sock_opt(int sockfd)
{
    int nRecvBuf=256*1024;
    int nSendBuf=256*1024;
    int reuse_addr = 1;
    socklen_t optlen = sizeof(int);
    setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,&nRecvBuf,optlen); 
    setsockopt(sockfd,SOL_SOCKET,SO_SNDBUF,&nSendBuf,optlen);
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&reuse_addr,optlen);
}

int make_new_session(int new_accept_fd)
{
    printf("make_new_session\n");
    pthread_t the_shell_thread;
    int ret;
    char buf[512];
    set_useful_sock_opt(new_accept_fd);
    send_iac(new_accept_fd,DONT,TELOPT_ECHO);
    send_iac(new_accept_fd,DO,TELOPT_NAWS);
    send_iac(new_accept_fd,DO,TELOPT_LFLOW);
    send_iac(new_accept_fd,WILL,TELOPT_ECHO);
    send_iac(new_accept_fd,WILL,TELOPT_SGA);

    //set_fd_nonblock(new_accept_fd);
    
    printf("create shell thread!\n");
    pthread_create(&the_shell_thread, NULL, (void *)shell_thread_func, (void *)new_accept_fd);
}

int telnetd_remove_iac(char *pBuf, int uiLen)
{
    char *ptr0 = pBuf;
    char *ptr = ptr0;
    char *totty = ptr;
    char *end = ptr + uiLen;
    int num_totty;

    while (ptr < end) 
    {
        if (*ptr != IAC) 
        {
            char c = *ptr;

            *totty++ = c;
            ptr++;
            /* We map \r\n ==> \r for pragmatic reasons.
             * Many client implementations send \r\n when
             * the user hits the CarriageReturn key.
             */
            if (c == '\r' && ptr < end && (*ptr == '\n' || *ptr == '\0'))
                ptr++;
            continue;
        }

        if ((ptr+1) >= end)
            break;
        if (ptr[1] == TELNETD_TC_NOP) 
        { /* Ignore? (putty keepalive, etc.) */
            ptr += 2;
            continue;
        }
        if (ptr[1] == IAC) 
        { /* Literal IAC? (emacs M-DEL) */
            *totty++ = ptr[1];
            ptr += 2;
            continue;
        }

        /*
         * TELOPT_NAWS support!
         */
        if ((ptr+2) >= end) 
        {
            /* only the beginning of the IAC is in the
            buffer we were asked to process, we can't
            process this char. */
            break;
        }
        /*
         * IAC -> SB -> TELOPT_NAWS -> 4-byte -> IAC -> SE
         */
        if (ptr[1] == TELNETD_TC_SB && ptr[2] == TELOPT_NAWS) 
        {
            struct winsize ws;
            ws.ws_col = (ptr[3] << 8) | ptr[4];
            ws.ws_row = (ptr[5] << 8) | ptr[6];
            if ((ptr+8) >= end)
                break;    /* incomplete, can't process */
            ptr += 9;
            continue;
        }
        /* skip 3-byte IAC non-SB cmd */

        ptr += 3;
    }

    num_totty = totty - ptr0;
    
    return num_totty;

    /* move chars meant for the terminal towards the end of the buffer */
  
}

void *shell_thread_func(void *arg)
{
    int ret = 0;
    int fd = (int)arg;
    char c;
    char cmd_line[512];
    char *cmd_buf = malloc(512);
    memset(cmd_buf,0,512);
    char light[16] = {'\0'};
    int cmd_len = 0;
    int char_index = 0;
    int ii = 0;
    int jj = 0;
    int kk = 0;
    printf("thread_fun\n");
    int login = 1;
    while(1)
    {
        memset(cmd_line,0,512);
        //bzero(g_cmd,512);
        ret = read(fd,cmd_line,512);
        if(ret < 0)
        {
            usleep(1000);
            continue;
        
        }
        telnetd_remove_iac(cmd_line,strlen(cmd_line));

        if(login == 1)
        {
            kk++;
            if(kk == 3)
               login = 0; 
            continue;
        }
        
        if(strcmp(cmd_line,MOVE_UP) == 0)
        {
            keyboard_move_up(fd);
            continue;
        }
        else if(strcmp(cmd_line,MOVE_DOWN) == 0)
        {
            keyboard_move_down(fd);
            continue;
        }
        else if(strcmp(cmd_line,MOVE_LEFT) == 0)
        {
            if(cmd_index != has_input_cmd_num)
                keyboard_move_left(fd,strlen(g_cmd));
            else
                keyboard_move_left(fd,strlen(cmd_buf));
            continue;
        }
        
        else if(strcmp(cmd_line,MOVE_RIGHT) == 0)
        {
            keyboard_move_right(fd);
            continue;
        }

        else if(cmd_line[0] == '\b')
        {
            
            if(cmd_index != has_input_cmd_num)
            {
                
                keyboard_backsapce(g_cmd,strlen(g_cmd),fd);
                
                if(move_left_num > 0)
                {
                    ret = sprintf(light,"\033[%dD",move_left_num);
                    write(fd,light,ret);
                }
                continue;
            }
            else
            {
                keyboard_backsapce(cmd_buf,strlen(cmd_buf),fd);
                if(move_left_num == 0)
                {
                    ret = sprintf(light,"\033[1C");
                    write(fd,light,ret);
                }
                if(move_left_num > 0)
                {
                    ret = sprintf(light,"\033[%dD",move_left_num - 1);
                    write(fd,light,ret);
                }
                
            }
        }
        
        if((cmd_index != has_input_cmd_num) && (strlen(g_cmd) > 0) && (32 <= cmd_line[0]) && (cmd_line[0] <= 127))
        {
            printf("insert  cmd_char:%x\n",cmd_line[0]);
            insert_char(g_cmd,strlen(g_cmd),cmd_line[0],fd);
        }
        if(cmd_line[0] == '\r')
        {   
            move_left_num  =0;
            write_certain_bytes(fd,"\r\n[FSM]#",strlen("\r\n[FSM]#"));
            if(cmd_index != has_input_cmd_num)
            {
              
                printf("%s\r\n",g_cmd);
                fflush(stdout);
                goto CMD_OVER;
            }
            if(cmd_len > 0)
            {
                memset(g_cmd,0,512);
                sprintf(g_cmd,"%s",cmd_buf);
                printf("%s\r\n",g_cmd);
                fflush(stdout);
                goto CMD_OVER;
            }
            if(cmd_len == 0)
            {
                printf("\r\n");
                fflush(stdout);
                continue;
            }

CMD_OVER:
            memset(cmd_buf,0,512);
            jj  = 0;
            cmd_len = 0; 
            update_history_cmd(g_cmd);
        }
        else
        {
            if(move_left_num != 0 && strlen(cmd_buf) > 0)
            {
                memset(light,0,16);
                insert_char(cmd_buf,strlen(cmd_buf),cmd_line[0],fd);
                ret = sprintf(light,"\033[%dD",move_left_num);
                write(fd,light,ret);
                continue;
            }
            if(cmd_index == has_input_cmd_num)
            {
                write(fd,cmd_line,strlen(cmd_line));
            }
            for(ii = 0;ii < strlen(cmd_line);ii++)
            {
                cmd_buf[jj] = cmd_line[ii];
                cmd_buf[jj + 1] = '\0';
                jj++;
                cmd_len++;
            }

        }
        
    }
    return NULL;
    
}

int update_history_cmd(char *cmd)
{
    int ret = 0;
   
    if(has_input_cmd_num >= MAX_CMD_HISTORY_NUM)
        return;
    ret = sprintf(history_cmd->queue_cmd[has_input_cmd_num].cmd,"%s",cmd);
    history_cmd->queue_cmd[has_input_cmd_num].size = ret;
    has_input_cmd_num++;
    history_cmd->all_cmd_num = has_input_cmd_num;
    cmd_index = has_input_cmd_num;
    return 0;
}

void keyboard_move_up(int fd)
{
    move_left_num  = 0;
    if(cmd_index == 0)
        return;
    memset(g_cmd,0,512);
    char buf[512] = {'\0'};
    int ret = 0;
    write(fd,"\033[1K",7);
 
    ret = sprintf(buf,"\r[FSM]#%s",history_cmd->queue_cmd[cmd_index - 1].cmd);
 
    write_certain_bytes(fd,buf,strlen(buf));
    cmd_index--;
    sprintf(g_cmd,"%s",history_cmd->queue_cmd[cmd_index].cmd);
}
void keyboard_move_down(int fd)
{
    move_left_num  = 0;
    if(cmd_index == has_input_cmd_num)
        return;
    memset(g_cmd,0,512);
    char buf[512] = {'\0'};
    int ret = 0;
    write(fd,"\033[1K",7);
 
    ret = sprintf(buf ,"\r[FSM]#%s",history_cmd->queue_cmd[cmd_index + 1].cmd);

    write_certain_bytes(fd,buf,strlen(buf));
    cmd_index++;
    sprintf(g_cmd,"%s",history_cmd->queue_cmd[cmd_index].cmd);
}

void keyboard_backsapce(char *cmd,int len,int fd)
{
    if(len <= 0 || move_left_num == len)
        return;
    char buf[512] = {'\0'};

    if(move_left_num != 0)
    {
        memmove(cmd + (len - 1 - move_left_num),cmd + (len - move_left_num),move_left_num);
        cmd[len - 1] = '\0';
    }
    
    else
    {
        cmd[len - 1] = '\0';
    }
    write(fd,"\033[1K",7);
    sprintf(buf,"\r[FSM]#%s",cmd);
 
    write_certain_bytes(fd,buf,strlen(buf));
    
}

void insert_char(char *cmd,int len,char c,int fd)
{
    if(len <= 0 || len > 512 - 1)
        return;
    char buf[512] = {'\0'};
    char light[512] = {'\0'};
    int ret = 0;
    if(move_left_num == 0)
    {
        cmd[len] = c;
        cmd[len + 1] = '\0';
        write(fd,"\033[2K",7);
        ret = sprintf(buf,"\r[FSM]#%s",cmd);
        write_certain_bytes(fd,buf,ret);

        
       // write_certain_bytes(fd,&c,1);
        return;
    }
    if(move_left_num != 0 && move_left_num < len)
    {
        memmove(cmd + (len - move_left_num + 1),cmd + (len -  move_left_num),move_left_num);
        cmd[len -  move_left_num] = c;
        cmd[len + 1] = '\0';
        write(fd,"\033[1K",7);
        ret = sprintf(buf,"\r[FSM]#%s",cmd);
        write_certain_bytes(fd,buf,ret);
        
        //write_certain_bytes(fd,&c,1);
        return;
    }
}

void keyboard_move_left(int fd,int len)
{
    if(move_left_num >= len)
    {
        printf("left len:%d\n",move_left_num);
        return;
    }
    write_certain_bytes(fd,"\033[D",6);
    move_left_num++;
    printf("left len:%d\n",move_left_num);
}

void keyboard_move_right(int fd)
{
    if(move_left_num > 0)
    {
        write_certain_bytes(fd,"\033[C",6);
        move_left_num--;
    }
    else
        return;
}

int main()
{
    history_cmd = malloc(sizeof(CMD_QUEUE));
    memset(history_cmd,0,sizeof(CMD_QUEUE));
    telnet_ser();
}