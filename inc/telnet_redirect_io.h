#ifndef __TELNET_REDIRECT_IO__H
#define __TELNET_REDIRECT_IO__H
#include "telnet_com.h"



void redirect_io(TELNET_MGR *tel_mgr);
void restore_ori_io();
void save_ori_io();
unsigned int out_n_to_rn(char *src,char *dest);


#endif

