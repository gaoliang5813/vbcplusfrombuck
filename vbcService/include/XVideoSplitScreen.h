/**
* Copyright(c) 2020, 合肥祥云汇智科技有限公司
* All rights reserved.
*
* 文件名称：XVideoSplitScreen.h
* 摘要:调用顺序：先initSplitFilter,然后addStream，最后opreate
*
* 作者: 梁元琨
* 完成日期: 2020-12-22
**/
#ifndef __X_VIDEO_SPLIT_SCREEN_H__
#define __X_VIDEO_SPLIT_SCREEN_H__

#include "XVideoEffect.h"
#include <vector>

class Stream;
class AVFrame;
class AVFilterContext;
class AVFilterGraph;
class AVFormatContext;
class AVCodecContext;

class XVideoSplitScreen : public XVideoEffect{
public:
    XVideoSplitScreen(int width, int height, int mode);
    ~XVideoSplitScreen();

    XVideoEffect* clone();

    void addStream(Stream* stream);
    AVFrame * operate(AVFrame* yuvFrame);
    AVFrame * operate(AVFrame* yuvFrame1,AVFrame* yuvFrame2);
    AVFrame * operate(AVFrame* yuvFrame1,AVFrame* yuvFrame2,AVFrame* yuvFrame3,AVFrame* yuvFrame4);
    AVFrame * operate(AVFrame* yuvFrame1,AVFrame* yuvFrame2,AVFrame* yuvFrame3,AVFrame* yuvFrame4,AVFrame* yuvFrame5,AVFrame* yuvFrame6);
    void release();
    int initSplitFilter(int flag);

private:
    std::vector<Stream*> mStreams;
    AVFrame* mFiltFrame;
    int initFilter(const char *filter_spec, int streamNum);
    AVFilterContext* mBufferSinkCtx;
    AVFilterContext** mBufferSrcCtx;
    AVFilterGraph* mFilterGraph;
    int mInWidth;
    int mInHeight;
    int mNumStreams = 0;
};


#endif /*X_VIDEO_SPLIT_SCREEN*/