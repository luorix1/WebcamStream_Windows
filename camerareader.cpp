#include <stdio.h>
#include <winsock.h>
#include <io.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <SDL.h>
#include <curl.h>
#include <WinInet.h>

#define __STDC_CONSTANT_MACROS

extern "C"
{
#include "libavutil/mathematics.h"
#include "libavutil/time.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libavfilter/avfiltergraph.h"
};

#pragma comment(lib, "SDL.lib")
#pragma comment(lib, "SDLmain.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment (lib, "WinInet.lib")


// fixes SDL error stemming from outdated, wrongly defined variables
#ifdef main
#undef main
#endif


#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)  

int thread_exit = 0;
static int videoindex = -1;                //video index in stream.
static int audioindex = -1;                //audio index in stream.
static int vFrameCount = 0;
static AVFormatContext* vifmtCtx;
static AVFormatContext* vofmtCtx;

AVCodecID en_codec_id = AV_CODEC_ID_H264;
AVCodecID au_codec_id = AV_CODEC_ID_AAC;


typedef struct FilteringContext
{
	AVFilterContext* buffersink_ctx;
	AVFilterContext* buffersrc_ctx;
	AVFilterGraph* filter_graph;
} FilteringContext;

static FilteringContext* filter_ctx;
SDL_Overlay* bmp;


//keeps track of pts to collect delayed frames and prevent non monotonic PTS
int nextPTS()
{
	static int static_pts = 0;
	return static_pts++;
}

static int initFilter(AVCodecContext* dec_ctx, AVCodecContext* enc_ctx)
{
	filter_ctx = (FilteringContext*)av_malloc(sizeof(*filter_ctx));
	if (!filter_ctx)
		return AVERROR(ENOMEM);
	filter_ctx->buffersrc_ctx = NULL;
	filter_ctx->buffersink_ctx = NULL;
	filter_ctx->filter_graph = NULL;

	char args[512];
	int ret = 0;
	AVFilter* buffersrc = NULL;
	AVFilter* buffersink = NULL;
	AVFilterContext* buffersrc_ctx = NULL;
	AVFilterContext* buffersink_ctx = NULL;
	AVFilterInOut* outputs = avfilter_inout_alloc();
	AVFilterInOut* inputs = avfilter_inout_alloc();
	AVFilterGraph* filter_graph = avfilter_graph_alloc();
	if (!outputs || !inputs || !filter_graph)
	{
		ret = AVERROR(ENOMEM);
		goto end; //free I/O
	}
	buffersrc = avfilter_get_by_name("buffer");
	buffersink = avfilter_get_by_name("buffersink");
	if (!buffersrc || !buffersink)
	{
		av_log(NULL, AV_LOG_ERROR, "filtering source or sink element not found\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	ret = snprintf(args, sizeof(args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt, dec_ctx->time_base.num, dec_ctx->time_base.den,
		dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den);
	if (ret == sizeof(args) || ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "args is too short for format.");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
		goto end;
	}
	ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
		goto end;
	}
	ret = av_opt_set_bin(buffersink_ctx, "pix_fmts", (uint8_t*)&enc_ctx->pix_fmt, sizeof(enc_ctx->pix_fmt), AV_OPT_SEARCH_CHILDREN);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "set output pixel format wrong.");
		goto end;
	}
	outputs->name = av_strdup("in");
	outputs->filter_ctx = buffersrc_ctx;
	outputs->pad_idx = 0;
	outputs->next = NULL;

	inputs->name = av_strdup("out");
	inputs->filter_ctx = buffersink_ctx;
	inputs->pad_idx = 0;
	inputs->next = NULL;
	if (!outputs->name || !inputs->name) {
		ret = AVERROR(ENOMEM);
		goto end;
	}
	if ((ret = avfilter_graph_parse_ptr(filter_graph, "null", &inputs, &outputs, NULL)) < 0)
		goto end;
	if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
		goto end;
	/* Fill FilteringContext */
	filter_ctx->buffersrc_ctx = buffersrc_ctx;
	filter_ctx->buffersink_ctx = buffersink_ctx;
	filter_ctx->filter_graph = filter_graph;
