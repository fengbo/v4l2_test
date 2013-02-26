/*
T
	fengbo
  date:2012.9.25
*/

#include"video.h"
struct buffer *buffers = NULL;
static void errno_exit(const char *s);

static int xioctl(int fd,int request,void *arg);

static void creat_file(void);

static void mainloop(void);

static void stop_capturing(void);

static void start_capturing(void);

static void init_device(void);

static void close_device(void);

static void open_device(void);

//static void init_read(unsigned int buffer_size);

static void init_mmap(void);

static int read_frame(void);

static void free_mmap(void);
//////////////////////////////////////////////////////
void init_video(void){
	
	dev_name = "/dev/video0";	//设备接口
	creat_file();	
	open_device();			//打开设备
	init_device();			//获得设备相关信息，对设备进行设置
	//init_mmap();			//内核空间缓冲区和用户缓冲区设置
	start_capturing();		//视频进入缓冲区和启动视频采集
//	mainloop();//一帧完后读取缓冲区数据
	stop_capturing();
	close_device();		//关闭
	free_mmap();		//释放
	fclose(outfile);	//关闭文件
}
static void errno_exit (const char *s){
	fprintf(stderr,"%s error %d,%s\n",s,errno,strerror(errno));//errno是那个错误编号
								  //流文件中
	exit(EXIT_FAILURE);
}
/////////////////////////////////////////////////////
static int xioctl(int fb,int request,void *arg){
	int r;
	do{
		r = ioctl(fd,request,arg); 
	   }
	while(-1 == r && EINTR == errno);
return r;
	
}
//////////////////////creat_file//////////////新建一个yuv类文件
static void creat_file(void){
	outfile = fopen("file_video.yuv","wb");
	if(outfile == NULL){
		errno_exit("can not open the file_video!\n");
	}
}
//////////////////////////////process_image///////////
static void process_image(const void *p,int len){//图像取出来--文件形式
	fputc('.',stdout);
	if(len > 0){
		fputc('.',stdout);
		fwrite(p,len,1,outfile);
	}
	printf("writting into outfile successfuly.\n");
	fflush(stdout);//刷新
}
/////////////////////////////////close_device//////////////////////
static void close_device(void){
	if(close (fd) == -1)
		errno_exit("close");
		fd = -1;
	printf("close the video0 success!\n");
}
/////////////////////////////////open_device/////////////////////////
static void open_device(void){
	struct stat st;
	if(stat (dev_name,&st) == -1){
		fprintf(stderr,"Cannot identify '%s': %d, %s\n",dev_name,errno,strerror (errno));
		exit(EXIT_FAILURE);
	}

	if(!S_ISCHR(st.st_mode)){
		fprintf(stderr,"%s is no device \n",dev_name);
		exit(EXIT_FAILURE);
	}
	fd = open(dev_name,O_RDWR | O_NONBLOCK,0);//打开设备非阻塞模式
	if(fd == -1){
		fprintf(stderr,"cannot open '%s':%d,%s\n",dev_name,errno,strerror(errno));
		exit(EXIT_FAILURE);
	}
	else
		printf("open %s is success \n",dev_name);
}
////////////////////////////////////////init_device//////////
static void init_device(void){
	struct v4l2_capability cap;	//设备的功能
	struct v4l2_cropcap cropcap;	//
	struct v4l2_crop crop;		//视频信号矩形边框
	struct v4l2_format fmt;		//帧的格式
	//unsigned int min;
	
	if(xioctl(fd,VIDIOC_QUERYCAP,&cap) == -1){	//查询驱动功能
		if(EINVAL == errno){
			fprintf(stderr,"%s is no v4l2 device\n",dev_name);
			exit(EXIT_FAILURE);
		}
		else
			errno_exit("VIDIOC_QUERYCAP");
	}
	
	if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){//匹配Type of date stream查看是否支持
		fprintf(stderr,"%s is no video capture device",dev_name);
		exit(EXIT_FAILURE);
	}
	printf("capture device set success\n");
	// selet video input ,video standard and tune here
	CLEAR(cropcap);
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(xioctl(fd,VIDIOC_CROPCAP,&cropcap) == 0){	//查询驱动的修剪能力
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; //rest to default ?
		if(xioctl(fd,VIDIOC_S_CROP,&crop) == -1){	//设置视频信号边框
			switch(errno){
				case EINVAL:	break;
			
				default:	break;
			}
		}	
	}
	else{ }//errors ignord
	
	CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = 640;	//设置帧的格式
	fmt.fmt.pix.height = 480;	//
	fmt.fmt.pix.pixelformat	= V4L2_PIX_FMT_YUYV;//yuyv 格式
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	
	if(xioctl(fd,VIDIOC_S_FMT,&fmt) == -1)	//驱动的频捕格式
		errno_exit("VIDIOC_S_FMT");
		cap_image_size = fmt.fmt.pix.sizeimage;
	printf("set the v4l2_format is success\n");
	//////////////////////////////////////
		init_mmap();//设置mmap  与请求缓冲区并通过mmap函数映射到用户空间
	
}
////////////////////////////////////////////init_mmp////////////////////
static void init_mmap(void){
	struct v4l2_requestbuffers req;
	CLEAR(req);
	req.count = 4;//申请缓冲区4个
	count = &req.count; //
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory =V4L2_MEMORY_MMAP;
	//设置视频缓冲规格
	if(xioctl(fd,VIDIOC_REQBUFS,&req) == -1){//分配内核内存申请用户不能使用
		if(EINVAL ==errno){
			fprintf(stderr,"%s does not support" "memory mapping\n",dev_name);
			exit(EXIT_FAILURE);
		}
		else
			errno_exit("VIDIOC_REQBUFS");
	}
	if(req.count<2){
		fprintf(stderr,"insufficient buffer memory on %s \n",dev_name);
		exit(EXIT_FAILURE);
	}
	buffers = calloc(req.count,sizeof(* buffers));	//分配内核缓冲区内存
	if(! buffers){
		fprintf(stderr,"out of memory\n");
		exit(EXIT_FAILURE);
	}
	for(n_buffers = 0; n_buffers < req.count; ++n_buffers){
		struct v4l2_buffer buf;
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;	//?
		buf.index = n_buffers;

		if(xioctl(fd,VIDIOC_QUERYBUF,&buf) == -1)//把idioc_querybuf内核中分配的数据缓冲区信息
			errno_exit("VIDIOC_QUERYBUF");

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = mmap(NULL,buf.length,PROT_READ | PROT_WRITE,MAP_SHARED,fd,buf.m.offset);//map device memory to appication address sapce 内核空间地址映射到用户空间
		if(MAP_FAILED == buffers[n_buffers].start)
			errno_exit("mmap");
	}
	printf("mmap is successfuly\n");
}
////////////////////////////////////
static void free_mmap(void){
	int a;
	for(a = 0; a < *count; a ++){
		munmap(buffers[a].start,buffers[a].length);
	}
	free(buffers);
	printf("munmap and freeing the buffers is successfuly\n");
}
//////////////////////////////////////////capturing//////////
static void stop_capturing(void){
	enum v4l2_buf_type type; ///?
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(xioctl(fd,VIDIOC_STREAMOFF,&type) == -1)
		errno_exit("VIDIOC_STREAMOFF");
	printf("stop the stop capturing successfuly!\n");
}
static void start_capturing(void){
	unsigned int i;
	enum v4l2_buf_type type;
	for(i =0 ; i < n_buffers; ++ i){
		struct v4l2_buffer buf;
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;//放入的编号
		if(xioctl(fd,VIDIOC_QBUF,&buf) == -1)	//投放视频缓冲区到视频缓冲区队列中
		errno_exit("VIDIOC_QBUF");
		}
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if(xioctl(fd,VIDIOC_STREAMON,&type) == -1)//启动视频采集命令
			errno_exit("VIDIOC_STREAMON");
		printf("the video is seting well\n");
		mainloop();
}

