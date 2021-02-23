//
// Created by jackie on 12/15/20.
//

#ifndef __VBCSERVICE_XVIDEOLOGO_H__
#define __VBCSERVICE_XVIDEOLOGO_H__

#include <string>
#include "opencv2/core.hpp"
#include "XVideoEffect.h"

class AVFrame;
class AVFilterContext;
class AVFilterGraph;

class XVideoLogo : public XVideoEffect {
public:

    XVideoLogo();
    ~XVideoLogo();

    void setImageFile(const char* imageFile);
    void setImageMat(const cv::Mat* imageMat);
    void setImagePos(int pos);
    const char* getImageFile() const;
    const cv::Mat* getImageMat() const;
    int getPosition() const;

    XVideoEffect* clone();

    AVFrame* operate(AVFrame* yuvFrame);
    void release();
    int initLogoGraph(const char *picturename, int pos);
    int initLogoGraph(const char *picturename, int posX, int posY);
    int convertPosModeToXY(int pos, int& posX, int& posY);
private:
    std::string mImageFile = "../resources/kxwellLogo.png";
    const cv::Mat* mImageMat;
    int mPos;

    int initFilter(const char* filtersDescr);
    AVFrame* mFiltFrame;
    AVFilterContext* mBufferSinkCtx;
    AVFilterContext* mBufferSrcCtx;
    AVFilterGraph* mFilterGraph;
    int mInWidth;
    int mInHeight;

};


#endif //__VBCSERVICE_XVIDEOLOGO_H__
