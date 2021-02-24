//
// Created by jackie on 12/15/20.
//

#include "VideoCap.h"
#include "CommonFunction.h"
#include "Logger.h"
#include "opencv2/opencv.hpp"
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
#include "libavdevice/avdevice.h"
#include "libavutil/time.h"
}

using namespace std;

// 判断文件名是否是视频文件.
bool isNameFileType(string name) {
    if (name.empty()) {
        LOG_ERROR << "文件名不能为空！";
        return false;
    }
    if (name.find(".mp4") != std::string::npos || name.find(".MP4") != std::string::npos) {
        return true;
    }
    if (name.find(".flv") != std::string::npos || name.find(".FLV") != std::string::npos) {
        return true;
    }
    if (name.find(".mkv") != std::string::npos || name.find(".MKV") != std::string::npos) {
        return true;
    }
    if (name.find(".avi") != std::string::npos || name.find(".AVI") != std::string::npos) {
        return true;
    }
    return false;
}

// 判断文件名是否是RTSP文件.
bool isNameRTSPType(string name) {
    if (name.empty()) {
        LOG_ERROR << "文件名不能为空！";
        return false;
    }

    return name.find("rtsp:") != string::npos;
}

VideoCap::VideoCap(const char* url) {
    avdevice_register_all();
    mCapturedFrame = av_frame_alloc();
    mCapturedPacket = new AVPacket();
    memset(mCapturedPacket, 0, sizeof(*mCapturedPacket));
    open(url);
}

VideoCap::VideoCap(const char* cam, const char* audio)  {
    avdevice_register_all();
    mCapturedFrame = av_frame_alloc();
    mCapturedPacket = new AVPacket();
    mframeWithFace = av_frame_alloc();
    memset(mCapturedPacket, 0, sizeof(*mCapturedPacket));
    open(cam, audio);
}

VideoCap::~VideoCap() {
    if (mCapturedFrame != nullptr) {
        av_frame_free(&mCapturedFrame);
        mCapturedFrame = nullptr;
    }

    if (mCapturedPacket != nullptr) {
        av_packet_free(&mCapturedPacket);
        mCapturedPacket = nullptr;
    }
}

// Open rtsp camera or file.
int VideoCap::open(const char* camUrl) {
    if (!isValidCameraInput(camUrl)) {
        return -1;
    }

    mCamUrl = camUrl;
    int rv = avformat_open_input(&mVideoInputFormatContext, camUrl, NULL, NULL);
    if (rv != 0) {
        LOG_ERROR << "无法打开" << camUrl;
        return printFFMPEGError(rv);
    }
    if ((rv = avformat_find_stream_info(mVideoInputFormatContext, NULL)) != 0) {
        LOG_ERROR << "在" << camUrl << "中找不到视频流和音频流！";
        return printFFMPEGError(rv);
    }
    if (initVideoAudioDecodeEnv(mVideoInputFormatContext) != 0) {
        LOG_ERROR << "为" << camUrl << "初始化音视频解码环境失败!";
        return -1;
    }
    if (initVideoScaleEnv() != 0) {
        LOG_ERROR << "为" << camUrl << "初始化色彩空间转换及缩放环境失败!";
        return -1;
    }

    /*
    if (isCameraType()) {
        // Open default aac audio file, no need to decode and resample
        if ((rv = avformat_open_input(&mAudioInputFormatContext, DEFAULT_AUDIO_FILE, NULL, NULL)) != 0) {
            LOG_ERROR << "无法打开" << DEFAULT_AUDIO_FILE;
            return printFFMPEGError(rv);
        }
        if ((rv = avformat_find_stream_info(mAudioInputFormatContext, NULL)) != 0) {
            LOG_ERROR << "在" << DEFAULT_AUDIO_FILE << "中找不到音频流！";
            return printFFMPEGError(rv);
        }

        initAudioDecodeEnv(mAudioInputFormatContext);
    }
     */

    mOpened = true;
    return 0;
}

