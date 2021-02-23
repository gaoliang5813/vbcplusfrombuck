/**
* Copyright (c) 2020，合肥祥云汇智科技有限公司
* All rights reserved.
*
* 文件名称：XVideoLogo.cpp
* 摘要：FFMPEG加图片水印接口封装
*
* 当前版本：1.1
* 作者：梁元琨
* 完成日期：2020-12-16
**/

#include "XVideoLogo.h"
#include "CommonFunction.h"
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/opt.h"
}

XVideoLogo::XVideoLogo()
    : mImageMat(nullptr), mPos(0),
    mInWidth(MONITOR_WINDOW_WIDTH), mInHeight(MONITOR_WINDOW_HEIGHT)
{
}

XVideoLogo::~XVideoLogo() {

}

void XVideoLogo::setImageFile(const char *imageFile) {
    mImageFile = imageFile;
}

void XVideoLogo::setImageMat(const cv::Mat *imageMat) {
    mImageMat = imageMat;
}

void XVideoLogo::setImagePos(int pos) {
    mPos = pos;
}

const char* XVideoLogo::getImageFile() const {
    return mImageFile.c_str();
}

const cv::Mat* XVideoLogo::getImageMat() const {
    return mImageMat;
}

int XVideoLogo::getPosition() const {
    return mPos;
}

AVFrame* XVideoLogo::operate(AVFrame *yuvFrame) {
    if (mBufferSrcCtx == nullptr || yuvFrame == nullptr) {
        return nullptr;
    }
    mFiltFrame = av_frame_alloc();
    //将传入的帧设置入filtergraph
    if (av_buffersrc_add_frame_flags(mBufferSrcCtx, yuvFrame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
        av_frame_free(&mFiltFrame);
        return NULL;
    }
    //得到处理后的帧
    av_buffersink_get_frame(mBufferSinkCtx, mFiltFrame);

    return mFiltFrame;
}

void XVideoLogo::release() {
    if (mFiltFrame != nullptr) {
        av_frame_free(&mFiltFrame);
        mFiltFrame = nullptr;
    }
}

XVideoEffect* XVideoLogo::clone() {
    XVideoLogo* xVideoLogo = new XVideoLogo();
    xVideoLogo->mInWidth = PGM_WIDTH;
    xVideoLogo->mInHeight = PGM_HEIGHT;
    xVideoLogo->mImageFile = "../resources/kxwellLargeLogo.png";
    xVideoLogo->initLogoGraph(xVideoLogo->mImageFile.c_str(), mPos);
    return xVideoLogo;
}

/* 初始化filter函数 */
int XVideoLogo::initFilter(const char* filtersDescr){
    int ret = 0;
    const AVFilter *bufferSrc = avfilter_get_by_name("buffer");
    const AVFilter *bufferSink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    char args[1024] = { 0 };
    snprintf(args, sizeof(args), "video_size=%dx%d:pix_fmt=0:time_base=1/25:pixel_aspect=0/1", mInWidth, mInHeight);

    enum AVPixelFormat pixFmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE};

    mFilterGraph = avfilter_graph_alloc();
    if (!outputs || !inputs || !mFilterGraph) {
        ret = AVERROR(ENOMEM);
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        return ret;
    }

    /* 创建并向FilterGraph中添加一个Filter。in链 */
    ret= avfilter_graph_create_filter(&mBufferSrcCtx, bufferSrc, "in",
                                      args, NULL, mFilterGraph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        return ret;
    }

    /* 创建并向FilterGraph中添加一个Filter。out链 */
    ret = avfilter_graph_create_filter(&mBufferSinkCtx, bufferSink, "out",
                                       NULL, NULL, mFilterGraph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        return ret;
    }

    /* 将二进制选项转化为整数列表 */
    ret = av_opt_set_int_list(mBufferSinkCtx, "pix_fmts", pixFmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        return ret;
    }

    /*
     * Set the endpoints for the filter graph. The mFilterGraph will
     * be linked to the graph described by filters_descr.
     */

    /*
     * The buffer source output must be connected to the input pad of
     * the first filter described by filters_descr; since the first
     * filter input label is not specified, it is set to "in" by
     * default.
     */
    outputs->name = av_strdup("in");
    outputs->filter_ctx = mBufferSrcCtx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    /*
     * The buffer sink input must be connected to the output pad of
     * the last filter described by filters_descr; since the last
     * filter output label is not specified, it is set to "out" by
     * default.
     */
    inputs->name = av_strdup("out");
    inputs->filter_ctx = mBufferSinkCtx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    /* 将一串通过字符串描述的Graph添加到FilterGraph中 */
    if ((ret = avfilter_graph_parse_ptr(mFilterGraph, filtersDescr,
                                        &inputs, &outputs, NULL)) < 0){
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        return ret;
    }

    /* 检查FilterGraph的配置。 */
    if ((ret = avfilter_graph_config(mFilterGraph, NULL)) < 0){
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        return ret;
    }


    return ret;
}


/* 将传来的图片名称，坐标位置转化为ffmpeg语句，传入初始化filter函数中 */
int XVideoLogo::initLogoGraph(const char *picturename, int posX, int posY) {
    mImageFile = picturename;
    if (mImageFile.empty()) {
        mImageFile = "../resources/kxwellLogo.png";
    }
    char filtersDescr[512] = { 0 };

    snprintf(filtersDescr, sizeof(filtersDescr), "movie=%s[wm];[in][wm]overlay=x=%d:y=%d[out]", mImageFile.c_str(), posX, posY);

    if (this->initFilter(filtersDescr) < 0)
        return -1;
    return 0;

}

int XVideoLogo::initLogoGraph(const char *picturename, int pos){
    mImageFile = picturename;
    if (mImageFile.empty()) {
        mImageFile = "../resources/kxwellLogo.png";
    }
    mPos = pos;
    if (pos < 0 || pos > 9) {
        return -1;
    }

    int posX = -1, posY = -1;
    convertPosModeToXY(pos, posX, posY);
    return initLogoGraph(mImageFile.c_str(), posX, posY);
}

int XVideoLogo::convertPosModeToXY(int pos, int& posX, int& posY) {
    if (pos < 0 || pos > 9) {
        return -1;
    }

    if (pos == 0) {
        posX = posY = 60 * mInHeight / PGM_HEIGHT;
    }
    else {
        posX = ((pos - 1) % 3) * mInWidth / 3;
        posY = ((pos - 1) / 3) * mInHeight / 3;
    }
    return 0;
}
