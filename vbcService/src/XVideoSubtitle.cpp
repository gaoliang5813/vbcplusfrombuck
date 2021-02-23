//
// Created by jackie on 12/15/20.
//

#include "XVideoSubtitle.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "Logger.h"
#include "CommonFunction.h"

XVideoSubtitle::XVideoSubtitle()
    :mText(""), mPos(0), mInWidth(MONITOR_WINDOW_WIDTH), mInHeight(MONITOR_WINDOW_HEIGHT)
{
}

XVideoSubtitle::~XVideoSubtitle() {
}

void XVideoSubtitle::setPosition(int pos) {
    mPos = pos;
}

void XVideoSubtitle::setText(const char *text) {
    mText = text;
}

const char* XVideoSubtitle::getText() const {
    return mText.c_str();
}

int XVideoSubtitle::getPosition() const {
    return mPos;
}

XVideoEffect* XVideoSubtitle::clone() {
    XVideoSubtitle* xVideoSubtitle = new XVideoSubtitle();
    xVideoSubtitle->mText = mText;
    xVideoSubtitle->mPos = mPos;
    xVideoSubtitle->mInWidth = PGM_WIDTH;
    xVideoSubtitle->mInHeight = PGM_HEIGHT;
    xVideoSubtitle->mFontSize = (PGM_HEIGHT / MONITOR_WINDOW_HEIGHT) * mFontSize;
    xVideoSubtitle->initSubtitleGraph(mText.c_str(), mPos);
    return xVideoSubtitle;
}

AVFrame* XVideoSubtitle::operate(AVFrame *yuvFrame) {
    if (mBufferSrcCtx == nullptr || yuvFrame == nullptr) {
        return nullptr;
    }
    if ((mFiltFrame = av_frame_alloc()) == nullptr) {
        return nullptr;
    }
    //将传入的帧设置入filtergraph
    if (av_buffersrc_add_frame_flags(mBufferSrcCtx, yuvFrame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
        av_frame_free(&mFiltFrame);
        return nullptr;
    }
    //得到处理后的帧
    if (av_buffersink_get_frame(mBufferSinkCtx, mFiltFrame) < 0) {
        av_frame_free(&mFiltFrame);
        return nullptr;
    }

    return mFiltFrame;
}

void XVideoSubtitle::release() {
    if (mFiltFrame != nullptr) {
        av_frame_free(&mFiltFrame);
        mFiltFrame = nullptr;
    }
}

/* 初始化filter函数 */
int XVideoSubtitle::initFilter(const char* filtersDescr){
    int ret = 0;
    const AVFilter *bufferSrc = avfilter_get_by_name("buffer");
    const AVFilter *bufferSink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    char args[512] = { 0 };
    const char* argFormat = "video_size=%dx%d:pix_fmt=0:time_base=1/25:pixel_aspect=0/1";
    snprintf(args, sizeof(args), argFormat, mInWidth, mInHeight);

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

int XVideoSubtitle::initSubtitleGraph(const char *subtitle, int posX, int posY){
    if (posX < 0 || posX > mInWidth || posY < 0 || posY > mInHeight) {
        return -1;
    }

    mText = subtitle;
    char filtersDescr[1024] = { 0 };
    const char* filterFormat = "drawtext=fontfile=../resources/FreeSansOblique.ttf:fontcolor=black:fontsize=%d:text=%s:x=%d:y=%d";
    snprintf(filtersDescr, sizeof(filtersDescr), filterFormat, mFontSize, subtitle, posX, posY);
    
    if (initFilter(filtersDescr) < 0) {
        LOG_ERROR << "initFilter() failed!";
        return -1;
    }
    LOG_INFO << "initFilter() succeed.";
    return 0;
}

/* 将传来的字幕属性，坐标位置转化为ffmpeg语句，传入初始化filter函数中 */
int XVideoSubtitle::initSubtitleGraph(const char *subtitle, int pos){
    if (pos < 0 || pos > 9) {
        return -1;    
    }

    mText = subtitle;
    mPos = pos;
    int posX = -1, posY = -1;
    convertPosModeToXY(pos, posX, posY);
    return initSubtitleGraph(subtitle, posX, posY);
}

int XVideoSubtitle::update(const char *subtitle, int pos) {
    mText = subtitle;
    mPos = pos;
    const char *target = "drawtext";
    const char *cmd = "reinit";
    char arg[1024] = { 0 };
    int posX = -1, posY = -1;
    convertPosModeToXY(pos, posX, posY);
    snprintf(arg, sizeof(arg), "text=%s:x=%d:y=%d", subtitle, posX, posY);
    char res[512] = { 0 };
    return avfilter_graph_send_command(mFilterGraph, target, cmd, arg, res, sizeof(res), 1);

}

/* 更改字幕文字内容 */
int XVideoSubtitle::updateSubtitleText(const char *text) {
    mText = text;
    const char *target = "drawtext";
    const char *cmd = "reinit";
    char arg[1024] = { 0 };
    snprintf(arg, sizeof(arg), "text=%s", text);
    char res[512] = { 0 };
    return avfilter_graph_send_command(mFilterGraph, target, cmd, arg, res, sizeof(res), 1);
}

/* 更改字幕文字内容 */
int XVideoSubtitle::updateSubtitleText(int isTimeFlag) {
    const char *target = "drawtext";
    const char *cmd = "reinit";
    char arg[1024] = { 0 };
    char timeText[256] = {0};
    
    time_t timer = time(NULL);
    strftime(timeText, sizeof(timeText), "%Y-%m-%d %H:%M:%S", localtime(&timer));

    snprintf(arg, sizeof(arg), "text=%s", timeText);
    char res[512] = { 0 };
    return avfilter_graph_send_command(mFilterGraph, target, cmd, arg, res, sizeof(res), 1);
}

/* 更改字幕坐标位置 */
int XVideoSubtitle::updateSubtitleXY(int posX, int posY) {
    if (posX < 0 || posX > mInWidth || posY < 0 || posY > mInHeight) {
        return -1;
    }
    const char *target = "drawtext";
    const char *cmd = "reinit";
    char arg[512] = { 0 };
    snprintf(arg, sizeof(arg), "x=%d:y=%d", posX, posY);

    char res[512] = { 0 };
    return avfilter_graph_send_command(mFilterGraph, target, cmd, arg, res, sizeof(res), 1);
}

int XVideoSubtitle::updateSubtitlePos(int pos) {
    mPos = pos;
    int posX = -1, posY = -1;
    convertPosModeToXY(pos, posX, posY);
    return updateSubtitleXY(posX, posY);
}

int XVideoSubtitle::convertPosModeToXY(int pos, int& posX, int& posY) {
    if (pos < 0 || pos > 9) {
        return -1;
    }

    if (pos == 0) {
        posX = mInWidth / 3;
        posY = mInHeight * 2 / 3;
    }
    else {
        posX = ((pos - 1) % 3) * mInWidth / 3;
        posY = ((pos - 1) / 3) * mInHeight / 3;
    }
    return 0;
}