int VideoCap::open(const char* camUrl, const char* audioDev) {
    if (!isValidCameraInput(camUrl)) {
        return -1;
    }
    if (audioDev == nullptr) {
        // 只打开摄像头，或者视频文件，或者RTSP摄像头.
        return open(camUrl);
    }

    // 同时打开摄像头和麦克风.
    AVInputFormat* videoInputFormat = av_find_input_format("v4l2");
    AVInputFormat* audioInputFormat = av_find_input_format("alsa");
    mCamUrl = camUrl;
    mAudioDevice = audioDev;

    int rv = avformat_open_input(&mVideoInputFormatContext, camUrl, videoInputFormat, NULL);
    if (rv != 0) {
        LOG_ERROR << "无法打开" << camUrl;
        return printFFMPEGError(rv);
    }
    rv = avformat_open_input(&mAudioInputFormatContext, audioDev, audioInputFormat, NULL);
    if (rv != 0) {
        LOG_ERROR << "无法打开" << audioDev;
        return printFFMPEGError(rv);
    }

    if ((rv = avformat_find_stream_info(mVideoInputFormatContext, NULL)) != 0) {
        LOG_ERROR << "在" << camUrl << "中找不到视频流！";
        return printFFMPEGError(rv);
    }
    if ((rv = avformat_find_stream_info(mAudioInputFormatContext, NULL)) != 0) {
        LOG_ERROR << "在" << audioDev << "中找不到音频流!";
        return printFFMPEGError(rv);
    }

    if (initVideoDecodeEnv(mVideoInputFormatContext) != 0) {
        LOG_ERROR << "为" << mCamUrl << "初始化视频解码环境失败！";
        return -1;
    }
    if (initAudioDecodeEnv(mAudioInputFormatContext) != 0) {
        LOG_ERROR << "为" << audioDev << "初始化音频解码环境失败！";
        return -1;
    }
    if (initVideoScaleEnv() != 0) {
        LOG_ERROR << "为" << camUrl << "初始化色彩空间转换及缩放环境失败!";
        return -1;
    }

    mOpened = true;
    return 0;
}

bool VideoCap::isValidCameraInput(const char *url) {
    if (url == nullptr) {
        LOG_ERROR << "非法的摄像头输入!";
        return false;
    }

    if (isNameFileType(url)) {
        mType = VIDEO_CAP_TYPE_FILE;
    }
    else if (isNameRTSPType(url)) {
        mType = VIDEO_CAP_TYPE_RTSP;
    }
    else {
        mType = VIDEO_CAP_TYPE_CAMERA;
    }
    return true;
}

