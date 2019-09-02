#include "telnet_com.h"
#include "telnet.h"
#include "telnet_redirect_io.h"
#include "telnet_getsymbol_addr.h"
#include "log.h"
#include <arpa/telnet.h>
#include <errno.h>
#include <pthread.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> /* the L2 protocols */


#define MOVE_UP                 "\033[A"
#define MOVE_DOWN               "\033[B"
#define MOVE_RIGHT              "\033[C"
#define MOVE_LEFT               "\033[D"
#define CLEAR_RIGHT_CHAR        "\033[K"
#define CLEAR_LEFT_CHAR         "\033[1K"
#define CLEAR_LINE				"\033[2K"
#define LOC_START				"\033[0;0H"
#define CLEAR_TERM        		"\033[2J"
#define CTRL_C                  "\003"
#define CTRL_L					12


#define KEY_RCP					"\033[u"

#define BUFSIZE 1024
#define DEBUG_PORT  19000

#define DEBUG_SHELL "[debug_shell]#"

/*the telnet server,for socket creat and accept connection*/
unsigned int telnet_ser()
{
    int ret = 0;
    socklen_t cli_size;
    socklen_t bind_size;
    int on = 1;

    int server;
    int client;

    struct timeval tv;
    
    fd_set rfdset;
    int max_fd;
    int count;
    
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    memset(&server_addr,0,sizeof(server_addr));
    memset(&client_addr,0,sizeof(client_addr));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr  = htonl(INADDR_ANY);
    server_addr.sin_port = htons(DEBUG_PORT);

    server = socket(AF_INET,SOCK_STREAM,0);
    if(server < 0)
    {
        TEL_ERROR("socket failed!with error %s\n",strerror(errno));
        return -1;
    }
    
    setsockopt(server,SOL_SOCKET,SO_REUSEADDR,(const char *)&on, sizeof(on));
    bind_size = sizeof(struct sockaddr);
    ret = bind(server,(struct sockaddr *)&server_addr,bind_size);
    if(ret < 0)
    {
        TEL_ERROR("bind error!with error %s\n",strerror(errno));
        return -1;
    }
    
    listen(server,5);
    cli_size = sizeof(struct sockaddr);

    while(1)
    {
        FD_ZERO(&rfdset);
        FD_SET(server,&rfdset);
        max_fd = server;

        tv.tv_sec = 0;
        tv.tv_usec = 20000;
        
        count = select(max_fd + 1,&rfdset,NULL,NULL,&tv);
        if(count <= 0)
        {
            continue;
        }
        if(FD_ISSET(server,&rfdset))
        {
            client = accept(server,(struct sockaddr *)&client_addr,&cli_size);
            if(client < 0)
            {
                 TEL_ERROR("accept error!with error %s\n",strerror(errno));
            }
            TEL_INFO("accept a connecttion!\n");
            process_thread(client);
        }
    }
    return 0;


}


unsigned int write_reliable(unsigned int fd, const void *buf, size_t count)
{
    unsigned int ret;

AGAIN:
    ret = write(fd, buf, count);
    if (ret < 0 && EINTR == errno)
        goto AGAIN;

    return ret;
}

unsigned int write_certain_bytes(unsigned int fd, const void *buf, size_t count)
{
    unsigned int ret;
    unsigned int left = count;
    unsigned int finished = 0;

    while (left > 0)
    {
        ret = write_reliable(fd, buf + finished, left);
        if (ret < 0)
        {
            return ret;
        }

        finished += ret;
        left -= ret;
    }

    return 0;
}


unsigned int read_reliable(unsigned int fd, void *buf, size_t count)
{
    unsigned int ret;

AGAIN:
    ret = read(fd, buf, count);
    if (ret < 0 && EINTR == errno)
    {
        goto AGAIN;
    }
    return ret;
}


unsigned int read_certain_bytes(unsigned int fd, void *buf, size_t count)
{
    unsigned int ret;
    unsigned int left = count;
    unsigned int finished = 0;

    while (left > 0)
    {
        ret = read_reliable(fd, buf + finished, left);
        if (ret < 0)
            return ret;

        finished += ret;
        left -= ret;
    }

    return 0;
}

