#include "capture.hpp"
#include "encoder.hpp"
#include "sender.hpp"
#include <iostream>

using namespace std;

int main(int argc, char **argv)
{
	FILE *fp;
	int fd;
	if((fd = open("out.h264", O_CREAT | O_WRONLY, 444)) == -1){
		cout << "Failed to open output file" << endl;
		exit(-1);
	}
	fp = fopen("out.yuv", "w+");
	if(fp == NULL){
		cout << "Failed to open output file" << endl;
		exit(EXIT_FAILURE);
	}
	Capture cap(fp, "/dev/video0");
/******************** Set capturing parameters. ***************************************
 *	Procesure:
 *		1. Initialize width and height based on the camera you use;
 *		2. Initialize buffer that holds per frame captured frome camera;
 *		3. Set I/O method;
 *		4. Initialize some variables that will be used in the following steps;
 *		5. (Optional) Initialize variables for SDL2 displaying on localhost.
 *
 *************************************************************************************/
	cap.set_width(640);
	cap.set_height(480);

	if(DEBUG){
		cout << cap.get_width() << endl;
		cout << cap.get_height() << endl;
	}

	if(cap.allocate())
		cout << "Success" << endl;
	else 
		cout << "Failure" << endl;
	cap.set_io_method(IO_METHOD_MMAP);
	cap.get_io_method();
	
	cap.init_fd(-1);
	cap.init_n_buffers(0);
	cap.init_buffers();

	cap.set_screen(800, 600);
	cap.set_pix(640, 480);

	cout << "Width of content:\t" << cap.get_pix_w() << endl;
	cout << "Height of content:\t" << cap.get_pix_h() << endl;
	cout << "Width of screen:\t" << cap.get_screen_w() << endl;
	cout << "Height of screen:\t" << cap.get_screen_h() << endl;

/******************* Set encoding parameters. *******************************
 *	Procesure:
 *		1. Set width and height;
 * 		2. Set encoding parameters for H264;
 *		3. Open encoder for encoding 
 *
 ****************************************************************************/
	Encoder encoder(640, 480);
	
	cout << "Width of encoder:\t" << encoder.get_width() << endl;
	cout << "Height of encoder:\t" << encoder.get_height() << endl;
	if(!encoder.set_param()){
		cout << "Failed to initialize parameters" << endl;
		exit(-1);
	}
	else{
		cout << "Parameter initializatoin success!" << endl;	
	}
	encoder.open_encoder();

/******************** Set sending parameters. **************************************
 *	Procesure:
 *		1. Set destination IP address that you want to stream your video to;
 *		2. Set destination port number which is for client;
 *		3. Set base port nubmer which is for server;
 *		4. Set parameters for RTP protocols;
 *		5. Set parameters for H264 protocols
 *
 ***********************************************************************************/
	Sender sender;
	sender.set_dst_ip("127.0.0.1");
	sender.set_destport(9000);		// remote destination port number
	sender.set_baseport(8000);		// local port number

	cout << "Baseport--> " << sender.get_baseport() << endl;
	cout << "Destport--> " << sender.get_destport() << endl;
	sender.setRTPParams();
	sender.setParamForH264();
/************************* Start to work. ***********************************/
	cap.open_device();
	cap.init_device();
	cap.start_capture();
	for(int i = 0; i < 200; i++){
		cap.mainloop(1);			// for capturing, maybe this could be changed as thread 1
		encoder.encode(cap.yuv420p, fd);	// for encoding, maybe this could be changed as thread 2
		for(encoder.nal = encoder.nals; encoder.nal < encoder.nals + encoder.nnal; 	encoder.nal++){
			sender.sendNalu(encoder.nal->p_payload, encoder.nal->i_payload);		// for sending, maybe this could be changed as thread 3
		}
	}
	cap.stop_capture();
	cap.uninit_device();
	cap.close_device();
	encoder.close_encoder();
	sender.BYEDestroy(RTPTime(10.0), "Time's up.", 9);

	return 0;
}