int VideoCap::initVideoAudioDecodeEnv(AVFormatContext *videoAudioFormatContext) {
    if (videoAudioFormatContext == nullptr) {
        LOG_ERROR << mCamUrl << ":非法的视频格式上下文!";
        return -1;
    }

    for (unsigned int i = 0; i < videoAudioFormatContext->nb_streams; ++i) {
        if (videoAudioFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if ((mVideoStream = videoAudioFormatContext->streams[i]) == nullptr) {
                LOG_ERROR << "在" << mCamUrl << "中找不到视频流！";
                return -1;
            }
            if ((mVideoDecoder = avcodec_find_decoder(mVideoStream->codecpar->codec_id)) == nullptr) {
                LOG_ERROR << "无法为" << mCamUrl << "找到视频解码器";
                return -1;
            }
            if ((mVideoDecodeContext = avcodec_alloc_context3(mVideoDecoder)) == nullptr) {
                LOG_ERROR << "无法为" << mCamUrl << "的视频解码器分配解码上下文";
                return -1;
            }
            avcodec_parameters_to_context(mVideoDecodeContext, mVideoStream->codecpar);
            if (avcodec_open2(mVideoDecodeContext, mVideoDecoder, NULL) < 0) {
                LOG_ERROR << "在" << mCamUrl << "中打开视频解码器上下文失败!";
                return -1;
            }

            mVideoIndex = i;
            mCapWidth = mVideoDecodeContext->width;
            mCapHeight = mVideoDecodeContext->height;
            if (mVideoStream->avg_frame_rate.num == 0 || mVideoStream->avg_frame_rate.den == 0) {
                mFPS = FPS;
            }
            else {
                mFPS = mVideoStream->avg_frame_rate.num / mVideoStream->avg_frame_rate.den;
            }
        }

        if (videoAudioFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if ((mAudioStream = videoAudioFormatContext->streams[i]) == nullptr) {
                LOG_ERROR << "在" << mCamUrl << "中找不到音频流！";
                return -1;
            }
            if ((mAudioDecoder = avcodec_find_decoder(mAudioStream->codecpar->codec_id)) == nullptr) {
                LOG_ERROR << "无法为" << mCamUrl << "找到音频解码器";
                return -1;
            }
            if ((mAudioDecodeContext = avcodec_alloc_context3(mAudioDecoder)) == nullptr) {
                LOG_ERROR << "无法为" << mCamUrl << "的音频解码器分配解码上下文";
                return -1;
            }
            avcodec_parameters_to_context(mAudioDecodeContext, mAudioStream->codecpar);
            if (avcodec_open2(mAudioDecodeContext, mAudioDecoder, NULL) < 0) {
                LOG_ERROR << "在" << mCamUrl << "中打开音频解码器上下文失败!";
                return -1;
            }
            mAudioIndex = i;
            // 初始化音频重采样环境
            initAudioResampleEnv();
        }
    }
    return 0;
}

int VideoCap::initVideoDecodeEnv(AVFormatContext *videoFormatContext) {
    if (videoFormatContext == nullptr) {
        LOG_ERROR << mCamUrl << ":非法的视频格式上下文!";
        return -1;
    }

    for (unsigned int i = 0; i < videoFormatContext->nb_streams; ++i) {
        if (videoFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if ((mVideoStream = videoFormatContext->streams[i]) == nullptr) {
                LOG_ERROR << "在" << mCamUrl << "中找不到视频流！";
                return -1;
            }
            if ((mVideoDecoder = avcodec_find_decoder(mVideoStream->codecpar->codec_id)) == nullptr) {
                LOG_ERROR << "无法为" << mCamUrl << "找到视频解码器";
                return -1;
            }
            if ((mVideoDecodeContext = avcodec_alloc_context3(mVideoDecoder)) == nullptr) {
                LOG_ERROR << "无法为" << mCamUrl << "的视频解码器分配解码上下文";
                return -1;
            }
            avcodec_parameters_to_context(mVideoDecodeContext, mVideoStream->codecpar);
            if (avcodec_open2(mVideoDecodeContext, mVideoDecoder, NULL) < 0) {
                LOG_ERROR << "在" << mCamUrl << "中打开视频解码器上下文失败!";
                return -1;
            }

            mVideoIndex = i;
            mCapWidth = mVideoDecodeContext->width;
            mCapHeight = mVideoDecodeContext->height;
            if (mVideoStream->avg_frame_rate.num == 0 || mVideoStream->avg_frame_rate.den == 0) {
                mFPS = FPS;
            }
            else {
                mFPS = mVideoStream->avg_frame_rate.num / mVideoStream->avg_frame_rate.den;
            }
            return 0;
        }
    }
    return -1;
}

