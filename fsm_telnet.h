#ifndef __FSM_TELNET__H
#define __FSM_TELNET__H



#define BUFSIZE 1024
#define PORT  12000
#define MAX_CMD_HISTORY_NUM 500

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


typedef struct tag_telnet_info
{
    CMD_QUEUE *history_cmd;
    int has_input_cmd_num;
    int cmd_index;

    int fd_server;
    int fd_conn;

    int fd_accect;

    int move_left_num;
    int move_right_num;

    char g_cmd[512];
}TELNET_MGR;

void process_thread(void *arg);
int telnet_ser();

void proccess_cmd(char *cmd_line);
void *shell_thread_func(void *arg);
void keyboard_move_up(TELNET_MGR *tel_mgr);
void keyboard_move_down(TELNET_MGR *tel_mgr);
void keyboard_backsapce(TELNET_MGR *tel_mgr,char *cmd,int len);
void keyboard_move_left(TELNET_MGR *tel_mgr,int len);
void keyboard_move_right(TELNET_MGR *tel_mgr);
void insert_char(TELNET_MGR *tel_mgr,char *cmd,int len,char c);
int make_new_session(TELNET_MGR *tel_mgr);
void send_iac(int sockfd, char cmd, char opt);
void set_useful_sock_opt(int sockfd);
int telnetd_remove_iac(char *pBuf, int uiLen);
int write_reliable(int fd, const void *buf, size_t count);
int write_certain_bytes(int fd, const void *buf, size_t count);
int read_reliable(int fd, void *buf, size_t count);
int read_certain_bytes(int fd, void *buf, size_t count);
int update_history_cmd(TELNET_MGR *tel_mgr,char *cmd);









#define MAX_CMD_HISTORY_NUM  500


#endif

