#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <queue>
#include <iomanip>
#include <pthread.h>		// For multi-thread processing

/* For jrtp library */
#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtpudpv4transmitter.h>
#include <jrtplib3/rtpipv4address.h>
#include <jrtplib3/rtpsessionparams.h>
#include <jrtplib3/rtppacket.h>
#include <jrtplib3/rtcpapppacket.h>
#include <jrtplib3/rtperrors.h>
#include <jrtplib3/rtpsourcedata.h>

/* For network functions */
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* For Simple Direct Layer ver.2 (SDL2) */
#include <SDL2/SDL.h>

/* For FFmpeg library */
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <signal.h>
#include <string.h>
}

#define DEBUG		0

#define IMAGE_WIDTH	640
#define IMAGE_HEIGHT	480

// refresh event for SDL 2.0
#define SFM_REFRESH_EVENT	(SDL_USEREVENT + 1)

using namespace std;
using namespace jrtplib;

pthread_mutex_t		mutex; 			// for synchronizing  multi-thread

int 			write_fd;
int 			thread_exit = 0;
int 			main_cond;		// Used in main thread
int 			recv_cond;		// Used in receiving thread
FILE 			*fp;
int 			cache_count = 0;
char 			tmp[32];					// Used to store cache file name

fstream 		fs;			// for buffering stream data
queue<string> 		cache_q;		// Cache queue is used to store cache files.
						// For now, there are 10 cache files in cache directory.

class UserSession : public RTPSession
{
public:
	void OnBYEPacket(RTPSourceData *srcdat){
		recv_cond = 0;
		fs.close();
		cout << "Receiving: length is " << cache_q.size() << endl;
	}
	void OnAPPPacket(RTCPAPPPacket *apppacket, 
			const RTPTime &receivetime, 
			const RTPAddress *senderaddress){
		if(strcmp((const char *)apppacket->GetAPPData(), "init") != 0)
			fs.close();
//		pthread_mutex_unlock(&mutex);
		if(cache_count == 10)
			cache_count = 0;
		sprintf(tmp, "./cache/cache_%d", cache_count);
		fs.open(tmp, fstream::in | fstream::out | fstream::trunc);
		cache_q.push(tmp);
		cache_count++;
	}
};

void handler(int arg)
{
	main_cond = 0;
}

void checkerror(int rtperr)
{
	if(rtperr < 0){
		cout << "Error: " << RTPGetErrorString(rtperr) << endl;
		exit(-1);
	}
}

void error_handle(const char *errorInfo){
	printf("%s error!\n",errorInfo);
	exit(EXIT_FAILURE);
}

int sfp_refresh_thread(void *opaque)
{
	while(thread_exit == 0){
		SDL_Event event;
		event.type = SFM_REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(40);
	}

	return 0;
}

int init_mutex(pthread_mutex_t mutex){
	int ret;
	ret = pthread_mutex_init(&mutex, NULL);
	if(ret != 0){
		return 0;	
	}
	else{
		return 1;	
	}
}

/* Thread for receiving video stream */
void *recv_thr(void *arg)
{
	recv_cond = 1;
	long total = 0;
	string ipstr;
	uint16_t destport, baseport;
	uint32_t destip;

	int num;
	fp = fopen("res.h264", "w+");

	UserSession sess;
	destport = 8000;
	baseport = 9000;

	if(DEBUG){
		cout << "Enter destination IP address: " << endl;
		cin >> ipstr;
		cout << endl;
	}
	else
		ipstr = "127.0.0.1";

	destip = inet_addr(ipstr.c_str());
	if(destip == INADDR_NONE){
		cerr << "Bad IP Address" << endl;
		return ((void *)-1);
	}
	destip = ntohl(destip);

	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;

	transparams.SetPortbase(baseport);
	sessparams.SetOwnTimestampUnit(1.0/25.0);

	int status;
	status = sess.Create(sessparams, &transparams);
	checkerror(status);

	RTPIPv4Address addr(destip, destport);
	status = sess.AddDestination(addr);
	checkerror(status);

	while(recv_cond){
//	for(;;){
//		pthread_mutex_lock(&mutex);
		sess.BeginDataAccess();
		if(sess.GotoFirstSourceWithData()){
			do{
				RTPPacket *pack;
				while((pack = sess.GetNextPacket()) != NULL){
				//	total += pack->GetPayloadLength();
					if(DEBUG){
						printf("Got packet!\t");
						printf("Length of data slice is %lu\n", pack->GetPayloadLength());
					}
					fwrite(pack->GetPayloadData(), pack->GetPayloadLength(), 1, fp);
					fs.write((const char *)pack->GetPayloadData(), pack->GetPayloadLength());
//					fs.write("hello world\n", 12);
				}
			} while(sess.GotoNextSourceWithData());
		}

		sess.EndDataAccess();
		status = sess.Poll();
		checkerror(status);
	}
//	cout << total << endl;
//	pthread_mutex_unlock(&mutex);
	sess.BYEDestroy(RTPTime(15, 0), 0, 0);

	fclose(fp);

	return ((void *)1);
}