int VideoCap::initAudioDecodeEnv(AVFormatContext *audioFormatContext) {
    if (audioFormatContext == nullptr) {
        LOG_ERROR << mCamUrl << ":非法的音频格式上下文!";
        return -1;
    }
    for (unsigned int i = 0; i < audioFormatContext->nb_streams; ++i) {
        if (audioFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if ((mAudioStream = audioFormatContext->streams[i]) == nullptr) {
                LOG_ERROR << "在" << mCamUrl << "中找不到音频流！";
                return -1;
            }
            if ((mAudioDecoder = avcodec_find_decoder(mAudioStream->codecpar->codec_id)) == nullptr) {
                LOG_ERROR << "无法为" << mCamUrl << "找到音频解码器";
                return -1;
            }
            if ((mAudioDecodeContext = avcodec_alloc_context3(mAudioDecoder)) == nullptr) {
                LOG_ERROR << "无法为" << mCamUrl << "的音频解码器分配解码上下文";
                return -1;
            }
            avcodec_parameters_to_context(mAudioDecodeContext, mAudioStream->codecpar);
            if (avcodec_open2(mAudioDecodeContext, mAudioDecoder, NULL) < 0) {
                LOG_ERROR << "在" << mCamUrl << "中打开音频解码器上下文失败!";
                return -1;
            }
            mAudioIndex = i;
            if (!isCameraType()) {
                initAudioResampleEnv();
            }
            return 0;
        }
    }
    return -1;
}

int VideoCap::initVideoScaleEnv() {
    mScaleContext = sws_getContext(mCapWidth, mCapHeight, mVideoDecodeContext->pix_fmt,
                                   MONITOR_WINDOW_WIDTH, MONITOR_WINDOW_HEIGHT, AV_PIX_FMT_YUV420P,
                                   SWS_BICUBIC, 0, 0, 0);
    mUpscaleContext = sws_getContext(mCapWidth, mCapHeight, mVideoDecodeContext->pix_fmt,
                                     PGM_WIDTH, PGM_HEIGHT, AV_PIX_FMT_YUV420P,
                                     SWS_BICUBIC, 0, 0, 0);


    mAvFrametoRGBContext = sws_getContext(mCapWidth, mCapHeight, AV_PIX_FMT_YUV420P,
                                          mCapWidth, mCapHeight, AV_PIX_FMT_BGR24,
                                          SWS_FAST_BILINEAR, 0, 0, 0);
    mCameratoYUV420Context = sws_getContext(mCapWidth, mCapHeight, mVideoDecodeContext->pix_fmt,
                                            mCapWidth, mCapHeight, AV_PIX_FMT_YUV420P,
                                            SWS_FAST_BILINEAR, 0, 0, 0);
    mYUV420toCameraContext = sws_getContext(mCapWidth, mCapHeight, AV_PIX_FMT_YUV420P,
                                            mCapWidth, mCapHeight, mVideoDecodeContext->pix_fmt,
                                            SWS_FAST_BILINEAR, 0, 0, 0);
    if (mScaleContext == nullptr || mUpscaleContext == nullptr
        || mAvFrametoRGBContext == nullptr || mCameratoYUV420Context ==nullptr
        || mYUV420toCameraContext == nullptr) {
        return -1;
    }

    return 0;
}

