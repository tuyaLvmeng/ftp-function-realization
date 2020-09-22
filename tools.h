#ifndef TOOLS_H
#define TOOLS_H
#include <stdio.h>
#include <stdbool.h>

// 从键盘获取字符串
char* get_str(char* str,size_t hope_len);

// 从键盘获取密码
char* get_passwd(char* pd,size_t hope_len,bool is_show);

#endif//TOOLS_H



