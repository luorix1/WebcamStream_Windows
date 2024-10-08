
// CLS_DlgStreamPusher.cpp : 实现文件
//

#include "stdafx.h"
#include "CLS_StreamPusher.h"
#include "CLS_DlgStreamPusher.h"
#include ".\common\Public_Function.h"
//#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include <curl.h>
#include <fstream>
#include <iostream>

#define PUSH_ADDR_NUM 1

static  Uint8* audio_chunk;
static  Uint32  audio_len;
static  Uint8* audio_pos;
static int64_t audio_callback_time;

std::string name;
std::string student_id;
std::string lec_id;
std::string endpoint;
std::string table;

string strPushAddr[PUSH_ADDR_NUM] = { "endpoint" };
DWORD dwFlag;

CURL* curl;
CURLcode res;

std::string str;

int CurlErrorMessageBox()
{
	int msgboxID = MessageBox(
		NULL,
		"HTTP POST failed",
		"please retry",
		MB_ICONEXCLAMATION | MB_RETRYCANCEL
	);

	return msgboxID;
}

int LecIdMessageBox()
{
	int msgboxID = MessageBox(
		NULL,
		"Failed to retrieve ID of test",
		"please retry",
		MB_ICONEXCLAMATION | MB_YESNO
	);

	return msgboxID;
}

int LoginErrorMessageBox()
{
	int msgboxID = MessageBox(
		NULL,
		"Failed to log in",
		"please retry",
		MB_ICONEXCLAMATION | MB_YESNO
	);

	name = "";
	student_id = "";

	return msgboxID;
}

int TerminationErrorMessageBox()
{
	int msgboxID = MessageBox(
		NULL,
		"Failed to save video to server",
		"please retry",
		MB_ICONEXCLAMATION | MB_YESNO
	);

	return msgboxID;
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

void stream_start() {
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, "http://3.35.240.138:3333/streaming_start");
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		std::string command = "num=" + student_id + "&name=" + name + "&lec_id=" + lec_id + "&mac=1";
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, command.c_str());

		// Perform the request, res will get the return code
		res = curl_easy_perform(curl);
		// Check for errors
		if (res != CURLE_OK)
		{
			//error case
			TRACE("identification failed\n");
			LecIdMessageBox();
		}

		curl_easy_cleanup(curl);
	}
}

void get_tablename() {
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, "http://3.35.240.138:3333/Identification");

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &table);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		std::string command = "num=" + student_id + "&name=" + name + "&mac=1";
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, command.c_str());

		// Perform the request, res will get the return code
		res = curl_easy_perform(curl);
		// Check for errors
		if (res != CURLE_OK)
		{
			//error case
			TRACE("identification failed\n");
			LecIdMessageBox();
		}

		curl_easy_cleanup(curl);
	}
}

void get_endpoint() {
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, "http://3.35.240.138:3333/return_endpoint");

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &endpoint);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		std::string tmp1 = table.substr(0, table.find("_"));
		std::string tmp2 = table.substr(table.find("_") + 1, table.length());
		std::string tmp3 = tmp2.substr(0, tmp2.find("_"));
		std::string tmp4 = tmp2.substr(tmp2.find("_") + 1, tmp2.length());
		std::string tmp5 = tmp4.substr(0, tmp4.find("_"));
		lec_id = tmp1 + "." + tmp3 + "_" + tmp5;

		std::string command = "num=" + student_id + "&name=" + name + "&lec_id=" + lec_id + "&mac=1";
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, command.c_str());

		// Perform the request, res will get the return code
		res = curl_easy_perform(curl);
		// Check for errors
		if (res != CURLE_OK)
		{
			TRACE("getting endpoint failed\n");
			LoginErrorMessageBox();
		}
		curl_easy_cleanup(curl);
	}
}


void stream_termination() {
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, "http://3.35.240.138:3333/streaming_termination");

		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		std::string command = "num=" + student_id + "&name=" + name + "&lec_id=" + lec_id + "&mac=1";
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, command.c_str());

		// Perform the request, res will get the return code
		res = curl_easy_perform(curl);
		// Check for errors
		if (res != CURLE_OK)
		{
			TRACE("getting endpoint failed\n");
			TerminationErrorMessageBox();
		}
		curl_easy_cleanup(curl);
	}
}


static char* dup_wchar_to_utf8(wchar_t* w)
{
	char* s = NULL;
	int l = WideCharToMultiByte(CP_UTF8, 0, w, -1, 0, 0, 0, 0);
	s = (char*)av_malloc(l);
	if (s)
		WideCharToMultiByte(CP_UTF8, 0, w, -1, s, l, 0, 0);
	return s;
}

static inline int compute_mod(int a, int b)
{
	return a < 0 ? a % b + b : a % b;
}

CLS_DlgStreamPusher::CLS_DlgStreamPusher(CWnd* pParent /*=NULL*/)
	: CDialog(CLS_DlgStreamPusher::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_cstrFilePath = "";
	m_cstrPushAddr = "";

	m_bkBrush = NULL;
	m_pStreamInfo = NULL;
	m_pFmtRtmpCtx = NULL;
	m_pFmtVideoCtx = NULL;
	m_pFmtAudioCtx = NULL;
	m_pPushFileThrid = NULL;
	m_pCodecVideoCtx = NULL;
	m_pCodecAudioCtx = NULL;
	m_pPushStreamThrid = NULL;

	m_iFrameRate = -1;
	m_iVideoIndex = -1;
	m_iLstSelIndex = -1;
	m_iWindowWidth = -1;
	m_iWiddowHeight = -1;
	m_iVideoOutIndex = -1;
	m_iAudioOutIndex = -1;

	m_blUrl = FALSE;
	m_blPreview = FALSE;
	m_blVideoShow = TRUE;
	m_blAudioShow = FALSE;
	m_blPushSingle = TRUE;	//默认推送单个文件循环推送
	m_blPushCircle = TRUE;	//默认推送文件都是循环推送
	m_blPushStart = FALSE;
	m_blPushReady = FALSE;
	m_blPushSuccess = FALSE;
	m_blCreateVideoWin = FALSE;

	m_mapDeviceInfo.clear();
	m_mapPushFile.clear();
}

void CLS_DlgStreamPusher::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_USERNAME, m_EditUsername);
	DDX_Control(pDX, IDC_USERID, m_EditUserID);
	DDX_Control(pDX, IDC_CHK_SRC_TYPE, m_chkSrcType);
	DDX_Control(pDX, IDC_BTN_OPEN_LOCAL_FILE, m_btnOpenLocalFile);
	DDX_Control(pDX, IDC_STC_PREVIEW, m_stcPreview);
	DDX_Control(pDX, IDC_COB_DEVICE_VIDEO, m_cboDeviceVideo);
	DDX_Control(pDX, IDC_COB_DEVICE_AUDIO, m_cboDeviceAudio);
	DDX_Control(pDX, IDC_CHK_SHOW_VIDEO, m_chkShowVideo);
	DDX_Control(pDX, IDC_COB_RESOLUTION, m_cboResolution);
	DDX_Control(pDX, IDC_CHK_WRITE_FILE, m_chkWriteFile);
	DDX_Control(pDX, IDC_EDT_FRAMERATE, m_edtFrameRate);
	DDX_Control(pDX, IDC_LIST_LOCAL_FILES, m_lstLocalFiles);
	DDX_Control(pDX, IDC_STC_PUSH_STATUS, m_stcPushStatus);
	DDX_Control(pDX, IDC_BTN_OPEN_LOCAL_FILES, m_btnOpenFiles);
	DDX_Control(pDX, IDC_CBO_PUSH_ADDR, m_cboPushAddr);
	DDX_Control(pDX, IDC_STC_SEL_FILE_MEM, m_stcSelFile);
}

BEGIN_MESSAGE_MAP(CLS_DlgStreamPusher, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_EN_CHANGE(IDC_USERNAME, &CLS_DlgStreamPusher::OnEnChangeUsername)
	ON_EN_CHANGE(IDC_USERID, &CLS_DlgStreamPusher::OnEnChangeUserid)
	ON_BN_CLICKED(IDOK, &CLS_DlgStreamPusher::OnBnClickedOk)
	ON_BN_CLICKED(IDC_CHK_SRC_TYPE, &CLS_DlgStreamPusher::OnBnClickedChkSrcType)
	ON_BN_CLICKED(IDC_BTN_OPEN_LOCAL_FILE, &CLS_DlgStreamPusher::OnBnClickedBtnOpenLocalFile)
	ON_BN_CLICKED(IDC_BTN_PREVIEW, &CLS_DlgStreamPusher::OnBnClickedBtnPreview)
	ON_BN_CLICKED(IDCANCEL, &CLS_DlgStreamPusher::OnBnClickedCancel)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_CHK_SHOW_VIDEO, &CLS_DlgStreamPusher::OnBnClickedChkShowVideo)
	ON_BN_CLICKED(IDC_CHK_WRITE_FILE, &CLS_DlgStreamPusher::OnBnClickedChkWriteFile)
	ON_CBN_SELCHANGE(IDC_COB_DEVICE_VIDEO, &CLS_DlgStreamPusher::OnCbnSelchangeCobDeviceVideo)
	ON_CBN_SELCHANGE(IDC_COB_RESOLUTION, &CLS_DlgStreamPusher::OnCbnSelchangeCobResolution)
	ON_BN_CLICKED(IDC_BTN_REFRESHVIDEO, &CLS_DlgStreamPusher::OnBnClickedBtnRefreshvideo)
	ON_BN_CLICKED(IDC_BTN_OPEN_LOCAL_FILES, &CLS_DlgStreamPusher::OnBnClickedBtnOpenLocalFiles)
	ON_CBN_SELCHANGE(IDC_CBO_PUSH_ADDR, &CLS_DlgStreamPusher::OnCbnSelchangeCboPushAddr)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_LOCAL_FILES, &CLS_DlgStreamPusher::OnNMRClickListLocalFiles)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_LOCAL_FILES, &CLS_DlgStreamPusher::OnNMDblclkListLocalFiles)
	ON_STN_CLICKED(IDC_STC_PREVIEW, &CLS_DlgStreamPusher::OnStnClickedStcPreview)
	ON_STN_CLICKED(IDC_STC_RESOLUTION, &CLS_DlgStreamPusher::OnStnClickedStcResolution)
	ON_EN_CHANGE(IDC_EDT_FRAMERATE, &CLS_DlgStreamPusher::OnEnChangeEdtFramerate)
	ON_BN_CLICKED(IDC_BTN_ENDPOINT, &CLS_DlgStreamPusher::OnBnClickedBtnEndpoint)
	ON_STN_CLICKED(IDC_STC_PUSHER_ADDR, &CLS_DlgStreamPusher::OnStnClickedStcPusherAddr)
	ON_BN_CLICKED(IDC_TERMINATION, &CLS_DlgStreamPusher::OnBnClickedTermination)
END_MESSAGE_MAP()