///////////////////////////////////////mainloop/////////////////////////////////
static void mainloop(void){
		unsigned int count;
		count =100;
	for(;;){
		if(count <= 0){
			count=100;
		}
		printf("第%d",count);
		while(count  > 0){
			count--;
			fd_set fds;
			struct timeval tv;
			int r;
			FD_ZERO(&fds);//清零
			FD_SET(fd,&fds);//将fd加入fds中
			// Timeout
			tv.tv_sec = 2;
			tv.tv_usec = 0;
			r = select(fd + 1,&fds,NULL,NULL,&tv);//完成一帧的视频采集
			printf("幀视频采集到缓冲区完成,请继续读.....\n");
			if(r == -1){
				if(EINTR == errno)
					continue;
				errno_exit("select");
				}
			if(r == 0){
				fprintf(stderr,"select timeout \n");
				exit(EXIT_FAILURE);
				}
			if(read_frame()){
			//	printf("count = %d\n",count);
				break;
			//EAGAIN - continue select loop.
			}
		}
	}	
}
//////////////////////////////read_frame///////////////////
static int read_frame(void){
	struct v4l2_buffer buf;
	CLEAR(buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//?
	buf.memory = V4L2_MEMORY_MMAP;
	if(xioctl(fd,VIDIOC_DQBUF,&buf) == -1){	//从缓冲区中取得一个已经保存的一帧视频数据视频缓冲区
		switch(errno){
			case EAGAIN:	return 0;
			case EIO:	//Could ignore ei0,see,space
					//fall through
			default:	errno_exit("VIDIOC_DQBUF");
			}
	}
	assert(buf.index < n_buffers);//?
	// printf("length = %d /r",buffers[buf.index].length);
	//proccess_image(buffers[buf.index].start,buffers[buf.index].length);
	printf("image_size = %d, IO_METHOD_MMAP buffer.length = %d \t",cap_image_size,buffers[0].length);
	printf("the buffers start adder is = %p\n",buffers[0].start);
	process_image(buffers[0].start,cap_image_size);
	if(xioctl(fd,VIDIOC_QBUF,&buf) == -1){//在入队列
		errno_exit("VIDIOC_QBUF");
	}
return 1;
}