int VideoCap::initAudioResampleEnv() {
    if (mAudioStream == nullptr) {
        LOG_ERROR << "VideoCap::initAudioResampleEnv() failed, have no audio stream!";
        return -1;
    }
    const AVFilter* audioBufferSrc = avfilter_get_by_name("abuffer");
    const AVFilter* audioBufferSink = avfilter_get_by_name("abuffersink");
    mInputAudioFilter = avfilter_inout_alloc();
    mOutputAudioFilter = avfilter_inout_alloc();
    if (mInputAudioFilter == nullptr || mOutputAudioFilter == nullptr) {
        return -1;
    }

    if (mAudioDecodeContext->channel_layout == 0) {
        mAudioDecodeContext->channel_layout = av_get_default_channel_layout(mAudioDecodeContext->channels);
    }

    const enum AVSampleFormat OUTPUT_AUDIO_SAMPLE_FMTS[] = { AV_SAMPLE_FMT_FLTP, AV_SAMPLE_FMT_NONE };
    const int64_t OUTPUT_AUDIO_CHANNEL_LAYOUTS[] = { (int64_t)mAudioDecodeContext->channel_layout, -1 };
    const int OUTPUT_AUDIO_SAMPLE_RATES[] = { AAC_SAMPLE_RATE, -1 };
    if ((mAudioFilterGraph = avfilter_graph_alloc()) == nullptr) {
        return -1;
    }
    mAudioFilterGraph->nb_threads = 8;

    char args[512] = { 0 };
    snprintf(args, sizeof(args), "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%Id",
        mAudioStream->time_base.num, mAudioStream->time_base.den,
        mAudioDecodeContext->sample_rate, av_get_sample_fmt_name(mAudioDecodeContext->sample_fmt),
        mAudioDecodeContext->channel_layout);
    LOG_INFO << mCamUrl << " initAudioResampleEnv argument: " << args;

    int rv = avfilter_graph_create_filter(&mAudioBufferSrcContext, audioBufferSrc, "in", args, NULL, mAudioFilterGraph);
    if (rv < 0) {
        return -1;
    }
    if ((rv = avfilter_graph_create_filter(&mAudioBufferSinkContext, audioBufferSink, "out", NULL, NULL, mAudioFilterGraph)) < 0) {
        return -1;
    }

    if ((rv = av_opt_set_int_list(mAudioBufferSinkContext, "sample_fmts", OUTPUT_AUDIO_SAMPLE_FMTS, -1, AV_OPT_SEARCH_CHILDREN)) < 0) {
        return -1;
    }
    if ((rv = av_opt_set_int_list(mAudioBufferSinkContext, "channel_layouts", OUTPUT_AUDIO_CHANNEL_LAYOUTS, -1, AV_OPT_SEARCH_CHILDREN)) < 0) {
        return -1;
    }
    if ((rv = av_opt_set_int_list(mAudioBufferSinkContext, "sample_rates", OUTPUT_AUDIO_SAMPLE_RATES, -1, AV_OPT_SEARCH_CHILDREN)) < 0) {
        return -1;
    }
    mOutputAudioFilter->name = av_strdup("in");
    mOutputAudioFilter->filter_ctx = mAudioBufferSrcContext;
    mOutputAudioFilter->pad_idx = 0;
    mOutputAudioFilter->next = NULL;

    mInputAudioFilter->name = av_strdup("out");
    mInputAudioFilter->filter_ctx = mAudioBufferSinkContext;
    mInputAudioFilter->pad_idx = 0;
    mInputAudioFilter->next = NULL;

    if ((rv = avfilter_graph_parse_ptr(mAudioFilterGraph, "anull", &mInputAudioFilter, &mOutputAudioFilter, nullptr)) < 0) {
        return -1;
    }
    if ((rv = avfilter_graph_config(mAudioFilterGraph, NULL)) < 0) {
        return -1;
    }
    av_buffersink_set_frame_size(mAudioBufferSinkContext, 1024);

    return 0;
}

int VideoCap::grab() {
    int ret = -1;
    // 视频文件或RTSP流，无法确定下一个包是视频数据还是音频数据.
    av_packet_unref(mCapturedPacket);
    av_frame_unref(mCapturedFrame);
    if (isFileType() || isRTSPType()) {
        if ((ret = av_read_frame(mVideoInputFormatContext, mCapturedPacket)) < 0) {
            if (ret == AVERROR_EOF) {
                mEOF = true;
            }
            return -1;
        }

        // 视频数据
        if (mCapturedPacket->stream_index == mVideoIndex) {
            if ((ret = avcodec_send_packet(mVideoDecodeContext, mCapturedPacket)) < 0) {
                return -1;
            }
            if ((ret = avcodec_receive_frame(mVideoDecodeContext, mCapturedFrame)) < 0) {
                return -1;
            }
            return 0;
        }
        // 音频数据
        else if (mCapturedPacket->stream_index == mAudioIndex) {
            if ((ret = avcodec_send_packet(mAudioDecodeContext, mCapturedPacket)) < 0) {
                return -1;
            }
            if ((ret = avcodec_receive_frame(mAudioDecodeContext, mCapturedFrame)) < 0) {
                return -1;
            }
            return 1;
        }
    }

    // 摄像头
    if (isCameraType()) {
        if (haveVideo()) {
            return grabVideoPacket();
        }
        return -1;
        /*
        if (isVideoNextToCap()) {
            if (haveVideo()) {
                return grabVideoPacket();
            }
            return -1;
        }
        else {
            if (haveAudio()) {
                return grabAudioPacket();
            }
            return -1;
        }
         */
    }
    return -1;
}