/* Thread for decoding packet. */
void *decode_thr(void *arg)
{
	int ret,videoStream;
	AVFormatContext * formatContext=NULL;
	AVCodec * codec;
	AVCodecContext * codecContext;
	AVFrame * decodedFrame;
	AVPacket packet;
	uint8_t *decodedBuffer;
	unsigned int decodedBufferSize;
	int finishedFrame;

	av_register_all();
	avformat_network_init();
	write_fd = open("recv.yuv", O_RDWR | O_CREAT | O_TRUNC, 0666);
	if(write_fd < 0){
		perror("open");
		exit(1);
	}
while(!cache_q.empty()){
	string cache = cache_q.front();
	cout << cache.c_str() << endl;

	ret=avformat_open_input(&formatContext, /*"res.h264",*/cache.c_str(), NULL, NULL);
	if(ret<0)
		error_handle("avformat_open_input error");

	ret=avformat_find_stream_info(formatContext,NULL);
	if(ret<0)
		error_handle("av_find_stream_info");


	videoStream=0;
//	codecContext = avcodec_alloc_context3(codec);
	codecContext=formatContext->streams[videoStream]->codec;
	codec=avcodec_find_decoder(AV_CODEC_ID_H264);

	if(codec==NULL)
		error_handle("avcodec_find_decoder error!\n");

	ret=avcodec_open2(codecContext,codec,NULL);
	if(ret<0)
		error_handle("avcodec_open2");

	decodedFrame=av_frame_alloc();
	if(!decodedFrame)
		error_handle("avcodec_alloc_frame!");

	decodedBufferSize=avpicture_get_size(PIX_FMT_YUV420P,IMAGE_WIDTH,IMAGE_HEIGHT);
	decodedBuffer=(uint8_t *)malloc(decodedBufferSize);
	if(!decodedBuffer)
		error_handle("malloc decodedBuffer error!");

	av_init_packet(&packet);
	
	while(av_read_frame(formatContext,&packet)>=0){
			ret=avcodec_decode_video2(codecContext,decodedFrame,&finishedFrame,&packet);
			if(ret<0)
				error_handle("avcodec_decode_video2 error!");
			if(finishedFrame){
				avpicture_layout((AVPicture*)decodedFrame,PIX_FMT_YUV420P,IMAGE_WIDTH,IMAGE_HEIGHT,decodedBuffer,decodedBufferSize);
				ret=write(write_fd,decodedBuffer,decodedBufferSize);
//				ret = write(write_fd, "hello\n", 6);
				if(ret<0)
					error_handle("write yuv stream error!");
			}

		av_free_packet(&packet);
	}

	cout << "DEbug" << endl;
	avformat_close_input(&formatContext);
	free(decodedBuffer);
	av_free(decodedFrame);
	avcodec_close(codecContext);
	cout << "Cache file : " << cache.c_str() << endl;
	cache_q.pop();
} // end of while statement
	return ((void *)1);
}

