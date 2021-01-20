#include <stdio.h>
#include <iostream>
#pragma warning(disable:4996)
#define __STDC_CONSTANT_MACROS

extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"
#include "libavutil/time.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
};

//define functions for finding and listing devices
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

int main(int argc, char* argv[])
{
	//parameters for "reading camera"
	AVFormatContext* pFormatCtx;
	int	i;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;

	//parameters for "streaming"
	AVOutputFormat *ofmt = NULL;
	//Input AVFormatContext and Output AVFormatContext
	AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
	AVPacket pkt;

	const char *in_filename, *out_filename;

	int videoindex = -1;
	int frame_index = 0;
	int64_t start_time = 0;
	
	//Output URL [RTMP]
	out_filename = "rtmp://3.35.240.138:1935/webcam/2019-13902";

	//initialize for FFMPEG
	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	//Register Device
	avdevice_register_all();

	//Show Dshow Device
	show_dshow_device();
	
	//Setup for dshow
	AVInputFormat* ifmt = av_find_input_format("dshow");
	
	//Set own video device's name
	if (avformat_open_input(&pFormatCtx, "video=HD Webcam", ifmt, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}

	//Checking for errors in stream
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0){
		printf("Couldn't find stream information.\n");
		return -1;
	}

	videoindex = -1;

	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}

	if (videoindex == -1){
		printf("Couldn't find a video stream.\n");
		return -1;
	}

	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	
	if (pCodec == NULL){
		printf("Codec not found.\n");
		return -1;
	}

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0){
		printf("Could not open codec.\n");
		return -1;
	}

	//Set up AVFrame to hold decoded data
	AVFrame* pFrame;
	pFrame = av_frame_alloc();
	
	//variables to track frame
	int ret, got_picture;

	//allocate packet
	AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));

	//check size
	printf("w= %d h= %d\n", pCodecCtx->width, pCodecCtx->height);

	avformat_open_input(&ifmt_ctx, "video=HD Webcam", 0, 0);

	ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);




	for(i=0; i<ifmt_ctx->nb_streams; i++) 
		if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
			videoindex=i;
			break;
		}

	std::cout << "1";

	av_dump_format(ifmt_ctx, 0, "video=HD Webcam", 0);

	//Output
	
	avformat_alloc_output_context2(&ofmt_ctx, NULL, "flv", out_filename); //RTMP

	if (!ofmt_ctx) {
		printf( "Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	ofmt = ofmt_ctx->oformat;
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		//Create output AVStream according to input AVStream
		AVStream *in_stream = ifmt_ctx->streams[i];
		AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
		if (!out_stream) {
			printf( "Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}

		//Copy the settings of AVCodecContext
		ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
		if (ret < 0) {
			printf( "Failed to copy context from input to output stream codec context\n");
			goto end;
		}

		out_stream->codec->codec_tag = 0;
		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}
	//Dump Format
	av_dump_format(ofmt_ctx, 0, out_filename, 1);
	//Open output URL
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret - avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			printf( "Could not open output URL '%s'", out_filename);
			goto end;
		}
	}
	//Write file header
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		printf( "Error occurred when opening output URL\n");
		goto end;
	}

	start_time=av_gettime();
	while (1) {
		AVStream *in_stream, *out_stream;
		//Get an AVPacket
		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret < 0)
			break;
		//FIX£ºNo PTS (Example: Raw H.264)
		//Simple Write PTS
		if(pkt.pts==AV_NOPTS_VALUE){
			//Write PTS
			AVRational time_base1=ifmt_ctx->streams[videoindex]->time_base;
			//Duration between 2 frames (us)
			int64_t calc_duration=(double)AV_TIME_BASE/av_q2d(ifmt_ctx->streams[videoindex]->r_frame_rate);
			//Parameters
			pkt.pts=(double)(frame_index*calc_duration)/(double)(av_q2d(time_base1)*AV_TIME_BASE);
			pkt.dts=pkt.pts;
			pkt.duration=(double)calc_duration/(double)(av_q2d(time_base1)*AV_TIME_BASE);
		}
		//Important:Delay
		if(pkt.stream_index==videoindex){
			AVRational time_base=ifmt_ctx->streams[videoindex]->time_base;
			AVRational time_base_q={1,AV_TIME_BASE};
			int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
			int64_t now_time = av_gettime() - start_time;
			if (pts_time > now_time)
				av_usleep(pts_time - now_time);

		}

		in_stream  = ifmt_ctx->streams[pkt.stream_index];
		out_stream = ofmt_ctx->streams[pkt.stream_index];
		/* copy packet */
		//Convert PTS/DTS
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		//Print to Screen
		if(pkt.stream_index==videoindex){
			printf("Send %8d video frames to output URL\n",frame_index);
			frame_index++;
		}
		//ret = av_write_frame(ofmt_ctx, &pkt);
		ret = av_interleaved_write_frame(ofmt_ctx, &pkt);

		if (ret < 0) {
			printf( "Error muxing packet\n");
			break;
		}
		
		av_free_packet(&pkt);
		
	}
	//Write file trailer
	av_write_trailer(ofmt_ctx);
end:
	avformat_close_input(&ifmt_ctx);
	/* close output */
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
	if (ret < 0 && ret != AVERROR_EOF) {
		printf( "Error occurred.\n");
		return -1;
	}














	for (;;) {
		if (av_read_frame(pFormatCtx, packet) >= 0) {
			if (packet->stream_index == videoindex) {
				ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
				if (ret < 0) {
					printf("Decode Error.\n");
					return -1;
				}
				av_free_packet(packet);
			}
		}
	}

	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}