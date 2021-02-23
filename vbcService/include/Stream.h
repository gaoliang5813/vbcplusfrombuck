//
// Created by jackie on 12/15/20.
//

#ifndef __VBCSERVICE_STREAM_H__
#define __VBCSERVICE_STREAM_H__

#include <vector>
#include <mutex>
#include "boost/lockfree/spsc_queue.hpp"
#include <thread>
class VideoCap;
class AudioCap;
class XRtmp;
struct AVFrame;

const int MAX_QUEUE_SIZE = 100;
typedef boost::lockfree::spsc_queue<AVFrame*> FrameQueue;

class Stream {
public:
    Stream(const char* url);
    Stream(const char* cam, const char* audio, int streamIndex);
    virtual ~Stream();

    // Stream�Ĺ����߳�
    int work();
    int workAudio();

    bool isVideoPVWSource() const;
    bool isVideoPGMSource() const;
    bool isAudioPVWSource() const;
    bool isAudioPGMSource() const;

    void setVideoPVWSource(bool isVideoPVWSource);
    void setVideoPGMSource(bool isVideoPGMSource);
    void setAudioPVWSource(bool isAudioPVWSource);
    void setAudioPGMSource(bool isAudioPGMSource);

    AVFrame* getMergeVideoFrame();
    AVFrame* getPVWVideoFrame();
    AVFrame* getPGMVideoFrame();
    AVFrame* getPVWAudioFrame();
    AVFrame* getPGMAudioFrame();

    bool isCameraType() const;

    VideoCap* mVideoCap = nullptr;
    AudioCap* mAudioCap = nullptr;
private:
    Stream() = delete;

    void cycle();
    void workAudioCycle();

private:
    FrameQueue mAudioBufferQueue{MAX_QUEUE_SIZE};
    FrameQueue mMergeVideoBufferQueue{MAX_QUEUE_SIZE}; // ���Merge��������Ƶ֡.
    std::vector<AVFrame*> mMergeVideoBufferVector;
    std::mutex mMutex;
    FrameQueue mPVWVideoBufferQueue{MAX_QUEUE_SIZE}; // ���PVW��������Ƶ֡.
    FrameQueue mPGMVideoBufferQueue{MAX_QUEUE_SIZE}; // ���PGM��������Ƶ֡.

    FrameQueue mPVWAudioFrameQueue{MAX_QUEUE_SIZE}; // ���PVW��Ƶ֡.
    FrameQueue mPGMAudioFrameQueue{MAX_QUEUE_SIZE}; // ���PGM��Ƶ֡.

    bool mIsVideoPVWSource = false;
    bool mIsVideoPGMSource = false;
    bool mIsAudioPVWSource = false;
    bool mIsAudioPGMSource = false;

    XRtmp* mFile = nullptr;
    std::thread* mAudioThread = nullptr;
};

#endif //__VBCSERVICE_STREAM_H__