int video_thr(LPVOID lpParam)
{
	int iRet = -1;
	struct_stream_info* strct_streaminfo = NULL;
	int						iVideoPic = 0;
	int64_t					start_time = 0;
	AVPacket		pkt;

	CLS_DlgStreamPusher* pThis = (CLS_DlgStreamPusher*)lpParam;
	if (pThis == NULL) {
		TRACE("video_thr--pThis == NULL\n");
		return iRet;
	}
	if (NULL == pThis->m_pFmtVideoCtx || NULL == pThis->m_pCodecVideoCtx) {
		TRACE("NULL == pThis->m_pFmtVideoCtx || NULL == pThis->m_pCodecVideoCtx\n");
		return iRet;
	}

	strct_streaminfo = pThis->m_pStreamInfo;
	if (NULL == strct_streaminfo) {
		TRACE("NULL == strct_streaminfo\n");
		return iRet;
	}

	AVFrame* pFrame;
	pFrame = avcodec_alloc_frame();
	AVFrame* picture = avcodec_alloc_frame();

	int size = avpicture_get_size(pThis->m_pCodecVideoCtx->pix_fmt, pThis->m_iDstVideoWidth, pThis->m_iDstVideoHeight);

	//申请picture
	if (avpicture_alloc((AVPicture*)picture, AV_PIX_FMT_YUV420P, pThis->m_iSrcVideoWidth * 2, pThis->m_iSrcVideoHeight * 2) < 0) {
		TRACE("avpicture_alloc < 0");
		return iRet;
	}
	//
	int height = pThis->m_iDstVideoHeight;
	int width = pThis->m_iDstVideoWidth;

	int y_size = height * width;

	//原视频区域
	SDL_Rect sdlSrcRect;
	sdlSrcRect.x = 0;
	sdlSrcRect.y = 0;
	sdlSrcRect.w = pThis->m_iSrcVideoWidth;
	sdlSrcRect.h = pThis->m_iSrcVideoHeight;

	//从摄像头获取数据
	while (1) {
		if (strct_streaminfo->m_iAbortRequest) {
			break;
		}
		pkt.data = NULL;
		pkt.size = 0;
		if (av_read_frame(pThis->m_pFmtVideoCtx, &pkt) >= 0) {
			//解码显示
			if (pkt.stream_index != 0) {
				av_free_packet(&pkt);
				continue;
			}
			iRet = avcodec_decode_video2(pThis->m_pCodecVideoCtx, pFrame, &iVideoPic, &pkt);
			if (iRet < 0) {
				TRACE("Decode Error.\n");
				goto END;
			}
			av_free_packet(&pkt);
			if (iVideoPic <= 0) {
				TRACE("iVideoPic <= 0");
				goto END;
			}

			if (sws_scale(strct_streaminfo->m_pVideoSwsCtx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pThis->m_iSrcVideoHeight,
				picture->data, picture->linesize) < 0) {
				TRACE("sws_scale < 0");
				goto END;
			}

			//显示视频的区域
			SDL_Rect sdlRect;
			sdlRect.x = 0;
			sdlRect.y = 0;
			sdlRect.w = pThis->m_iDstVideoWidth;
			sdlRect.h = pThis->m_iDstVideoHeight;

			if (pThis->m_blVideoShow) {
				//显示视频
				SDL_UpdateYUVTexture(strct_streaminfo->m_pSdlTexture, NULL, picture->data[0], picture->linesize[0], picture->data[1], picture->linesize[1], picture->data[2], picture->linesize[2]);
				SDL_RenderClear(strct_streaminfo->m_pSdlRender);
				SDL_RenderCopy(strct_streaminfo->m_pSdlRender, strct_streaminfo->m_pSdlTexture, &sdlSrcRect, &sdlRect);
				SDL_RenderPresent(strct_streaminfo->m_pSdlRender);
			}

			if (pThis->m_blPushStart) {
				int iVideoFifoSize = av_fifo_space(strct_streaminfo->m_pVideoFifo);
				if (iVideoFifoSize >= size) {
					SDL_mutexP(strct_streaminfo->m_pVideoMutex);
					av_fifo_generic_write(strct_streaminfo->m_pVideoFifo, picture->data[0], y_size, NULL);
					av_fifo_generic_write(strct_streaminfo->m_pVideoFifo, picture->data[1], y_size / 4, NULL);
					av_fifo_generic_write(strct_streaminfo->m_pVideoFifo, picture->data[2], y_size / 4, NULL);
					SDL_mutexV(strct_streaminfo->m_pVideoMutex);
				}
			}
		}
	}

	iRet = 1;
END:
	av_frame_free(&pFrame);
	av_frame_free(&picture);

	return iRet;
}

int audio_thr(LPVOID lpParam)
{
	//采集音频，进行储存
	int iRet = -1;
	AVPacket		pkt;
	AVFrame* pFrame = NULL;

	CLS_DlgStreamPusher* pThis = (CLS_DlgStreamPusher*)lpParam;
	if (pThis == NULL) {
		TRACE("audio_thr--pThis == NULL\n");
		return iRet;
	}
	if (NULL == pThis->m_pFmtAudioCtx) {
		TRACE("NULL == pThis->m_pFmtAudioCtx\n");
		return iRet;
	}
	AVCodecContext* pCodec = pThis->m_pFmtAudioCtx->streams[0]->codec;
	if (NULL == pCodec) {
		TRACE("NULL == pCodec");
		return iRet;
	}

	struct_stream_info* pStrctStreamInfo = pThis->m_pStreamInfo;
	if (NULL == pStrctStreamInfo) {
		TRACE("NULL == pStrctStreamInfo\n");
		return iRet;
	}

	while (1) {
		if (pStrctStreamInfo->m_iAbortRequest) {
			break;
		}
		pkt.data = NULL;
		pkt.size = 0;
		if (av_read_frame(pThis->m_pFmtAudioCtx, &pkt) < 0 || _kbhit() != 0) {
			continue;
		}

		if (!pFrame) {
			if (!(pFrame = avcodec_alloc_frame())) {
				TRACE("!(strct_stream_info->m_pAudioFrame = avcodec_alloc_frame())\n");
				goto END;
			}
		}
		else {
			avcodec_get_frame_defaults(pFrame);
		}

		int gotframe = -1;
		if (avcodec_decode_audio4(pThis->m_pFmtAudioCtx->streams[0]->codec, pFrame, &gotframe, &pkt) < 0) {
			TRACE("can not decoder a frame\n");
			break;
		}
		av_free_packet(&pkt);

		if (!gotframe) {
			//没有获取到数据，继续下一次
			TRACE("avcodec_decode_audio4 gotframe is 0!\n");
			continue;
		}

		SDL_mutexP(pStrctStreamInfo->m_pAudioMutex);
		if (NULL == pStrctStreamInfo->m_pAudioFifo) {
			pStrctStreamInfo->m_pAudioFifo = av_audio_fifo_alloc(pThis->m_pFmtAudioCtx->streams[0]->codec->sample_fmt,
				pThis->m_pFmtAudioCtx->streams[0]->codec->channels, 30 * pFrame->nb_samples);
		}

		int buf_space = av_audio_fifo_space(pStrctStreamInfo->m_pAudioFifo);
		if (buf_space >= pFrame->nb_samples) {
			av_audio_fifo_write(pStrctStreamInfo->m_pAudioFifo, (void**)pFrame->data, pFrame->nb_samples);
		}
		else if (buf_space > 0) {
			av_audio_fifo_write(pStrctStreamInfo->m_pAudioFifo, (void**)pFrame->data, buf_space);
		}
		SDL_mutexV(pStrctStreamInfo->m_pAudioMutex);
	}
	iRet = 1;

END:
	av_frame_free(&pFrame);
	return iRet;
}

int push_thr(LPVOID lpParam)
{
	int iRet = -1;
	int64_t cur_pts_v = 0, cur_pts_a = 0;
	int	frame_video_index = 0;
	int frame_audio_index = 0;

	CLS_DlgStreamPusher* pThis = (CLS_DlgStreamPusher*)lpParam;
	if (NULL == pThis) {
		TRACE("NULL == pThis\n");
		return iRet;
	}

	struct_stream_info* pStrctStreamInfo = pThis->m_pStreamInfo;
	if (NULL == pStrctStreamInfo) {
		TRACE("NULL == pStrctStreamInfo\n");
		return iRet;
	}

	if (NULL == pThis->m_pFmtRtmpCtx) {
		TRACE("NULL == pThis->m_pFmtRtmpCtx\n");
		return iRet;
	}

	AVFrame* picture = av_frame_alloc();
	int size = avpicture_get_size(pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoIndex]->codec->pix_fmt,
		pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoIndex]->codec->width, pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoIndex]->codec->height);
	pStrctStreamInfo->m_pPushPicSize = (uint8_t*)av_malloc(size);

	AVPacket pkt;//视频包
	AVPacket pkt_out;//声音包
	AVFrame* frame;//声音帧
	while (1) {
		if (pStrctStreamInfo->m_iAbortRequest) {
			break;
		}
		if (av_compare_ts(cur_pts_v, pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->time_base,
			cur_pts_a, pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->time_base) <= 0) {
			//视频处理
			if (av_fifo_size(pStrctStreamInfo->m_pVideoFifo) >= size) {
				SDL_mutexP(pStrctStreamInfo->m_pVideoMutex);
				av_fifo_generic_read(pStrctStreamInfo->m_pVideoFifo, pStrctStreamInfo->m_pPushPicSize, size, NULL);
				SDL_mutexV(pStrctStreamInfo->m_pVideoMutex);

				avpicture_fill((AVPicture*)picture, pStrctStreamInfo->m_pPushPicSize,
					pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->codec->pix_fmt,
					pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->codec->width,
					pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->codec->height);

				//pts = n * (（1 / timbase）/ fps);
				picture->pts = frame_video_index * ((pThis->m_pFmtVideoCtx->streams[0]->time_base.den / pThis->m_pFmtVideoCtx->streams[0]->time_base.num) / pThis->m_iFrameRate);

				int got_picture = 0;
				av_init_packet(&pkt);
				pkt.data = NULL;
				pkt.size = 0;
				iRet = avcodec_encode_video2(pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->codec, &pkt, picture, &got_picture);
				if (iRet < 0) {
					//编码错误,不理会此帧
					av_free_packet(&pkt);
					continue;
				}

				if (got_picture == 1) {
					pkt.stream_index = pThis->m_iVideoOutIndex;
					pkt.pts = av_rescale_q(pkt.pts, pThis->m_pFmtVideoCtx->streams[0]->time_base,
						pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->time_base);
					pkt.dts = av_rescale_q(pkt.dts, pThis->m_pFmtVideoCtx->streams[0]->time_base,
						pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->time_base);
					if (pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->codec->coded_frame->key_frame) {
						pkt.flags |= AV_PKT_FLAG_KEY;
					}

					TRACE("------video pts-------is [%d]\n", pkt.pts);
					cur_pts_v = pkt.pts;

					if (av_interleaved_write_frame(pThis->m_pFmtRtmpCtx, &pkt) < 0) {
						TRACE("av_interleaved_write_frame video err!\n");
					}
					pThis->m_blPushSuccess = TRUE;
					av_free_packet(&pkt);
					frame_video_index++;
				}
			}
		}
		else {

			//Audio
			if (NULL == pStrctStreamInfo->m_pAudioFifo) {
				continue;//还未初始化fifo
			}
			if (av_audio_fifo_size(pStrctStreamInfo->m_pAudioFifo) >=
				(pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->frame_size > 0 ? pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->frame_size : 1024))
			{
				frame = av_frame_alloc();
				frame->nb_samples = pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->frame_size > 0 ? pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->frame_size : 1024;
				frame->channel_layout = pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->channel_layout;
				frame->format = pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->sample_fmt;
				frame->sample_rate = pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->sample_rate;
				av_frame_get_buffer(frame, 0);

				SDL_mutexP(pStrctStreamInfo->m_pAudioMutex);
				av_audio_fifo_read(pStrctStreamInfo->m_pAudioFifo, (void**)frame->data,
					(pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->frame_size > 0 ? pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->frame_size : 1024));
				SDL_mutexV(pStrctStreamInfo->m_pAudioMutex);

				av_init_packet(&pkt_out);
				int got_picture = -1;
				pkt_out.data = NULL;
				pkt_out.size = 0;

				frame->pts = frame_audio_index * pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->frame_size;
				if (avcodec_encode_audio2(pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec, &pkt_out, frame, &got_picture) < 0) {
					TRACE("can not decoder a frame");
					av_free_packet(&pkt_out);
					continue;
				}
				if (NULL == pkt_out.data) {
					TRACE("NULL == pkt_out.data");
					av_free_packet(&pkt_out);
					continue;
				}
				if (got_picture == 1) {
					pkt_out.stream_index = pThis->m_iAudioOutIndex;

					pkt_out.pts = av_rescale_q(pkt_out.pts, pThis->m_pFmtAudioCtx->streams[0]->time_base,
						pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->time_base);
					pkt_out.dts = av_rescale_q(pkt_out.dts, pThis->m_pFmtAudioCtx->streams[0]->time_base,
						pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->time_base);

					TRACE("~~~~audio pts~~~~~is [%d]\n", pkt_out.pts);

					cur_pts_a = pkt_out.pts;
					if (av_interleaved_write_frame(pThis->m_pFmtRtmpCtx, &pkt_out) < 0) {
						TRACE("av_interleaved_write_frame audio err!\n");
					}
					pThis->m_blPushSuccess = TRUE;
					av_free_packet(&pkt_out);
					frame_audio_index++;
				}
				av_frame_free(&frame);
			}
		}
	}

	//推送结束
	av_write_trailer(pThis->m_pFmtRtmpCtx);
	av_frame_free(&picture);

	return iRet;
}