/*accept a connection,and create a manager data for client.
will make a new thread and session for this connection*/
void process_thread(int fd)
{
    unsigned int fd_accept = fd;

    TELNET_MGR *tel_mgr = NULL;
    tel_mgr = malloc(sizeof(TELNET_MGR));
    if(NULL == tel_mgr)
    {
        printf("malloc failed!\n");
        TEL_ERROR("process_thread malloc failed\n");
        assert(0);
    }
    memset(tel_mgr,0,sizeof(TELNET_MGR));
    tel_mgr->history_cmd = malloc(sizeof(CMD_QUEUE));
    if(NULL == tel_mgr->history_cmd)
    {
        printf("malloc failed!\n");
        TEL_ERROR("process_thread malloc failed\n");
        assert(0);
    }
    memset(tel_mgr->history_cmd,0,sizeof(CMD_QUEUE));

    tel_mgr->fd_conn = fd_accept;
    tel_mgr->pid = getpid();
    
    make_new_session(tel_mgr);
    
    
}

/*telnet negotiation,send IAC*/
void send_iac(unsigned int sockfd, char cmd, char opt)
{
    char data[3] = {IAC, cmd, opt};
    write_certain_bytes(sockfd, data, sizeof(data));
}

/*the socket option*/
void set_useful_sock_opt(unsigned int sockfd)
{
    unsigned int nrecv_buf = 256 * 1024;
    unsigned int nsend_buf = 256 * 1024;
    unsigned int reuse_addr = 1;
    socklen_t optlen = sizeof(unsigned int);
    setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,&nrecv_buf,optlen); 
    setsockopt(sockfd,SOL_SOCKET,SO_SNDBUF,&nsend_buf,optlen);
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&reuse_addr,optlen);
}

/*create a new session thread*/
unsigned int make_new_session(TELNET_MGR *tel_mgr)
{
    TEL_INFO("make_new_session\n");
    pthread_t the_shell_thread;
    set_useful_sock_opt(tel_mgr->fd_conn);
    send_iac(tel_mgr->fd_conn,DONT,TELOPT_ECHO);
    send_iac(tel_mgr->fd_conn,DO,TELOPT_NAWS);
    send_iac(tel_mgr->fd_conn,DO,TELOPT_LFLOW);
    send_iac(tel_mgr->fd_conn,WILL,TELOPT_ECHO);
    send_iac(tel_mgr->fd_conn,WILL,TELOPT_SGA);
    
    printf("create  shell thread! client fd: %d\n",tel_mgr->fd_conn);
    TEL_INFO("create  shell thread! client fd: %d\n",tel_mgr->fd_conn);
    pthread_create(&the_shell_thread, NULL, (void *)shell_thread_func, (void *)tel_mgr);
    return 0;
}

