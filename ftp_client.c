#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <utime.h>
#include <getch.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ftp_client.h"

typedef struct sockaddr* SP;
int set_mtime(const char* path,const char* str)
{
	printf("mtime=%s\n",str);
	struct tm t = {};
	sscanf(str,"%4d%2d%2d%2d%2d%2d",
		&t.tm_year,&t.tm_mon,&t.tm_mday,&t.tm_hour,&t.tm_min,&t.tm_sec);
	t.tm_year -= 1900;
	t.tm_mon -= 1;
	time_t sec = mktime(&t);
	struct utimbuf ut = {};
	ut.modtime = sec;
	int ret = utime(path,&ut);
	printf("---%s--- utime ret=%d\n",path,ret);
	return ret;
}

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
	pwd_FTPClient(ftp);
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
	pwd_FTPClient(ftp);
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

void mdtm_FTPClient(FTPClient* ftp)
{
	printf("----------------------------\n");
	sprintf(ftp->buf,"MDTM %s %s/%s\n",ftp->mtime,ftp->path,ftp->file);
	printf("-------------%s---------------\n",ftp->buf);
	send_cmd(ftp);
	check_status(ftp,213,false);
	printf("----------------------------\n");
}

// put命令
void put_FTPClient(FTPClient* ftp,const char* file)
{
	strcpy(ftp->file,file);
	ftp->fd = open(file,O_RDONLY);
	if(0 > ftp->fd)
	{
		printf("文件 %s 不存在请检查!\n",file);
		return;
	}

	// 设置传输模式为二进制
	sprintf(ftp->buf,"TYPE I\n");
	send_cmd(ftp);
	check_status(ftp,200,false);

	sprintf(ftp->buf,"SIZE %s\n",file);
	send_cmd(ftp);
	check_status(ftp,550,false);
		
	get_mtime(file,ftp->mtime);

	if(213 == ftp->status)
	{
		size_t size = 0;
		sscanf(ftp->buf,"%*d %u",&size);
		sprintf(ftp->buf,"MDTM %s\n",file);
		send_cmd(ftp);
		check_status(ftp,213,false);
		if(213 == ftp->status)
		{
			char svr_mtime[15] = {};
			sscanf(ftp->buf,"%*d %s",svr_mtime);
			if(!strcmp(svr_mtime,ftp->mtime))
			{
				sprintf(ftp->buf,"REST %u\n",size);
				send_cmd(ftp);
				check_status(ftp,350,false);
				lseek(ftp->fd,size,SEEK_SET);
			}
		}
	}
	
	ftp->is_put = true;
	pasv_FTPClient(ftp);
	sprintf(ftp->buf,"STOR %s\n",file);
	send_cmd(ftp);
	check_status(ftp,150,false);
	
	size_t ret_size = 0;
	while(ret_size = read(ftp->fd,ftp->buf,BUF_SIZE))
	{
		send(ftp->cli_pasv,ftp->buf,ret_size,0);
	}
	close(ftp->fd);
	close(ftp->cli_pasv);

	check_status(ftp,226,false);

	mdtm_FTPClient(ftp);
	ftp->is_put = false;
}

// 计算文件的字节数
size_t file_size(const char* path)
{
	int fd = open(path,O_RDONLY);
	if(0 > fd) return 0;
	size_t size = lseek(fd,0,SEEK_END);
	close(fd);
	return size;
}

char* get_mtime(const char* path,char* str)
{
	struct stat buf = {};
	stat(path,&buf);
	struct tm* t = localtime(&buf.st_mtime);
	sprintf(str,"%04d%02d%02d%02d%02d%02d",
		t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
	return str;
}


// get命令
void get_FTPClient(FTPClient* ftp,const char* file)
{
	strcpy(ftp->file,file);

	sprintf(ftp->buf,"TYPE I\n");
	send_cmd(ftp);
	check_status(ftp,200,false);

	//1、获取文件的大小
	sprintf(ftp->buf,"SIZE %s\n",file);
	send_cmd(ftp);
	check_status(ftp,213,false);
	if(ftp->status != 213)
	{
		printf("文件 %s 不存在请检查!\n",file);
		return;
	}
	size_t size = 0;
	sscanf(ftp->buf,"%*d %u",&size);

	//2、获取文件的最后修改时间
	sprintf(ftp->buf,"MDTM %s\n",file);
	send_cmd(ftp);
	check_status(ftp,213,false);
	sscanf(ftp->buf,"%*d %s",ftp->mtime);

	pasv_FTPClient(ftp);
	ftp->is_get = true;
	
	//3、判断本地有同名文件，与服务器上的文件大小不同，时间戳相同
	ftp->fd = open(file,O_WRONLY|O_CREAT|O_EXCL,0644);
	if(0 > ftp->fd)
	{
		char cli_mtime[15] = {};
		size_t cli_size = file_size(file);
		if(size > cli_size && !strcmp(get_mtime(file,cli_mtime),ftp->mtime))
		{
			ftp->fd = open(file,O_WRONLY|O_CREAT|O_APPEND);
			sprintf(ftp->buf,"REST %u\n",cli_size);
			send_cmd(ftp);
			check_status(ftp,350,false);
			if(ftp->status != 350)
			{
				lseek(ftp->fd,0,SEEK_SET);
			}
		}
		else
		{
			printf("本地已经存在%s文件，是否覆盖(y/n)？",file);
			char cmd = getch();
			printf("%c\n",cmd);
			if('y' != cmd && 'Y' != cmd)
			{
				printf("放弃下载!\n");
				return;
			}
			ftp->fd = open(file,O_WRONLY|O_CREAT|O_TRUNC);
		}
	}
	sprintf(ftp->buf,"RETR %s\n",file);
	send_cmd(ftp);
	check_status(ftp,150,false);

	size_t ret_size = 0;
	while(ret_size = recv(ftp->cli_pasv,ftp->buf,BUF_SIZE,0))
	{
		write(ftp->fd,ftp->buf,ret_size);
	}
	close(ftp->cli_pasv);
	close(ftp->fd);

	check_status(ftp,226,false);
	ftp->is_get = false;
}

// bye命令
void bye_FTPClient(FTPClient* ftp)
{
	printf("%s %d %s\n",__FILE__,__LINE__,__func__);
	sprintf(ftp->buf,"QUIT\n");
	send_cmd(ftp);
	check_status(ftp,221,true);
}