int push_file_thr(LPVOID lpParam)
{
	int iRet = -1;
	int iFrameIndex = 0;
	int64_t iStartTime = 0;
	AVPacket pkt;
	STRCT_PUSH_FILE strct_push_file;

	CLS_DlgStreamPusher* pThis = (CLS_DlgStreamPusher*)lpParam;
	if (NULL == pThis) {
		TRACE("NULL == pThis\n");
		goto END;
	}

	struct_stream_info* pStrctStreamInfo = pThis->m_pStreamInfo;
	if (NULL == pStrctStreamInfo) {
		TRACE("NULL == pStrctStreamInfo\n");
		goto END;
	}

	if (NULL == pThis->m_pFmtRtmpCtx) {
		TRACE("NULL == pThis->m_pFmtRtmpCtx\n");
		goto END;
	}

	//当前时间
	int last_video_pts = 0;
	int last_video_dts = 0;
	int last_audio_pts = 0;
	int last_audio_dts = 0;

	bool		blReOpenFile = false;
	bool		blAddPkt = false;
	memcpy(&strct_push_file, &pThis->m_strctPushFile, sizeof(STRCT_PUSH_FILE));
	while (pThis->m_blPushCircle) {
		if (blReOpenFile) {
			blAddPkt = true;
			strct_push_file.m_pFmtFileCtx = pThis->OpenFile(pThis->m_strctPushFile.m_cstrFilePath);
			if (NULL == strct_push_file.m_pFmtFileCtx) {
				TRACE("NULL == pFmtCtx!\n");
				break;
			}
		}

		iFrameIndex = 0;
		iStartTime = av_gettime();
		blReOpenFile = true;	//设置循环播放

		int last_video_pts_need_clear = 0;
		int last_video_dts_need_clear = 0;
		int last_audio_pts_need_clear = 0;
		int last_audio_dts_need_clear = 0;
		while (1) {
			AVStream* pInStream, * pOutStream;

			//退出之后跳出循环
			if (pStrctStreamInfo->m_iAbortRequest) {
				break;
			}

			//从输入文件中读数据
			pkt.data = NULL;
			pkt.size = 0;
			if (av_read_frame(strct_push_file.m_pFmtFileCtx, &pkt) < 0) {
				pThis->m_blPushSuccess = FALSE;
				break;
			}

			if (pkt.pts == AV_NOPTS_VALUE) {
				AVRational strctTimeBase = strct_push_file.m_pFmtFileCtx->streams[pThis->m_iVideoIndex]->time_base;
				int64_t iDuration = (double)AV_TIME_BASE / av_q2d(strct_push_file.m_pFmtFileCtx->streams[pThis->m_iVideoIndex]->r_frame_rate);
				pkt.pts = (double)(iFrameIndex * iDuration) / (double)(av_q2d(strctTimeBase) * AV_TIME_BASE);
				pkt.dts = pkt.pts;
				pkt.duration = (double)iDuration / (double)(av_q2d(strctTimeBase) * AV_TIME_BASE);
			}

			//计算延迟
			if (pkt.stream_index == pThis->m_iVideoIndex) {
				AVRational strctTimeBase = strct_push_file.m_pFmtFileCtx->streams[pThis->m_iVideoIndex]->time_base;
				AVRational strctTimeBaseQ = { 1, AV_TIME_BASE };
				int64_t iPts = av_rescale_q(pkt.dts, strctTimeBase, strctTimeBaseQ);
				int64_t iNowTime = av_gettime() - iStartTime;
				if (iPts > iNowTime) {
					av_usleep(iPts - iNowTime);
				}
			}
			pInStream = strct_push_file.m_pFmtFileCtx->streams[pkt.stream_index];
			pOutStream = pThis->m_pFmtRtmpCtx->streams[pkt.stream_index];

			if (NULL == pInStream || NULL == pOutStream) {
				TRACE("NULL == pInStream || NULL == pOutStream!\n");
				break;
			}

			//时间戳
			pkt.pts = av_rescale_q_rnd(pkt.pts, pInStream->time_base, pOutStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.dts = av_rescale_q_rnd(pkt.dts, pInStream->time_base, pOutStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.duration = av_rescale_q(pkt.duration, pInStream->time_base, pOutStream->time_base);
			pkt.pos = -1;

			if (blAddPkt) {
				if (pkt.stream_index == pThis->m_iVideoIndex) {

					//计算增量
					int pts_duration = pkt.pts - last_video_pts_need_clear;
					int dts_duration = pkt.dts - last_video_dts_need_clear;

					//保存旧值
					last_video_pts_need_clear = pkt.pts;
					last_video_dts_need_clear = pkt.dts;

					//计算新的时间戳
					pkt.pts = pts_duration + last_video_pts;
					pkt.dts = dts_duration + last_video_dts;
				}
				else {

					//计算增量
					int pts_duration = pkt.pts - last_audio_pts_need_clear;
					int dts_duration = pkt.dts - last_audio_dts_need_clear;

					//保存旧值
					last_audio_pts_need_clear = pkt.pts;
					last_audio_dts_need_clear = pkt.dts;

					//计算新的时间戳
					pkt.pts = pts_duration + last_audio_pts;
					pkt.dts = dts_duration + last_audio_dts;
				}
			}

			if (pkt.stream_index == pThis->m_iVideoIndex) {
				last_video_pts = pkt.pts;
				last_video_dts = pkt.dts;
			}
			else {
				last_audio_pts = pkt.pts;
				last_audio_dts = pkt.dts;
			}

			TRACE("-----iFrameIndex is [%d]----\n", iFrameIndex);
			iRet = av_interleaved_write_frame(pThis->m_pFmtRtmpCtx, &pkt);
			if (iRet < 0) {
				av_free_packet(&pkt);
				TRACE("av_interleaved_write_frame err!\n");
				continue;
			}
			pThis->m_blPushSuccess = TRUE;

			av_free_packet(&pkt);
			iFrameIndex++;
		}

		//释放pFmtCtx
		if (NULL != strct_push_file.m_pFmtFileCtx) {
			avformat_close_input(&strct_push_file.m_pFmtFileCtx);
			avformat_free_context(strct_push_file.m_pFmtFileCtx);
			strct_push_file.m_pFmtFileCtx = NULL;
		}

		//退出之后跳出循环
		if (pStrctStreamInfo->m_iAbortRequest) {
			break;
		}
	}

	av_write_trailer(pThis->m_pFmtRtmpCtx);
	iRet = 0;
END:
	if (iRet != 0) {
		pThis->m_blPushSuccess = FALSE;
	}
	return iRet;
}

BOOL CLS_DlgStreamPusher::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	//数据结构初始化
	InitData();

	//界面信息初始化
	InitDlgItem();

	//SDL初始化
	InitSdl();

	OnBnClickedBtnPreview();

	return TRUE;
}


void CLS_DlgStreamPusher::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CLS_DlgStreamPusher::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

static int video_refresh_thread(void* opaque)
{
	struct_stream_info* pstrct_streaminfo = (struct_stream_info*)opaque;
	if (NULL == pstrct_streaminfo) {
		TRACE("NULL == pstrct_streaminfo");
		return -1;
	}
	while (!pstrct_streaminfo->m_iAbortRequest) {
		SDL_Event event;
		event.type = FF_VIDEO_REFRESH_EVENT;
		SDL_PushEvent(&event);
	}
	SDL_Delay(40);
	SDL_Event event;
	event.type = FF_BREAK_EVENT;
	SDL_PushEvent(&event);

	return 0;
}

void CLS_DlgStreamPusher::OnBnClickedChkSrcType()
{
	m_blUrl = TRUE;
	ShowControls(TRUE);
}


void CLS_DlgStreamPusher::OnBnClickedBtnOpenLocalFile()
{
	//打开本地文件
	CString szFilter = _T("All Files (*.*)|*.*|avi Files (*.avi)|*.avi|rmvb Files (*.rmvb)|*.rmvb|3gp Files (*.3gp)|*.3gp|mp3 Files (*.mp3)|*.mp3|mp4 Files (*.mp4)|*.mp4|mpeg Files (*.ts)|*.ts|flv Files (*.flv)|*.flv|mov Files (*.mov)|*.mov||");
	CFileDialog dlg(TRUE, NULL, NULL, OFN_PATHMUSTEXIST | OFN_HIDEREADONLY, szFilter, NULL);
	if (IDOK == dlg.DoModal()) {
		CString cstrFilePath = dlg.GetPathName();

		m_lstLocalFiles.DeleteAllItems();
		ClearFileMem();

		if (InsertFileList(cstrFilePath) < 0) {
			TRACE("InsertFileList err filepath is [%s]", cstrFilePath);
			return;
		}
	}
	else {
		MessageBox(_T("message"), NULL, MB_OK);
		m_cstrFilePath = "";
		return;
	}
}


void CLS_DlgStreamPusher::OnBnClickedBtnPreview()
{
	if (m_blPreview) {
		if (m_pFmtVideoCtx != NULL) {
			avformat_close_input(&m_pFmtVideoCtx);
			m_pFmtVideoCtx = NULL;
		}

		if (m_pFmtAudioCtx != NULL) {
			avformat_close_input(&m_pFmtAudioCtx);
			m_pFmtAudioCtx = NULL;
		}

		if (m_pStreamInfo->m_pVideoThr) {
			SDL_DetachThread(m_pStreamInfo->m_pVideoThr);
			m_pStreamInfo->m_pVideoThr = NULL;
		}

		if (m_pStreamInfo->m_pAudioThr) {
			SDL_DetachThread(m_pStreamInfo->m_pAudioThr);
			m_pStreamInfo->m_pAudioThr = NULL;
		}
	}

	CString cstrFrameRate = "30";

	m_iFrameRate = StrToInt(cstrFrameRate);

	if (CreateVideoWindow() < 0) {
		TRACE("Failed to create video window\n");
		goto END;
	}

	if (OpenCamera() < 0) {
		TRACE("OpenCamera failed!\n");
		goto END;
	}

	if (OpenAduio() < 0) {
		TRACE("OpenAduio failed!\n");
		goto END;
	}

	//开启视频采集线程
	if (m_pStreamInfo->m_pVideoThr == NULL) {
		m_pStreamInfo->m_pVideoThr = SDL_CreateThread(video_thr, NULL, (void*)this);
		if (m_pStreamInfo->m_pVideoThr == NULL) {
			TRACE("message\n");
			goto END;
		}
	}

	if (m_pStreamInfo->m_pAudioThr == NULL) {
		m_pStreamInfo->m_pAudioThr = SDL_CreateThread(audio_thr, NULL, (void*)this);
		if (m_pStreamInfo->m_pAudioThr == NULL) {
			TRACE("message\n");
			goto END;
		}
	}

	m_blPushReady = TRUE;
	m_blPreview = TRUE;
END:
	return;
}


void CLS_DlgStreamPusher::OnBnClickedCancel()
{

	//先释放相关资源
	UnInitInfo();

	//释放directshow
	CoUninitialize();

	CDialog::OnCancel();
}
static int audio_refresh_thread(void* opaque)
{
	struct_stream_info* pstrct_stream = (struct_stream_info*)opaque;
	if (NULL == pstrct_stream) {
		TRACE("NULL == pstrct_stream\n");
		return -1;
	}
	while (!pstrct_stream->m_iAbortRequest) {
		SDL_Event event;
		event.type = FF_AUDIO_REFRESH_EVENT;
		event.user.data1 = opaque;
		if (!pstrct_stream->m_iRefresh) {
			pstrct_stream->m_iRefresh = 1;
			SDL_PushEvent(&event);
		}
		//FIXME ideally we should wait the correct time but SDLs event passing is so slow it would be silly
		av_usleep(pstrct_stream->m_pAudioStream && pstrct_stream->m_iShowMode != SHOW_MODE_VIDEO ? 20 * 1000 : 5000);
	}
	return 0;
}

void CLS_DlgStreamPusher::InitDlgItem()
{
	//推流线路
	m_cboPushAddr.InsertString(0, strPushAddr[0].c_str());
	//m_cboPushAddr.InsertString(1, "qn");
	m_cboPushAddr.SetCurSel(1);
	OnCbnSelchangeCboPushAddr();

	//默认是网络流推送
	m_chkSrcType.SetCheck(1);
	m_blUrl = TRUE;
	m_chkWriteFile.SetCheck(TRUE);
	m_chkShowVideo.SetCheck(TRUE);
	ShowControls(TRUE);

	//初始化list
	m_lstLocalFiles.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	m_lstLocalFiles.InsertColumn(0, "column", LVCFMT_CENTER, 1000);

	//初始化背景刷
	if (NULL == m_bkBrush) {
		m_bkBrush = CreateSolidBrush(COLOR_BLACK);
	}

	//初始化右键菜单
	m_Rmenu.CreatePopupMenu();
	m_Rmenu.InsertMenu(0, MF_BYPOSITION, IDM_PBK_MNU_DELETE, "1");
	m_Rmenu.InsertMenu(1, MF_BYPOSITION, IDM_PBK_MNU_CLEAR, "2");
	m_Rmenu.InsertMenu(2, MF_BYPOSITION, IDM_PBK_MNU_SINGLE, "3");
	m_Rmenu.InsertMenu(3, MF_BYPOSITION, IDM_PBK_MNU_CIRCLE, "4");

	//初始化网络库
	av_register_all();
	avformat_network_init();
	avdevice_register_all();

	//初始化directshow库
	if (FAILED(CoInitialize(NULL))) {
		TRACE("CoInitialize Failed!\r\n");
		return;
	}

	//获取设备信息
	GetDevice();

	m_edtFrameRate.SetWindowText("30");

	return;
}

void CLS_DlgStreamPusher::InitSdl()
{
	//SDL初始化
	int flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
	int  sdlinit = SDL_Init(flags);
	if (sdlinit)
	{
		char* sss = (char*)SDL_GetError();
		TRACE("(Did you set the DISPLAY variable?)\n");
		TRACE("Could not initialize SDL - %s\n", SDL_GetError());
		return;
	}

	//设置SDL事件状态
	SDL_EventState(SDL_WINDOWEVENT, SDL_IGNORE);
	SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

	//初始化纹理
	m_blAudioShow = TRUE;
	m_blVideoShow = TRUE;

	//设置sdl显示模式
	m_pStreamInfo->m_iShowMode = SHOW_MODE_VIDEO;
}

void CLS_DlgStreamPusher::UnInitInfo()
{
	//释放相关资源
	m_blPushStart = FALSE;
	m_blPushReady = FALSE;
	m_blPreview = FALSE;
	m_blPushSuccess = FALSE;

	if (NULL != m_pStreamInfo) {
		m_pStreamInfo->m_iAbortRequest = 1;

		if (NULL != m_pPushStreamThrid) {
			SDL_WaitThread(m_pPushStreamThrid, NULL);
			m_pPushStreamThrid = NULL;
		}

		if (NULL != m_pPushFileThrid) {
			SDL_WaitThread(m_pPushFileThrid, NULL);
			m_pPushFileThrid = NULL;
		}

		if (m_pStreamInfo->m_pVideoThr) {
			SDL_WaitThread(m_pStreamInfo->m_pVideoThr, NULL);
			m_pStreamInfo->m_pVideoThr = NULL;
		}
		if (m_pStreamInfo->m_pAudioThr) {
			SDL_WaitThread(m_pStreamInfo->m_pAudioThr, NULL);
			m_pStreamInfo->m_pAudioThr = NULL;
		}
		if (m_pStreamInfo->m_pVideoRefreshThr) {
			SDL_WaitThread(m_pStreamInfo->m_pVideoRefreshThr, NULL);
			m_pStreamInfo->m_pVideoRefreshThr = NULL;
		}
		if (m_pStreamInfo->m_pAudioRefreshThr) {
			SDL_WaitThread(m_pStreamInfo->m_pAudioRefreshThr, NULL);
			m_pStreamInfo->m_pAudioRefreshThr = NULL;
		}

		if (m_pStreamInfo->m_pVideoSwsCtx) {
			sws_freeContext(m_pStreamInfo->m_pVideoSwsCtx);
			m_pStreamInfo->m_pVideoSwsCtx = NULL;
		}

		if (m_pStreamInfo->m_pAudioFifo) {
			av_audio_fifo_free(m_pStreamInfo->m_pAudioFifo);
			m_pStreamInfo->m_pAudioFifo = NULL;
		}
		if (m_pStreamInfo->m_pVideoFifo) {
			av_fifo_free(m_pStreamInfo->m_pVideoFifo);
			m_pStreamInfo->m_pVideoFifo = NULL;
		}

		if (m_pStreamInfo->m_pAudioMutex) {
			SDL_DestroyMutex(m_pStreamInfo->m_pAudioMutex);
			m_pStreamInfo->m_pAudioMutex = NULL;
		}
		if (m_pStreamInfo->m_pVideoMutex) {
			SDL_DestroyMutex(m_pStreamInfo->m_pVideoMutex);
			m_pStreamInfo->m_pVideoMutex = NULL;
		}

		if (m_pStreamInfo->m_pPushPicSize) {
			av_freep(&m_pStreamInfo->m_pPushPicSize);
		}

		if (m_pStreamInfo->m_pVideoStream) {
			if (m_pStreamInfo->m_pVideoStream->codec) {
				avcodec_close(m_pStreamInfo->m_pVideoStream->codec);
				m_pStreamInfo->m_pVideoStream->codec = NULL;
			}
		}

		if (m_pStreamInfo->m_pAudioStream) {
			if (m_pStreamInfo->m_pAudioStream->codec) {
				avcodec_close(m_pStreamInfo->m_pAudioStream->codec);
				m_pStreamInfo->m_pAudioStream->codec = NULL;
			}
		}
		m_pStreamInfo->m_iAbortRequest = 0;
	}

	if (NULL != m_pCodecVideoCtx) {
		avcodec_close(m_pCodecVideoCtx);
		m_pCodecVideoCtx = NULL;
	}

	if (NULL != m_pCodecAudioCtx) {
		avcodec_close(m_pCodecAudioCtx);
		m_pCodecAudioCtx = NULL;
	}

	if (NULL != m_pFmtVideoCtx) {
		avformat_close_input(&m_pFmtVideoCtx);
		avformat_free_context(m_pFmtVideoCtx);
		m_pFmtVideoCtx = NULL;
	}

	if (NULL != m_pFmtAudioCtx) {
		avformat_close_input(&m_pFmtAudioCtx);
		avformat_free_context(m_pFmtAudioCtx);
		m_pFmtAudioCtx = NULL;
	}

	ClearFileMem();
	/*
	if (NULL != m_pFmtRtmpCtx){
		avio_close(m_pFmtRtmpCtx->pb);
		avformat_close_input(&m_pFmtAudioCtx);
		avformat_free_context(m_pFmtRtmpCtx);
		m_pFmtRtmpCtx = NULL;
	}
	*/
	return;
}

HBRUSH CLS_DlgStreamPusher::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	switch (nCtlColor) {
	case CTLCOLOR_STATIC:
		if (pWnd->GetDlgCtrlID() == IDC_STC_PREVIEW) {
			pDC->SetBkMode(TRANSPARENT);
			pDC->SetBkColor(COLOR_BLACK);
			return m_bkBrush;
		}
	}
	return   hbr;
}

