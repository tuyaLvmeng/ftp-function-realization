#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <getch.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ftp_client.h"

typedef struct sockaddr* SP;

static void check_status(FTPClient* ftp,int status,bool die)
{
	size_t ret_size = recv(ftp->cli_sock,ftp->buf,BUF_SIZE,0);
	if(0 == ret_size)
	{
		perror("recv");
		exit(EXIT_FAILURE);
	}

	ftp->buf[ret_size] = '\0';
	printf("%s",ftp->buf);	
	
	sscanf(ftp->buf,"%d",&ftp->status);
	if(ftp->status != status && die)
	{
		exit(EXIT_FAILURE);
	}
}

void send_cmd(FTPClient* ftp)
{
	int ret_size = send(ftp->cli_sock,ftp->buf,strlen(ftp->buf),0);
	if(0>= ret_size)
	{
		perror("send");
		exit(EXIT_FAILURE);
	}
}

// 创建FTP客户端对象
FTPClient* create_FTPClient(const char* ip,short port)
{
	FTPClient* ftp = malloc(sizeof(FTPClient));
	ftp->cli_sock = socket(AF_INET,SOCK_STREAM,0);
	if(0 > ftp->cli_sock)
	{
		free(ftp);
		perror("socket");
		return NULL;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);

	if(connect(ftp->cli_sock,(SP)&addr,sizeof(addr)))
	{
		free(ftp);
		perror("connect");
		return NULL;
	}

	ftp->buf = malloc(BUF_SIZE);
	check_status(ftp,220,true);
	return ftp;
}

// 销毁FTP客户端对象
void destory_FTPClient(FTPClient* ftp)
{
	printf("%s %d %s\n",__FILE__,__LINE__,__func__);
	close(ftp->cli_sock);
	free(ftp->buf);
	free(ftp);
}

// 向服务器发送用户名
void user_FTPClient(FTPClient* ftp,const char* user)
{
	printf("%s %d %s\n",__FILE__,__LINE__,__func__);
	sprintf(ftp->buf,"USER %s\n",user);
	send_cmd(ftp);
	check_status(ftp,331,true);
}

// 向服务器发送密码
void pass_FTPClient(FTPClient* ftp,const char* pass)
{
	printf("%s %d %s\n",__FILE__,__LINE__,__func__);
	sprintf(ftp->buf,"PASS %s\n",pass);
	send_cmd(ftp);
	check_status(ftp,230,true);
	
	sprintf(ftp->buf,"SYST\n");
	send_cmd(ftp);
	check_status(ftp,215,false);

	sprintf(ftp->buf,"OPTS UTF8 ON\n");
	send_cmd(ftp);
	check_status(ftp,200,false);

}

// pwd命令
void pwd_FTPClient(FTPClient* ftp)
{
	printf("%s %d %s\n",__FILE__,__LINE__,__func__);
	sprintf(ftp->buf,"PWD\n");
	send_cmd(ftp);
	check_status(ftp,257,true);

	sscanf(ftp->buf,"%*d \"%s",ftp->path);
	ftp->path[strlen(ftp->path)-1] = '\0';
}

// cd命令
void cd_FTPClient(FTPClient* ftp,const char* path)
{
	printf("%s %d %s\n",__FILE__,__LINE__,__func__);
	sprintf(ftp->buf,"CWD %s\n",path);
	printf("cd:%s-------\n",ftp->buf);
	send_cmd(ftp);
	check_status(ftp,250,false);
}

// mkdir命令
void mkdir_FTPClient(FTPClient* ftp,const char* dir)
{
	printf("%s %d %s\n",__FILE__,__LINE__,__func__);
	sprintf(ftp->buf,"MKD %s\n",dir);
	send_cmd(ftp);
	check_status(ftp,257,false);
}

void pasv_FTPClient(FTPClient* ftp)
{
	printf("%s %d %s\n",__FILE__,__LINE__,__func__);
	sprintf(ftp->buf,"PASV\n");
	send_cmd(ftp);
	check_status(ftp,227,true);

	unsigned char ip1,ip2,ip3,ip4,port1,port2;
	sscanf(ftp->buf,"227 Entering Passive Mode (%hhu,%hhu,%hhu,%hhu,%hhu,%hhu",&ip1,&ip2,&ip3,&ip4,&port1,&port2);

	ftp->cli_pasv = socket(AF_INET,SOCK_STREAM,0);
	if(0 > ftp->cli_pasv)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons((port1<<8)+port2);
	addr.sin_addr.s_addr = (ip4<<24)+(ip3<<16)+(ip2<<8)+ip1;
	printf("ip=%s\n",inet_ntoa(addr.sin_addr));

	if(connect(ftp->cli_pasv,(SP)&addr,sizeof(addr)))
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}
}

// ls命令
void ls_FTPClient(FTPClient* ftp)
{
	printf("%s %d %s\n",__FILE__,__LINE__,__func__);
	pasv_FTPClient(ftp);
	sprintf(ftp->buf,"LIST -al\n");
	send_cmd(ftp);
	
	check_status(ftp,150,true);

	size_t ret_size = 0;
	while(ret_size = recv(ftp->cli_pasv,ftp->buf,BUF_SIZE-1,0))
	{
		ftp->buf[ret_size] = '\0';
		printf("%s",ftp->buf);
	}
	close(ftp->cli_pasv);

	check_status(ftp,226,true);
}

// put命令
void put_FTPClient(FTPClient* ftp,const char* file)
{
	printf("%s %d %s\n",__FILE__,__LINE__,__func__);
	int fd = open(file,O_RDONLY);
	if(0 > fd)
	{
		printf("打开 %s 文件失败，请检查!\n",file);
		return;
	}
	
	pasv_FTPClient(ftp);
	sprintf(ftp->buf,"STOR %s\n",file);
	send_cmd(ftp);
	check_status(ftp,150,false);

	size_t ret_size = 0;
	while(ret_size = read(fd,ftp->buf,BUF_SIZE))
	{
		send(ftp->cli_pasv,ftp->buf,ret_size,0);
	}
	close(fd);
	close(ftp->cli_pasv);

	check_status(ftp,226,false);
}

// get命令
void get_FTPClient(FTPClient* ftp,const char* file)
{
	printf("%s %d %s\n",__FILE__,__LINE__,__func__);
	sprintf(ftp->buf,"SIZE %s\n",file);
	send_cmd(ftp);
	check_status(ftp,213,false);
	if(213 != ftp->status)
	{
		printf("文件 %s 不存在，请检查!\n",file);
		return;
	}

	pasv_FTPClient(ftp);
	sprintf(ftp->buf,"RETR %s\n",file);
	send_cmd(ftp);

	int fd = open(file,O_WRONLY|O_CREAT|O_EXCL,0644);
	if(0 > fd)
	{
		printf("本地已经存在%s文件，是否覆盖(y/n)？",file);
		char cmd = getch();
		printf("%c\n",cmd);
		if('y' != cmd && 'Y' != cmd)
		{
			printf("放弃下载!\n");
			return;
		}
	}

	check_status(ftp,150,false);

	size_t ret_size = 0;
	while(ret_size = recv(ftp->cli_pasv,ftp->buf,BUF_SIZE,0))
	{
		write(fd,ftp->buf,ret_size);
	}
	close(ftp->cli_pasv);
	close(fd);

	check_status(ftp,226,false);
}

// bye命令
void bye_FTPClient(FTPClient* ftp)
{
	printf("%s %d %s\n",__FILE__,__LINE__,__func__);
	sprintf(ftp->buf,"QUIT\n");
	send_cmd(ftp);
	check_status(ftp,221,true);
}

