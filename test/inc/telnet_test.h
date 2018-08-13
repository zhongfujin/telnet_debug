#ifndef __TELNET_TEST__H
#define __TELNET_TEST__H

#include "telnet_com.h"
#include "telnet_getsymbol_addr.h"
#include "log.h"
#include "telnet_redirect_io.h"

unsigned int empty_test();
unsigned int int_test(unsigned int a,unsigned int b);
unsigned int char_test(int a,char b);
unsigned int string_test(int a,char *str);


#endif