int CLS_DlgStreamPusher::GetDeviceInfo(int _iDeviceType)
{
	int iRet = -1;
	ICreateDevEnum* pDevEnum;
	IEnumMoniker* pEnumMon;
	IMoniker* pMoniker;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum, (LPVOID*)&pDevEnum);
	if (SUCCEEDED(hr))
	{
		if (_iDeviceType == n_Video) {
			hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumMon, 0);
		}
		else {
			hr = pDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &pEnumMon, 0);
		}
		if (hr == S_FALSE)
		{
			TRACE("没有找到合适的音视频设备！\n");
			hr = VFW_E_NOT_FOUND;
			return hr;
		}
		pEnumMon->Reset();
		ULONG cFetched;
		while (hr = pEnumMon->Next(1, &pMoniker, &cFetched), hr == S_OK)
		{
			IPropertyBag* pProBag;
			hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (LPVOID*)&pProBag);
			if (SUCCEEDED(hr))
			{
				VARIANT varTemp;
				varTemp.vt = VT_BSTR;
				hr = pProBag->Read(L"FriendlyName", &varTemp, NULL);
				if (SUCCEEDED(hr))
				{
					iRet = 0;
					_bstr_t bstr_t(varTemp.bstrVal);
					std::string strDeviceName = bstr_t;

					//将取到的设备名称存入容器中
					std::vector<std::string> vecDeviceName;
					vecDeviceName.clear();
					vecDeviceName.push_back(strDeviceName);
					SysFreeString(varTemp.bstrVal);

					//如果容器相应的键值有值就量进行相应的添加处理，没值的话整体进行添加处理
					std::map<int, std::vector<std::string>>::iterator iterInfo = m_mapDeviceInfo.find(_iDeviceType);
					if (iterInfo == m_mapDeviceInfo.end()) {
						m_mapDeviceInfo.insert(map<int, std::vector<std::string>>::value_type(_iDeviceType, vecDeviceName));
					}
					else {
						m_mapDeviceInfo[_iDeviceType].push_back(strDeviceName);
					}
				}
				pProBag->Release();
			}
			pMoniker->Release();
		}
		pEnumMon->Release();
	}

	return iRet;
}

void UpdateSampleDisplay(struct_stream_info* _pstrct_streaminfo, short* samples, int samples_size)//CLS_DlgStreamPusher::
{
	int size, len;

	size = samples_size / sizeof(short);
	while (size > 0) {
		len = SAMPLE_ARRAY_SIZE - _pstrct_streaminfo->m_iSampleArrayIndex;
		if (len > size)
			len = size;
		//memcpy(_pstrct_streaminfo->m_iSampleArray + _pstrct_streaminfo->m_iSampleArrayIndex, samples, len * sizeof(short));
		samples += len;
		_pstrct_streaminfo->m_iSampleArrayIndex += len;
		if (_pstrct_streaminfo->m_iSampleArrayIndex >= SAMPLE_ARRAY_SIZE)
			_pstrct_streaminfo->m_iSampleArrayIndex = 0;
		size -= len;
	}
}

void  fill_audio(void* udata, Uint8* stream, int len)//CLS_DlgStreamPusher::
{
	struct_stream_info* struct_stream = (struct_stream_info*)udata;
	if (struct_stream == NULL) {
		TRACE("struct_stream == NULL \n");
		return;
	}
	audio_callback_time = av_gettime();

	int frame_size = av_samples_get_buffer_size(NULL, struct_stream->m_AudioPrm.channels, 1, struct_stream->m_AudioPrm.fmt, 1);
	int len1 = 0;

	if (audio_len == 0)	/*  Only  play  if  we  have  data  left  */
		return;
	len = (len > audio_len ? audio_len : len);	/*  Mix as  much  data  as  possible  */

	while (len > 0 && audio_len > 0) {
		if (struct_stream->m_iAudioBufIndex >= struct_stream->m_iAudioBufSize) {
			if (struct_stream->m_iAduioPktSize < 0) {
				struct_stream->m_pAudioBuf = struct_stream->m_uSilenceBuf;
				struct_stream->m_iAudioBufSize = sizeof(struct_stream->m_uSilenceBuf) / frame_size * frame_size;
			}
			else {
				if (struct_stream->m_iShowMode == SHOW_MODE_WAVES) {
					UpdateSampleDisplay(struct_stream, (int16_t*)struct_stream->m_pAudioBuf, struct_stream->m_iAduioPktSize);
				}
				struct_stream->m_iAudioBufSize = struct_stream->m_iAduioPktSize;
			}
		}

		len1 = struct_stream->m_iAudioBufSize - struct_stream->m_iAudioBufIndex;
		if (len1 > len)
			len1 = len;
		//memcpy(stream, (uint8_t *)struct_stream->m_pAudioBuf + struct_stream->m_iAudioBufIndex, len1);
		audio_len -= len;
		len -= len1;
		stream += len1;
		struct_stream->m_iAudioBufIndex += len1;
	}
	audio_len = 0;

	struct_stream->m_iAudioWriteBufSize = struct_stream->m_iAudioBufSize - struct_stream->m_iAudioBufIndex;
}

void CLS_DlgStreamPusher::InitData()
{
	m_pStreamInfo = (struct_stream_info*)calloc(1, sizeof(struct_stream_info));
	if (NULL == m_pStreamInfo)
	{
		TRACE("m_streamstate is NULL!\n");
		return;
	}

	//初始化流媒体
	m_pStreamInfo->m_pFormatCtx = avformat_alloc_context();
	if (NULL == m_pStreamInfo->m_pFormatCtx) {
		TRACE("NULL == m_pStreamInfo->m_pFormatCtx\n");
		return;
	}

	m_pStreamInfo->m_pAudioThr = NULL;
	m_pStreamInfo->m_pVideoThr = NULL;
	m_pStreamInfo->m_pShowScreen = NULL;
	m_pStreamInfo->m_pScreenSurface = NULL;
	m_pStreamInfo->m_xLeft = 0;
	m_pStreamInfo->m_yTop = 0;
	m_pStreamInfo->m_width = 0;
	m_pStreamInfo->m_height = 0;
	m_pStreamInfo->m_pAudioStream = NULL;
	m_pStreamInfo->m_pAudioFifo = NULL;
	m_pStreamInfo->m_pVideoFifo = NULL;
	m_pStreamInfo->m_pAudioMutex = SDL_CreateMutex();
	m_pStreamInfo->m_pVideoMutex = SDL_CreateMutex();
	m_pStreamInfo->m_pVideoStream = NULL;
	m_pStreamInfo->m_pAudioFrame = NULL;
	m_pStreamInfo->m_pAudioBuf = NULL;
	m_pStreamInfo->m_iAudioBufSize = 0;
	m_pStreamInfo->m_iAudioBufIndex = 0;
	m_pStreamInfo->m_iAduioPktSize = 0;
	m_pStreamInfo->m_iSampleArrayIndex = 0;
	m_pStreamInfo->m_iAbortRequest = 0;
	m_pStreamInfo->m_iRefresh = 0;
	m_pStreamInfo->m_iShowMode = SHOW_MODE_NONE;
	m_pStreamInfo->m_pAudioRefreshThr = NULL;
	m_pStreamInfo->m_pVideoRefreshThr = NULL;
}