bool VideoCap::isVideoNextToCap() const {
    if (isCameraType()) {
        AVRational videoTimeBase = { 1, mFPS };
        AVRational audioTimeBase = { 1, getSampleRate() };
        if (av_compare_ts(mVideoPts, videoTimeBase, mAudioPts, audioTimeBase) <= 0) {
            return true;
        }
    }
    return false;
}

int VideoCap::grabVideoPacket() {
    if (mVideoStream == nullptr) {
        LOG_ERROR << mCamUrl << "中没有视频流，无法获取视频帧!";
        return -1;
    }

    int ret = -1;
    av_packet_unref(mCapturedPacket);
    if ((ret = av_read_frame(mVideoInputFormatContext, mCapturedPacket)) < 0) {
        return -1;
    }
    if ((ret = avcodec_send_packet(mVideoDecodeContext, mCapturedPacket)) < 0) {
        return -1;
    }
    if ((ret = avcodec_receive_frame(mVideoDecodeContext, mCapturedFrame)) < 0) {
        return -1;
    }
    if (!isCameraType()) {
        mVideoPts++;
        return 0;
    }

    AVFrame* frame = av_frame_alloc();
    frame->format = AV_PIX_FMT_YUV420P;
    frame->width = mCapturedFrame->width;
    frame->height = mCapturedFrame->height;
    av_frame_get_buffer(frame, 0);
    int h = sws_scale(mCameratoYUV420Context, mCapturedFrame->data, mCapturedFrame->linesize, 0, mCapturedFrame->height,
                      frame->data, frame->linesize);
    if (h <= 0 || frame->width != mCapturedFrame->width || frame->height != mCapturedFrame->height) {
        av_frame_free(&frame);
        return -1;
    }

    AVFrame* faceFrame = findFaceInYUVFrame(frame);

    h = sws_scale(mYUV420toCameraContext, faceFrame->data, faceFrame->linesize, 0, faceFrame->height,
                  mCapturedFrame->data, mCapturedFrame->linesize);
    if (h <= 0) {
        return -1;
    }

    av_frame_unref(frame);
    av_frame_free(&frame);
    mVideoPts++;
    return 0;
}

int VideoCap::grabAudioPacket() {
    if (mAudioStream == nullptr) {
        LOG_ERROR << mCamUrl << "中没有音频流，无法获取音频帧!";
        return -1;
    }

    int ret = -1;
    av_packet_unref(mCapturedPacket);
    if ((ret = av_read_frame(mAudioInputFormatContext, mCapturedPacket)) < 0) {
        return -1;
    }
    if ((ret = avcodec_send_packet(mAudioDecodeContext, mCapturedPacket)) < 0) {
        return -1;
    }
    if ((ret = avcodec_receive_frame(mAudioDecodeContext, mCapturedFrame)) < 0) {
        return -1;
    }
    mAudioPts += mCapturedFrame->nb_samples;
    return 1;
}

