#include <stdint.h>
#include <getch.h>
#include "tools.h"

// 清理输入缓冲区
static void clear_stdin(void)
{
    stdin->_IO_read_ptr = stdin->_IO_read_end;
}

// 从键盘获取字符串
char* get_str(char* str,size_t hope_len)
{
    if(NULL == str || 0 == hope_len) return NULL;

    size_t index = 0;
    while(index < hope_len-1)
    {
        int8_t key_val = getch();
        if(10 == key_val) break;
        if(127 == key_val)
        {
            if(index > 0)
            {
                printf("\b \b");
                index--;
            }
            continue;
        }
        printf("%c",key_val);
        str[index++] = key_val;
    }
    
    str[index] = '\0';
    
    printf("\n");

    clear_stdin();

    return str;
}

// 从键盘获取密码
char* get_passwd(char* pd,size_t hope_len,bool is_show)
{
    if(NULL == pd) return NULL;

    size_t index = 0;
    while(index < hope_len-1)
    {
        int32_t key_val = getch();
        if(127 == key_val)
        {
            if(index > 0)
            {
                index--;
                if(is_show) printf("\b \b");
            }
        }
        else if(10 == key_val)
        {
            break;
        }
        else
        {
            pd[index++] = key_val;
            if(is_show) printf("*");
        }
    }

    pd[index] = '\0';

    printf("\n");

    clear_stdin();

    return pd;
}

