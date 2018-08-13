# telnet_debug
telnet_debug
编译进入build目录，执行cmake ../ && make
会生成libtelnet_dbg.so和telent_test
默认端口19000

可以有如此功能：
[root@zhongfujin build]# telnet 127.1 19000
Trying 127.0.0.1...
Connected to 127.1.
Escape character is '^]'.

Welcome to telnet debug terminal

[debug_shell]#empty_test()
empty_test
this is no param test.
value:0x0                (0)

[debug_shell]#int_test(26,33)
int_test
this is test int int: param1:26,param2:33.
value:0x0                (0)

[debug_shell]#
[debug_shell]#g_so_name_list
address:0xbe1f58 value:0xbe1f58           (12459864)

[debug_shell]#
输入已和函数名和一个参数，便可以执行该函数，并输出其中的打印以及函数的返回值。
输入一个全局变量名，可以打印出其地址。若该变量为整型或者浮点型，可打印出其值。

上述功能结合gdb可以触发函数，并进行调试。

上面的shell可以支持翻历史命令，退格，插入，等常用操作

其中的链表尾一个通用单向链表。
log为一个可注册的，多模块使用的，限制文件大小，可设置输出目录的，可设置多个级别以及单文件或者多文件输出的通用日志模块
