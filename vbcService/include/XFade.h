//
// Created by jackie on 12/15/20.
//

#ifndef __VBCSERVICE_XFADE_H__
#define __VBCSERVICE_XFADE_H__

#include "XVideoEffect.h"
#include <vector>

class Stream;
class AVFrame;
class AVFilterContext;
class AVFilterGraph;
class AVFormatContext;
class AVCodecContext;

class XFade : public XVideoEffect{
public:
    XFade();
    ~XFade();

    XVideoEffect* clone();

    void addStream(Stream* stream);
    AVFrame * operate(AVFrame* yuvFrame);
    AVFrame * operate(AVFrame* yuvFrame1,AVFrame* yuvFrame2);
    AVFrame * operate(AVFrame* yuvFrame1,AVFrame* yuvFrame2,AVFrame* yuvFrame3,AVFrame* yuvFrame4);
    void release();
    int initXfadeFilter(int flag);

private:
    std::vector<Stream*> mStreams;
    AVFrame* mFiltFrame;
    int initFilter(const char *filter_spec, int streamNum);
    AVFilterContext* mBufferSinkCtx;
    AVFilterContext** mBufferSrcCtx;
    AVFilterGraph* mFilterGraph;
    int mInWidth;
    int mInHeight;
};


#endif //__VBCSERVICE_XFADE_H__