cv::Mat VideoCap::convertYUVToRGB(AVFrame *yuvFrame) {
    AVFrame dst;
    memset(&dst, 0, sizeof(dst));
    int w = yuvFrame->width, h = yuvFrame->height;
    cv::Mat RGBMat;
    RGBMat = cv::Mat(h, w, CV_8UC3);
    dst.data[0] = (uint8_t *)RGBMat.data;
    avpicture_fill( (AVPicture *)&dst, dst.data[0], AV_PIX_FMT_BGR24, mCapWidth, mCapHeight);


    sws_scale(mAvFrametoRGBContext, yuvFrame->data, yuvFrame->linesize, 0, mCapHeight,
              dst.data, dst.linesize);
    // todo
    // sws_freeContext(mAvFrametoRGBContext);
    //cv::imwrite("1.jpg",RGBMat);
    return RGBMat;
}

AVFrame* VideoCap::findFaceInYUVFrame(AVFrame* yuvFrame) {
    cv::Mat rgbFrame;
    rgbFrame = convertYUVToRGB(yuvFrame);


    if(!rgbFrame.data){
        std::cout << "rgbFrame has no data" << std::endl;
    }
    if(rgbFrame.empty()){
        std::cout << "rgbFrame is empty" << std::endl;
    }

    // cv::imwrite("1.jpg",rgbFrame);
    //mMtcnn->findFace(rgbFrame);
    // cv::imwrite("2.jpg",rgbFrame);
    convertMatToFaceFrame(rgbFrame);
    rgbFrame.release();
    return mframeWithFace;
}

AVFrame* VideoCap::convertMatToFaceFrame(cv::Mat imageMat) {
    //cv::Mat src = cv::imread(imageFile);
    cv::Mat yuv;
    cv::Mat dst1;

    //imwrite("2.jpg",imageMat);
    //AVFrame* frame = av_frame_alloc();
    av_frame_unref(mframeWithFace);
    imageMat.convertTo(dst1, imageMat.type());
    if (mframeWithFace == nullptr || dst1.empty()) {
        return nullptr;
    }


    mframeWithFace->format = AV_PIX_FMT_YUV420P;
    mframeWithFace->width = dst1.cols;
    mframeWithFace->height = dst1.rows;

    av_frame_get_buffer(mframeWithFace, 0);
    av_frame_make_writable(mframeWithFace);

    cv::cvtColor(dst1, yuv, cv::COLOR_BGR2YUV_I420);

    int yLen = dst1.cols * dst1.rows;
    int uLen = yLen / 4;
    int vLen = yLen / 4;
    unsigned char *pdata = yuv.data;

    memcpy(mframeWithFace->data[0], pdata, yLen);
    memcpy(mframeWithFace->data[1], pdata + yLen, uLen);
    memcpy(mframeWithFace->data[2], pdata + yLen + uLen, vLen);

    yuv.release();
    dst1.release();
    imageMat.release();
    return mframeWithFace;
}


AVFrame* VideoCap::grabCapturedVideoFrame() const { return mCapturedFrame; }
AVFrame* VideoCap::grabCapturedAudioFrame() const { return mCapturedFrame; }

AVFrame* VideoCap::scale(AVFrame* inFrame) {
    if (inFrame == nullptr) {
        LOG_ERROR << "缩放失败：非法的输入！";
        return nullptr;
    }

    if (inFrame->width != mCapWidth || inFrame->height != mCapHeight) {
        LOG_ERROR << "缩放失败：输入YUV帧(" << inFrame->width << "x" << inFrame->height << "),"
                  << "只接受" << mCapWidth << "x" << mCapHeight << "尺寸的帧!";
        return nullptr;
    }

    AVFrame* scaledFrame = av_frame_alloc();
    memset(scaledFrame, 0, sizeof(*scaledFrame));
    scaledFrame->format = AV_PIX_FMT_YUV420P;
    scaledFrame->width = MONITOR_WINDOW_WIDTH;
    scaledFrame->height = MONITOR_WINDOW_HEIGHT;
    av_frame_get_buffer(scaledFrame, 0);
    int h = sws_scale(mScaleContext, inFrame->data, inFrame->linesize, 0, mVideoDecodeContext->height,
                  scaledFrame->data, scaledFrame->linesize);
    if (h <= 0 && (scaledFrame->width != MONITOR_WINDOW_WIDTH || scaledFrame->height != MONITOR_WINDOW_HEIGHT)) {
        av_frame_free(&scaledFrame);
        return nullptr;
    }
    return scaledFrame;
}

