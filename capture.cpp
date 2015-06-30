#include "capture.hpp"
#include <iostream>

using namespace std;

Capture::Capture(){

}

Capture::Capture(FILE *out_file, char *dev_name){
	this->fp = out_file;
	this->dev_name = dev_name;
}

void Capture::errno_exit(char *s){
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}

int Capture::xioctl(int fd, int request, void *arg){
	int r;
	do r = ioctl(fd, request, arg);
	while(-1 == r && EINTR == errno);

	return r;
}

char *Capture::process_image(char *p){
	char *y = yuv420p;
	char *u = &yuv420p[width * height];
	char *v = &yuv420p[width * height + width * height / 4];
	
	for(int j = 0; j < height; j++){
		for(int i = 0; i < width * 2; i++){
			if(i % 2 == 0){
				*y = p[i+j*width*2];
				++y;
			}	

			if(j % 2 == 0){
				if(i % 4 == 1){
					*u = p[j*width*2 + i];
					++u;
				}	
			}

			if(j % 2 == 1){
				if(i % 4 == 3){
					*v = p[j*width*2 + i];
					++v;
				}	
			}
		}	
	}	

	if(DEBUG)
		fwrite(yuv420p, width * height * 3 >> 1, 1, fp);
	else
		disp(yuv420p);

	return yuv420p;
}

int Capture::read_frame(void){
	struct v4l2_buffer buf;
	unsigned int i;
	
	switch(io_method){
	case IO_METHOD_READ:
		if(-1 == read(fd, buffers[0].start, buffers[0].length)){
			switch(errno){
			case EAGAIN:
				return 0;
			case EIO:
				/*Could ignore EIO, see spec.*/
				/*Fall through*/
			default:
				errno_exit("read");
			}	
		}
		break;

	case IO_METHOD_MMAP:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		if(-1 == xioctl(fd, VIDIOC_DQBUF, &buf)){
			switch(errno){
			case EAGAIN:
				return 0;

			case EIO:
				/*Could ignore EIO, see spec.*/
				/*Fall through*/
			default:
				errno_exit("VIDIOC_DQBUF");
			}	
		}

		assert(buf.index < n_buffers);

		process_image((char *)buffers[buf.index].start);

		if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");

		break;
	
	case IO_METHOD_USERPTR:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;

		if(-1 == xioctl(fd, VIDIOC_DQBUF, &buf)){
			switch(errno){
			case EAGAIN:
				return 0;

			case EIO:
				/*Could ignore EIO, see spec.*/
				/*Fall through*/
			default:
				errno_exit("VIDIOC_DQBUF");
			}	
		}
		for(i = 0; i < n_buffers; ++i)
			if(buf.m.userptr == (unsigned long)buffers[i].start && buf.length == buffers[i].length)
				break;
		assert(i < n_buffers);

		if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");
		break;
	}

	return 1;
}

void Capture::mainloop(int num){
	unsigned int count;
	count = num;
	while(count != 0){
		for(;;){
			fd_set fds;
			struct timeval tv;
			int r;

			FD_ZERO(&fds);
			FD_SET(fd, &fds);

			/* Timeout */
			tv.tv_sec = 2;
			tv.tv_usec = 0;

			r = select(fd + 1, &fds, NULL, NULL, &tv);

			if(-1 == r){
				if(EINTR == errno)
					continue;
				errno_exit("select");
			}

			if(0 == r){
				fprintf(stderr, "select timeout\n");
				exit(EXIT_FAILURE);
			}

			if(read_frame())
				break;
		}	
		count--;
	}
}