/*remove IAC charactor*/
unsigned int telnetd_remove_iac(char *pbuf, unsigned int uiLen)
{
    char *ptr0 = pbuf;
    char *ptr = ptr0;
    char *totty = ptr;
    char *end = ptr + uiLen;
    int num_totty;

    while (ptr < end) 
    {
        if (*ptr != IAC) 
        {
            char cc = *ptr;

            *totty++ = cc;
            ptr++;
            
            /* We map \r\n ==> \r for pragmatic reasons.
             * Many client implementations send \r\n when
             * the user hits the CarriageReturn key.
             */
            if (cc == '\r' && ptr < end && (*ptr == '\n' || *ptr == '\0'))
                ptr++;
            continue;
        }

        if ((ptr + 1) >= end)
            break;
        if (ptr[1] == NOP)    
        { 
        /* Ignore? (putty keepalive, etc.) */
            ptr += 2;
            continue;
        }
        if (ptr[1] == IAC) 
        {
        /* Literal IAC? (emacs M-DEL) */
            *totty++ = ptr[1];
            ptr += 2;
            continue;
        }

        /*
         * TELOPT_NAWS support!
         */
        if ((ptr + 2) >= end) 
        {
            /* only the beginning of the IAC is in the
            buffer we were asked to process, we can't
            process this char. */
            break;
        }
        
        /*
         * IAC -> SB -> TELOPT_NAWS -> 4-byte -> IAC -> SE
         */
        if (ptr[1] == SB && ptr[2] == TELOPT_NAWS) 
        {
            struct winsize ws;
            ws.ws_col = (ptr[3] << 8) | ptr[4];
            ws.ws_row = (ptr[5] << 8) | ptr[6];
            if ((ptr + 8) >= end)
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

/*the work thread,will recv the input from telnet client,
and keyboard,achieve the up,down,right,left,backspace and so on*/
void *shell_thread_func(void *arg)
{
    unsigned int ret = 0;
    TELNET_MGR *tel_mgr = (TELNET_MGR *)arg;
    char cmd_line[MAX_CMD_LEN];
    char *cmd_buf = NULL;
    char light[16] = {'\0'};
    char shell[32] = {'\0'};
    unsigned int ii = 0;
    unsigned int jj = 0;
    char *init_str = "\r\nWelcome to telnet debug terminal\r\n";
    int i = 0;
    cmd_buf = malloc(MAX_CMD_LEN);
    if(cmd_buf == NULL)
    {
        printf("malloc failed!\n");
        TEL_ERROR("shell_thread_func malloc failed!\n");
        return NULL;
    }
    write_certain_bytes(tel_mgr->fd_conn,init_str,strlen(init_str));
    sprintf(cmd_buf,"\r\n%s",DEBUG_SHELL);
    write_certain_bytes(tel_mgr->fd_conn,cmd_buf,strlen(cmd_buf));
    memset(cmd_buf,0,MAX_CMD_LEN);
    while(1)
    {
        memset(cmd_line,0,MAX_CMD_LEN);
        ret = read_reliable(tel_mgr->fd_conn,cmd_line,MAX_CMD_LEN);
        if(ret < 0)
        {
            usleep(1000);
            continue;
        
        }
        telnetd_remove_iac(cmd_line,strlen(cmd_line));
        
        /*clear IAC*/
        if(cmd_line[0] == -1 || cmd_line[0] == -3)
        {
            continue;
        }

        
        if(strcmp(cmd_line,CTRL_C) == 0)
        {
            write_certain_bytes(tel_mgr->fd_conn,"^C",strlen("^C"));
            memset(cmd_buf,0,MAX_CMD_LEN);
            sprintf(cmd_buf,"\r\n%s",DEBUG_SHELL);
            write_certain_bytes(tel_mgr->fd_conn,cmd_buf,strlen(cmd_buf));
            memset(cmd_buf,0,MAX_CMD_LEN);
            continue;
        }

		if(cmd_line[0] == CTRL_L)
		{
			write_certain_bytes(tel_mgr->fd_conn,CLEAR_TERM,strlen(CLEAR_TERM));
			write_certain_bytes(tel_mgr->fd_conn,"\033[0;0H",strlen("\033[0;0H"));
			write_certain_bytes(tel_mgr->fd_conn,"\033[2K",strlen("\033[2K"));
			memset(cmd_buf,0,MAX_CMD_LEN);
            sprintf(cmd_buf,"\r\n%s",DEBUG_SHELL);
			write_certain_bytes(tel_mgr->fd_conn,cmd_buf,strlen(cmd_buf));
            memset(cmd_buf,0,MAX_CMD_LEN);
			continue;
		}
        
        if(cmd_line[0] == 0x1b && cmd_line[1] == 0x5b && cmd_line[2] == 0x41)
        {
            keyboard_move_up(tel_mgr);
            memset(cmd_buf,0,MAX_CMD_LEN);
            strcpy(cmd_buf,tel_mgr->g_cmd);
            continue;
        }
        if(cmd_line[0] == 0x1b && cmd_line[1] == 0x5b && cmd_line[2] == 0x42)
        {
            keyboard_move_down(tel_mgr);
            memset(cmd_buf,0,MAX_CMD_LEN);
            strcpy(cmd_buf,tel_mgr->g_cmd);
            continue;
        }
        
        if(cmd_line[0] == 0x1b && cmd_line[1] == 0x5b && cmd_line[2] == 0x43)
        {
            keyboard_move_right(tel_mgr);
            continue;
        }
        
        if(cmd_line[0] == 0x1b && cmd_line[1] == 0x5b && cmd_line[2] == 0x44)
        {
            keyboard_move_left(tel_mgr,strlen(cmd_buf));
            continue;
        }
        

        
        if(cmd_line[0] == 0x08 || cmd_line[0] == 0x7f)
        {
            if(strlen(cmd_buf) == tel_mgr->move_left_num)
            {
            	continue;
            }
			write_certain_bytes(tel_mgr->fd_conn,"\010 \010",strlen("\010 \010"));
            keyboard_backsapce(tel_mgr,cmd_buf,strlen(cmd_buf));

			if(tel_mgr->move_left_num)
			{
				for(i = 0; i < tel_mgr->move_left_num;i++)
				{
					write_certain_bytes(tel_mgr->fd_conn,MOVE_LEFT,strlen(MOVE_LEFT));
				}
			}
			else
			{
            	write_certain_bytes(tel_mgr->fd_conn,MOVE_LEFT,strlen(MOVE_LEFT));
			}
            continue;
        }

		if(cmd_line[0] == '?')
		{
			telnet_terminal_help(tel_mgr->fd_conn);
			write_certain_bytes(tel_mgr->fd_conn,CLEAR_RIGHT_CHAR,strlen(CLEAR_RIGHT_CHAR));
			write_certain_bytes(tel_mgr->fd_conn,DEBUG_SHELL,strlen(DEBUG_SHELL));
			continue;
		}

		if(cmd_line[0] == '\t')
		{
			write_certain_bytes(tel_mgr->fd_conn,"help\r\n",strlen("help\r\n"));
			write_certain_bytes(tel_mgr->fd_conn,CLEAR_RIGHT_CHAR,strlen(CLEAR_RIGHT_CHAR));
			write_certain_bytes(tel_mgr->fd_conn,DEBUG_SHELL,strlen(DEBUG_SHELL));
			continue;
		}

		
        
        if((tel_mgr->move_left_num > 0) && (strlen(cmd_buf) > 0) && (32 <= cmd_line[0]) && (cmd_line[0] <= 127))
        {
            insert_char(tel_mgr,cmd_buf,strlen(cmd_buf),cmd_line[0]);
            continue;
        }
        
        if(cmd_line[0] == '\r')
        {   
            tel_mgr->move_left_num = 0;    
            if(strlen(cmd_buf) > 0)
            {
                memset(tel_mgr->g_cmd,0,MAX_CMD_LEN);
                sprintf(tel_mgr->g_cmd,"%s",cmd_buf);
                TEL_INFO("server %s\r\n",tel_mgr->g_cmd);
                ret = sprintf(shell,"\r\n");
                write_certain_bytes(tel_mgr->fd_conn,shell,ret);
                goto CMD_OVER;
            }

            /*judge the input len*/
            if(strlen(cmd_buf) == 0)
            {
                ret = sprintf(shell,"\r\n%s",DEBUG_SHELL);
                write_certain_bytes(tel_mgr->fd_conn,shell,ret);
                continue;
            }

CMD_OVER:
            
            if(cmd_buf[0] != '\r')
            {
                update_history_cmd(tel_mgr,cmd_buf);
                
                if(strcmp("quit",cmd_buf) == 0 || strcmp("exit",cmd_buf) == 0)
                {
                    close(tel_mgr->fd_conn);
					free(cmd_buf);
					free(tel_mgr);
                    return NULL;
                }
                
                save_ori_io();
                redirect_io(tel_mgr);
                proccess_cmd(cmd_buf);
                restore_ori_io();
                memset(cmd_buf,0,MAX_CMD_LEN);
                sprintf(cmd_buf,"\r\n%s",DEBUG_SHELL);
                write_certain_bytes(tel_mgr->fd_conn,cmd_buf,strlen(cmd_buf));
                memset(cmd_buf,0,MAX_CMD_LEN);
            
            }
        }
        else
        {
            if(tel_mgr->move_left_num != 0 && strlen(cmd_buf) > 0)
            {
                memset(light,0,16);
                insert_char(tel_mgr,cmd_buf,strlen(cmd_buf),cmd_line[0]);
                ret = sprintf(light,"\033[%dD",tel_mgr->move_left_num);
                write_certain_bytes(tel_mgr->fd_conn,light,ret);
                continue;
            }
            write_certain_bytes(tel_mgr->fd_conn,cmd_line,strlen(cmd_line));
            for(ii = 0;ii < strlen(cmd_line);ii++)
            {
                jj = strlen(cmd_buf);
                cmd_buf[jj] = cmd_line[ii];
                cmd_buf[jj + 1] = '\0';
            }
        }
    }
    return NULL;
}

/*update the history cmd*/
unsigned int update_history_cmd(TELNET_MGR *tel_mgr,char *cmd)
{
    unsigned int ret = 0;
   
    if(tel_mgr->has_input_cmd_num >= MAX_CMD_HISTORY_NUM)
        return 0;
    ret = sprintf(tel_mgr->history_cmd->queue_cmd[tel_mgr->has_input_cmd_num].cmd,"%s",cmd);
    tel_mgr->history_cmd->queue_cmd[tel_mgr->has_input_cmd_num].size = ret;
    tel_mgr->has_input_cmd_num++;
    if(tel_mgr->has_input_cmd_num > MAX_CMD_HISTORY_NUM)
    {
        tel_mgr->has_input_cmd_num = MAX_CMD_HISTORY_NUM;
    }
    tel_mgr->history_cmd->all_cmd_num = tel_mgr->has_input_cmd_num;
    tel_mgr->cmd_index = tel_mgr->has_input_cmd_num;
    return 0;
}


void keyboard_move_up(TELNET_MGR *tel_mgr)
{
    tel_mgr->move_left_num  = 0;
    if(tel_mgr->cmd_index == 0)
    {
        return;
    }
    memset(tel_mgr->g_cmd,0,MAX_CMD_LEN);
    char buf[MAX_CMD_LEN] = {'\0'};
    unsigned int ret = 0;
    write_certain_bytes(tel_mgr->fd_conn,CLEAR_LEFT_CHAR,strlen(CLEAR_LEFT_CHAR));
    
 
    ret = sprintf(buf,"\r%s%s",DEBUG_SHELL,tel_mgr->history_cmd->queue_cmd[tel_mgr->cmd_index - 1].cmd);
 
    write_certain_bytes(tel_mgr->fd_conn,buf,strlen(buf));
    tel_mgr->cmd_index--;
    sprintf(tel_mgr->g_cmd,"%s",tel_mgr->history_cmd->queue_cmd[tel_mgr->cmd_index].cmd);
    
}


void keyboard_move_down(TELNET_MGR *tel_mgr)
{
    tel_mgr->move_left_num  = 0;
    if(tel_mgr->cmd_index == tel_mgr->has_input_cmd_num)
    {
        return;
    }
    memset(tel_mgr->g_cmd,0,MAX_CMD_LEN);
    char buf[MAX_CMD_LEN] = {'\0'};
    unsigned int ret = 0;
    write_certain_bytes(tel_mgr->fd_conn,CLEAR_LEFT_CHAR,strlen(CLEAR_LEFT_CHAR));
 
    ret = sprintf(buf ,"\r%s%s",DEBUG_SHELL,tel_mgr->history_cmd->queue_cmd[tel_mgr->cmd_index + 1].cmd);

    write_certain_bytes(tel_mgr->fd_conn,buf,strlen(buf));
    tel_mgr->cmd_index++;
    sprintf(tel_mgr->g_cmd,"%s",tel_mgr->history_cmd->queue_cmd[tel_mgr->cmd_index].cmd);
}


void keyboard_backsapce(TELNET_MGR *tel_mgr,char *cmd,unsigned int len)
{
    if(len <= 0 || tel_mgr->move_left_num == len)
    {
        return;
    }
	int i = 0;
    char buf[MAX_CMD_LEN] = {'\0'};

    if(tel_mgr->move_left_num > 0)
    {
        write_certain_bytes(tel_mgr->fd_conn,CLEAR_RIGHT_CHAR,strlen(CLEAR_RIGHT_CHAR));
        memmove(cmd + (len - 1 - tel_mgr->move_left_num),cmd + (len - tel_mgr->move_left_num),tel_mgr->move_left_num);
        cmd[len - 1] = '\0';
		write_certain_bytes(tel_mgr->fd_conn,cmd + (len - 1 - tel_mgr->move_left_num),tel_mgr->move_left_num);

    }
    
    else
    {
        cmd[len - 1] = '\0';
		write_certain_bytes(tel_mgr->fd_conn," ",1);
    }
	
}


void insert_char(TELNET_MGR *tel_mgr,char *cmd,unsigned int len,char c)
{
	int i = 0;
    if(len <= 0 || len > MAX_CMD_LEN - 1)
    {
        return;
    }
    char buf[MAX_CMD_LEN] = {'\0'};
    char light[16] = {0};
   
    unsigned int ret = 0;
    if(tel_mgr->move_left_num > 0 && tel_mgr->move_left_num <= len)
    {
    	//write_certain_bytes(tel_mgr->fd_conn," ",1);
        memmove(cmd + (len - tel_mgr->move_left_num + 1),cmd + (len -  tel_mgr->move_left_num),tel_mgr->move_left_num);
        cmd[len -  tel_mgr->move_left_num] = c;
        //cmd[len + 1] = '\0';
        write_certain_bytes(tel_mgr->fd_conn,"\033[1@",strlen("\033[1@"));
		write_certain_bytes(tel_mgr->fd_conn,cmd + len -  tel_mgr->move_left_num,1);
 
        return;
    }
}


void keyboard_move_left(TELNET_MGR *tel_mgr,unsigned int len)
{
    
    if(tel_mgr->move_left_num >= len)
    {
        return;
    }
    write_certain_bytes(tel_mgr->fd_conn,MOVE_LEFT,strlen(MOVE_LEFT));
    tel_mgr->move_left_num++;
}


void keyboard_move_right(TELNET_MGR *tel_mgr)
{
    if(tel_mgr->move_left_num > 0)
    {
        write_certain_bytes(tel_mgr->fd_conn,MOVE_RIGHT,strlen(MOVE_RIGHT));
        tel_mgr->move_left_num--;
    }
    else
    {
        return;
    }
}

/*the log reg*/
unsigned int telnet_reg_log()
{
    TEL_LOG_REG_INFO reg_info;
    memset(&reg_info,0,sizeof(TEL_LOG_REG_INFO));
    reg_info.module_id = TEL_LOG_MODULE_ID_TELNET;
    reg_info.level = TEL_LOG_SECURITY_DEBUG;
    reg_info.mulitifile = FALSE;
    reg_info.max_log_size = 1 * 1024 *1024;
    strcpy(reg_info.file_name,"telnet_log");

    return tel_log_reg(&reg_info,sizeof(reg_info) / sizeof(TEL_LOG_REG_INFO));         
}


int write_str(int fd,char *str)
{
	return write_certain_bytes(fd,str,strlen(str));
}

void telnet_terminal_help(int fd)
{
	write_str(fd,"\r\nshow		--show command");
	write_str(fd,"\r\nnode		--enter node mode");
	write_str(fd,"\r\ncomp		--enter comp mode");
	write_str(fd,"\r\nproc		--enter proc mode");
	write_str(fd,"\r\nCtrl + L	--clear the terminal");
	write_str(fd,"\r\ndebug		--enter debug mode");
	write_str(fd,"\r\n");
	
}





