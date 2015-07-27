#include "encoder.hpp"
/* This is the implementation of class Encoder*/

using namespace std;

Encoder::Encoder(int w, int h){
	this->width = w;
	this->height = h;
}

int Encoder::set_param(void){
	this->param = (x264_param_t *)malloc(sizeof(x264_param_t));
	if(NULL == this->param){
		fprintf(stderr, "Failed allocate for x264_param_t\n");
		return 0;
	}
	else{
		x264_param_default_preset(this->param, "fast", "zerolatency");	// Original option is [veryfast].
		this->param->i_threads = X264_SYNC_LOOKAHEAD_AUTO;
		this->param->i_width = this->width;
		this->param->i_height = this->height;
		this->param->i_keyint_max = 25;
		this->param->b_intra_refresh = 0;
		this->param->i_frame_total = 0;
		this->param->i_frame_reference = 4;

		this->param->i_bframe = 2;
		this->param->b_open_gop = 0;
		this->param->i_bframe_pyramid = 0;
		this->param->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;

		this->param->i_log_level = X264_LOG_DEBUG;

		this->param->rc.i_bitrate = 192 * 480;
		this->param->i_fps_den = 1;
		this->param->i_fps_num = 25;
		this->param->i_timebase_den = this->param->i_fps_num;
		this->param->i_timebase_num = this->param->i_fps_den;

		this->param->b_annexb = 1;
		this->param->b_repeat_headers = 1;

		x264_param_apply_profile(param, "baseline");

		return 1;
	}
}

void Encoder::open_encoder(void){
	this->encoder = x264_encoder_open(this->param);
}

void Encoder::close_encoder(void){
	x264_encoder_close(this->encoder);
}

/* Note:
 *	parameter char *s should share the same size
 *	with per frame of raw video.*/
void Encoder::encode(char *s, int fd){
	x264_picture_alloc(&pic_in, X264_CSP_I420, this->width, this->height);

	pic_in.img.plane[0] = (unsigned char*)s;
	pic_in.img.plane[1] = pic_in.img.plane[0] + this->width * this->height;
	pic_in.img.plane[2] = pic_in.img.plane[1] + this->width * this->height / 4;

	pic_in.i_pts = this->user_pts;
//	int nnal;
//	x264_nal_t *nals;
//	cout << "B" << endl;
	x264_encoder_encode(encoder, &nals, &nnal, &pic_in, &pic_out);
//	cout << "D" << endl;
//	x264_nal_t *nal;
	for(nal = nals; nal < nals + nnal; nal++)
		write(fd, nal->p_payload, nal->i_payload);
}

void Encoder::set_width(int w){
	this->width = w;
}

int Encoder::get_width(void){
	return this->width;
}

void Encoder::set_height(int h){
	this->height = h;
}

int Encoder::get_height(void){
	return this->height;
}
