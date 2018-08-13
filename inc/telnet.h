#ifndef __TELNET__H
#define __TELNET__H

#include "telnet_com.h"
#include "log.h"
void process_thread(int fd);
unsigned int telnet_ser();

void proccess_cmd(char *cmd_line);
void *shell_thread_func(void *arg);
void keyboard_move_up(TELNET_MGR *tel_mgr);
void keyboard_move_down(TELNET_MGR *tel_mgr);
void keyboard_backsapce(TELNET_MGR *tel_mgr,char *cmd,unsigned int len);
void keyboard_move_left(TELNET_MGR *tel_mgr,unsigned int len);
void keyboard_move_right(TELNET_MGR *tel_mgr);
void insert_char(TELNET_MGR *tel_mgr,char *cmd,unsigned int len,char c);
unsigned int make_new_session(TELNET_MGR *tel_mgr);
void send_iac(unsigned int sockfd, char cmd, char opt);
void set_useful_sock_opt(unsigned int sockfd);
unsigned int telnetd_remove_iac(char *pBuf, unsigned int uiLen);
unsigned int write_reliable(unsigned int fd, const void *buf, size_t count);
unsigned int write_certain_bytes(unsigned int fd, const void *buf, size_t count);
unsigned int read_reliable(unsigned int fd, void *buf, size_t count);
unsigned int read_certain_bytes(unsigned int fd, void *buf, size_t count);
unsigned int update_history_cmd(TELNET_MGR *tel_mgr,char *cmd);
void *accept_thread_create(void *arg);



#define MAX_CMD_HISTORY_NUM  500


#endif