end:
	avfilter_inout_free(&inputs);
	avfilter_inout_free(&outputs);
	return ret;
}
static int encodeFrame(AVCodecContext* enc_ctx, AVFrame* filter_frame)
{
	//img_convert_ctx = sws_getContext(pDeCodecCtx->width, pDeCodecCtx->height, pDeCodecCtx->pix_fmt, 640, 480, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	int ret;
	int gotFrame;
	AVFrame* dstFrame;
	dstFrame = av_frame_alloc();
	dstFrame->linesize[0] = bmp->pitches[0];
	dstFrame->linesize[1] = bmp->pitches[2];
	dstFrame->linesize[2] = bmp->pitches[1];
	dstFrame->data[0] = bmp->pixels[0];
	dstFrame->data[1] = bmp->pixels[2];
	dstFrame->data[2] = bmp->pixels[1];
	//sws_scale(convert_ctx,(const unsigned char* const*)filter_frame,filter_frame->linesize,0,480,dstFrame->data,dstFrame->linesize);
	AVPacket encPkt;
	encPkt.data = NULL;
	encPkt.size = 0;
	av_init_packet(&encPkt);
	av_opt_set(enc_ctx->priv_data, "tune", "zerolatency", 0);
	dstFrame->pts = nextPTS();
	ret = avcodec_encode_video2(enc_ctx, &encPkt, dstFrame, &gotFrame);
	av_frame_free(&dstFrame);
	if (ret < 0)
	{
		return ret;
	}
	if (gotFrame)
	{
		encPkt.stream_index = videoindex;
		encPkt.dts = av_rescale_q_rnd(encPkt.dts, enc_ctx->time_base, vofmtCtx->streams[videoindex]->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		encPkt.pts = av_rescale_q_rnd(encPkt.pts, enc_ctx->time_base, vofmtCtx->streams[videoindex]->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		encPkt.duration = av_rescale_q(encPkt.duration, enc_ctx->time_base, vofmtCtx->streams[videoindex]->time_base);

		if (encPkt.stream_index == videoindex) {
			AVRational time_base = vofmtCtx->streams[videoindex]->time_base;
			AVRational time_base_q = { 1,AV_TIME_BASE };
			int64_t pts_time = av_rescale_q(encPkt.dts, time_base, time_base_q);
			int64_t now_time = av_gettime();
			if (pts_time > now_time)
				av_usleep(pts_time - now_time);
		}

		av_interleaved_write_frame(vofmtCtx, &encPkt);
		vFrameCount++;
	}
	av_free_packet(&encPkt);
	return ret;
}

static int filterOutputFrame(AVFrame* frame, AVCodecContext* enc_ctx)
{

	AVFrame* filterFrame;
	//av_log(NULL, AV_LOG_INFO, "Pushing decoded frame to filters\n");
	int ret = av_buffersrc_add_frame_flags(filter_ctx->buffersrc_ctx, frame, 0);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
		return ret;
	}
	while (1)
	{
		filterFrame = av_frame_alloc();
		if (!filterFrame)

		{
			ret = AVERROR(ENOMEM);
			break;
		}
		ret = av_buffersink_get_frame(filter_ctx->buffersink_ctx, filterFrame);
		if (ret < 0)
		{
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				ret = 0;
			av_frame_free(&filterFrame);
			break;
		}
		ret = encodeFrame(enc_ctx, filterFrame);
		av_frame_free(&filterFrame);

		if (ret < 0)
			break;
	}
	return ret;
}

int sfp_refresh_thread(void* opaque)
{
	thread_exit = 0;
	while (!thread_exit)
	{
		SDL_Event event;
		event.type = SFM_REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(40);
	}
	thread_exit = 0;
	//Break
	SDL_Event event;
	event.type = SFM_BREAK_EVENT;
	SDL_PushEvent(&event);

	return 0;
}


//Show Dshow Device
void show_dshow_device() {
	AVFormatContext* pFormatCtx = avformat_alloc_context();
	AVDictionary* options = NULL;
	av_dict_set(&options, "list_devices", "true", 0);
	AVInputFormat* iformat = av_find_input_format("dshow");
	printf("========Device Info=============\n");
	avformat_open_input(&pFormatCtx, "video=dummy", iformat, &options);
	printf("================================\n");
}


size_t CurlWrite_CallbackFunc_StdString(void* contents, size_t size, size_t nmemb, std::string* s)
{
	size_t newLength = size * nmemb;
	try
	{
		s->append((char*)contents, newLength);
	}
	catch (std::bad_alloc& e)
	{
		//handle memory problem
		return 0;
	}
	return newLength;
}

int main(int argc, char* argv[])
{
	//***************************************************** get lecture id(s) for student ***************************************
	std::cout << "Before starting, please open a cmd prompt(WINDOWS KEY + R) and type in 'ipconfig /all'" << std::endl;
	std::cout << "Find the correct MAC address for the adapter you are using (normally has 'WiFi' in the name" << std::endl;
	std::cout << "Input MAC address here: ";
	std::string MAC;
	std::cin >> MAC;
	std::cout << std::endl;
	
	
	std::string student_id, name;
tryAgain:
	std::cout << "Enter name here: ";
	std::cin >> name;
	std::cout << std::endl;

	std::cout << "Enter student id here: ";
	std::cin >> student_id;
	std::cout << std::endl;

	std::string s;
	DWORD dwFlag;
	
	CURL* curl;
	CURLcode res;
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, "http://3.35.240.138:3333/Identification");

		//curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); //only for https
		//curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); //only for https
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		std::string command = "num=" + student_id + "&name=" + name;
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, command.c_str());

		// Perform the request, res will get the return code
		res = curl_easy_perform(curl);
		// Check for errors
		if (res != CURLE_OK || s == "error")
		{
			//fprintf(stderr, "curl command failed: %s\n",curl_easy_strerror(res));
			std::cout << "Error in curl command" << std::endl;
			goto tryAgain;
		}

		curl_easy_cleanup(curl);
	}
	

	/*
	std::string command = "curl -X POST http://3.35.240.138:3333/Identification -d num=" + student_id + " -d name=" + name;
	system(command.c_str());
	*/

	std::cout << std::endl;	
	std::cout << std::endl;


	//***************************************************** get lecture id from server ********************************************
	
	std::string lec_id;
	lec_id = s;
	s.clear();
	
	//std::cout << lec_id << std::endl;

	/*
	std::cout << "Input lecture id given above: ";
	std::string lec_id;
	std::cin >> lec_id;
	std::cout << std::endl;
	*/

	/*
	std::string command = "curl -X POST http://3.35.240.138:3333/return_endpoint -d num=" + student_id + " -d name=" + name + " -d lec_id=" + lec_id;
	system(command.c_str());
	*/

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, "http://3.35.240.138:3333/return_endpoint");

		//curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); //only for https
		//curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); //only for https
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		std::string command = "num=" + student_id + "&name=" + name + "&lec_id=" + lec_id;
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, command.c_str());

		// Perform the request, res will get the return code
		res = curl_easy_perform(curl);
		// Check for errors
		if (res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
		}
		curl_easy_cleanup(curl);

	}
	//std::cout << s;

	std::string endpoint, table;

	size_t previous = 0, current;
	current = s.find('\n');
	endpoint = s.substr(previous, current);
	table = s.substr(current + 1, s.length());

	//std::cout << endpoint << std::endl;
	//std::cout << table << std::endl;

	std::cout << std::endl;
	std::cout << std::endl;

	//***************************************************************setup for streaming and SDL ******************************************
	int ret;
	AVPacket tranPkt;
	enum AVMediaType type;
	AVCodecContext* pDeCodecCtx, * pEnCodecCtx;
	AVCodec* pDeCodec, * pEnCodec;
	AVFrame* pFrame, * pFrameYUV;
	AVStream* out_stream;
	SDL_Thread* video_tid;
	int screen_w = 0, screen_h = 0;
	int got_frame;
	av_register_all();
	avfilter_register_all();
	avformat_network_init();
	vifmtCtx = avformat_alloc_context();
	avdevice_register_all();


	//*************************************************************** display dshow devices ******************************************
	show_dshow_device();
	
	std::string out_filename = endpoint;
	
	//*************************************************************** input endpoint found above *************************************
	/*
	std::cout << "Enter endpoint (first line shown above starting with rtmp://): ";
	std::string endpoint;
	std::cin >> endpoint;
	out_filename = endpoint.c_str();
	
	std::cout << std::endl;

	std::cout << "Enter table name (second line shown above): ";
	std::cin >> table;
	*/
	//********************************************************* packet alloc **********************************************************
	av_init_packet(&tranPkt);
	tranPkt.data = NULL;
	tranPkt.size = 0;

	avformat_alloc_output_context2(&vofmtCtx, NULL, "flv", out_filename.c_str());

	AVOutputFormat* vofmt = NULL;
	vofmt = vofmtCtx->oformat;

	//---------------------------- open camera (and audio-->unfinished) ---------------------------------------------------------------
	AVInputFormat* vifmt = av_find_input_format("dshow");
	/*
	std::string cam, in;
	std::cout << "Enter name of dshow device without quotation marks for video ((ex) Webcam): ";
	
	while (in != ".") {
		std::cin >> in;
		cam = cam + in + " ";
	}
	cam = cam.erase(cam.length() - 1);
	*/
	std::string name1, name2;
	//std::string name3;
	std::cin >> name1 >> name2;
	std::string cam = name1 + " " + name2;

	std::string fullName = "video=" + cam;
	//std::string fullName = "video=HD Webcam:audio=마이크(Realtek(R) Audio)";
	std::cout << std::endl;

	std::cout << "At any time, to end the streaming service, press 'N' on your keyboard and hold down until terminated." << std::endl;
	std::cout << std::endl;

	// used to set the input options
	AVDictionary* inOptions = NULL;
	// set input resolution
	av_dict_set(&inOptions, "video_size", "640x360", 0);
	av_dict_set(&inOptions, "r", "30", 0);

	if (avformat_open_input(&vifmtCtx, fullName.c_str(), vifmt, &inOptions) != 0){
		printf("Couldn't open video stream.\n");
		goto end;
		//return -1;
	}

	if (avformat_find_stream_info(vifmtCtx, NULL) < 0){
		printf("Couldn't find stream information.\n");
		goto end;
		//return -1;
	}

	for (int i = 0; i < vifmtCtx->nb_streams; i++)
	{
		if (vifmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
			videoindex = i;
			break;
		}

		else if (vifmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			audioindex = i;
		}
	}

	std::cout << audioindex;

	if (videoindex == -1)
	{
		printf("Couldn't find a video stream.\n");
		goto end;
		//return -1;
	}


	// --------------------------find index of video in stream, begin decode  ----------------------------------
	pDeCodecCtx = vifmtCtx->streams[videoindex]->codec;
	pDeCodec = avcodec_find_decoder(pDeCodecCtx->codec_id);
	pEnCodec = avcodec_find_encoder(en_codec_id);
	if (pDeCodec == NULL || pEnCodec == NULL)
	{
		printf("Codec not found.\n");
		goto end;
		//return -1;
	}
	pEnCodecCtx = avcodec_alloc_context3(pEnCodec);
	if (!pEnCodecCtx)
	{
		printf("Could not allocate video codec context\n");
		goto end;
		//return -1;
	}
	if (avcodec_open2(pDeCodecCtx, pDeCodec, NULL) < 0)
	{
		printf("Could not open Decodec.\n");
		goto end;
		//return -1;
	}

	av_log_set_level(AV_LOG_QUIET);

	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	pEnCodecCtx->width = pDeCodecCtx->width;
	pEnCodecCtx->height = pDeCodecCtx->height;
	pEnCodecCtx->bit_rate = 1000000;
	pEnCodecCtx->time_base.num = 1;
	pEnCodecCtx->time_base.den = 30;
	pEnCodecCtx->max_b_frames = 1;
	pEnCodecCtx->gop_size = 25;
	pEnCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	
	if (avcodec_open2(pEnCodecCtx, pEnCodec, NULL) < 0)
	{
		printf("Could not open Encodec.\n");
		goto end;
		//return -1;
	}
	if ((ret = initFilter(pDeCodecCtx, pEnCodecCtx)) < 0)
	{
		goto end;
		//return -1;
	}
	//----------------------
	out_stream = avformat_new_stream(vofmtCtx, pDeCodecCtx->codec);
	out_stream->codec = pEnCodecCtx;
	out_stream->codec->codec_tag = 0;
	if (vofmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
	{
		out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}
	//av_dump_format(vofmtCtx, 0, out_filename, 0);
	av_dump_format(vofmtCtx, 0, out_filename.c_str(), 1);
	if (!(vofmt->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&vofmtCtx->pb, out_filename.c_str(), AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			printf("could not open output URL");
			goto end;
			//return -1;
		}
	}
	avformat_write_header(vofmtCtx, NULL);


	//-0----------------------------------------------------------------------------SDL----------------------------
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		goto end;
		//return -1;
	}
	screen_w = pDeCodecCtx->width;
	screen_h = pDeCodecCtx->height;                                          //get width and height
	SDL_Surface* screen = SDL_SetVideoMode(screen_w, screen_h, 0, 0);
	if (!screen)
	{
		printf("SDL: could not set video mode - exiting:%s\n", SDL_GetError());
		goto end;
		//return -1;
	}
	bmp = SDL_CreateYUVOverlay(screen_w, screen_h, SDL_YV12_OVERLAY, screen);
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = screen_w;
	rect.h = screen_h;

	struct SwsContext* img_convert_ctx;
	//img_convert_ctx = sws_getContext(pDeCodecCtx->width, pDeCodecCtx->height, pDeCodecCtx->pix_fmt, 1280, 720, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	img_convert_ctx = sws_getContext(pDeCodecCtx->width, pDeCodecCtx->height, pDeCodecCtx->pix_fmt, 640, 360, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	video_tid = SDL_CreateThread(sfp_refresh_thread, NULL);
	SDL_Event event;

	//------------------------------------------------------------SDL End---------------------------------------------------
	int cnt = 0;


	while (1)
	{
		cnt++;

		if (cnt % 100 == 0)
			if (!InternetGetConnectedState(&dwFlag, 0)) {
				std::cout << "RESTART CLIENT PLEASE";
				exit(0);
			}

		if (GetAsyncKeyState(0x4E) & 0x8000){
			std::string command = "curl -X POST http://3.35.240.138:3333/streaming_termination -d num=" + student_id + " -d lec_id=" + lec_id + " -d mac=" + MAC + " -d tablename=" + table;
			system(command.c_str());
			break;
		}

		//Wait
		//
		SDL_WaitEvent(&event);
		if (event.type == SFM_REFRESH_EVENT)
		{
			if (av_read_frame(vifmtCtx, &tranPkt) >= 0)
			{
				if (tranPkt.stream_index == videoindex && (filter_ctx->filter_graph))
				{
					tranPkt.dts = av_rescale_q_rnd(tranPkt.dts, vifmtCtx->streams[videoindex]->time_base, pDeCodecCtx->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
					tranPkt.pts = av_rescale_q_rnd(tranPkt.pts, vifmtCtx->streams[videoindex]->time_base, pDeCodecCtx->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
					ret = avcodec_decode_video2(pDeCodecCtx, pFrame, &got_frame, &tranPkt);
					if (ret < 0)
					{
						printf("Decode Error.\n");
						goto end;
						//return -1;
					}
					if (got_frame)
					{
						vFrameCount++;

						ret = filterOutputFrame(pFrame, pEnCodecCtx);

						SDL_LockYUVOverlay(bmp);
						pFrameYUV->pts = av_frame_get_best_effort_timestamp(pFrameYUV);

						pFrameYUV->data[0] = bmp->pixels[0];
						pFrameYUV->data[1] = bmp->pixels[2];
						pFrameYUV->data[2] = bmp->pixels[1];
						pFrameYUV->linesize[0] = bmp->pitches[0];
						pFrameYUV->linesize[1] = bmp->pitches[2];
						pFrameYUV->linesize[2] = bmp->pitches[1];
						sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pDeCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
						SDL_UnlockYUVOverlay(bmp);
						SDL_DisplayYUVOverlay(bmp, &rect);
					}
				}
				av_free_packet(&tranPkt);
			}
			else
			{
				thread_exit = 1;
			}
		}
		else if (event.type == SDL_QUIT)
		{
			thread_exit = 1;
		}
		else if (event.type == SFM_BREAK_EVENT)
		{
			break;
		}
	}
	av_write_trailer(vofmtCtx);
	av_free_packet(&tranPkt);
end:
	sws_freeContext(img_convert_ctx);
	SDL_Quit();
	avcodec_close(pDeCodecCtx);
	avfilter_graph_free(&filter_ctx->filter_graph);
	av_free(filter_ctx);
	av_free(pFrameYUV);
	av_free(pFrame);
	avcodec_close(pEnCodecCtx);
	avformat_close_input(&vifmtCtx);
	avformat_free_context(vofmtCtx);
	return 0;
}


/*
int main(int argc, char* argv[]) {
	av_register_all();
	AVFormatContext* pFormatCtx;

	// Open video file
	if (av_open_input_file(&pFormatCtx, argv[1], NULL, 0, NULL) != 0)
		return -1; // Couldn't open file

	// Retrieve stream information
	if (av_find_stream_info(pFormatCtx) < 0)
		return -1; // Couldn't find stream information

	// Dump information about file onto standard error
	dump_format(pFormatCtx, 0, argv[1], 0);

	int i;
	AVCodecContext* pCodecCtx;

	// Find the first video stream
	videoStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	if (videoStream == -1)
		return -1; // Didn't find a video stream

	  // Get a pointer to the codec context for the video stream
	pCodecCtx = pFormatCtx->streams[videoStream]->codec;

	AVCodec* pCodec;

	// Find the decoder for the video stream
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found
	}
	// Open codec
	if (avcodec_open(pCodecCtx, pCodec) < 0)
		return -1; // Could not open codec

	AVFrame* pFrame;

	// Allocate video frame
	pFrame = avcodec_alloc_frame();

}
*/