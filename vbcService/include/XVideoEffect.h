//
// Created by jackie on 12/15/20.
//

#ifndef __VBCSERVICE_XVIDEOEFFECT_H__
#define __VBCSERVICE_XVIDEOEFFECT_H__

#define NUM_VIDEO_EFFECTS 8

class AVFrame;

class XVideoEffect {
public:
    XVideoEffect();
    ~XVideoEffect();
    virtual AVFrame* operate(AVFrame* yuvFrame) = 0;
    virtual void release() = 0;
    virtual XVideoEffect* clone() = 0;

    void setMode(int mode);
    int getMode();
private:
    int mMode;
};


#endif //__VBCSERVICE_XVIDEOEFFECT_H__