/* Thread for displaying */
void *d2d_thr(void *arg)
{
	// fp = fopen("res.h264", "r");
	// if(fp == NULL){
	// 	fprintf(stderr, "Thread d2d: Failed to open encoded file.\n");
	// 	return ((void *)-1);
	// }

	AVFormatContext	*pFormatCtx;
	int				i, videoindex;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	AVFrame	*pFrame,*pFrameYUV;
	uint8_t *out_buffer;
	AVPacket *packet;
	int ret, got_picture;

	//------------SDL----------------
	int screen_w,screen_h;
	SDL_Window *screen; 
	SDL_Renderer* sdlRenderer;
	SDL_Texture* sdlTexture;
	SDL_Rect sdlRect;
	SDL_Thread *video_tid;
	SDL_Event event;

	struct SwsContext *img_convert_ctx;

	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	if(avformat_open_input(&pFormatCtx,"res.h264",NULL,NULL)!=0){
		printf("Couldn't open input stream.\n");
		return ((void *)-1);
	}
	if(avformat_find_stream_info(pFormatCtx,NULL)<0){
		printf("Couldn't find stream information.\n");
		return ((void *)-1);
	}
	videoindex=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++) 
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
			videoindex=i;
			break;
		}
	if(videoindex==-1){
		printf("Didn't find a video stream.\n");
		return ((void *)-1);
	}
	pCodecCtx=pFormatCtx->streams[videoindex]->codec;
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL){
		printf("Codec not found.\n");
		return ((void *)-1);
	}
	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
		printf("Could not open codec.\n");
		return ((void *)-1);
	}
	pFrame=av_frame_alloc();
	pFrameYUV=av_frame_alloc();
	out_buffer=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

	//Output Info-----------------------------
	printf("---------------- File Information ---------------\n");
	av_dump_format(pFormatCtx,0,"res.h264",0);
	printf("-------------------------------------------------\n");
	
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 
		pCodecCtx->width, pCodecCtx->height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 
	

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {  
		printf( "Could not initialize SDL - %s\n", SDL_GetError()); 
		return ((void *)-1);
	} 
	//SDL 2.0 Support for multiple windows
	screen_w = pCodecCtx->width;
	screen_h = pCodecCtx->height;
	screen = SDL_CreateWindow("Simplest ffmpeg player's Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screen_w, screen_h,SDL_WINDOW_OPENGL);

	if(!screen) {  
		printf("SDL: could not create window - exiting:%s\n",SDL_GetError());  
		return ((void *)-1);
	}
	sdlRenderer = SDL_CreateRenderer(screen, -1, 0);  
	//IYUV: Y + U + V  (3 planes)
	//YV12: Y + V + U  (3 planes)
	sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,pCodecCtx->width,pCodecCtx->height);  

	sdlRect.x=0;
	sdlRect.y=0;
	sdlRect.w=screen_w;
	sdlRect.h=screen_h;

	packet=(AVPacket *)av_malloc(sizeof(AVPacket));

	video_tid = SDL_CreateThread(sfp_refresh_thread,NULL,NULL);
	//------------SDL End------------
	//Event Loop
	
	for (;;) {
		//Wait
		SDL_WaitEvent(&event);
		if(event.type==SFM_REFRESH_EVENT){
			//------------------------------
			if(av_read_frame(pFormatCtx, packet)>=0){
				if(packet->stream_index==videoindex){
					ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
					if(ret < 0){
						printf("Decode Error.\n");
						return ((void *)-1);
					}
					if(got_picture){
						sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
						//SDL---------------------------
						SDL_UpdateTexture( sdlTexture, NULL, pFrameYUV->data[0], pFrameYUV->linesize[0] );  
						SDL_RenderClear( sdlRenderer );  
						//SDL_RenderCopy( sdlRenderer, sdlTexture, &sdlRect, &sdlRect );  
						SDL_RenderCopy( sdlRenderer, sdlTexture, NULL, NULL);  
						SDL_RenderPresent( sdlRenderer );  
						//SDL End-----------------------
					}
				}
				av_free_packet(packet);
			}else{
				//Exit Thread
				thread_exit=1;
				break;
			}
		}else if(event.type==SDL_QUIT){
			thread_exit=1;
			break;
		}

	}

	sws_freeContext(img_convert_ctx);

	SDL_Quit();
	//--------------
	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return ((void *)1);
}

int main(int argc, char **argv)
{
/*	if(init_mutex(mutex)){
		printf("Initialize mutex succeeded\n");	
	}
	else{
		printf("Initialize mutex failed\n");	
	}
	*/
	pthread_t recv_tid, decode_tid, d2d_tid;
	int err;
	void *tret;
	main_cond = 1;

	signal(SIGINT, handler);

	err = pthread_create(&recv_tid, NULL, recv_thr, NULL);
	if(err != 0){
		fprintf(stderr, "Can't create receiving thread\n");
		exit(-1);
	}

	
	err = pthread_join(recv_tid, &tret);
	if(err != 0){
		fprintf(stderr, "Can't join with thread %lu\n", recv_tid);	
		exit(-1);
	}
	
	
/*if(DEBUG){*/
	err = pthread_create(&decode_tid, NULL, decode_thr, NULL);
	if(err != 0){
		fprintf(stderr, "Can't create decoding thread\n");
		exit(-1);
	}
/*
	err = pthread_join(decode_tid, &tret);
	if(err != 0){
		fprintf(stderr, "Can't join with thread %lu\n", decode_tid);
		exit(-1);
	}
}

	err = pthread_create(&d2d_tid, NULL, d2d_thr, NULL);
	if(err != 0){
		fprintf(stderr, "Cat't create displaying thread\n");
		exit(-1);
	}
	err = pthread_join(d2d_tid, &tret);
	if(err != 0){
		fprintf(stderr, "Can't join with thread %lu\n", d2d_tid);
		exit(-1);
	}
	*/
	while(main_cond);

	return 0;
}
