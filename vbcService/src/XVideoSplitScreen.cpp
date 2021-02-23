/**
* Copyright(c) 2020, 合肥祥云汇智科技有限公司
* All rights reserved.
*
* 文件名称：XVideoSplitScreen.cpp
* 摘要:调用顺序：先initSplitFilter,然后addStream，最后opreate
*
* 作者: 梁元琨
* 完成日期: 2020-12-22
**/

#include "XVideoSplitScreen.h"
#include "Stream.h"
#include "Logger.h"
#include "CommonFunction.h"
extern "C" {
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
}

XVideoSplitScreen::XVideoSplitScreen(int width, int height, int mode)
    :mInWidth(width), mInHeight(height)
{
    int numStreams = 2;
    if (mode == 9) {
        mNumStreams = numStreams = 6;
    }
    else if (mode == 4) {
        mNumStreams = numStreams = 4;
    }
    setMode(mode);
    mBufferSrcCtx = (AVFilterContext**)av_malloc(numStreams * sizeof(AVFilterContext*));
    mBufferSinkCtx = NULL;
    initSplitFilter(mode);
}

XVideoSplitScreen::~XVideoSplitScreen() {
}

void XVideoSplitScreen::addStream(Stream* stream) {
    if (stream != nullptr) {
        mStreams.push_back(stream);
    }
}

AVFrame* XVideoSplitScreen::operate(AVFrame* yuvFrame) {
    mFiltFrame = av_frame_alloc();
    //得到处理后的帧
    if (av_buffersink_get_frame(mBufferSinkCtx, mFiltFrame) < 0) {
        return nullptr;
    }

    return mFiltFrame;
}

XVideoEffect* XVideoSplitScreen::clone() {
    return new XVideoSplitScreen(PGM_WIDTH, PGM_HEIGHT, getMode());
}

