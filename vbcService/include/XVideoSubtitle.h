//
// Created by jackie on 12/15/20.
//

#ifndef __VBCSERVICE_XVIDEOSUBTITLE_H__
#define __VBCSERVICE_XVIDEOSUBTITLE_H__

#include <string>
#include "XVideoEffect.h"
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/opt.h"
}

class XVideoSubtitle : public XVideoEffect {
public:
    XVideoSubtitle();
    ~XVideoSubtitle();

    void setText(const char* text);
    void setPosition(int pos);
    const char* getText() const;
    int getPosition() const ;

    XVideoEffect* clone();

    AVFrame* operate(AVFrame* yuvFrame);
    void release();

    int initSubtitleGraph(const char *subtitle, int pos);
    int initSubtitleGraph(const char *subtitle, int posX, int posY);
    int update(const char* subtitle, int pos);
    int updateSubtitleXY(int posX, int posY);
    int updateSubtitlePos(int pos);
    int updateSubtitleText(const char *text);
    int updateSubtitleText(int isTimeFlag);

    int convertPosModeToXY(int pos, int& posX, int& posY);
private:
    std::string mText;
    int mPos;

    int initFilter(const char* filtersDescr);
    AVFrame *mFiltFrame;
    AVFilterContext *mBufferSinkCtx;
    AVFilterContext *mBufferSrcCtx;
    AVFilterGraph *mFilterGraph;
    int mInWidth;
    int mInHeight;
    int mFontSize = 40;
};

#endif //__VBCSERVICE_XVIDEOSUBTITLE_H__