char* CLS_DlgStreamPusher::GetDeviceName(int _iDeviceType)
{
	char* pDeviceName = NULL;
	CString cstrDevName = "";
	CString cstrOutDevName = "";
	if (_iDeviceType == n_Audio) {
		m_cboDeviceAudio.GetWindowText(cstrDevName);
	}
	else if (_iDeviceType == n_Video) {
		m_cboDeviceVideo.GetWindowText(cstrDevName);
	}

	AnsiToUTF8(cstrDevName.GetBuffer(), cstrOutDevName);
	if (_iDeviceType == n_Audio) {
		cstrOutDevName = "audio=" + cstrDevName;
	}
	else if (_iDeviceType == n_Video) {
		cstrOutDevName = "video=" + cstrDevName;
	}

	wchar_t* wstring = string_to_wstring(cstrOutDevName.GetBuffer());
	pDeviceName = dup_wchar_to_utf8(wstring);

	return pDeviceName;
}

struct_stream_info* CLS_DlgStreamPusher::GetStreamStrcInfo()
{
	return m_pStreamInfo;
}

void CLS_DlgStreamPusher::EventLoop(struct_stream_info* _pstrct_streaminfo)
{
	if (NULL == _pstrct_streaminfo) {
		TRACE("NULL == _pstrct_streaminfo");
		return;
	}

	SDL_Event event;
	double incr, pos, frac;
	for (;;) {

		break;
		double x;
		//判断退出
		if (_pstrct_streaminfo->m_iAbortRequest) {
			break;
		}

		SDL_WaitEvent(&event);
		switch (event.type) {
		case FF_AUDIO_REFRESH_EVENT:
			RefreshScreen(event.user.data1);
			_pstrct_streaminfo->m_iRefresh = 0;
			break;
		case FF_VIDEO_REFRESH_EVENT:
			break;
		case FF_BREAK_EVENT:
			break;
		case FF_QUIT_EVENT:
			StopStream(event.user.data1);
			break;
		default:
			break;
		}
	}
}

void CLS_DlgStreamPusher::RefreshScreen(void* opaque)
{
	struct_stream_info* strct_streaminfo = (struct_stream_info*)opaque;
	if (NULL == strct_streaminfo) {
		TRACE("NULL == strct_streaminfo");
		return;
	}

	DisplayScreen(strct_streaminfo);
}

void CLS_DlgStreamPusher::DisplayScreen(struct_stream_info* _pstrct_streaminfo)
{
	if (_pstrct_streaminfo->m_pAudioFrame && _pstrct_streaminfo->m_iShowMode != SHOW_MODE_VIDEO) {
		DisplayAudio(_pstrct_streaminfo);
	}
	else {
		DisplayVideo(_pstrct_streaminfo);
	}
}

void CLS_DlgStreamPusher::DisplayAudio(struct_stream_info* _pstrct_streaminfo)
{
	if (audio_callback_time == 0) {
		TRACE("audio_callback_time == 0");
		return;
	}
	int i, i_start, x, y1, y, ys, delay, n, nb_display_channels;
	int ch, channels, h, h2, bgcolor, fgcolor;
	int16_t time_diff;
	int rdft_bits, nb_freq;

	for (rdft_bits = 1; (1 << rdft_bits) < 2 * _pstrct_streaminfo->m_height; rdft_bits++)
		;
	nb_freq = 1 << (rdft_bits - 1);

	/* compute display index : center on currently output samples */
	channels = _pstrct_streaminfo->m_AudioPrm.channels;
	nb_display_channels = channels;
	if (!_pstrct_streaminfo->m_iPaused) {
		int data_used = _pstrct_streaminfo->m_iShowMode == SHOW_MODE_WAVES ? _pstrct_streaminfo->m_width : (2 * nb_freq);
		n = 2 * channels;
		delay = _pstrct_streaminfo->m_iAudioWriteBufSize;
		delay /= n;

		/* to be more precise, we take into account the time spent since
		the last buffer computation */
		if (audio_callback_time) {
			time_diff = av_gettime() - audio_callback_time;
			delay -= (time_diff * _pstrct_streaminfo->m_AudioPrm.freq) / 1000000;
		}

		delay += 2 * data_used;
		if (delay < data_used)
			delay = data_used;

		i_start = x = compute_mod(_pstrct_streaminfo->m_iSampleArrayIndex - delay * channels, SAMPLE_ARRAY_SIZE);
		if (_pstrct_streaminfo->m_iShowMode == SHOW_MODE_WAVES) {
			h = INT_MIN;
			for (i = 0; i < 1000; i += channels) {
				int idx = (SAMPLE_ARRAY_SIZE + x - i) % SAMPLE_ARRAY_SIZE;
				int a = _pstrct_streaminfo->m_iSampleArray[idx];
				int b = _pstrct_streaminfo->m_iSampleArray[(idx + 4 * channels) % SAMPLE_ARRAY_SIZE];
				int c = _pstrct_streaminfo->m_iSampleArray[(idx + 5 * channels) % SAMPLE_ARRAY_SIZE];
				int d = _pstrct_streaminfo->m_iSampleArray[(idx + 9 * channels) % SAMPLE_ARRAY_SIZE];
				int score = a - d;
				if (h < score && (b ^ c) < 0) {
					h = score;
					i_start = idx;
				}
			}
		}

		_pstrct_streaminfo->m_iAudioLastStart = i_start;
	}


	//背景色
	bgcolor = SDL_MapRGB(_pstrct_streaminfo->m_pScreenSurface->format, 0x00, 0x00, 0x00);
	if (_pstrct_streaminfo->m_iShowMode == SHOW_MODE_WAVES) {
		SDL_Rect sdl_rect;
		_pstrct_streaminfo->m_xLeft = 0;
		_pstrct_streaminfo->m_yTop = 0;
		sdl_rect.x = _pstrct_streaminfo->m_xLeft;
		sdl_rect.y = _pstrct_streaminfo->m_yTop;
		sdl_rect.w = _pstrct_streaminfo->m_width;
		sdl_rect.h = _pstrct_streaminfo->m_height;

		FillRec(_pstrct_streaminfo->m_pScreenSurface, _pstrct_streaminfo->m_xLeft, _pstrct_streaminfo->m_yTop, _pstrct_streaminfo->m_width, _pstrct_streaminfo->m_height, bgcolor);

		fgcolor = SDL_MapRGB(_pstrct_streaminfo->m_pScreenSurface->format, 0xff, 0xff, 0xff);

		/* total height for one channel */
		h = _pstrct_streaminfo->m_height / nb_display_channels;
		/* graph height / 2 */
		h2 = (h * 9) / 20;
		for (ch = 0; ch < nb_display_channels; ch++) {
			i = i_start + ch;
			y1 = _pstrct_streaminfo->m_yTop + ch * h + (h / 2); /* position of center line */
			for (x = 0; x < _pstrct_streaminfo->m_width; x++) {
				y = (_pstrct_streaminfo->m_iSampleArray[i] * h2) >> 15;
				if (y < 0) {
					y = -y;
					ys = y1 - y;
				}
				else {
					ys = y1;
				}

				FillRec(_pstrct_streaminfo->m_pScreenSurface, _pstrct_streaminfo->m_xLeft + x, ys, 1, y, fgcolor);

				i += channels;
				if (i >= SAMPLE_ARRAY_SIZE)
					i -= SAMPLE_ARRAY_SIZE;
			}
		}

		fgcolor = SDL_MapRGB(_pstrct_streaminfo->m_pScreenSurface->format, 0x00, 0x00, 0xff);

		for (ch = 1; ch < nb_display_channels; ch++) {
			y = _pstrct_streaminfo->m_yTop + ch * h;

			FillRec(_pstrct_streaminfo->m_pScreenSurface, _pstrct_streaminfo->m_xLeft, y, _pstrct_streaminfo->m_width, 1, fgcolor);
		}
		if (SDL_UpdateWindowSurface(_pstrct_streaminfo->m_pShowScreen) != 0) {
			TRACE("SDL_UpdateWindowSurface ERR");
		}
		//SDL_CreateRGBSurface();
		SDL_UpdateTexture(_pstrct_streaminfo->m_pSdlTexture, NULL, _pstrct_streaminfo->m_pScreenSurface->pixels, _pstrct_streaminfo->m_pScreenSurface->pitch);
		SDL_RenderClear(_pstrct_streaminfo->m_pSdlRender);
		SDL_RenderCopy(_pstrct_streaminfo->m_pSdlRender, _pstrct_streaminfo->m_pSdlTexture, &sdl_rect, &sdl_rect);
		SDL_RenderPresent(_pstrct_streaminfo->m_pSdlRender);
	}
}

void CLS_DlgStreamPusher::DisplayVideo(struct_stream_info* _pstrct_streaminfo)
{

}

void CLS_DlgStreamPusher::FillRec(SDL_Surface* screen,
	int x, int y, int w, int h, int color)
{
	if (NULL == screen) {
		TRACE("NULL == screen");
		return;
	}

	//区域填充
	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	SDL_FillRect(screen, &rect, color);
}

void CLS_DlgStreamPusher::StopTest()
{
	SDL_Event event;
	event.type = FF_QUIT_EVENT;
	event.user.data1 = m_pStreamInfo;
	{
		SDL_PushEvent(&event);
	}
}

void CLS_DlgStreamPusher::StopStream(void* opaque)
{
	struct_stream_info* pstrct_streaminfo = (struct_stream_info*)opaque;
	if (NULL == pstrct_streaminfo) {
		TRACE("NULL == pstrct_streaminfo");
		return;
	}

	//做停止流推送的操作
	SDL_WaitThread(pstrct_streaminfo->m_pAudioThr, NULL);
	SDL_WaitThread(pstrct_streaminfo->m_pAudioRefreshThr, NULL);
	SDL_WaitThread(pstrct_streaminfo->m_pVideoThr, NULL);
	SDL_WaitThread(pstrct_streaminfo->m_pVideoRefreshThr, NULL);

	pstrct_streaminfo->m_pAudioThr = NULL;
	pstrct_streaminfo->m_pVideoThr = NULL;
	pstrct_streaminfo->m_pAudioRefreshThr = NULL;
	pstrct_streaminfo->m_pVideoRefreshThr = NULL;
}

void CLS_DlgStreamPusher::OnBnClickedChkShowVideo()
{
	m_blVideoShow = TRUE;
}

void CLS_DlgStreamPusher::GetResolution(int _iVideoIndex)
{
	string strResolution = "";
	IBaseFilter* pVideoCapFilter;       // 视频捕获滤波器
	IGraphBuilder* pGraph;
	ICaptureGraphBuilder2* pDevGraphBuilder;
	HRESULT hr = NULL;
	int iSize = 0;
	int iCount = 0;

	hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL,
		CLSCTX_INPROC, IID_ICaptureGraphBuilder2, (void**)&pDevGraphBuilder);
	if (S_OK != hr) {
		TRACE("S_OK != hr\n");
		return;
	}

	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void**)&pGraph);
	if (S_OK != hr) {
		TRACE("S_OK != hr\n");
		return;
	}

	pDevGraphBuilder->SetFiltergraph(pGraph);

	if (!BindFilter(_iVideoIndex, &pVideoCapFilter, n_Video)) {
		TRACE("!BindFilter(0, &pVideoCapFilter, n_Video)\n");
		return;
	}

	CComPtr<IAMStreamConfig> pCfg = 0;
	hr = pDevGraphBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVideoCapFilter, IID_IAMStreamConfig, (void**)&pCfg);
	if (S_OK != hr) {
		TRACE("S_OK != hr\n");
		return;
	}

	hr = pCfg->GetNumberOfCapabilities(&iCount, &iSize);
	if (S_OK != hr) {
		TRACE("S_OK != hr\n");
		return;
	}

	if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS)) {
		for (int iFormat = 0; iFormat < iCount; iFormat++) {
			VIDEO_STREAM_CONFIG_CAPS pVideoStreamConfig;
			VIDEOINFOHEADER* pVideoHeadInfo = NULL;
			BITMAPINFOHEADER* pBitMapHeadInfo = NULL;
			AM_MEDIA_TYPE* pMediaConfig = NULL;
			hr = pCfg->GetStreamCaps(iFormat, &pMediaConfig, (BYTE*)&pVideoHeadInfo);
			if (S_OK != hr) {
				TRACE("S_OK != hr\n");
				return;
			}
			pVideoHeadInfo = (VIDEOINFOHEADER*)pMediaConfig->pbFormat;
			pBitMapHeadInfo = &pVideoHeadInfo->bmiHeader;
			int iWidth = pBitMapHeadInfo->biWidth;
			int iHeight = pBitMapHeadInfo->biHeight;
			if (iWidth == 0 || iHeight == 0) {
				continue;
			}

			//组成字符串添加到下拉框中
			strResolution = IntToStr(iWidth);
			strResolution += " * ";
			strResolution += IntToStr(iHeight);
			m_cboResolution.InsertString(m_cboResolution.GetCount(), strResolution.c_str());

			map<int, int> mapResolution;
			mapResolution.insert(map<int, int>::value_type(iWidth, iHeight));
			m_mapResolution[m_mapResolution.size()] = mapResolution;
		}
	}
}

