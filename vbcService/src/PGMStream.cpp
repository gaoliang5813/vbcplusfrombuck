#include "PGMStream.h"
#include "Logger.h"
#include "VideoCap.h"
#include "AudioCap.h"
#include "XRtmp.h"
#include "XFade.h"
#include "XVideoLogo.h"
#include "XVideoSubtitle.h"
#include "XVideoSplitScreen.h"
#include "StreamManager.h"

PGMStream::PGMStream(){
    for (int i = 0; i < NUM_VIDEO_EFFECTS; ++i) {
        mPGMVideoEffects[i] = nullptr;
    }
    mVideoScaleContext = sws_getContext(PGM_WIDTH, PGM_HEIGHT, AV_PIX_FMT_YUV420P,
                   MONITOR_WINDOW_WIDTH, MONITOR_WINDOW_HEIGHT, AV_PIX_FMT_YUV420P,
                   SWS_BICUBIC, 0, 0, 0);;

    mPGMSourceAudioFrame = av_frame_alloc();
    mPGMRtmp = new XRtmp(PGM_URL);
    mPGMRtmp->addVideoStreamHighRes(FPS);
    mPGMRtmp->addAudioStream();
    mPGMRtmp->sendHeader();


//    mPGMFile = new XRtmp("output.mp4");
//    mPGMFile->addVideoStreamHighRes(FPS);
//    mPGMFile->addAudioStream();
//    mPGMFile->sendHeader();
}

void PGMStream::setXFade(bool set) {
    if (set) {
        if (mPGMVideoEffects[XFADE] == nullptr) {
            mPGMVideoEffects[XFADE] = new XFade();
        }
    }
    else {
        if (mPGMVideoEffects[XFADE] != nullptr) {
            delete mPGMVideoEffects[XFADE];
            mPGMVideoEffects[XFADE] = nullptr;
        }
    }
}

void PGMStream::setLogo(bool set) {
    if (set) {
        if (mPGMVideoEffects[LOGO] == nullptr) {
            mPGMVideoEffects[LOGO] = new XVideoLogo();
        }
    }
    else {
        if (mPGMVideoEffects[LOGO] != nullptr) {
            delete mPGMVideoEffects[LOGO];
            mPGMVideoEffects[LOGO] = nullptr;
        }
    }
}

void PGMStream::setSubtitle(bool set) {
    if (set) {
        if (mPGMVideoEffects[TEXT] == nullptr) {
            mPGMVideoEffects[TEXT] = new XVideoSubtitle();
        }
    }
    else {
        if (mPGMVideoEffects[TEXT] != nullptr) {
            delete mPGMVideoEffects[TEXT];
            mPGMVideoEffects[TEXT] = nullptr;
        }
    }
}

int PGMStream::getPGMSourceNum() const {
    int numPGMSources = 0;
    for (int i = 0; i < MAX_PVW_SOURCE; ++i) {
        if (mVideoStreamIndex[i] != -1) {
            ++numPGMSources;
        }
    }
    return numPGMSources;
}

AVFrame* PGMStream::getMergeVideoFrame() {
    if (getPGMSourceNum() <= 0) {
        return (AVFrame*)getDefaultImageLowResolution();
    }
    AVFrame* frame = nullptr;
    mMergeVideoBufferQueue.pop(frame);
    return frame;
}

void PGMStream::scaleAndAddMergeVideoFrame(AVFrame *videoFrame) {
    if (videoFrame == nullptr) {
        return;
    }

    AVFrame* scaledFrame = av_frame_alloc();
    memset(scaledFrame, 0, sizeof(*scaledFrame));
    scaledFrame->format = AV_PIX_FMT_YUV420P;
    scaledFrame->width = MONITOR_WINDOW_WIDTH;
    scaledFrame->height = MONITOR_WINDOW_HEIGHT;
    av_frame_get_buffer(scaledFrame, 0);
    int h = sws_scale(mVideoScaleContext, videoFrame->data, videoFrame->linesize, 0, PGM_HEIGHT,
                      scaledFrame->data, scaledFrame->linesize);
    if (h <= 0) {
        av_frame_free(&scaledFrame);
        return;
    }
    addMergeVideoFrame(scaledFrame);
}

void PGMStream::addMergeVideoFrame(AVFrame *videoFrame) {
    if (videoFrame != nullptr) {
        if(!mMergeVideoBufferQueue.push(videoFrame)) {
            av_frame_free(&videoFrame);
        }
    }
}

