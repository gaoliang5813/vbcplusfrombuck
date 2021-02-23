#include "PVWStream.h"
#include "Logger.h"
#include "StreamManager.h"
#include "XFade.h"
#include "XVideoLogo.h"
#include "XVideoSubtitle.h"
#include "XVideoSplitScreen.h"
#include "CommonFunction.h"
#include "VideoCap.h"

extern "C" {
#include "libavutil/frame.h"
}

PVWStream::PVWStream(){
    for (int i = 0; i < NUM_VIDEO_EFFECTS; ++i) {
        mPVWVideoEffects[i] = nullptr;
    }
}

PVWStream::~PVWStream() {
}

void PVWStream::setLogo(bool set) {
    if (set) {
        if (mPVWVideoEffects[LOGO] == nullptr) {
            mPVWVideoEffects[LOGO] = new XVideoLogo();
            ((XVideoLogo*)mPVWVideoEffects[LOGO])->initLogoGraph("", 0);
        }
    }
    else {
        if (mPVWVideoEffects[LOGO] != nullptr) {
            delete mPVWVideoEffects[LOGO];
            mPVWVideoEffects[LOGO] = nullptr;
        }
    }
}

void PVWStream::setSubtitle(bool set) {
    if (set) {
        if (mPVWVideoEffects[TEXT] == nullptr) {
            mPVWVideoEffects[TEXT] = new XVideoSubtitle();
            ((XVideoSubtitle*)mPVWVideoEffects[TEXT])->initSubtitleGraph("", 0);
        }
    }
    else {
        delete mPVWVideoEffects[TEXT];
        mPVWVideoEffects[TEXT] = nullptr;
    }
}

void PVWStream::setSplitScreen(bool set, int mode) {
    if (set) {
        if (mPVWVideoEffects[SPLIT_SCREEN] == nullptr) {
            mPVWVideoEffects[SPLIT_SCREEN] = new XVideoSplitScreen(MONITOR_WINDOW_WIDTH, MONITOR_WINDOW_HEIGHT, mode);
        }
    }
    else {
        if (mPVWVideoEffects[SPLIT_SCREEN] != nullptr) {
            delete mPVWVideoEffects[SPLIT_SCREEN];
            mPVWVideoEffects[SPLIT_SCREEN] = nullptr;
        }
    }
}

AVFrame* PVWStream::getMergeVideoFrame() {
    if (getPVWSourceNum() <= 0) {
        return (AVFrame*)getDefaultImageLowResolution();
    }
    AVFrame* frame = nullptr;
    mMergeVideoBufferQueue.pop(frame);
    return frame;
}

void PVWStream::work() {
    for (;;) {
        cycle();
    }
}

void PVWStream::cycle() {
    int pvwSourceNum = getPVWSourceNum();
    if (mAudioStreamIndex < 0 && (pvwSourceNum <= 0 || pvwSourceNum > MAX_PVW_SOURCE)) {
        return;
    }

    if(pvwSourceNum == 1) {
        cycleOneSource();
    }
    else if (pvwSourceNum == 2) {
        cycleTwoSource();
    }
    else if (pvwSourceNum == 4) {
        cycleFourSource();
    }
    /* push audio */
    if(mAudioStreamIndex != -1) {
        Stream* audioStream = StreamManager::getInstance()->getStream(mAudioStreamIndex);
        if(!audioStream || !audioStream->isAudioPVWSource()){
            return;
        }
        addMergeAudioFrame(audioStream->getPVWAudioFrame());
    }
}

void PVWStream::cycleOneSource() {
    Stream* stream = StreamManager::getInstance()->getStream(mVideoStreamIndex[0]);
    if(stream == nullptr || !stream->isVideoPVWSource()) {
        return;
    }
    AVFrame* tempFrame = stream->getPVWVideoFrame();
    if (tempFrame == nullptr) {
        return;
    }

    AVFrame* resultFrame = tempFrame;

    for (int i = 1; i < NUM_VIDEO_EFFECTS; ++i) {
        if (mPVWVideoEffects[i] != nullptr) {
            resultFrame = mPVWVideoEffects[i]->operate(tempFrame);
            av_frame_free(&tempFrame);
            if (resultFrame != nullptr) {
                tempFrame = resultFrame;
            }
            else {
                return;
            }
        }
    }

    addMergeVideoFrame(resultFrame);
}

void PVWStream::cycleTwoSource() {
    static int64_t sNextVideoPts = 0;
    Stream* stream1 = StreamManager::getInstance()->getStream(mVideoStreamIndex[0]);
    Stream* stream2 = StreamManager::getInstance()->getStream(mVideoStreamIndex[1]);
    if(!stream1 || !stream2) {
        return;
    }

    AVFrame* tempFrame1 = stream1->getPVWVideoFrame();
    if (tempFrame1 == nullptr) {
        //return;
    }

    AVFrame* tempFrame2 = stream2->getPVWVideoFrame();
    if (tempFrame2 == nullptr) {
        av_frame_free(&tempFrame1);
        return;
    }

    AVFrame* resultFrame = nullptr;
    if(tempFrame1 && tempFrame2) {
        XVideoSplitScreen* xVideoSplitScreen = (XVideoSplitScreen*)mPVWVideoEffects[SPLIT_SCREEN];
        if (xVideoSplitScreen == nullptr) {
            return ;
        }
        tempFrame1->pts = tempFrame2->pts = sNextVideoPts;
        sNextVideoPts++;
        if ((resultFrame = xVideoSplitScreen->operate(tempFrame1, tempFrame2)) == nullptr) {
            av_frame_free(&tempFrame1);
            av_frame_free(&tempFrame2);
           // LOG_ERROR << "Process split screen(mode 2) failed!";
            return ;
        }

        av_frame_free(&tempFrame1);
        av_frame_free(&tempFrame2);
        AVFrame* tempFrame = resultFrame;
        for (int i = 1; i < NUM_VIDEO_EFFECTS; ++i) {
            if (mPVWVideoEffects[i] != nullptr) {
                resultFrame = mPVWVideoEffects[i]->operate(tempFrame);
                av_frame_free(&tempFrame);
                if (resultFrame != nullptr) {
                    tempFrame = resultFrame;
                }
                else {
                    return;
                }
            }
        }
    }

    addMergeVideoFrame(resultFrame);
}

