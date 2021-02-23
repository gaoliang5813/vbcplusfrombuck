//
// Created by jackie on 1/8/21.
//
#include "Logger.h"
#include "CommonFunction.h"
extern "C" {
#include "libavutil/error.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}
#include "opencv2/opencv.hpp"
using namespace cv;
const char* lowResoltionImage = "../resources/kxwell640_360.png";
const char* highResoltionImage = "../resources/kxwellHigh.png";
const char* lowResTransparentImage = "../resources/trans640_360.png";
const char* highResTransparentImage = "../resources/trans1080p.png";

int printFFMPEGError(int errorCode) {
    if (errorCode == 0) {
        return 0;
    }
    char errorBuf[1024] = { 0 };
    av_strerror(errorCode, errorBuf, sizeof(errorBuf) - 1);
    LOG_ERROR << errorBuf;
    return errorCode;
}

AVFrame* convertImageToFrame(const char* imageFile);

void* getDefaultImageLowResolution() {
    //return convertImageToFrame(lowResTransparentImage);
    return convertImageToFrame(lowResoltionImage);
}

void* getDefaultImageHighResolution() {
    return convertImageToFrame(highResTransparentImage);
    //return convertImageToFrame(highResoltionImage);
}

void* getSilentAudioFrame(int sampleRate) {
    static bool sIsFirst = true;
    static AVFrame* frame = av_frame_alloc();
    AVFrame* outFrame = nullptr;
    if (sIsFirst) {
        if (frame == nullptr) {
            return nullptr;
        }

        frame->sample_rate = sampleRate;
        frame->format = AV_SAMPLE_FMT_FLTP; /*默认的format:AV_SAMPLE_FMT_FLTP*/
        frame->channel_layout = av_get_default_channel_layout(2);
        frame->channels = 2;
        frame->nb_samples = 1024; /*默认的sample大小:1024*/
        int ret = av_frame_get_buffer(frame, 0);
        if(ret < 0) {
            av_frame_free(&frame);
            return NULL;
        }

        av_samples_set_silence(frame->data, 0, frame->nb_samples, frame->channels, AV_SAMPLE_FMT_FLTP);
        outFrame = av_frame_clone(frame);
        sIsFirst = false;
    }
    else {
        outFrame = av_frame_clone(frame);
    }

    return outFrame;
}

AVFrame* convertImageToFrame(const char* imageFile) {
    cv::Mat src = cv::imread(imageFile);
    cv::Mat yuv;
    cv::Mat dst1;

    AVFrame* frame = av_frame_alloc();
    src.convertTo(dst1, src.type());
    if (frame == nullptr || dst1.empty()) {
        return nullptr;
    }

    frame->format = AV_PIX_FMT_YUV420P;
    frame->width = dst1.cols;
    frame->height = dst1.rows;

    av_frame_get_buffer(frame, 0);
    av_frame_make_writable(frame);

    cv::cvtColor(dst1, yuv, cv::COLOR_BGR2YUV_I420);

    int yLen = dst1.cols * dst1.rows;
    int uLen = yLen / 4;
    int vLen = yLen / 4;
    unsigned char *pdata = yuv.data;

    memcpy(frame->data[0], pdata, yLen);
    memcpy(frame->data[1], pdata + yLen, uLen);
    memcpy(frame->data[2], pdata + yLen + uLen, vLen);

    src.release();
    yuv.release();
    dst1.release();

    return frame;
}
