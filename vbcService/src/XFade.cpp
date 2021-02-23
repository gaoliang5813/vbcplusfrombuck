/**
* Copyright(c) 2020, 合肥祥云汇智科技有限公司
* All rights reserved.
*
* 文件名称：XFade.cpp
* 摘要:调用顺序：先initSplitFilter,然后addStream，最后opreate
*
* 作者: 梁元琨
* 完成日期: 2020-12-22
**/

#include "XFade.h"
#include "Stream.h"
#include "Logger.h"
extern "C"
{
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
}

XFade::XFade()
        :mInWidth(1920), mInHeight(1080)
{
    mBufferSrcCtx = (AVFilterContext**)av_malloc(2*sizeof(AVFilterContext*));    //2为两路视频流
    mBufferSinkCtx = NULL;
}

XFade::~XFade() {
}

void XFade::addStream(Stream* stream) {
    if (stream != nullptr) {
        mStreams.push_back(stream);
    }
}

AVFrame* XFade::operate(AVFrame* yuvFrame) {

    return mFiltFrame;
}

XVideoEffect* XFade::clone() {
    return new XFade();
}

AVFrame* XFade::operate(AVFrame* yuvFrame1,AVFrame* yuvFrame2) {
    mFiltFrame = av_frame_alloc();

    //将传入的帧设置入filtergraph
    if (av_buffersrc_add_frame_flags(mBufferSrcCtx[0], yuvFrame1, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        LOG_ERROR << "Error while feeding the filtergraph\n!";
        return nullptr;
    }

    if (av_buffersrc_add_frame_flags(mBufferSrcCtx[1], yuvFrame2, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        LOG_ERROR << "Error while feeding the filtergraph\n!";
        return nullptr;
    }


    //得到处理后的帧
    if (av_buffersink_get_frame(mBufferSinkCtx, mFiltFrame) < 0) {
        return nullptr;
    }

    return mFiltFrame;
}


void XFade::release() {
    if (mFiltFrame != nullptr) {
        av_frame_free(&mFiltFrame);
        mFiltFrame = nullptr;
    }
}

//flag 1为淡入淡出，2为左转，3为右转，4为上转，5为下转
int XFade::initXfadeFilter(int flag)
{
    char filtersDescr[512];
    int	streamNum = 2;
    int ret;
      
    if(flag == 1) {
    // snprintf(filtersDescr, sizeof(filtersDescr), "[in0][in1]xfade=transition=fade:duration=5:offset=5[out]");
        snprintf(filtersDescr, sizeof(filtersDescr), "[in0][in1]xfade=transition=wipeleft:duration=2:offset=5[out]");
    }
     else if (flag == 2) {
       snprintf(filtersDescr, sizeof(filtersDescr), "[in0][in1]xfade=transition=wiperight:duration=2:offset=5[out]");
     }
    else if (flag == 3) {
        snprintf(filtersDescr, sizeof(filtersDescr), "[in0][in1]xfade=transition=wipeup:duration=2:offset=5[out]");
    }
    else if (flag == 4) {
        snprintf(filtersDescr, sizeof(filtersDescr), "[in0][in1]xfade=transition=wipedown:duration=2:offset=5[out]");
    }
    else if (flag == 5) {
        snprintf(filtersDescr, sizeof(filtersDescr), "[in0][in1]xfade=transition=wipeleft:duration=2:offset=5[out]");
    }
    else {
        LOG_ERROR << "initXfadeFilter failed!";
        return -1;
    }


    ret = initFilter(filtersDescr,streamNum);

    return ret;
}


/* 初始化filter函数 */
int XFade::initFilter(const char *filtersDescr, int streamNum)
{
    char args[512];
    char padName[10];
    int ret = 0;
    int i;
    const AVFilter **buffersrc = (const AVFilter**)av_malloc(streamNum*sizeof(AVFilter*));
    const AVFilter *buffersink = NULL;

    AVFilterInOut **outputs = (AVFilterInOut**)av_malloc(streamNum*sizeof(AVFilterInOut*));
    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVFilterGraph *mFilterGraph = avfilter_graph_alloc();

    //AVFilterContext **mBufferSrcCtx = (AVFilterContext**)av_malloc(2*sizeof(AVFilterContext*));    //2为两路视频流

    enum AVPixelFormat pixFmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE};
    const char* argFormat = "video_size=%dx%d:pix_fmt=0:time_base=1/25000:pixel_aspect=0/1";
    snprintf(args, sizeof(args), argFormat, mInWidth, mInHeight);

    for (i = 0; i < streamNum; i++)
    {
        buffersrc[i] = NULL;
        mBufferSrcCtx[i] = NULL;
        outputs[i] = avfilter_inout_alloc();
    }
    if (!outputs || !inputs || !mFilterGraph) {
        ret = AVERROR(ENOMEM);
        avfilter_inout_free(&inputs);
        av_free(buffersrc);
        avfilter_inout_free(outputs);
        av_free(outputs);
        return ret;
    }

    for (i = 0; i < streamNum; i++)
    {
        buffersrc[i] = avfilter_get_by_name("buffer");
    }
    buffersink = avfilter_get_by_name("buffersink");
    if (!buffersrc || !buffersink) {
        LOG_ERROR << "filtering source or sink element not found\n";
        ret = AVERROR_UNKNOWN;
        avfilter_inout_free(&inputs);
        av_free(buffersrc);
        avfilter_inout_free(outputs);
        av_free(outputs);
        return ret;
    }

    /* 创建并向FilterGraph中添加一个Filter。in链 */
    for (i = 0; i < streamNum; i++)
    {

        snprintf(padName, sizeof(padName), "in%d", i);
        ret = avfilter_graph_create_filter(&(mBufferSrcCtx[i]), buffersrc[i], padName,
                                           args, NULL, mFilterGraph);
        if (ret < 0) {
            LOG_ERROR << "Cannot create buffer source\n";
            avfilter_inout_free(&inputs);
            av_free(buffersrc);
            avfilter_inout_free(outputs);
            av_free(outputs);
            return ret;
        }
    }

    /* 创建并向FilterGraph中添加一个Filter。out链 */
    ret = avfilter_graph_create_filter(&mBufferSinkCtx, buffersink, "out",
                                       NULL, NULL, mFilterGraph);
    if (ret < 0) {
        LOG_ERROR << "Cannot create buffer sink\n";
        avfilter_inout_free(&inputs);
        av_free(buffersrc);
        avfilter_inout_free(outputs);
        av_free(outputs);
        return ret;
    }

    /* 将二进制选项转化为整数列表 */
    ret = av_opt_set_bin(mBufferSinkCtx, "pix_fmts",
                         (uint8_t*)pixFmts, sizeof(AV_PIX_FMT_YUV420P),
                         AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOG_ERROR << "Cannot set output pixel format\n";
        avfilter_inout_free(&inputs);
        av_free(buffersrc);
        avfilter_inout_free(outputs);
        av_free(outputs);
        return ret;
    }

    /* 筛选器图的端点. */
    for (i = 0; i < streamNum; i++)
    {
        snprintf(padName, sizeof(padName), "in%d", i);
        outputs[i]->name = av_strdup(padName);
        outputs[i]->filter_ctx = mBufferSrcCtx[i];
        outputs[i]->pad_idx = 0;
        if (i == streamNum - 1)
            outputs[i]->next = NULL;
        else
            outputs[i]->next = outputs[i + 1];
    }

    inputs->name = av_strdup("out");
    inputs->filter_ctx = mBufferSinkCtx;
    inputs->pad_idx = 0;
    inputs->next = NULL;
    if (!outputs[0]->name || !inputs->name) {
        ret = AVERROR(ENOMEM);
        avfilter_inout_free(&inputs);
        av_free(buffersrc);
        avfilter_inout_free(outputs);
        av_free(outputs);
        return ret;
    }

    /* 将一串通过字符串描述的Graph添加到FilterGraph中 */
    if ((ret = avfilter_graph_parse_ptr(mFilterGraph, filtersDescr, &inputs, outputs, NULL)) < 0){
        avfilter_inout_free(&inputs);
        av_free(buffersrc);
        avfilter_inout_free(outputs);
        av_free(outputs);
        return ret;
    }

    /* 检查FilterGraph的配置。 */
    if ((ret = avfilter_graph_config(mFilterGraph, NULL)) < 0){
        avfilter_inout_free(&inputs);
        av_free(buffersrc);
        avfilter_inout_free(outputs);
        av_free(outputs);
        return ret;
    }

    return ret;
}