void PVWStream::cycleFourSource() {
    Stream* stream1 = StreamManager::getInstance()->getStream(mVideoStreamIndex[0]);
    Stream* stream2 = StreamManager::getInstance()->getStream(mVideoStreamIndex[1]);
    Stream* stream3 = StreamManager::getInstance()->getStream(mVideoStreamIndex[2]);
    Stream* stream4 = StreamManager::getInstance()->getStream(mVideoStreamIndex[3]);
    if(!stream1 || !stream2 || !stream3 || !stream4) {
        return;
    }
    AVFrame* tempFrame1 = stream1->getPVWVideoFrame();
    AVFrame* tempFrame2 = stream2->getPVWVideoFrame();
    AVFrame* tempFrame3 = stream3->getPVWVideoFrame();
    AVFrame* tempFrame4 = stream4->getPVWVideoFrame();

    AVFrame* resultFrame = nullptr;
    if(tempFrame1 && tempFrame2 && tempFrame3 && tempFrame4) {
        XVideoSplitScreen* xVideoSplitScreen = (XVideoSplitScreen*)mPVWVideoEffects[SPLIT_SCREEN];
        if (xVideoSplitScreen == nullptr) {
            return ;
        }
        if ((resultFrame = xVideoSplitScreen->operate(tempFrame1, tempFrame2, tempFrame3, tempFrame4)) == nullptr) {
            av_frame_free(&tempFrame1);
            av_frame_free(&tempFrame2);
            av_frame_free(&tempFrame3);
            av_frame_free(&tempFrame4);
            LOG_ERROR << "Process split screen(mode 4) failed!";
            return ;
        }

        // skip XVideoSplitScreen.
        AVFrame* tempFrame = resultFrame;
        for (int i = 1; i < NUM_VIDEO_EFFECTS; ++i) {
            if (mPVWVideoEffects[i] != nullptr) {
                resultFrame = mPVWVideoEffects[i]->operate(tempFrame);
                av_frame_free(&tempFrame);
                if (resultFrame != nullptr) {
                    tempFrame = resultFrame;
                }
                else {
                    return;
                }
            }
        }
    }

    addMergeVideoFrame(resultFrame);
}

void PVWStream::addMergeAudioFrame(AVFrame *audioFrame) {
    if (audioFrame != nullptr) {
        if(!mMergeAudioBufferQueue.push(audioFrame)) {
            av_frame_free(&audioFrame);
        }
    }
}

AVFrame* PVWStream::getMergeAudioFrame() {
    AVFrame* frame = nullptr;
    mMergeAudioBufferQueue.pop(frame);
    Stream* audioStream = StreamManager::getInstance()->getStream(mAudioStreamIndex);
    if (audioStream && frame) {
        return frame;
    }

    // TODO: find why this cause large time delay
    //return (AVFrame*)getSilentAudioFrame(audioStream->mVideoCap->getSampleRate());
    return frame;
}

void PVWStream::addMergeVideoFrame(AVFrame *videoFrame) {
    if (videoFrame == nullptr) {
        return;
    }
    if (videoFrame->width != MONITOR_WINDOW_WIDTH || videoFrame->height != MONITOR_WINDOW_HEIGHT) {
        LOG_ERROR << "PVWStream adding video frame with wrong size: " << videoFrame->width << "x" << videoFrame->height;
        av_frame_free(&videoFrame);
        return;
    }
    if (videoFrame != nullptr) {
        //mMergeVideoBufferQueue.push(videoFrame);
        if(!mMergeVideoBufferQueue.push(videoFrame)) {
            av_frame_free(&videoFrame);
        }
//        while (!mMergeVideoBufferQueue.push(videoFrame)) {
//            ;
//        }
    }
}

bool PVWStream::isScreenSplited() const { return mPVWVideoEffects[SPLIT_SCREEN] != nullptr; }

void PVWStream::setAudioStreamIndex(int audioStreamIndex){
    mAudioStreamIndex = audioStreamIndex;
};

void PVWStream::setSources(const std::vector<int> &streamIndex) {
    for (int i = 0; i < MAX_PVW_SOURCE; ++i) {
        mVideoStreamIndex[i] = -1;
    }

    for (int i = 0; i < streamIndex.size() && i < MAX_PVW_SOURCE; ++i) {
        mVideoStreamIndex[i] = streamIndex.at(i);
    }
    setAudioStreamIndex(streamIndex[0]);
}

void PVWStream::setSource(int streamIndex) {
    for (int i = 0; i < MAX_PVW_SOURCE; ++i) {
        mVideoStreamIndex[i] = -1;
    }
    mVideoStreamIndex[0] = streamIndex;
    setAudioStreamIndex(streamIndex);
}

int PVWStream::getPVWSourceNum() const {
    int numPVWSources = 0;
    for (int i = 0; i < MAX_PVW_SOURCE; ++i) {
        if (mVideoStreamIndex[i] != -1) {
            ++numPVWSources;
        }
    }
    return numPVWSources;
}