AVFrame* VideoCap::upscale(AVFrame* inFrame) {
    if (inFrame == nullptr) {
        LOG_ERROR << "缩放失败：非法的输入！";
        return nullptr;
    }

    if (inFrame->width != mCapWidth || inFrame->height != mCapHeight) {
        LOG_ERROR << "缩放失败：输入YUV帧(" << inFrame->width << "x" << inFrame->height << "),"
                  << "只接受" << mCapWidth << "x" << mCapHeight << "尺寸的帧!";
        return nullptr;
    }

    AVFrame* scaledFrame = av_frame_alloc();
    memset(scaledFrame, 0, sizeof(*scaledFrame));
    scaledFrame->format = AV_PIX_FMT_YUV420P;
    scaledFrame->width = PGM_WIDTH;
    scaledFrame->height = PGM_HEIGHT;
    av_frame_get_buffer(scaledFrame, 0);
    int h = sws_scale(mUpscaleContext, inFrame->data, inFrame->linesize, 0, mVideoDecodeContext->height,
                  scaledFrame->data, scaledFrame->linesize);
    if (h <= 0 && (scaledFrame->width != PGM_WIDTH || scaledFrame->height != PGM_HEIGHT)) {
        LOG_ERROR << "sws_scale() 缩放失败!";
        av_frame_free(&scaledFrame);
        return nullptr;
    }
    return scaledFrame;
}

AVFrame* VideoCap::resample(AVFrame* inFrame) {
    if (isCameraType()) {
        return av_frame_clone(inFrame);
    }
    int ret = -1;
    LOG_DEBUG << "Audio resample pts: " << inFrame->pts;
    if ((ret = av_buffersrc_add_frame_flags(mAudioBufferSrcContext, inFrame, AV_BUFFERSRC_FLAG_PUSH)) < 0) {
        LOG_ERROR << "Audio resample failed! add frame to buffer source error:" << ret;
        //printFFMPEGError(ret);
        return nullptr;
    }

    AVFrame* outFrame = av_frame_alloc();
    if ((ret = av_buffersink_get_frame_flags(mAudioBufferSinkContext, outFrame, AV_BUFFERSINK_FLAG_NO_REQUEST)) < 0) {
        LOG_ERROR << "Audio resample failed! get frame from buffer sink error:" << ret;
        av_frame_free(&outFrame);
        return nullptr;
    }
    return outFrame;
}

bool VideoCap::reachEOF() const { return mEOF; }

bool VideoCap::isOpened() const { return mOpened; }
bool VideoCap::isFileType() const { return mType == VIDEO_CAP_TYPE_FILE; }
bool VideoCap::isCameraType() const { return mType == VIDEO_CAP_TYPE_CAMERA; }
bool VideoCap::isRTSPType() const { return mType == VIDEO_CAP_TYPE_RTSP; }

bool VideoCap::haveVideo() const { return mVideoStream != nullptr; }
bool VideoCap::haveAudio() const {return mAudioStream != nullptr; }

const char* VideoCap::getCameraUrl() const { return mCamUrl.c_str(); }
const char* VideoCap::getAudioDeviceName() const { return mAudioDevice.c_str(); }

int VideoCap::getWidth() const { return mCapWidth; }
int VideoCap::getHeight() const { return mCapHeight; }
int VideoCap::getFPS() const { return mFPS; }
int VideoCap::getSampleRate() const {
    if (haveAudio()) {
        return mAudioDecodeContext->sample_rate;
    }
    else {
        return AAC_SAMPLE_RATE;
    }
}

AVStream* VideoCap::getVideoStream() const { return mVideoStream; }
AVStream* VideoCap::getAudioStream() const { return mAudioStream; }
