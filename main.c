#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "ftp_client.h"

// 创建FTP客户端对象
FTPClient* ftp = NULL;

void sigint(int num)
{
	if(ftp->is_get)
	{
		printf("------------------------\n");
		close(ftp->cli_pasv);
		close(ftp->cli_sock);
		close(ftp->fd);
		set_mtime(ftp->file,ftp->mtime);
		printf("------------------------\n");
	}
	if(ftp->is_put)
	{
		close(ftp->cli_pasv);
		close(ftp->fd);
		mdtm_FTPClient(ftp);
		close(ftp->cli_sock);
	}
	exit(EXIT_SUCCESS);
}

int main(int argc,const char* argv[])
{
	signal(SIGINT,sigint);
	if(2 == argc)
	{
		ftp = create_FTPClient(argv[1],21);
	}
	else if(3 == argc)
	{
		short port = atoi(argv[2]);
		ftp = create_FTPClient(argv[1],port);
	}
	else
	{
		printf("use：ftp xxx.xxx.xxx.xxx [port]\n");
		return EXIT_SUCCESS;
	}

	// 输入用户名
	char user[20] = {};
	printf("Name:");
	get_str(user,sizeof(user));
	user_FTPClient(ftp,user);
	
	// 密码并登录
	char pass[20] = {};
	printf("Password:");
	get_passwd(pass,sizeof(pass),0);
	pass_FTPClient(ftp,pass);

	// 循环的输入命令、执行命令
	char cmd[256] = {};
	for(;;)
	{
		printf("ftp> ");
		get_str(cmd,sizeof(cmd));

		if(0 == strncmp("pwd",cmd,3))
		{
			pwd_FTPClient(ftp);
		}
		else if(0 == strncmp("cd ",cmd,3))
		{
			cd_FTPClient(ftp,cmd+3);
		}
		else if(0 == strncmp("mkdir ",cmd,6))
		{
			mkdir_FTPClient(ftp,cmd+6);
		}
		else if(0 == strncmp("ls",cmd,2))
		{
			ls_FTPClient(ftp);
		}
		else if(0 == strncmp("put ",cmd,4))
		{
			put_FTPClient(ftp,cmd+4);
		}
		else if(0 == strncmp("get ",cmd,4))
		{
			get_FTPClient(ftp,cmd+4);
		}
		else if(0 == strncmp("bye",cmd,3))
		{
			bye_FTPClient(ftp);
			destory_FTPClient(ftp);
			return EXIT_SUCCESS;
		}
		else if('!' == cmd[0])
		{
			system(cmd+1);
		}
		else
		{
			printf("指针未定义!!!\n");
		}
	}
}