bool CLS_DlgStreamPusher::BindFilter(int iDeviceID, IBaseFilter** pOutFilter, DeviceType deviceType)
{
	if (iDeviceID < 0) return false;
	// 枚举所有的视频设备  
	ICreateDevEnum* pCreateDevEnum;
	//生成设备枚举器pCreateDevEnum  
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum,
		NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum);
	if (hr != NOERROR) return false;
	IEnumMoniker* pEM;
	// 创建视频输入设备类枚举器  
	if (deviceType == n_Video)
		hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEM, 0);
	// 音频设备枚举器  
	else
		hr = pCreateDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &pEM, 0);
	if (hr != NOERROR) return false;
	pEM->Reset();  // 复位该设备  
	ULONG cFetched;
	IMoniker* pM;
	int indexDev = 0;
	// 获取设备  
	while (hr = pEM->Next(1, &pM, &cFetched), hr == S_OK, indexDev <= iDeviceID)
	{
		IPropertyBag* pBag;
		// 获取该设备属性集  
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pBag);
		if (SUCCEEDED(hr))
		{
			VARIANT var;
			var.vt = VT_BSTR;
			hr = pBag->Read(L"FriendlyName", &var, NULL);
			if (hr == NOERROR)
			{
				// 采集设备与捕获滤波器捆绑  
				if (indexDev == iDeviceID) pM->BindToObject(0, 0, IID_IBaseFilter, (void**)pOutFilter);
				SysFreeString(var.bstrVal);
			}
			pBag->Release();
		}
		pM->Release();
		indexDev++;
	}
	return true;
}

void CLS_DlgStreamPusher::OnBnClickedChkWriteFile()
{
}

void CLS_DlgStreamPusher::OnBnClickedOk()
{
	stream_start();
	if (m_blPushStart) {
		GetDlgItem(IDOK)->EnableWindow(FALSE);
	}

	if (!m_blPushReady && m_blUrl) {
		MessageBox(_T("请先预览设置！\n"));
		return;
	}

	if (OpenRtmpAddr() < 0) {
		MessageBox("推送失败！\n");
		return;
	}
	m_blPushStart = TRUE;

	GetDlgItem(IDC_STC_PREVIEW)->ShowWindow(FALSE);

	if (m_blUrl) {
		//开启采集推送线程
		if (m_pPushStreamThrid == NULL) {
			m_pPushStreamThrid = SDL_CreateThread(push_thr, NULL, (void*)this);
			if (NULL == m_pPushStreamThrid) {
				MessageBox("创建推送线程失败！\n");
				return;
			}
		}
	}
}

int CLS_DlgStreamPusher::OpenCamera()
{
	int iRet = -1;
	AVInputFormat* pVideoInputFmt = NULL;
	AVCodec* pCodec = NULL;

	if (NULL == m_pStreamInfo) {
		TRACE("NULL == m_pStreamInfo\n");
		return iRet;
	}

	//根据设备名称打开设备
	char* psDevName = GetDeviceName(n_Video);
	if (psDevName == NULL) {
		TRACE("video_thr--psDevName == NULL");
		return iRet;
	}

	pVideoInputFmt = av_find_input_format("dshow");
	if (pVideoInputFmt == NULL) {
		TRACE("pVideoInputFmt == NULL\n");
		return iRet;
	}

	AVDictionary* inOptions = NULL;
	//av_dict_set(&inOptions, "video_size", "640x480", 0);
	//av_dict_set(&inOptions, "r", "30", 0);

	//打开摄像头
	m_pFmtVideoCtx = avformat_alloc_context();
	if (avformat_open_input(&m_pFmtVideoCtx, psDevName, pVideoInputFmt, &inOptions) != 0) {
		TRACE("avformat_open_input err!\n");
		return iRet;
	}

	if (avformat_find_stream_info(m_pFmtVideoCtx, &inOptions) < 0) {
		TRACE("avformat_find_stream_info(m_pFmtVideoCtx, NULL) < 0\n");
		return iRet;
	}

	if (NULL == m_pFmtVideoCtx->streams) {
		TRACE("NULL == m_pFmtVideoCtx->streams");
		return iRet;
	}

	for (int i = 0; i < m_pFmtVideoCtx->nb_streams; i++) {
		if (m_pFmtVideoCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			m_iVideoIndex = i;
			break;
		}
	}

	if (m_iVideoIndex < 0) {
		TRACE("m_iVideoIndex < 0\n");
		return iRet;
	}
	m_pFmtVideoCtx->streams[m_iVideoIndex]->time_base.den = m_iFrameRate;
	m_pFmtVideoCtx->streams[m_iVideoIndex]->time_base.num = 1;

	m_pCodecVideoCtx = m_pFmtVideoCtx->streams[m_iVideoIndex]->codec;
	if (NULL == m_pCodecVideoCtx) {
		TRACE("NULL == m_pCodecVideoCtx");
		return iRet;
	}

	//获取视频的宽与高
	m_iSrcVideoHeight = m_pCodecVideoCtx->height;
	m_iSrcVideoWidth = m_pCodecVideoCtx->width;

	//初始化视频预览窗口
	InitVideoWindow();

	//需要将采集信息写文件，打开解码器
	pCodec = avcodec_find_decoder(m_pCodecVideoCtx->codec_id);
	if (pCodec == NULL) {
		TRACE("avcodec_find_decoder<0");
		return iRet;
	}
	if (avcodec_open2(m_pCodecVideoCtx, pCodec, NULL) < 0) {
		TRACE("avcodec_open2<0");
		return iRet;
	}

	iRet = 0;
	return iRet;
}

AVFormatContext* CLS_DlgStreamPusher::OpenFile(CString _cstrFilePath)
{
	AVFormatContext* pFileFmtCtx = NULL;
	if (_cstrFilePath == "") {
		MessageBox("打开文件有误！\n");
		goto END;
	}

	pFileFmtCtx = avformat_alloc_context();
	if (avformat_open_input(&pFileFmtCtx, _cstrFilePath, 0, 0) < 0) {
		TRACE("avformat_open_input file err ! (%s)", _cstrFilePath.GetBuffer());
		goto END;
	}

	if (avformat_find_stream_info(pFileFmtCtx, 0) < 0) {
		TRACE("avformat_find_stream_info err!\n");
		goto END;
	}

END:
	return pFileFmtCtx;
}

int CLS_DlgStreamPusher::OpenAduio()
{
	int iRet = -1;
	AVInputFormat* pAudioInputFmt = NULL;
	AVCodec* pCodec = NULL;

	//根据设备名称打开设备
	char* psDevName = GetDeviceName(n_Audio);
	if (psDevName == NULL) {
		TRACE("OpenAduio--psDevName == NULL");
		return iRet;
	}
	pAudioInputFmt = av_find_input_format("dshow");
	if (pAudioInputFmt == NULL) {
		TRACE("pAudioInputFmt == NULL\n");
		return iRet;
	}

	//打开麦克风
	m_pFmtAudioCtx = avformat_alloc_context();
	if (avformat_open_input(&m_pFmtAudioCtx, psDevName, pAudioInputFmt, NULL) != 0) {
		TRACE("avformat_open_input err!\n");
		return iRet;
	}
	if (avformat_find_stream_info(m_pFmtAudioCtx, NULL) < 0) {
		TRACE("avformat_find_stream_info(m_pFmtAudioCtx, NULL) < 0\n");
		return iRet;
	}

	for (int i = 0; i < m_pFmtAudioCtx->nb_streams; i++) {
		if (m_pFmtAudioCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			m_iAudioIndex = i;
			break;
		}
	}

	if (m_iAudioIndex < 0) {
		TRACE("m_iAudioIndex < 0\n");
		return iRet;
	}

	m_pCodecAudioCtx = m_pFmtAudioCtx->streams[m_iAudioIndex]->codec;
	if (NULL == m_pCodecAudioCtx) {
		TRACE("NULL == m_pCodecAudioCtx");
		return iRet;
	}

	//需要将采集信息写文件，打开解码器
	pCodec = avcodec_find_decoder(m_pCodecAudioCtx->codec_id);
	if (pCodec == NULL) {
		TRACE("avcodec_find_decoder<0");
		return iRet;
	}
	if (avcodec_open2(m_pCodecAudioCtx, pCodec, NULL) < 0) {
		TRACE("avcodec_open2<0");
		return iRet;
	}

	iRet = 0;
	return iRet;
}

int CLS_DlgStreamPusher::OpenRtmpAddr()
{
	int iRet = -1;
	AVOutputFormat* pStreamOutfmt = NULL;

	//进行推流操作
	if (m_cstrPushAddr == "") {
		MessageBox(_T("Failed to find RTMP address"));
		return iRet;
	}

	if (NULL == m_pStreamInfo) {
		MessageBox(_T("Failed to find stream info"));
		return iRet;
	}

	//根据推流地址获取到AVFormatContext
	//m_cstrPushAddr = "test.flv";
	avformat_alloc_output_context2(&m_pFmtRtmpCtx, NULL, "flv", m_cstrPushAddr);
	if (NULL == m_pFmtRtmpCtx) {
		TRACE("NULL == m_pFmtRtmpCtx\n");
		return iRet;
	}

	pStreamOutfmt = m_pFmtRtmpCtx->oformat;
	if (NULL == pStreamOutfmt) {
		TRACE("NULL == pStreamOutfmt\n");
		return iRet;
	}

	if (m_blUrl) {
		if (NULL == m_pCodecVideoCtx) {
			TRACE("NULL == m_pCodecVideoCtx\n");
			return iRet;
		}
		if (OpenRtmpUrl() < 0) {
			TRACE("OpenRtmpUrl err\n");
			return iRet;
		}
	}
	else if (OpenRtmpFile() < 0) {
		TRACE("OpenRtmpFile err!\n");
		return iRet;
	}

	//写头
	if (!(pStreamOutfmt->flags & AVFMT_NOFILE)) {
		iRet = avio_open(&m_pFmtRtmpCtx->pb, m_cstrPushAddr, AVIO_FLAG_WRITE);
		if (iRet < 0) {
			TRACE("Could not open output URL '%s'", m_cstrPushAddr);
			return iRet;
		}
	}

	iRet = avformat_write_header(m_pFmtRtmpCtx, NULL);
	if (iRet < 0) {
		TRACE("Error occurred when opening output URL\n");
		return iRet;
	}

	return iRet;
}

