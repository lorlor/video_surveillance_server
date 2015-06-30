#ifndef _ENCODER_H_
#define _ENCODER_H_

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <iostream>
extern "C"{
#include <x264.h>
}

#define CLEAR(x)	(memset(&x), sizeof(0))

class Encoder{
public:
	Encoder(int w, int h);
	int set_param(void);
	void open_encoder(void);
	void close_encoder(void);
	void encode(char *s, int fd);
	void set_width(int w);
	void set_height(int h);
	int get_width(void);
	int get_height(void);
public:
	int		width;
	int		height;
	x264_param_t	*param;
	x264_t		*encoder;
	x264_picture_t	pic_in, pic_out;
	long user_pts;

	int nnal;
	x264_nal_t *nals;
	x264_nal_t *nal;
};

#endif		// encoder
