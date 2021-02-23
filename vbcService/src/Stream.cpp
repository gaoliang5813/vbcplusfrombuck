//
// Created by jackie on 12/15/20.
//
#include <thread>
#include "Stream.h"
#include "VideoCap.h"
#include "AudioCap.h"
#include "Logger.h"
#include "XRtmp.h"
#include "CommonFunction.h"
extern "C" {
#include "libavutil/time.h"
#include "libavutil/frame.h"
}

Stream::Stream(const char* url) {
    mVideoCap = new VideoCap(url);
}

Stream::Stream(const char* cam, const char* audio, int streamIndex) {
    mVideoCap = new VideoCap(cam, nullptr);
    if(audio != nullptr) {
        mAudioCap = new AudioCap(audio);
    }
    char fileName[64] = {0};
    snprintf(fileName, sizeof(fileName), "stream_%d_output.mp4", streamIndex);
    mFile = new XRtmp(fileName);
    mFile->addVideoStreamHighRes(FPS);
    mFile->addAudioStream();
    mFile->sendHeader();

    if (isCameraType()) {
        mVideoCap->mMtcnn = new mtcnn(mVideoCap->getHeight(), mVideoCap->getWidth());
    }

}

Stream::~Stream() {
    if (mVideoCap != nullptr) {
        delete mVideoCap;
        mVideoCap = nullptr;
    }
}

int Stream::workAudio() {
    for (;;) {
        workAudioCycle();
    }
}

void Stream::workAudioCycle() {
    int ret = mAudioCap->grab();

    if (ret == 1) {
        AVFrame *frame1 = av_frame_alloc();
        AVFrame *frame = nullptr;
        AVFrame *capturedFrame = mAudioCap->grabCapturedMicAudioFrame();

        frame = mAudioCap->resample(capturedFrame);

        if(frame != nullptr) {

            frame1 = av_frame_clone(frame);

//        frame1 = av_frame_clone(capturedFrame);
            if (isAudioPVWSource()) {
                if (frame1 != nullptr) {
                    LOG_INFO << " push mic  frame->nb_samples = " << frame1->nb_samples;
                    if (!mPVWAudioFrameQueue.push(frame1)) {
                        av_frame_free(&frame1);
                    }
                }
            }
//        av_frame_free(&frame1);
            if (isAudioPGMSource()) {
                if (frame != nullptr) {
                    if (!mPGMAudioFrameQueue.push(frame)) {
                        av_frame_free(&frame);
                    }
                }
            }
            //av_frame_free(&capturedFrame);
        }

    }

}

int Stream::work() {

    if(isCameraType()){
        mAudioThread = new std::thread(&Stream::workAudio, this);
    }

    for (;;) {
        if (mVideoCap->isFileType()) {
            av_usleep(5000);
        }
        cycle();

    }
    return 0;
}


void Stream::cycle() {
    int ret = mVideoCap->grab();
    AVFrame* scaledFrame = nullptr;

    if(!mVideoCap->isCameraType()){
        av_usleep(3333);
    }

    if (ret == 0) {
        AVFrame* capturedFrame = mVideoCap->grabCapturedVideoFrame();
        if ((scaledFrame = mVideoCap->scale(capturedFrame)) == nullptr) {
            av_frame_unref(capturedFrame);
            return;
        }

        if (isVideoPGMSource()) {
            AVFrame* upFrame = nullptr;
            upFrame = mVideoCap->upscale(capturedFrame);
            if(!mPGMVideoBufferQueue.push(upFrame)) {
                av_frame_free(&upFrame);
            }
        }

       if (isVideoPVWSource()) {
            AVFrame* pvwFrame = nullptr;
            //av_frame_make_writable(pvwFrame);
            if ((pvwFrame = av_frame_clone(scaledFrame)) != nullptr) {
                if(!mPVWVideoBufferQueue.push(pvwFrame)) {
                    av_frame_free(&pvwFrame);
                }

            }
        }

        if(!mMergeVideoBufferQueue.push(scaledFrame)) {
            av_frame_free(&scaledFrame);
        }
    }
    else if (ret == 1) {
        AVFrame* frame1 = nullptr;
        AVFrame* frame = nullptr;
        AVFrame* capturedFrame = mVideoCap->grabCapturedAudioFrame();
        frame1 = av_frame_clone(capturedFrame);
        if (isAudioPVWSource()) {
            if ((frame = mVideoCap->resample(frame1)) != nullptr) {
                if(!mPVWAudioFrameQueue.push(frame)) {
                    av_frame_free(&frame);
                }
            }
        }
        av_frame_free(&frame1);
        if (isAudioPGMSource()) {
            if ((frame = mVideoCap->resample(capturedFrame)) != nullptr) {
                if(!mPGMAudioFrameQueue.push(frame)) {
                    av_frame_free(&frame);
                }
            }
        }
    }
}