void Capture::stop_capture(void){
	enum v4l2_buf_type type;
	switch(io_method){
	case IO_METHOD_READ:
		/*Nothing to do*/
		break;
	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if(-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
			errno_exit("VIDIOC_STREAMOFF");

		break;
	}
}

void Capture::start_capture(void){
	enum v4l2_buf_type type;
	switch(io_method){
	case IO_METHOD_READ:
		/*Nothing to do*/
		break;
	
	case IO_METHOD_MMAP:
		for(int i = 0; i < n_buffers; ++i){
			struct v4l2_buffer buf;
			
			CLEAR(buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;

			if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if(-1 == xioctl(fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");

		break;

	case IO_METHOD_USERPTR:
		for(int i = 0; i < n_buffers; ++i){
			struct v4l2_buffer buf;

			CLEAR(buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;
			buf.m.userptr = (unsigned long)buffers[i].start;
			buf.length = buffers[i].length;

			if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if(-1 == xioctl(fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");

		break;
	}
}

void Capture::uninit_device(void){
	switch(io_method){
	case IO_METHOD_READ:
		free(buffers[0].start);
		break;

	case IO_METHOD_MMAP:
		for(int i = 0; i < n_buffers; ++i)
			if(-1 == munmap(buffers[i].start, buffers[i].length))
				errno_exit("munmap");

		break;

	case IO_METHOD_USERPTR:
		for(int i = 0; i < n_buffers; ++i)
			free(buffers[i].start);
		break;
	}

	free(buffers);
}

void Capture::init_device(void){
	struct v4l2_capability	cap;
	struct v4l2_cropcap	cropcap;
	struct v4l2_crop	crop;
	struct v4l2_format	fmt;

	unsigned int min;

	if(-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)){
		if(EINVAL == errno){
			fprintf(stderr, "%s is no v4l2 device\n", dev_name);
			exit(EXIT_FAILURE);
		}	
		else {
			errno_exit("VIDIOC_QUERYCAP");	
		}
	}

	if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
		fprintf(stderr, "%s is no video capture device\n", dev_name);
		exit(EXIT_FAILURE);
	}

	switch(io_method){
	case IO_METHOD_READ:
		if(!(cap.capabilities & V4L2_CAP_READWRITE)){
			fprintf(stderr, "%s does not support read i/o\n", dev_name);
			exit(EXIT_FAILURE);
		}

		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if(!(cap.capabilities & V4L2_CAP_STREAMING)){
			fprintf(stderr, "%s does not support streaming i/o \n", dev_name);
			exit(EXIT_FAILURE);
		}

		break;
	}
	
	/* Select video input, video standard and tune here. */

	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)){
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; 	/*reset to default*/

		if(-1 == xioctl(fd, VIDIOC_S_CROP, &crop)){
			switch(errno){
			case EINVAL:
				/*Cropping not supported. */
				break;

			default:
				/*Errors ignored.*/
				break;
			}	
		}
	}
	else{
		/*Errors ignored. */	
	}

	CLEAR(fmt);

	fmt.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width	= width;
	fmt.fmt.pix.height	= height;
	fmt.fmt.pix.pixelformat	= V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field	= V4L2_FIELD_INTERLACED;

	if(-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
		errno_exit("VIDIOC_S_FMT");

	/*Note: VIDIOCS_S_FMT may change width and height. */

	/*Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if(fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if(fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;
	
	switch(io_method){
	case IO_METHOD_READ:
		init_read(fmt.fmt.pix.sizeimage);
		break;
	
	case IO_METHOD_MMAP:
		init_mmap();
		break;

	case IO_METHOD_USERPTR:
		init_userp(fmt.fmt.pix.sizeimage);
		break;
	}
}

void Capture::close_device(void){
	if(-1 == close(fd))
		errno_exit("close");
	
	fd = -1;
}

void Capture::open_device(void){
	struct stat st;

	if(-1 == stat(dev_name, &st)){
		fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if(!S_ISCHR(st.st_mode)){
		fprintf(stderr, "%s is no device \n", dev_name);
		exit(EXIT_FAILURE);
	}

	fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);

	if(-1 == fd){
		fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void Capture::init_read(unsigned int buffer_size){
	buffers = (struct buffer *)calloc(1, sizeof(*buffers));

	if(!buffers){
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	buffers[0].length = buffer_size;
	buffers[0].start = malloc(buffer_size);

	if(!buffers[0].start){
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}
}

void Capture::init_mmap(void){
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count	= 4;
	req.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory	= V4L2_MEMORY_MMAP;

	if(-1 == xioctl(fd, VIDIOC_REQBUFS, &req)){
		if(EINVAL == errno){
			fprintf(stderr, "%s does not support memory mapping\n", dev_name);
			exit(EXIT_FAILURE);
		}	
		else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	if(req.count < 2){
		fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
		exit(EXIT_FAILURE);
	}

	buffers = (struct buffer *)calloc(req.count, sizeof(*buffers));

	if(!buffers){
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for(n_buffers = 0; n_buffers < req.count; ++n_buffers){
		struct v4l2_buffer buf;
		CLEAR(buf);

		buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory	= V4L2_MEMORY_MMAP;
		buf.index	= n_buffers;

		if(-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
			errno_exit("VIDIOC_QUERYBUF");

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = 
			mmap(NULL,	/*start anywhere*/
				buf.length,
				PROT_READ | PROT_WRITE /*required*/,
				MAP_SHARED /*recommended*/,
				fd, buf.m.offset);

		if(MAP_FAILED == buffers[n_buffers].start)
			errno_exit("mmap");
	}
}

void Capture::init_userp(unsigned int buffer_size){
	struct v4l2_requestbuffers req;
	CLEAR(req);

	req.count	= 4;
	req.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory	= V4L2_MEMORY_USERPTR;

	if(-1 == xioctl(fd, VIDIOC_REQBUFS, &req)){
		if(EINVAL == errno){
			fprintf(stderr, "%s does not support uesr pointer i/o\n", dev_name);
			exit(EXIT_FAILURE);
		}	
		else{
			errno_exit("VIDIOC_REQBUFS");	
		}
	}

	buffers = (struct buffer *)calloc(4, sizeof(*buffers));
	if(!buffers){
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for(n_buffers = 0; n_buffers < 4; ++n_buffers){
		buffers[n_buffers].length = buffer_size;
		buffers[n_buffers].start = malloc(buffer_size);

		if(!buffers[n_buffers].start){
			fprintf(stderr, "Out of memory \n");
			exit(EXIT_FAILURE);
		}
	}
}

void Capture::set_io_method(enum method way){
	if(way > 3 || way < 0){
		fprintf(stderr, "Invalid io method code\n");
		exit(EXIT_FAILURE);
	}
	else{
		this->io_method = way;	
	}
}

void Capture::get_io_method(void){
	if(io_method == 0)
		cout << "Method: IO_METHOD_READ" << endl;
	if(io_method == 1)
		cout << "Method: IO_METHOD_MMAP" << endl;
	if(io_method == 2)
		cout << "Method: IO_METHOD_USERPTR" << endl;
}

void Capture::set_device(char *name){
	this->dev_name = name;
}

char *Capture::get_device(void){
	return this->dev_name;
}

void Capture::set_width(int w){
	this->width = w;
}

int Capture::get_width(void){
	return this->width;
}

void Capture::set_height(int h){
	this->height = h;
}

int Capture::get_height(void){
	return this->height;
}

int Capture::open_output(char *s){
	fp = fopen(s, "a+");
	if(fp == NULL){
		fprintf(stderr, "Failed to open output file\n");
		return 0;
	}
	else
		return 1;
}

int Capture::allocate(void){
	yuv420p = (char *)malloc(width * height * 3 >> 1);
	if(yuv420p == NULL){
		fprintf(stderr, "Error: failed to allocate\n");
		return 0;
	}
	else{
		return 1;	
	}
}

void Capture::init_fd(int fd){
	this->fd = fd;
}

void Capture::init_n_buffers(int n_buffers){
	this->n_buffers = n_buffers;
}

void Capture::init_buffers(void){
	this->buffers = NULL;
}

void Capture::set_screen(int w, int h){
	this->screen_w = w;
	this->screen_h = h;
} 

int Capture::get_screen_h(void){
	return this->screen_h;
}

int Capture::get_screen_w(void){
	return this->screen_w;
}

void Capture::set_pix(int w, int h){
	this->pix_w = w;
	this->pix_h = h;
}

int Capture::get_pix_w(void){
	return this->pix_w;
}

int Capture::get_pix_h(void){
	return this->pix_h;
}

int Capture::disp(char *s){
	if(SDL_Init(SDL_INIT_VIDEO) < 0){
		fprintf(stderr, "Cannot initialize: %s\n", SDL_GetError());
		return 0;
	}

	screen = SDL_CreateWindow("Disp", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if(!screen){
		fprintf(stderr, "SDL: cannot create window %s\n", SDL_GetError());	
		return 0;
	}

	rect.x = 0;
	rect.y = 0;
	rect.w = screen_w;
	rect.h = screen_h;
	renderer = SDL_CreateRenderer(screen, -1, 0);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, pix_w, pix_h);

	SDL_UpdateTexture(texture, NULL, s, pix_w);

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, &rect);
	SDL_RenderPresent(renderer);

	SDL_Delay(40);
}