void PGMStream::work() {
    for (;;) {
        cycle();
    }
}

void PGMStream::cycle() {

    int pgmSourceNum = getPGMSourceNum();
    if (pgmSourceNum <= 0 || pgmSourceNum > MAX_PVW_SOURCE) {
        return ;
    }

    Stream *audioStream = StreamManager::getInstance()->getStream(mAudioStreamIndex);
    bool bPGMHaveAudio = false;
    if(nullptr != audioStream && nullptr != audioStream->mVideoCap){
	    if(audioStream->mVideoCap->isCameraType()){
	    	if(nullptr != audioStream->mAudioCap){
			    bPGMHaveAudio = audioStream->mAudioCap->haveAudio();
	    	}
	    }
	    else{
		    bPGMHaveAudio = audioStream->mVideoCap->haveAudio();
	    }
    }

    static int64_t sNextVideoPts = 0;
    if (av_compare_ts(mPGMRtmp->getNextVideoPts(), {1, FPS}, mPGMRtmp->getNextAudioPts(), {1, AAC_SAMPLE_RATE}) <= 0
    || audioStream == nullptr || !bPGMHaveAudio) {
        AVFrame *resultFrame = nullptr;
        AVFrame* tempFrame = nullptr;
        if (pgmSourceNum == 1) {
            Stream *stream = StreamManager::getInstance()->getStream(mVideoStreamIndex[0]);
            if (!stream) {
                return;
            }

            resultFrame = stream->getPGMVideoFrame();
            if (resultFrame == nullptr) {
                return;
            }


            tempFrame = resultFrame;
            for (int i = 1; i < NUM_VIDEO_EFFECTS; ++i) {
                if (mPGMVideoEffects[i] != nullptr) {
                    resultFrame = mPGMVideoEffects[i]->operate(tempFrame);
                    av_frame_free(&tempFrame);
                    if (resultFrame != nullptr) {
                        tempFrame = resultFrame;
                    }
                    else {
                        return;
                    }
                }
            }
        } else if (pgmSourceNum == 2) {
            //static int64_t sNextVideoPts = 0;
            Stream *stream1 = StreamManager::getInstance()->getStream(mVideoStreamIndex[0]);
            Stream *stream2 = StreamManager::getInstance()->getStream(mVideoStreamIndex[1]);
            if (!stream1 || !stream2) {
                return;
            }
            AVFrame *tempFrame1 = stream1->getPGMVideoFrame();
            if (tempFrame1 == nullptr) {
                return;
            }
            AVFrame *tempFrame2 = stream2->getPGMVideoFrame();
            if (tempFrame2 == nullptr) {
                av_frame_free(&tempFrame1);
                return;
            }

            if (tempFrame1 && tempFrame2) {
                tempFrame1->pts = tempFrame2->pts = sNextVideoPts;
                ++sNextVideoPts;
                XVideoSplitScreen *xVideoSplitScreen = (XVideoSplitScreen *) mPGMVideoEffects[SPLIT_SCREEN];
                if (xVideoSplitScreen == nullptr) {
                    return;
                }
                resultFrame = xVideoSplitScreen->operate(tempFrame1, tempFrame2);
                if (resultFrame == nullptr) {
                    av_frame_free(&tempFrame1);
                    av_frame_free(&tempFrame2);
                    return;
                }

                av_frame_free(&tempFrame1);
                av_frame_free(&tempFrame2);

                tempFrame = resultFrame;
                for (int i = 1; i < NUM_VIDEO_EFFECTS; ++i) {
                    if (mPGMVideoEffects[i] != nullptr) {
                        resultFrame = mPGMVideoEffects[i]->operate(tempFrame);
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
        } else if (pgmSourceNum == 4) {
            Stream *stream1 = StreamManager::getInstance()->getStream(mVideoStreamIndex[0]);
            Stream *stream2 = StreamManager::getInstance()->getStream(mVideoStreamIndex[1]);
            Stream *stream3 = StreamManager::getInstance()->getStream(mVideoStreamIndex[2]);
            Stream *stream4 = StreamManager::getInstance()->getStream(mVideoStreamIndex[3]);
            if (!stream1 || !stream2 || !stream3 || !stream4) {
                return;
            }
            AVFrame *tempFrame1 = stream1->getPGMVideoFrame();
            if (tempFrame1 == nullptr) {
                return;
            }
            AVFrame *tempFrame2 = stream2->getPGMVideoFrame();
            if (tempFrame2 == nullptr) {
                av_frame_free(&tempFrame1);
                return;
            }
            AVFrame *tempFrame3 = stream3->getPGMVideoFrame();
            if (tempFrame3 == nullptr) {
                av_frame_free(&tempFrame1);
                av_frame_free(&tempFrame2);
                return;
            }
            AVFrame *tempFrame4 = stream4->getPGMVideoFrame();
            if (tempFrame4 == nullptr) {
                av_frame_free(&tempFrame1);
                av_frame_free(&tempFrame2);
                av_frame_free(&tempFrame3);
                return;
            }

            if (tempFrame1 && tempFrame2 && tempFrame3 && tempFrame4) {
                XVideoSplitScreen *xVideoSplitScreen = (XVideoSplitScreen *) mPGMVideoEffects[SPLIT_SCREEN];
                if (xVideoSplitScreen == nullptr) {
                    return;
                }
                if ((resultFrame = xVideoSplitScreen->operate(tempFrame1, tempFrame2, tempFrame3, tempFrame4)) ==
                    nullptr) {
                    av_frame_free(&tempFrame1);
                    av_frame_free(&tempFrame2);
                    av_frame_free(&tempFrame3);
                    av_frame_free(&tempFrame4);
                    return;
                }
                av_frame_free(&tempFrame1);
                av_frame_free(&tempFrame2);
                av_frame_free(&tempFrame3);
                av_frame_free(&tempFrame4);

                tempFrame = resultFrame;
                for (int i = 1; i < NUM_VIDEO_EFFECTS; ++i) {
                    if (mPGMVideoEffects[i] != nullptr) {
                        resultFrame = mPGMVideoEffects[i]->operate(tempFrame);
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
        }
        // TODO: send before scale
        AVFrame* resultFileFrame = nullptr;
        resultFileFrame = av_frame_clone(resultFrame);
        scaleAndAddMergeVideoFrame(resultFrame);

        mPGMRtmp->encodeVideoAndSend(resultFrame);

	// TODO: file saving strategy.
//        mPGMFile->encodeVideoAndSend(resultFileFrame);
	    av_frame_free(&resultFrame);
	    av_frame_free(&resultFileFrame);
    }
    else {
        /* push audio */
        if (mAudioStreamIndex != -1) {
            AVFrame *audioFrame = audioStream->getPGMAudioFrame();
            if (!audioStream || audioFrame == nullptr) {
                av_frame_free(&audioFrame);
                return;
            }
            AVFrame* audioFileFrame = nullptr;
            audioFileFrame = av_frame_clone(audioFrame);
            mPGMRtmp->encodeAudioAndSend(audioFrame);
//            mPGMFile->encodeAudioAndSend(audioFileFrame);
            av_frame_free(&audioFrame);
            av_frame_free(&audioFileFrame);
            //addPGMAudioFrame(audioStream->getPGMAudioFrame());
        }
    }
}

void PGMStream::addPGMAudioFrame(AVFrame *audioFrame) {
    if (audioFrame != nullptr) {
        while (!mPGMAudioBufferQueue.push(audioFrame)) {
            ;
        }
    }
}

void PGMStream::setAudioStreamIndex(int audioStreamIndex){
    mAudioStreamIndex = audioStreamIndex;
};

void PGMStream::setSplitScreen(bool set) {
    if (set) {
        if (mPGMVideoEffects[SPLIT_SCREEN] == nullptr) {
            mPGMVideoEffects[SPLIT_SCREEN] = new XVideoSplitScreen(PGM_WIDTH, PGM_HEIGHT, 2);
        }
    }
    else {
        if (mPGMVideoEffects[SPLIT_SCREEN] != nullptr) {
            delete mPGMVideoEffects[SPLIT_SCREEN];
            mPGMVideoEffects[SPLIT_SCREEN] = nullptr;
        }
    }
}

void PGMStream::setSources(const std::vector<int> &streamIndex) {
    for (int i = 0; i < MAX_PVW_SOURCE; ++i) {
        mVideoStreamIndex[i] = -1;
    }

    for (int i = 0; i < streamIndex.size() && i < MAX_PVW_SOURCE; ++i) {
        mVideoStreamIndex[i] = streamIndex.at(i);
    }
}

bool PGMStream::isScreenSplited() const { return mPGMVideoEffects[SPLIT_SCREEN] != nullptr; }

