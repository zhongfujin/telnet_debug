#include "telnet_test.h"


unsigned int empty_test()
{
    TELD_PRINTF("empty_test\n");
    TELD_PRINTF("this is no param test.\n");
    return 0;
}


unsigned int int_test(unsigned int aa,unsigned int bb)
{
    TELD_PRINTF("int_test\n");
    TELD_PRINTF("this is test int int: param1:%d,param2:%d.\n",aa,bb);
    return 0;
}


unsigned int char_test(int aa,char bb)
{
    TELD_PRINTF("char_test\n");
    TELD_PRINTF("this is test int char: param1:%d,param2:%c.\n",aa,bb);
    return 0;
}


unsigned int string_test(int aa,char *str)
{
    TELD_PRINTF("string_test\n");
    TELD_PRINTF("this is test int char*:%d,%s\n",aa,str);
    return 0;
}