int CLS_DlgStreamPusher::OpenRtmpUrl()
{
	int iRet = -1;
	//视频推流信息
	if (NULL != m_pFmtVideoCtx) {
		m_pStreamInfo->m_pVideoStream = avformat_new_stream(m_pFmtRtmpCtx, NULL);
		if (!m_pStreamInfo->m_pVideoStream) {
			TRACE("Failed allocating output stream\n");
			goto END;
		}
		m_pStreamInfo->m_pVideoStream->codec->codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		m_pStreamInfo->m_pVideoStream->codec->codec_tag = 0;
		m_pStreamInfo->m_pVideoStream->codec->height = m_iDstVideoHeight;
		m_pStreamInfo->m_pVideoStream->codec->width = m_iDstVideoWidth;
		m_pStreamInfo->m_pVideoStream->codec->time_base.den = m_iFrameRate;
		m_pStreamInfo->m_pVideoStream->codec->time_base.num = 1;
		m_pStreamInfo->m_pVideoStream->codec->sample_aspect_ratio = m_pCodecVideoCtx->sample_aspect_ratio;
		m_pStreamInfo->m_pVideoStream->codec->pix_fmt = AV_PIX_FMT_YUV420P;
		// take first format from list of supported formats
		m_pStreamInfo->m_pVideoStream->codec->bit_rate = 900000;
		m_pStreamInfo->m_pVideoStream->codec->rc_max_rate = 900000;
		m_pStreamInfo->m_pVideoStream->codec->rc_min_rate = 900000;
		m_pStreamInfo->m_pVideoStream->codec->gop_size = m_pCodecVideoCtx->gop_size;
		m_pStreamInfo->m_pVideoStream->codec->qmin = 5;
		m_pStreamInfo->m_pVideoStream->codec->qmax = 51;
		m_pStreamInfo->m_pVideoStream->codec->max_b_frames = m_pCodecVideoCtx->max_b_frames;
		m_pStreamInfo->m_pVideoStream->r_frame_rate = m_pFmtVideoCtx->streams[m_iVideoIndex]->r_frame_rate;

		m_iVideoOutIndex = m_pStreamInfo->m_pVideoStream->index;

		if (m_pFmtRtmpCtx->oformat->flags & AVFMT_GLOBALHEADER)
			m_pStreamInfo->m_pVideoStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

		//打开视频编码器
		if ((avcodec_open2(m_pStreamInfo->m_pVideoStream->codec, m_pStreamInfo->m_pVideoStream->codec->codec, NULL)) < 0) {
			TRACE("can not open the encoder\n");
			goto END;
		}

		m_pStreamInfo->m_pVideoFifo = av_fifo_alloc(30 * avpicture_get_size(AV_PIX_FMT_YUV420P, m_pStreamInfo->m_pVideoStream->codec->width, m_pStreamInfo->m_pVideoStream->codec->height));
	}

	//音频推流信息
	if (NULL != m_pFmtAudioCtx) {
		m_pStreamInfo->m_pAudioStream = avformat_new_stream(m_pFmtRtmpCtx, NULL);
		if (NULL == m_pStreamInfo->m_pAudioStream) {
			TRACE("NULL == m_pStreamInfo->m_pAudioStream");
			goto END;
		}
		if (NULL == m_pFmtRtmpCtx->streams[m_iAudioIndex]) {
			TRACE("NULL == m_pFmtRtmpCtx->streams[m_iAudioIndex]\n");
			goto END;
		}
		m_pStreamInfo->m_pAudioStream->codec->codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
		m_pStreamInfo->m_pAudioStream->codec->sample_rate = m_pCodecAudioCtx->sample_rate;
		m_pStreamInfo->m_pAudioStream->codec->channel_layout = m_pFmtRtmpCtx->streams[m_iAudioIndex]->codec->channel_layout;
		m_pStreamInfo->m_pAudioStream->codec->channels = av_get_channel_layout_nb_channels(m_pStreamInfo->m_pAudioStream->codec->channel_layout);
		if (m_pStreamInfo->m_pAudioStream->codec->channel_layout == 0) {
			m_pStreamInfo->m_pAudioStream->codec->channel_layout = AV_CH_LAYOUT_STEREO;
			m_pStreamInfo->m_pAudioStream->codec->channels = av_get_channel_layout_nb_channels(m_pStreamInfo->m_pAudioStream->codec->channel_layout);
		}
		m_pStreamInfo->m_pAudioStream->codec->codec_type = AVMEDIA_TYPE_AUDIO;
		m_pStreamInfo->m_pAudioStream->codec->sample_fmt = m_pStreamInfo->m_pAudioStream->codec->codec->sample_fmts[0];
		AVRational time_base = { 1, m_pStreamInfo->m_pAudioStream->codec->sample_rate };
		m_pStreamInfo->m_pAudioStream->time_base = time_base;
		m_pFmtAudioCtx->streams[0]->time_base = time_base;
		m_iAudioOutIndex = m_pStreamInfo->m_pAudioStream->index;

		m_pStreamInfo->m_pAudioStream->codec->codec_tag = 0;
		if (m_pFmtRtmpCtx->oformat->flags & AVFMT_GLOBALHEADER)
			m_pStreamInfo->m_pAudioStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

		//打开音频编码器
		if (avcodec_open2(m_pStreamInfo->m_pAudioStream->codec, m_pStreamInfo->m_pAudioStream->codec->codec, 0) < 0) {
			TRACE("avcodec_open2 failed !\n");
			goto END;
		}
	}

	iRet = 0;
END:
	return iRet;
}
int CLS_DlgStreamPusher::OpenRtmpFile()
{
	int iRet = -1;
	if (NULL == m_strctPushFile.m_pFmtFileCtx) {
		TRACE("NULL == m_pFmtFileCtx");
		goto END;
	}
	if (NULL == m_pFmtRtmpCtx) {
		TRACE("NULL == m_pFmtRtmpCtx\n");
		goto END;
	}

	for (int i = 0; i < m_strctPushFile.m_pFmtFileCtx->nb_streams; i++) {
		AVStream* pInStream = NULL;
		if (m_strctPushFile.m_pFmtFileCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			pInStream = m_strctPushFile.m_pFmtFileCtx->streams[i];
			if (NULL == pInStream) {
				TRACE("NULL == pInStream\n");
				goto END;
			}
			m_pStreamInfo->m_pVideoStream = avformat_new_stream(m_pFmtRtmpCtx, pInStream->codec->codec);
			if (NULL == m_pStreamInfo->m_pVideoStream) {
				TRACE("NULL == m_pStreamInfo->m_pVideoStream\n");
				goto END;
			}
			m_iVideoIndex = i;
			//拷贝视频编码器信息
			if (avcodec_copy_context(m_pStreamInfo->m_pVideoStream->codec, pInStream->codec) < 0) {
				TRACE("avcodec_copy_context video err!\n");
				goto END;
			}
			m_pStreamInfo->m_pVideoStream->codec->codec_tag = 0;

			if (m_pFmtRtmpCtx->oformat->flags & AVFMT_GLOBALHEADER) {
				m_pStreamInfo->m_pVideoStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
			}
		}
		else if (m_strctPushFile.m_pFmtFileCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			pInStream = m_strctPushFile.m_pFmtFileCtx->streams[i];
			if (NULL == pInStream) {
				TRACE("NULL == pInStream\n");
				goto END;
			}
			m_pStreamInfo->m_pAudioStream = avformat_new_stream(m_pFmtRtmpCtx, pInStream->codec->codec);
			if (NULL == m_pStreamInfo->m_pAudioStream) {
				TRACE("NULL == m_pStreamInfo->m_pAudioStream\n");
				goto END;
			}
			m_iAudioIndex = i;
			//拷贝音频编码器信息
			if (avcodec_copy_context(m_pStreamInfo->m_pAudioStream->codec, pInStream->codec) < 0) {
				TRACE("avcodec_copy_context audio err!\n");
				goto END;
			}

			m_pStreamInfo->m_pAudioStream->codec->codec_tag = 0;
			if (m_pFmtRtmpCtx->oformat->flags & AVFMT_GLOBALHEADER) {
				m_pStreamInfo->m_pAudioStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
			}
		}
		else {
			break;
		}
	}

	iRet = 0;
END:
	return iRet;
}

void CLS_DlgStreamPusher::OnCbnSelchangeCobDeviceVideo()
{
	m_mapResolution.clear();
	m_cboResolution.ResetContent();
	int iCurSelIndex = m_cboDeviceVideo.GetCurSel();
	if (iCurSelIndex < 0) {
		TRACE("iCurSelIndex < 0\n");
		return;
	}
	GetResolution(iCurSelIndex);
	m_cboResolution.SetCurSel(0);

	//获取当前分辨率
	OnCbnSelchangeCobResolution();
}


void CLS_DlgStreamPusher::OnCbnSelchangeCobResolution()
{
	int iSelResIndex = m_cboResolution.GetCurSel();
	if (iSelResIndex < 0) {
		TRACE("iSelResIndex < 0\n");
		return;
	}

	map<int, int> mapResolution;
	mapResolution = m_mapResolution[iSelResIndex];
	map<int, int>::iterator iter = mapResolution.begin();
	for (; iter != mapResolution.end(); iter++) {
		m_iDstVideoWidth = iter->first;
		m_iDstVideoHeight = iter->second;
	}

	if (UpdateSdlInfo() < 0) {
		TRACE("更新分辨率失败！\n");
	}
}


void CLS_DlgStreamPusher::OnBnClickedBtnRefreshvideo()
{
	//设备刷新获取
	GetDevice();
}

void CLS_DlgStreamPusher::GetDevice()
{
	m_mapDeviceInfo.clear();
	m_cboDeviceVideo.ResetContent();
	m_cboDeviceAudio.ResetContent();
	//获取当前连接的视频设备
	int iRet = GetDeviceInfo(n_Video);
	if (iRet < 0) {
		TRACE("获取视频设备失败！");
		return;
	}

	//获取当前连接输入的音频设备
	iRet = GetDeviceInfo(n_Audio);
	if (iRet < 0) {
		TRACE("获取音频设备失败！");
		return;
	}

	//将获取到的设备信息插入下拉框中
	std::map<int, std::vector<std::string>>::iterator iter = m_mapDeviceInfo.begin();
	for (; iter != m_mapDeviceInfo.end(); iter++) {
		if (n_Video == iter->first) {
			//视频设备
			for (int i = 0; i < iter->second.size(); i++) {
				int iCount = m_cboDeviceVideo.GetCount();
				m_cboDeviceVideo.InsertString(iCount, iter->second[i].c_str());
			}
			m_cboDeviceVideo.SetCurSel(0);
		}
		else if (n_Audio == iter->first) {
			//音频设备
			for (int i = 0; i < iter->second.size(); i++) {
				int iCount = m_cboDeviceAudio.GetCount();
				m_cboDeviceAudio.InsertString(iCount, iter->second[i].c_str());
			}
			m_cboDeviceAudio.SetCurSel(0);
		}
	}

	//分辨率初始化
	OnCbnSelchangeCobDeviceVideo();
}

int CLS_DlgStreamPusher::CreateVideoWindow()
{
	if (m_blCreateVideoWin) {
		return 0;
	}

	int iRet = -1;

	//将CSTATIC控件和sdl显示窗口关联 
	HWND hWnd = this->GetDlgItem(IDC_STC_PREVIEW)->GetSafeHwnd();
	if (hWnd != NULL) {
		if (NULL != m_pStreamInfo && NULL == m_pStreamInfo->m_pShowScreen) {
			m_pStreamInfo->m_pShowScreen = SDL_CreateWindowFrom((void*)hWnd);
			if (m_pStreamInfo->m_pShowScreen == NULL) {
				MessageBox(_T("初始化视频窗口失败！\n"));
				return iRet;
			}
		}
	}

	iRet = 0;
	return iRet;
}

void CLS_DlgStreamPusher::OnBnClickedBtnOpenLocalFiles()
{
	m_lstLocalFiles.DeleteAllItems();
	ClearFileMem();

	//获取一个文件夹下的所有文件信息
	CString cstrFilePath = "";
	TCHAR cBuffer[MAX_PATH];
	BROWSEINFO bi;
	bi.hwndOwner = NULL;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = cBuffer;
	bi.lpszTitle = "选择文件夹";
	bi.ulFlags = BIF_EDITBOX;
	bi.lpfn = NULL;
	bi.iImage = NULL;
	LPITEMIDLIST plDList = SHBrowseForFolder(&bi);
	if (NULL == plDList) {
		TRACE("NULL == plDList\n");
		return;
	}
	SHGetPathFromIDList(plDList, cBuffer);
	cstrFilePath = cBuffer;

	GetFiles(cstrFilePath);
}

void CLS_DlgStreamPusher::GetFiles(CString _cstrFilePath)
{
	CFileFind fileFinder;
	//判断文件路径是否存在
	_cstrFilePath += _T("//*.*");

	BOOL blFinish = fileFinder.FindFile(_cstrFilePath);
	while (blFinish) {
		blFinish = fileFinder.FindNextFile();
		if (fileFinder.IsDirectory() && !fileFinder.GetFilePath()) {
			//目录进行递归查找
			GetFiles(fileFinder.GetFilePath());
		}
		else {
			CString cstrFileName = fileFinder.GetFileName();
			int iDotPos = cstrFileName.ReverseFind('.');
			CString cstrFileExt = cstrFileName.Right(cstrFileName.GetLength() - iDotPos - 1);
			if (cstrFileExt == _T("mp4") || cstrFileExt == _T("mkv") || cstrFileExt == _T("avi")
				|| cstrFileExt == _T("wma") || cstrFileExt == _T("rmvb") || cstrFileExt == _T("rm")
				|| cstrFileExt == _T("3gp") || cstrFileExt == _T("mid") || cstrFileExt == _T("flv")
				|| cstrFileExt == _T("flash") || cstrFileExt == _T("ts") || cstrFileExt == _T("mov")) {

				//将找到的文件插入到列表
				if (InsertFileList(fileFinder.GetFilePath()) < 0) {
					TRACE("InsertFileList err filepath is [%s]", fileFinder.GetFilePath());
					continue;
				}
			}
		}
	}

	fileFinder.Close();
}