AVFrame* XVideoSplitScreen::operate(AVFrame* yuvFrame1,AVFrame* yuvFrame2) {
    mFiltFrame = av_frame_alloc();

    //将传入的帧设置入filtergraph
    if (av_buffersrc_add_frame_flags(mBufferSrcCtx[0], yuvFrame1, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        LOG_ERROR << "XVideoSplitScreen::operate() Error while feeding the filtergraph with yuvFrame1!\n";
        av_frame_free(&mFiltFrame);
        return nullptr;
    }

    if (av_buffersrc_add_frame_flags(mBufferSrcCtx[1], yuvFrame2, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        LOG_ERROR << "XVideoSplitScreen::operate() Error while feeding the filtergraph with yuvFrame2!\n";
        av_frame_free(&mFiltFrame);
        return nullptr;
    }


    //得到处理后的帧
    //av_usleep(1000);
    int rv = av_buffersink_get_frame_flags(mBufferSinkCtx, mFiltFrame,0);
    if (rv < 0) {
        printFFMPEGError(rv);
        // TODO: unref yuvFrame1 and yuvFrame2
        av_frame_unref(yuvFrame1);
        av_frame_unref(yuvFrame2);
        av_frame_free(&mFiltFrame);
        return nullptr;
    }

    return mFiltFrame;
}

AVFrame* XVideoSplitScreen::operate(AVFrame* yuvFrame1, AVFrame* yuvFrame2, AVFrame* yuvFrame3, AVFrame* yuvFrame4) {
    mFiltFrame = av_frame_alloc();

    //将传入的帧设置入filtergraph
    if (av_buffersrc_add_frame_flags(mBufferSrcCtx[0], yuvFrame1, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        LOG_ERROR << "Error while feeding the filtergraph\n!";
        av_frame_free(&mFiltFrame);
        return nullptr;
    }

    if (av_buffersrc_add_frame_flags(mBufferSrcCtx[1], yuvFrame2, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        LOG_ERROR << "Error while feeding the filtergraph\n!";
        av_frame_free(&mFiltFrame);
        return nullptr;
    }

    if (av_buffersrc_add_frame_flags(mBufferSrcCtx[2], yuvFrame3, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        LOG_ERROR << "Error while feeding the filtergraph\n!";
        av_frame_free(&mFiltFrame);
        return nullptr;
    }

    if (av_buffersrc_add_frame_flags(mBufferSrcCtx[3], yuvFrame4, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        LOG_ERROR << "Error while feeding the filtergraph\n!";
        av_frame_free(&mFiltFrame);
        return nullptr;
    }


    //得到处理后的帧
    if (av_buffersink_get_frame(mBufferSinkCtx, mFiltFrame) < 0) {
        av_frame_unref(yuvFrame1);
        av_frame_unref(yuvFrame2);
        av_frame_unref(yuvFrame3);
        av_frame_unref(yuvFrame4);
        av_frame_free(&mFiltFrame);
        return nullptr;
    }

    return mFiltFrame;
}

AVFrame* XVideoSplitScreen::operate(AVFrame* yuvFrame1, AVFrame* yuvFrame2, AVFrame* yuvFrame3, AVFrame* yuvFrame4, AVFrame* yuvFrame5, AVFrame* yuvFrame6) {
    mFiltFrame = av_frame_alloc();

    //将传入的帧设置入filtergraph
    if (av_buffersrc_add_frame_flags(mBufferSrcCtx[0], yuvFrame1, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        LOG_ERROR << "XVideoSplitScreen::operate(6) Error while feeding the filtergraph with yuvFrame1!\n";
        av_frame_free(&mFiltFrame);
        return nullptr;
    }

    if (av_buffersrc_add_frame_flags(mBufferSrcCtx[1], yuvFrame2, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        LOG_ERROR << "XVideoSplitScreen::operate(6) Error while feeding the filtergraph with yuvFrame2!\n";
        av_frame_free(&mFiltFrame);
        return nullptr;
    }

    if (av_buffersrc_add_frame_flags(mBufferSrcCtx[2], yuvFrame3, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        LOG_ERROR << "XVideoSplitScreen::operate(6) Error while feeding the filtergraph with yuvFrame3!\n";
        av_frame_free(&mFiltFrame);
        return nullptr;
    }

    if (av_buffersrc_add_frame_flags(mBufferSrcCtx[3], yuvFrame4, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        LOG_ERROR << "XVideoSplitScreen::operate(6) Error while feeding the filtergraph with yuvFrame4!\n";
        av_frame_free(&mFiltFrame);
        return nullptr;
    }

    if (av_buffersrc_add_frame_flags(mBufferSrcCtx[4], yuvFrame5, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        LOG_ERROR << "XVideoSplitScreen::operate(6) Error while feeding the filtergraph with yuvFrame5!\n";
        av_frame_free(&mFiltFrame);
        return nullptr;
    }

    if (av_buffersrc_add_frame_flags(mBufferSrcCtx[5], yuvFrame6, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        LOG_ERROR << "XVideoSplitScreen::operate(6) Error while feeding the filtergraph with yuvFrame6!\n";
        av_frame_free(&mFiltFrame);
        return nullptr;
    }


    //得到处理后的帧
    int ret = av_buffersink_get_frame(mBufferSinkCtx, mFiltFrame) ;
    if( ret < 0) {
        // printFFMPEGError(ret);
        av_frame_free(&mFiltFrame);
        return nullptr;
    }

    mFiltFrame->best_effort_timestamp = yuvFrame3->best_effort_timestamp;
    return mFiltFrame;
}

void XVideoSplitScreen::release() {
    if (mFiltFrame != nullptr) {
        av_frame_free(&mFiltFrame);
        mFiltFrame = nullptr;
    }
}

//flag 2为左右拼接，3为上下拼接，4为四路拼接，5为左上，6为右上，7为左下，8为右下 9为六路
int XVideoSplitScreen::initSplitFilter(int flag)
{
    char filtersDescr[1024] = {0};
    char specTemp[256] = {0};
    unsigned int j = 0;
    unsigned int k = 1;
    unsigned int xCoor;
    unsigned int yCoor;
    int	streamNum = 2;
    int stride = 2;
    int strideTwo =4;
    int ret = -1;

    if(flag == 4){
        streamNum = 4;
    }
    if(flag == 9){
        streamNum = 6;
        snprintf(filtersDescr, sizeof(filtersDescr), "color=c=black@1:s=%dx%d[x0];", MERGE_WIDTH, MERGE_HEIGHT);
    }
    else {
        snprintf(filtersDescr, sizeof(filtersDescr), "color=c=black@1:s=%dx%d[x0];", mInWidth, mInHeight);
    }

    int mult6Array[6][4] = {{2,2,0,0}, {2,2,2,0}, {1,1,0,2}, {1,1,1,2}, {1,1,2,2}, {1,1,3,2}};

    //通过flag参数，设置filter位置
    for (j = 0; j < streamNum; j++)
    {
        //预留特效接口
        //snprintf(specTemp, sizeof(specTemp), "[in%d]null[ine%d];", j, j);
        //strcat(filtersDescr, specTemp);
        xCoor = j % stride;
        yCoor = j / stride;
        if(flag == 4) {
            snprintf(specTemp, sizeof(specTemp),
                     "[in%d]scale=w=%d:h=%d[inn%d];[x%d][inn%d]overlay=%d*%d/%d:%d*%d/%d[x%d];", j,
                     mInWidth / stride, mInHeight / stride, j, k - 1, j,
                     mInWidth, xCoor, stride, mInHeight, yCoor, stride, k);
        }else if(flag == 2) {
            snprintf(specTemp, sizeof(specTemp),
                     "[in%d]scale=w=%d:h=%d[inn%d];[x%d][inn%d]overlay=%d*%d/%d:%d*%d/%d+%d[x%d];", j,
                     mInWidth / stride, mInHeight / stride, j, k - 1, j,
                     mInWidth, xCoor, stride, mInHeight, yCoor, stride, mInHeight/4, k);
        }else if(flag == 3) {
            snprintf(specTemp, sizeof(specTemp),
                     "[in%d]scale=w=%d:h=%d[inn%d];[x%d][inn%d]overlay=%d*%d/%d+%d:%d*%d/%d[x%d];", j,
                     mInWidth / stride, mInHeight / stride, j, k - 1, j,
                     mInWidth, yCoor, stride, mInWidth/4, mInHeight, xCoor, stride, k);
        }else if(flag == 5) {
            snprintf(specTemp, sizeof(specTemp),
                     "[in%d]scale=w=%d:h=%d[inn%d];[x%d][inn%d]overlay=%d*%d/%d:%d*%d/%d[x%d];", j,
                     (mInWidth)*j / strideTwo, (mInHeight)*j / strideTwo, j, k - 1, j,
                     mInWidth, 0, stride, mInHeight, 0, stride, k);
        }else if(flag == 6) {
            snprintf(specTemp, sizeof(specTemp),
                     "[in%d]scale=w=%d:h=%d[inn%d];[x%d][inn%d]overlay=%d*%d/%d:%d*%d/%d[x%d];", j,
                     (mInWidth)*j / stride, (mInHeight)*j / stride, j, k - 1, j,
                     mInWidth, j, stride, mInHeight, 0, stride, k);
        }else if(flag == 7) {
            snprintf(specTemp, sizeof(specTemp),
                     "[in%d]scale=w=%d:h=%d[inn%d];[x%d][inn%d]overlay=%d*%d/%d:%d*%d/%d[x%d];", j,
                     (mInWidth)*j / stride, (mInHeight)*j / stride, j, k - 1, j,
                     mInWidth, 0, stride, mInHeight, j, stride, k);
        }else if(flag == 8) {
            snprintf(specTemp, sizeof(specTemp),
                     "[in%d]scale=w=%d:h=%d[inn%d];[x%d][inn%d]overlay=%d*%d/%d:%d*%d/%d[x%d];", j,
                     (mInWidth)*j / stride, (mInHeight)*j / stride, j, k - 1, j,
                     mInWidth, j, stride, mInHeight, j, stride, k);
        } else if (flag == 9){
            snprintf(specTemp, sizeof(specTemp),
                      "[in%d]scale=w=%d:h=%d[inn%d];[x%d][inn%d]overlay=%d:%d[x%d];", j,
                      MERGE_WIDTH * mult6Array[j][0] / 4, MERGE_HEIGHT * mult6Array[j][1] / 3, j, k - 1, j,
                      MERGE_WIDTH * mult6Array[j][2] / 4, MERGE_HEIGHT * mult6Array[j][3] / 3, k);

        }
        k++;
        strcat(filtersDescr, specTemp);
    }
    snprintf(specTemp, sizeof(specTemp), "[x%d]null[out]", k - 1);
    strcat(filtersDescr, specTemp);

    LOG_INFO<<filtersDescr;
    ret = initFilter(filtersDescr,streamNum);

    return ret;
}


/* 初始化filter函数 */
int XVideoSplitScreen::initFilter(const char *filtersDescr, int streamNum)
{
    char args[512];
    char padName[10];
    int ret = 0;
    int i = 0;
    const AVFilter **buffersrc = (const AVFilter**)av_malloc(streamNum*sizeof(AVFilter*));
    const AVFilter *buffersink = NULL;

    AVFilterInOut **outputs = (AVFilterInOut**)av_malloc(streamNum*sizeof(AVFilterInOut*));
    AVFilterInOut *inputs = avfilter_inout_alloc();
    mFilterGraph = avfilter_graph_alloc();

    enum AVPixelFormat pixFmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE};
    const char* argFormat = "video_size=%dx%d:pix_fmt=0:time_base=1/30:pixel_aspect=0/1";
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
