/*
  测试v4l2的头文件
	fenbo........
  date: 2012.9.25 
*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include<getopt.h>
#include<fcntl.h>
#include<errno.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/time.h>
#include<sys/mman.h>
#include<sys/ioctl.h>
#include<asm/types.h>
#include<linux/videodev2.h>

#define CLEAR(x) memset(&(x),0,sizeof(x))

struct buffer{	//图像显示的缓冲区
	void * start;
	size_t	length;	//unsigned int
};
static char * dev_name = NULL;
//static io_method	io = IO_METHOD_MMAP; //2
static int	fd = -1;
static int	* count = NULL;
//struct buffer	* buffers = NULL;
static unsigned int	n_buffers = 0;
static FILE * outfile = 0;
static unsigned int cap_image_size = 0; //to keep the real image size!
//enum v4l2_buf_type * cap_type;
////////////////////////////////////
void init_video(void);
