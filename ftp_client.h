#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H
#include <stdbool.h>
#define BUF_SIZE 4096

typedef struct FTPClient
{
	int cli_sock;	// 命令通道socket描述符
	int cli_pasv;	// 数据通道socket描述符
	int fd;			// 正在操作的文件描述符
	short port;		// 端口号
	char ip[16];	// 服务器ip
	char path[256];	// 服务器当前路径
	char file[256]; // 当前正在操作的文件
	char mtime[15]; // 文件的最后修改时间
	int  status;	// 返回码
	char* buf;		// 执行结果
	bool is_get;	// 是否正在下载
	bool is_put;	// 是否正在上传
}FTPClient;

// 创建FTP客户端对象
FTPClient* create_FTPClient(const char* ip,short port);

// 销毁FTP客户端对象
void destory_FTPClient(FTPClient* ftp);

// 向服务器发送用户名
void user_FTPClient(FTPClient* ftp,const char* user);

// 向服务器发送密码
void pass_FTPClient(FTPClient* ftp,const char* pass);

// pwd命令
void pwd_FTPClient(FTPClient* ftp);

// cd命令
void cd_FTPClient(FTPClient* ftp,const char* path);

// mkdir命令
void mkdir_FTPClient(FTPClient* ftp,const char* dir);

// ls命令
void ls_FTPClient(FTPClient* ftp);

// put命令
void put_FTPClient(FTPClient* ftp,const char* file);

// get命令
void get_FTPClient(FTPClient* ftp,const char* file);

// bye命令
void bye_FTPClient(FTPClient* ftp);

// 设置文件的最后修改时间
int set_mtime(const char* path,const char* str);

// 获取文件的最后修改时间
char* get_mtime(const char* path,char* str);

// mdtm命令
void mdtm_FTPClient(FTPClient* ftp);

#endif//FTP_CLIENT_H



