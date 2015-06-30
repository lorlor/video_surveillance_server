#ifndef _CAPTURE_H_
#define _CAPTURE_H_
#include <cstdio>
#include <cstdlib>
#include <assert.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <string.h>	// for memset

#include <asm/types.h>
#include <linux/videodev2.h>

/* For SDL2 */
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>

#define CLEAR(x)	memset(&(x), 0, sizeof(x))
#define DEBUG	1

struct buffer {
	void *start;
	size_t length;
};

enum method {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR
};

class Capture
{
	public:	
		Capture();
		Capture(FILE *out_file, char *dev_name);
		void errno_exit(char *s);
		int xioctl(int fd, int request, void *arg);
		char *process_image(char *p);
		int read_frame(void);
		void mainloop(int num);
		
		void stop_capture(void);
		void start_capture(void);

		void uninit_device(void);
		void init_device(void);

		void close_device(void);
		void open_device(void);

		void init_read(unsigned int buffer_size);
		void init_mmap(void);
		void init_userp(unsigned int buffer_size);

		void set_io_method(enum method way);
		void get_io_method(void);

		void set_device(char *name);
		char *get_device(void);	

		void set_width(int w);
		int get_width(void);
		void set_height(int h);
		int get_height(void);

		int open_output(char *s);
		int allocate(void);
		
		void init_fd(int fd);
		void init_n_buffers(int n_buffers);
		void init_buffers(void);

		// Functions for SDL2
		void set_screen(int w, int h);
		int get_screen_w(void);
		int get_screen_h(void);

		void set_pix(int w, int h);
		int get_pix_w(void);
		int get_pix_h(void);

		int disp(char *s);

	protected:
		enum method io_method;
		char *dev_name;
		int width, height;
		int fd;				//initial value is -1
		unsigned int n_buffers;		//initial value is 0
		FILE *fp;
		struct buffer *buffers;

		// Variables for SDL2
		int screen_w;
		int screen_h;
		int pix_w;
		int pix_h;
		SDL_Texture	*texture;
		SDL_Renderer	*renderer;
		SDL_Rect	rect;
		SDL_Window	*screen;
	public:
		char *yuv420p;
};

#endif		//class Capture