void CLS_DlgStreamPusher::ShowControls(BOOL _blShow)
{
	m_btnOpenLocalFile.ShowWindow(!_blShow);
	m_btnOpenFiles.ShowWindow(!_blShow);
	m_lstLocalFiles.ShowWindow(!_blShow);
	m_stcSelFile.ShowWindow(!_blShow);
	GetDlgItem(IDC_STC_SEL_FILE)->ShowWindow(!_blShow);


	GetDlgItem(IDC_STC_DEVICE_VIDEO)->ShowWindow(_blShow);
	GetDlgItem(IDC_STC_DEVICE_AUDIO)->ShowWindow(_blShow);
	GetDlgItem(IDC_STC_RESOLUTION)->ShowWindow(!_blShow);
	GetDlgItem(IDC_STC_FRAME)->ShowWindow(!_blShow);
	GetDlgItem(IDC_BTN_REFRESHVIDEO)->ShowWindow(_blShow);
	GetDlgItem(IDC_CHK_SHOW_VIDEO)->ShowWindow(!_blShow);
	GetDlgItem(IDC_STC_PREVIEW)->ShowWindow(_blShow);
	GetDlgItem(IDC_BTN_PREVIEW)->ShowWindow(_blShow);
	GetDlgItem(IDC_STC_PUSHER_ADDR)->ShowWindow(!_blShow);
	GetDlgItem(IDC_CBO_PUSH_ADDR)->ShowWindow(!_blShow);
	m_cboDeviceAudio.ShowWindow(_blShow);
	m_cboDeviceVideo.ShowWindow(_blShow);
	m_cboResolution.ShowWindow(!_blShow);
	m_edtFrameRate.ShowWindow(!_blShow);
}


void CLS_DlgStreamPusher::OnCbnSelchangeCboPushAddr()
{
	//当前正在推送进行停止操作提示
	if (m_blPushSuccess) {
		MessageBox(_T("请先停止当前推流操作！\n"));
		return;
	}

	m_cstrPushAddr = strPushAddr[0].c_str();
}

BOOL CLS_DlgStreamPusher::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int iMenuId = LOWORD(wParam);
	switch (iMenuId)
	{
	case IDM_PBK_MNU_DELETE:
		OnDeleteItem();
		break;
	case IDM_PBK_MNU_CLEAR:
		OnClearItems();
		break;
	case IDM_PBK_MNU_SINGLE:
		m_blPushSingle = TRUE;
		break;
	case IDM_PBK_MNU_CIRCLE:
		m_blPushSingle = FALSE;
		break;
	case IDC_CLOSE:
		OnDestroy();
		break;
	default:
		break;
	}
	return CDialog::OnCommand(wParam, lParam);
}

void CLS_DlgStreamPusher::OnDeleteItem()
{
	//删除所选中的文件
	if (m_iLstSelIndex < 0) {
		TRACE("m_iLstSelIndex < 0\n");
		return;
	}

	//删除当前文件申请的内存
	map<int, STRCT_PUSH_FILE>::iterator iter = m_mapPushFile.begin();
	for (; iter != m_mapPushFile.end(); iter++) {
		if (m_iLstSelIndex == iter->first) {
			STRCT_PUSH_FILE strct_push_file = iter->second;

			if (NULL != strct_push_file.m_pFmtFileCtx) {
				avformat_close_input(&strct_push_file.m_pFmtFileCtx);
				avformat_free_context(strct_push_file.m_pFmtFileCtx);
				strct_push_file.m_pFmtFileCtx = NULL;
			}

			m_mapPushFile.erase(iter);
			break;
		}
	}

	m_lstLocalFiles.DeleteItem(m_iLstSelIndex);
}

void CLS_DlgStreamPusher::OnClearItems()
{
	//清空整个文件列表
	m_lstLocalFiles.DeleteAllItems();
	ClearFileMem();
}

int CLS_DlgStreamPusher::InsertFileList(CString _cstrFilePath)
{
	int iRet = -1;
	STRCT_PUSH_FILE strct_push_file;

	//将文件名称插入到列表中
	int iItemCount = m_lstLocalFiles.GetItemCount();
	m_lstLocalFiles.InsertItem(iItemCount, _cstrFilePath);

	AVFormatContext* pFileFmtCtx = OpenFile(_cstrFilePath);
	if (NULL == pFileFmtCtx) {
		TRACE("NULL == pFileFmtCtx!\n");
		goto END;
	}

	strct_push_file.m_cstrFilePath = _cstrFilePath;
	strct_push_file.m_pFmtFileCtx = pFileFmtCtx;
	m_mapPushFile.insert(map<int, STRCT_PUSH_FILE>::value_type(iItemCount, strct_push_file));

	iRet = 0;
END:
	return iRet;
}

void CLS_DlgStreamPusher::ClearFileMem()
{
	map<int, STRCT_PUSH_FILE>::iterator iter = m_mapPushFile.begin();
	for (; iter != m_mapPushFile.end(); iter++) {
		STRCT_PUSH_FILE strct_push_file = iter->second;

		if (NULL != strct_push_file.m_pFmtFileCtx) {
			avformat_close_input(&strct_push_file.m_pFmtFileCtx);
			avformat_free_context(strct_push_file.m_pFmtFileCtx);
			strct_push_file.m_pFmtFileCtx = NULL;
		}
	}
	m_mapPushFile.clear();
}

void CLS_DlgStreamPusher::OnNMRClickListLocalFiles(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	CPoint ptCur;
	GetCursorPos(&ptCur);

	NM_LISTVIEW* pNmListView = (NM_LISTVIEW*)pNMHDR;
	if (-1 == pNmListView->iItem) {
		TRACE("-1 == pNmListView->iItem\n");
		return;
	}

	m_iLstSelIndex = pNmListView->iItem;
	m_Rmenu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_LEFTALIGN, ptCur.x, ptCur.y, this);
	*pResult = 0;
}

void CLS_DlgStreamPusher::OnNMDblclkListLocalFiles(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	NM_LISTVIEW* pNmListView = (NM_LISTVIEW*)pNMHDR;
	if (-1 == pNmListView->iItem) {
		TRACE("-1 == pNmListView->iItem\n");
		return;
	}

	m_iLstSelIndex = pNmListView->iItem;
	if (m_iLstSelIndex >= m_mapPushFile.size()) {
		TRACE("m_iLstSelIndex >= m_mapPushFile.size() index is [%d]", m_iLstSelIndex);
		return;
	}

	memcpy(&m_strctPushFile, &m_mapPushFile[m_iLstSelIndex], sizeof(STRCT_PUSH_FILE));

	m_stcSelFile.SetWindowText(m_strctPushFile.m_cstrFilePath);
	*pResult = 0;
}

int CLS_DlgStreamPusher::InitVideoWindow()
{
	int iRet = -1;
	RECT rectDisPlay;

	if (m_blCreateVideoWin) {
		goto END;
	}
	this->GetDlgItem(IDC_STC_PREVIEW)->GetWindowRect(&rectDisPlay);
	m_pStreamInfo->m_xLeft = rectDisPlay.left;
	m_pStreamInfo->m_yTop = rectDisPlay.top;
	m_pStreamInfo->m_width = m_iDstVideoWidth;
	m_pStreamInfo->m_height = m_iDstVideoHeight;

	m_iWindowWidth = rectDisPlay.right - rectDisPlay.left;
	m_iWiddowHeight = rectDisPlay.bottom - rectDisPlay.top;

	if (NULL == m_pStreamInfo->m_pShowScreen) {
		MessageBox(_T("初始化视频窗口失败！\n"));
		goto END;
	}

	m_pStreamInfo->m_pSdlRender = SDL_CreateRenderer(m_pStreamInfo->m_pShowScreen, -1, 0);
	if (NULL == m_pStreamInfo->m_pSdlRender) {
		MessageBox(_T("初始化视频窗口失败！\n"));
		goto END;
	}

	if (UpdateSdlInfo() < 0) {
		TRACE("UpdateSdlInfo < 0");
		goto END;
	}

	m_blCreateVideoWin = TRUE;

	iRet = 0;
END:
	return iRet;
}

void CLS_DlgStreamPusher::OnDestroy()
{
	CDialog::OnDestroy();
}

int CLS_DlgStreamPusher::UpdateSdlInfo()
{
	int iRet = -1;

	if (NULL == m_pStreamInfo->m_pSdlRender) {
		TRACE("NULL == m_pStreamInfo->m_pSdlRender");
		goto END;
	}

	//获取视频宽高之后创建纹理
	m_pStreamInfo->m_pSdlTexture = SDL_CreateTexture(m_pStreamInfo->m_pSdlRender, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, m_iDstVideoWidth, m_iDstVideoHeight);
	if (NULL == m_pStreamInfo->m_pSdlTexture) {
		MessageBox(_T("初始化视频窗口失败！\n"));
		goto END;
	}

	m_pStreamInfo->m_pVideoSwsCtx = sws_getContext(m_iSrcVideoWidth, m_iSrcVideoHeight, m_pCodecVideoCtx->pix_fmt,
		m_iDstVideoWidth, m_iDstVideoHeight, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	/*
	m_pStreamInfo->m_pVideoSwsCtx = sws_getContext(m_iSrcVideoWidth, m_iSrcVideoHeight, m_pCodecVideoCtx->pix_fmt,
		640, 480, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	*/
	if (NULL == m_pStreamInfo->m_pVideoSwsCtx) {
		TRACE("NULL == strct_streaminfo->m_pVideoSwsCtx\n");
		goto END;
	}

	iRet = 0;

END:
	return iRet;
}


void CLS_DlgStreamPusher::OnBnClickedBtnEndpoint()
{
	// TODO: 咯扁俊 牧飘费 舅覆 贸府扁 内靛甫 眠啊钦聪促.
	CString m_Username;
	CString m_UserID;

	m_EditUsername.GetWindowText(m_Username);
	m_EditUserID.GetWindowText(m_UserID);

	CT2CA _username(m_Username);
	std::string username(_username);

	CT2CA _userID(m_UserID);
	std::string userID(_userID);

	name = username.c_str();
	student_id = userID.c_str();

	table = "";
	endpoint = "";

	get_tablename();
	get_endpoint();

	m_cstrPushAddr = endpoint.c_str();

}


void CLS_DlgStreamPusher::OnEnChangeEdtFramerate()
{
	// TODO:  RICHEDIT 牧飘费牢 版快, 捞 牧飘费篮
	// CDialog::OnInitDialog() 窃荐甫 犁瘤沥 
	//窍绊 付胶农俊 OR 楷魂窍咯 汲沥等 ENM_CHANGE 敲贰弊甫 瘤沥窍咯 CRichEditCtrl().SetEventMask()甫 龋免窍瘤 臼栏搁
	// 捞 舅覆 皋矫瘤甫 焊郴瘤 臼嚼聪促.

	// TODO:  咯扁俊 牧飘费 舅覆 贸府扁 内靛甫 眠啊钦聪促.
}

void CLS_DlgStreamPusher::OnStnClickedStcPreview()
{
	// TODO: 咯扁俊 牧飘费 舅覆 贸府扁 内靛甫 眠啊钦聪促.
}


void CLS_DlgStreamPusher::OnStnClickedStcResolution()
{
	// TODO: 咯扁俊 牧飘费 舅覆 贸府扁 内靛甫 眠啊钦聪促.
}


void CLS_DlgStreamPusher::OnStnClickedStcPusherAddr()
{
	// TODO: 咯扁俊 牧飘费 舅覆 贸府扁 内靛甫 眠啊钦聪促.
}


void CLS_DlgStreamPusher::OnEnChangeUsername()
{
	// TODO:  RICHEDIT 牧飘费牢 版快, 捞 牧飘费篮
	// CDialog::OnInitDialog() 窃荐甫 犁瘤沥 
	//窍绊 付胶农俊 OR 楷魂窍咯 汲沥等 ENM_CHANGE 敲贰弊甫 瘤沥窍咯 CRichEditCtrl().SetEventMask()甫 龋免窍瘤 臼栏搁
	// 捞 舅覆 皋矫瘤甫 焊郴瘤 臼嚼聪促.

	// TODO:  咯扁俊 牧飘费 舅覆 贸府扁 内靛甫 眠啊钦聪促.
}


void CLS_DlgStreamPusher::OnEnChangeUserid()
{
	// TODO:  RICHEDIT 牧飘费牢 版快, 捞 牧飘费篮
	// CDialog::OnInitDialog() 窃荐甫 犁瘤沥 
	//窍绊 付胶农俊 OR 楷魂窍咯 汲沥等 ENM_CHANGE 敲贰弊甫 瘤沥窍咯 CRichEditCtrl().SetEventMask()甫 龋免窍瘤 臼栏搁
	// 捞 舅覆 皋矫瘤甫 焊郴瘤 臼嚼聪促.

	// TODO:  咯扁俊 牧飘费 舅覆 贸府扁 内靛甫 眠啊钦聪促.
}


void CLS_DlgStreamPusher::OnBnClickedTermination()
{
	// TODO: 咯扁俊 牧飘费 舅覆 贸府扁 内靛甫 眠啊钦聪促.
	stream_termination();
}