AVFrame* Stream::getMergeVideoFrame() {
    if (!mVideoCap->haveVideo()) {
        return (AVFrame*)getDefaultImageLowResolution();
    }

    if (mVideoCap->isFileType() && mVideoCap->reachEOF()) {
        return (AVFrame*)getDefaultImageLowResolution();
    }

    AVFrame* frame = nullptr;
    mMergeVideoBufferQueue.pop(frame);
    return frame;
}

AVFrame* Stream::getPVWVideoFrame() {
    if (!isVideoPVWSource()) {
        LOG_ERROR << "Stream-" << mVideoCap->getCameraUrl() << " is not PVW video Source!";
        return nullptr;
    }
    AVFrame* frame = nullptr;
    mPVWVideoBufferQueue.pop(frame);
    return frame;
}

AVFrame* Stream::getPGMVideoFrame() {
    if (!mVideoCap->haveVideo()) {
        return (AVFrame*)getDefaultImageHighResolution();
    }
    if (mPGMVideoBufferQueue.empty()) {
        //LOG_ERROR << "Stream-" << mVideoCap->getCameraUrl() << " PGMVideoBuffer is null!";
        return nullptr;
    }
    if (!isVideoPGMSource()) {
        LOG_ERROR << "Stream-" << mVideoCap->getCameraUrl() << " is not PGM video Source!";
        return nullptr;
    }
    AVFrame* frame = nullptr;
    mPGMVideoBufferQueue.pop(frame);
    return frame;
}

AVFrame* Stream::getPVWAudioFrame() {
    if (!isAudioPVWSource()) {
        LOG_ERROR << "Stream-" << mVideoCap->getCameraUrl() << " is not PVW audio Source!";
        return nullptr;
    }
    if (mVideoCap->isFileType() && mVideoCap->reachEOF()) {
        return (AVFrame*)getSilentAudioFrame(mVideoCap->getSampleRate());
    }
    AVFrame* frame = nullptr;
    mPVWAudioFrameQueue.pop(frame);
    return frame;
}

AVFrame* Stream::getPGMAudioFrame() {
    if (mPGMAudioFrameQueue.empty()) {
        return nullptr;
    }
    if (!isAudioPGMSource()) {
        LOG_ERROR << "Stream-" << mVideoCap->getCameraUrl() << " is not PGM audio Source!";
        return nullptr;
    }
    AVFrame* frame = nullptr;
    mPGMAudioFrameQueue.pop(frame);
    //while (!mPGMAudioFrameQueue.pop(frame)) {
        //;
    //}
    return frame;
}

bool Stream::isCameraType() const {
    return mVideoCap->isCameraType();
}

bool Stream::isVideoPVWSource() const { return mIsVideoPVWSource; }
bool Stream::isVideoPGMSource() const { return mIsVideoPGMSource; }
bool Stream::isAudioPVWSource() const { return mIsAudioPVWSource; }
bool Stream::isAudioPGMSource() const { return mIsAudioPGMSource; }

void Stream::setVideoPVWSource(bool isVideoPVWSource) {
    mIsVideoPVWSource = isVideoPVWSource;
    if (!isVideoPVWSource) {
        AVFrame* frame = nullptr;
        while (mPVWVideoBufferQueue.pop(frame)) {
            av_frame_free(&frame);
        }
    }
}

void Stream::setVideoPGMSource(bool isVideoPGMSource) {
    mIsVideoPGMSource = isVideoPGMSource;
    if (!isVideoPGMSource) {
        AVFrame* frame = nullptr;
        while (mPGMVideoBufferQueue.pop(frame)) {
            av_frame_free(&frame);
        }
    }
}

void Stream::setAudioPVWSource(bool isAudioPVWSource) {
    mIsAudioPVWSource = isAudioPVWSource;
    if (!isAudioPVWSource) {
        AVFrame* frame = nullptr;
        while (mPVWAudioFrameQueue.pop(frame)) {
            av_frame_free(&frame);
        }
    }
}

void Stream::setAudioPGMSource(bool isAudioPGMSource) {
    mIsAudioPGMSource = isAudioPGMSource;
    if (!isAudioPGMSource) {
        AVFrame* frame = nullptr;
        while (mPGMAudioFrameQueue.pop(frame)) {
            av_frame_free(&frame);
        }
    }
}
