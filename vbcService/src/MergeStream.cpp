#include "MergeStream.h"
#include "PGMStream.h"
#include "PVWStream.h"
#include "XRtmp.h"
#include "Stream.h"
#include "CommonFunction.h"
#include "XVideoSplitScreen.h"
#include "StreamManager.h"
#include "VideoCap.h"
#include "Logger.h"
#include "AudioCap.h"

extern "C" {
#include "libavutil/time.h"
#include "libavutil/frame.h"
}

MergeStream::MergeStream() {
    mXVideoSplitScreen = new XVideoSplitScreen(MONITOR_WINDOW_WIDTH, MONITOR_WINDOW_HEIGHT, 9);
    //mXVideoSplitScreen->initSplitFilter(9);

    mXRtmp = new XRtmp(MERGE_URL);
    mXRtmp->addVideoStreamLowRes(FPS);
    mXRtmp->addAudioStream();
    mXRtmp->sendHeader();
}

MergeStream::~MergeStream() {
    if (mXRtmp != nullptr) {
        delete mXRtmp;
        mXRtmp = nullptr;
    }
}

void MergeStream::setPVWStream(PVWStream *pvwStream) { mPVWStream = pvwStream; }
void MergeStream::setPGMStream(PGMStream *pgmStream) { mPGMStream = pgmStream; }
void MergeStream::setStreamList(const StreamList &streamList) { mStreamList = streamList; }

int MergeStream::work() {
    for (;;) {
        av_usleep(100);
        cycle();
    }
    return 0;
}

void MergeStream::cycle() {
    bool bPVWHaveAudio = false;
    Stream* audioStream = StreamManager::getInstance()->getStream(mPVWStream->mAudioStreamIndex);
	if(nullptr != audioStream && nullptr != audioStream->mVideoCap){
		if(audioStream->mVideoCap->isCameraType()){
			if(nullptr != audioStream->mAudioCap){
				bPVWHaveAudio = audioStream->mAudioCap->haveAudio();
			}
		}
		else{
			bPVWHaveAudio = audioStream->mVideoCap->haveAudio();
		}
	}

    if (av_compare_ts(mXRtmp->getNextVideoPts(), {1, FPS}, mXRtmp->getNextAudioPts(), {1, AAC_SAMPLE_RATE}) <= 0
        || audioStream == nullptr || !bPVWHaveAudio) {

        static AVFrame *sPGMFrame = (AVFrame *) getDefaultImageLowResolution();
        static AVFrame *sPVWFrame = (AVFrame *) getDefaultImageLowResolution();
        static AVFrame *sFrame1 = (AVFrame *) getDefaultImageLowResolution();
        static AVFrame *sFrame2 = (AVFrame *) getDefaultImageLowResolution();
        static AVFrame *sFrame3 = (AVFrame *) getDefaultImageLowResolution();
        static AVFrame *sFrame4 = (AVFrame *) getDefaultImageLowResolution();
        static int sNextVideoPts = 0;



        AVFrame *frame1 = mStreamList[0]->getMergeVideoFrame();
        if (frame1 != nullptr) {
            if (sFrame1) {
                av_frame_free(&sFrame1);
            }
            sFrame1 = av_frame_clone(frame1);
        }
        AVFrame *frame2 = mStreamList[1]->getMergeVideoFrame();
        if (frame2 != nullptr) {
            //av_frame_copy(sFrame2, frame2);
            if (sFrame2) {
                av_frame_free(&sFrame2);
            }
            sFrame2 = av_frame_clone(frame2);
        }
        AVFrame *frame3 = mStreamList[2]->getMergeVideoFrame();
        if (frame3 != nullptr) {
            //av_frame_copy(sFrame3, frame3);
            if (sFrame3) {
                av_frame_free(&sFrame3);
            }
            sFrame3 = av_frame_clone(frame3);
        }

        AVFrame *frame4 = mStreamList[3]->getMergeVideoFrame();
        if (frame4 != nullptr) {
            //av_frame_copy(sFrame4, frame4);
            if (sFrame4) {
                av_frame_free(&sFrame4);
            }
            sFrame4 = av_frame_clone(frame4);
        }
        AVFrame *pvwFrame = mPVWStream->getMergeVideoFrame();
        if (pvwFrame != nullptr) {
            //av_frame_copy(sPVWFrame, pvwFrame);
            if (sPVWFrame) {
                av_frame_free(&sPVWFrame);
            }
            sPVWFrame = av_frame_clone(pvwFrame);
        }
        AVFrame *pgmFrame = mPGMStream->getMergeVideoFrame();

        if (pgmFrame != nullptr) {
            //av_frame_copy(sPGMFrame, pgmFrame);
            if (sPGMFrame) {
                av_frame_free(&sPGMFrame);
            }
            sPGMFrame = av_frame_clone(pgmFrame);
        }
        AVFrame *realPGMFrame = (pgmFrame == nullptr) ? sPGMFrame : pgmFrame;
        AVFrame *realPVWFrame = (pvwFrame == nullptr) ? sPVWFrame : pvwFrame;
        AVFrame *realFrame1 = (frame1 == nullptr) ? sFrame1 : frame1;
        AVFrame *realFrame2 = (frame2 == nullptr) ? sFrame2 : frame2;
        AVFrame *realFrame3 = (frame3 == nullptr) ? sFrame3 : frame3;
        AVFrame *realFrame4 = (frame4 == nullptr) ? sFrame4 : frame4;
        if (!realPGMFrame || !realPVWFrame || !realFrame1 || !realFrame2 || !realFrame3 || !realFrame4) {
            if (pgmFrame) {
                av_frame_free(&pgmFrame);
            }
            if (pvwFrame) {
                av_frame_free(&pvwFrame);
            }
            if (frame1) {
                av_frame_free(&frame1);
            }
            if (frame2) {
                av_frame_free(&frame2);
            }
            if (frame3) {
                av_frame_free(&frame3);
            }
            if (frame4) {
                av_frame_free(&frame4);
            }
            return;
        }

        realPGMFrame->pts = sNextVideoPts;
        realPVWFrame->pts = sNextVideoPts;
        realFrame1->pts = sNextVideoPts;
        realFrame2->pts = sNextVideoPts;
        realFrame3->pts = sNextVideoPts;
        realFrame4->pts = sNextVideoPts;
        sNextVideoPts++;




        AVFrame *resultFrame = mXVideoSplitScreen->operate(realPGMFrame, realPVWFrame, realFrame1, realFrame2,
                                                           realFrame3, realFrame4);


        if (pgmFrame) {
            av_frame_free(&pgmFrame);
        }
        if (pvwFrame) {
            av_frame_free(&pvwFrame);
        }
        if (frame1) {
            av_frame_free(&frame1);
        }
        if (frame2) {
            av_frame_free(&frame2);
        }
        if (frame3) {
            av_frame_free(&frame3);
        }
        if (frame4) {
            av_frame_free(&frame4);
        }
        //AVFrame* resultFrame = mXVideoSplitScreen->operate(inFrame1, inFrame2, inFrame3, inFrame4, inFrame5, inFrame6);
        if (resultFrame != nullptr) {
            mXRtmp->encodeVideoAndSend(resultFrame);
            av_frame_free(&resultFrame);
        }
    }
    else {
        AVFrame* audioFrame = mPVWStream->getMergeAudioFrame();
        if (audioFrame != nullptr) {
            mXRtmp->encodeAudioAndSend(audioFrame);
            av_frame_free(&audioFrame);
        }
    }
}
