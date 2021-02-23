#ifndef __PGM_STREAM_H__
#define __PGM_STREAM_H__

#include <string>
#include <vector>
#include "Stream.h"
#include "XVideoEffect.h"
#include "CommonFunction.h"
extern "C"{
#include "libavutil/pixfmt.h"
#include "libswscale/swscale.h"
};

class XRtmp;
struct SwsContext;
struct AVCodec;
struct AVCodecContext;

class PGMStream
{
public:
    PGMStream();

    void setSources(const std::vector<int>& streamIndex);

    void setXFade(bool);
    void setLogo(bool);
    void setSubtitle(bool);
    void setSplitScreen(bool);

    bool isScreenSplited() const;

    void work();
    void cycle();

    AVFrame* getMergeVideoFrame();

    void setAudioStreamIndex(int audioStreamIndex);

private:
    int getPGMSourceNum() const;

    void addMergeVideoFrame(AVFrame* videoFrame);
    void addPGMAudioFrame(AVFrame* audioFrame);
    void scaleAndAddMergeVideoFrame(AVFrame* videoFrame);
public:
    XVideoEffect* mPGMVideoEffects[NUM_VIDEO_EFFECTS];
    FrameQueue mMergeVideoBufferQueue{MAX_QUEUE_SIZE};
    FrameQueue mPGMAudioBufferQueue{MAX_QUEUE_SIZE};

    AVFrame* mPGMSourceAudioFrame = nullptr;
    int mVideoStreamIndex[MAX_PVW_SOURCE] = { -1, -1, -1, -1 };
    int mAudioStreamIndex = -1;

    SwsContext* mVideoScaleContext = nullptr;

    AVFrame* mResultAudioFrame = nullptr;

    XRtmp* mPGMRtmp = nullptr;
    XRtmp* mPGMFile = nullptr;
};

#endif /*__PGM_STREAM_H__*/