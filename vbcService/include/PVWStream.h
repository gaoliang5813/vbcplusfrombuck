#ifndef __PVW_STREAM_H__
#define __PVW_STREAM_H__

#include <string>
#include <vector>
#include "Stream.h"
#include "XVideoEffect.h"
#include "CommonFunction.h"

class PVWStream
{
public:
    PVWStream();
    virtual ~PVWStream();

    void work();
    void cycle();

    void setLogo(bool);
    void setSubtitle(bool);
    void setSplitScreen(bool, int);

    void setSources(const std::vector<int>& streamIndex);
    void setSource(int streamIndex);

    bool isScreenSplited() const;

    AVFrame* getMergeVideoFrame();
    AVFrame* getMergeAudioFrame();
private:
    int getPVWSourceNum() const;

    void addMergeVideoFrame(AVFrame* videoFrame);
    void addMergeAudioFrame(AVFrame* audioFrame);

    void setAudioStreamIndex(int audioStreamIndex);

    void cycleOneSource();
    void cycleTwoSource();
    void cycleFourSource();
public:
    XVideoEffect* mPVWVideoEffects[NUM_VIDEO_EFFECTS] = { nullptr };
    FrameQueue mMergeVideoBufferQueue{MAX_QUEUE_SIZE};
    FrameQueue mMergeAudioBufferQueue{MAX_QUEUE_SIZE};

    int mVideoStreamIndex[MAX_PVW_SOURCE] = { -1, -1, -1, -1 };
    int mAudioStreamIndex = -1;
};

#endif /*__PVW_STREAM_H__*/
