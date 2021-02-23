//
// Created by jackie on 12/15/20.
//

#include "StreamManager.h"
#include "Logger.h"
#include "XVideoSubtitle.h"
#include "XVideoLogo.h"
#include "XVideoSplitScreen.h"
#include "XFade.h"
#include <thread>
#include "PVWStream.h"
#include "PGMStream.h"
#include "MergeStream.h"

StreamManager* StreamManager::mInstance = nullptr;

StreamManager::StreamManager():mPVWSourceIndex(-1), mPGMSourceIndex(-1) {
    mPVWStream = new PVWStream();
    mPGMStream = new PGMStream();
    mMergeStream = new MergeStream();
}

void StreamManager::updateSubtitle(const char *text, int pos) {
    mPVWStream->setSubtitle(true);
    XVideoSubtitle* videoSubtitle = (XVideoSubtitle*)mPVWStream->mPVWVideoEffects[TEXT];
    videoSubtitle->update(text, pos);
}

void StreamManager::updateLogo(const char *logoFile, int pos) {
    mPVWStream->setLogo(true);
}

void StreamManager::updateSplitScreen(const vector<int> streamSrcList, int mode) {
    setStreamsToPVW(streamSrcList);
	mPVWStream->setSplitScreen(true, mode);
	//XVideoSplitScreen* videoSplitScreen = (XVideoSplitScreen*)mPVWStream->mPVWVideoEffects[SPLIT_SCREEN];
	//videoSplitScreen->setMode(mode);

	//videoSplitScreen->initSplitFilter(mode);
}

int StreamManager::initStreams(const char* cameras[], const char* audioDevices[], int numStreams) {
    if (cameras == nullptr || numStreams <= 0) {
        return -1;
    }
    for (int i = 0; i < numStreams; ++i) {
        if (initStream(cameras[i], audioDevices[i],i) != 0) {
            return -1;
        }
    }

    return 0;
}

int StreamManager::initStream(const char* camera, const char* audioDevice, int streamIndex) {
    if (camera == nullptr) {
        LOG_ERROR << "Camera or RTMP URL unspecified!";
        return -1;
    }

    Stream* stream = new Stream(camera, audioDevice, streamIndex);
    if (stream == nullptr) {
        LOG_ERROR << "Cannot allocate Stream()!";
        return -1;
    }

    mStreamList.push_back(stream);

    return 0;
}

StreamManager* StreamManager::getInstance() {
    if (mInstance == nullptr) {
        mInstance = new StreamManager();
    }

    return mInstance;
}

void StreamManager::startStreams() {
    mMergeStream->setPGMStream(mPGMStream);
    mMergeStream->setPVWStream(mPVWStream);
    mMergeStream->setStreamList(mStreamList);

    int numStreams = mStreamList.size();
    std::thread* threads = new std::thread[numStreams+3];
    char threadDescription[100] = { 0 };
    const char* descFormat = "streamThread%d";
    for (int i = 0; i < numStreams; ++i) {
        snprintf(threadDescription, sizeof(threadDescription), descFormat, i);
        threads[i] = std::thread(&Stream::work, mStreamList[i]);
        pthread_setname_np(threads[i].native_handle(), threadDescription);
    }

    threads[numStreams] = std::thread(&PVWStream::work, mPVWStream);
    pthread_setname_np(threads[numStreams].native_handle(), "PVWStream::work");
    threads[numStreams + 1] = std::thread(&PGMStream::work, mPGMStream);
    pthread_setname_np(threads[numStreams+1].native_handle(), "PGMStream::work");

    threads[numStreams + 2] = std::thread(&MergeStream::work, mMergeStream);
    pthread_setname_np(threads[numStreams+2].native_handle(), "MergeStream::work");


    for (int i = 0; i < numStreams + 3; ++i) {
        if (threads[i].joinable()) {
            threads[i].join();
        }
    }

#if 0
        if (packetNum % 200 == 0) {
            stream->mXVideoSubtitle->updateSubtitleText("My name is Jackie.");
            stream->mXVideoSubtitle->updateSubtitleXY(rand() % 400, rand() % 400);
        }
#endif
}

void StreamManager::switchPVWToPGM(){
    if (mPVWSrcIndexList.empty() && mPGMSrcIndexList.empty()) {
        if (mPVWSourceIndex == mPGMSourceIndex) {
            return;
        }
    }
    for (int i = 0; i < NUM_VIDEO_EFFECTS; ++i) {
        if (mPGMStream->mPGMVideoEffects[i] != nullptr) {
            delete mPGMStream->mPGMVideoEffects[i];
            mPGMStream->mPGMVideoEffects[i] = nullptr;
        }
        if (mPVWStream->mPVWVideoEffects[i] != nullptr) {
            mPGMStream->mPGMVideoEffects[i] = mPVWStream->mPVWVideoEffects[i]->clone();
        }
    }
    mPGMStream->mAudioStreamIndex = mPVWStream->mAudioStreamIndex;
//    mStreamList[mPVWStream->mAudioStreamIndex]->setAudioPGMSource(true);
//    mStreamList[mPVWStream->mAudioStreamIndex]->setAudioPVWSource(false);


    for (int i = 0; i < MAX_PVW_SOURCE; ++i) {
        mPGMStream->mVideoStreamIndex[i] = -1;
    }
    if(!mPGMSrcIndexList.empty()){//Previous is Split Screen PGM stream
	    if(!mPVWSrcIndexList.empty())//Need to switch Split Screen PVW stream to PGM now
        {
		    vector<int> orgPGMSrcIndexList(mPGMSrcIndexList);
		    vector<int>().swap(mPGMSrcIndexList);
	    	for(auto streamIndex : mPVWSrcIndexList){

			    mPGMSrcIndexList.push_back(streamIndex);

			    if(orgPGMSrcIndexList.empty()){
			    	LOG_ERROR << "Logic failed, orgPGMSrcIndexList is empty";
			    }

	    		vector<int>::iterator ret;
	    		ret = find(orgPGMSrcIndexList.begin(), orgPGMSrcIndexList.end(), streamIndex);
	    		if(ret != orgPGMSrcIndexList.end())
			    {
				    orgPGMSrcIndexList.erase(ret);
				    continue;
			    }
	    		else{
	    			if(nullptr == mStreamList[streamIndex]){
	    				LOG_ERROR << "Logic Failed, mStreamList have no stream: " << streamIndex;
	    			}

				    mStreamList[streamIndex]->setVideoPGMSource(true);
	    		}
	    	}

	    	for(auto streamIndex : orgPGMSrcIndexList){
			    if(nullptr == mStreamList[streamIndex]){
				    LOG_ERROR << "Logic Failed, orgPGMSrcIndexList have no stream: " << streamIndex;
			    }

			    mStreamList[streamIndex]->setVideoPGMSource(false);
	    	}
	    }
	    else//Need to switch single PVW stream
	    {
	    	bool bIncludeSinglePVW = false;
	    	for(auto streamIndex : mPGMSrcIndexList){
	    		if(streamIndex == mPVWSourceIndex){
				    bIncludeSinglePVW = true;
				    continue;
	    		}

	    		if(nullptr != mStreamList[streamIndex]){
				    mStreamList[streamIndex]->setVideoPGMSource(false);
				    for (int i = 0; i < MAX_PVW_SOURCE; ++i) {
				         if (mPGMStream->mVideoStreamIndex[i] == streamIndex) {
				             mPGMStream->mVideoStreamIndex[i] = -1;
				         }
				    }
	    		}
	    	}

	    	if(mPVWSourceIndex < 0){
	    		LOG_ERROR << "Logic failed, mPVWSourceIndex < 0";
	    		return;
	    	}

	    	if(!bIncludeSinglePVW && nullptr != mStreamList[mPVWSourceIndex]){
			    mStreamList[mPVWSourceIndex]->setVideoPGMSource(true);
			    mPGMStream->mVideoStreamIndex[0] = mPVWSourceIndex;
	    	}
		    vector<int>().swap(mPGMSrcIndexList);
	    	mPGMSourceIndex = mPVWSourceIndex;
	    }
    }
    else//Previous is single PGM stream
    {
	    if(!mPVWSrcIndexList.empty())//Need to switch Split Screen PVW stream to PGM now
	    {
	    	bool bIncludeSinglePGM = false;
	    	for(auto streamIndex : mPVWSrcIndexList){

	    		mPGMSrcIndexList.push_back(streamIndex);

	    		if(streamIndex == mPGMSourceIndex){
				    bIncludeSinglePGM = true;
				    continue;
	    		}

	    		if(nullptr != mStreamList[streamIndex]){
				    mStreamList[streamIndex]->setVideoPGMSource(true);
	    		}
	    	}
	    	for (int i = 0; i < MAX_PVW_SOURCE && i < mPVWSrcIndexList.size(); ++i) {
	    	    mPGMStream->mVideoStreamIndex[i] = mPVWSrcIndexList[i];
	    	}
	    	mPGMStream->mAudioStreamIndex = mPVWSrcIndexList[0];
	    	mStreamList[mPVWSrcIndexList[0]]->setAudioPGMSource(true);

	    	if(!bIncludeSinglePGM && mPGMSourceIndex != -1 && nullptr != mStreamList[mPGMSourceIndex]) {
			    mStreamList[mPGMSourceIndex]->setVideoPGMSource(false);
	    	}
		    mPGMSourceIndex = -1;
	    }
	    else//Need to switch single PVW stream
	    {
		    if(mPVWSourceIndex < 0){
			    LOG_ERROR << "No PVW source to switch!";
			    return;
		    }
		    if (mPGMSourceIndex == mPVWSourceIndex) {
			    LOG_INFO << "PVW already in PGM";
			    return;
		    }

		    if( mPGMSourceIndex >= 0 ){
			    if(nullptr != mStreamList[mPGMSourceIndex]  && nullptr != mStreamList[mPVWSourceIndex]){
                    mStreamList[mPVWSourceIndex]->setVideoPGMSource(true);
                    mStreamList[mPVWSourceIndex]->setAudioPGMSource(true);
                    //usleep(30000);

                    mPGMStream->mVideoStreamIndex[0] = mPVWSourceIndex;
                    mPGMStream->mVideoStreamIndex[1] = -1;
                    mPGMStream->mVideoStreamIndex[2] = -1;
                    mPGMStream->mVideoStreamIndex[3] = -1;

				    mStreamList[mPGMSourceIndex]->setVideoPGMSource(false);
                    mStreamList[mPGMSourceIndex]->setAudioPGMSource(false);
			    }
		    }
		    else{
			    if(nullptr != mStreamList[mPVWSourceIndex]){
				    mStreamList[mPVWSourceIndex]->setVideoPGMSource(true);
				    mPGMStream->mVideoStreamIndex[0] = mPVWSourceIndex;
			    }
		    }
		    mPGMSourceIndex = mPVWSourceIndex;
	    }
    }
}

void StreamManager::setStreamToPVW(int indexPVWSource) {
    if (indexPVWSource < 0 || indexPVWSource > mStreamList.size() || mPVWSourceIndex == indexPVWSource) {
        return;
    }

    // Notify Stream
    mStreamList[indexPVWSource]->setAudioPVWSource(true);
    mStreamList[indexPVWSource]->setVideoPVWSource(true);
    //usleep(300000);
    mPVWStream->setSource(indexPVWSource);

    if(!mPVWSrcIndexList.empty()) {
        for(const auto& streamIndex : mPVWSrcIndexList) {
            if (nullptr != mStreamList[streamIndex] && streamIndex != indexPVWSource) {
                mStreamList[streamIndex]->setVideoPVWSource(false);
                mStreamList[streamIndex]->setAudioPVWSource(false);
            }
        }
        vector<int>().swap(mPVWSrcIndexList);
    }
    else {
        if (mPVWSourceIndex != -1) {
            mStreamList[mPVWSourceIndex]->setVideoPVWSource(false);
            mStreamList[mPVWSourceIndex]->setAudioPVWSource(false);
        }
    }

    // Notify PVW
    // reset PVW video effects
    //mPVWStream->setSplitScreen(false, 0);
    //mPVWStream->setSource(indexPVWSource);
    mPVWSourceIndex = indexPVWSource;
}

void StreamManager::setStreamsToPVW(const vector<int> &pvwSources) {
    mPVWStream->setSources(pvwSources);
    mStreamList[pvwSources[0]]->setAudioPVWSource(true);
	vector<int> orgSrcsIndex(mPVWSrcIndexList);
	vector<int>().swap(mPVWSrcIndexList);
	bool bIncludeSinglePVW = false;
	for(int i = 0; i < pvwSources.size();i++) {
		if (pvwSources[i] < 0 || pvwSources[i] > mStreamList.size() || i >= MAX_PVW_SOURCE ) {
			return;
		}
		mPVWSrcIndexList.push_back(pvwSources[i]);

		if(pvwSources[i] == mPVWSourceIndex && mPVWSourceIndex >= 0){
			bIncludeSinglePVW = true;
			continue;
		}

		vector<int>::iterator ret;
		ret = std::find(orgSrcsIndex.begin(), orgSrcsIndex.end(), pvwSources[i]);
		if(ret == orgSrcsIndex.end()){
			if(nullptr != mStreamList[pvwSources[i]]){
				mStreamList[pvwSources[i]]->setVideoPVWSource(true);
			}
		}
		else{
			continue;
		}
	}

	if(!bIncludeSinglePVW && mPVWSourceIndex >= 0 && nullptr != mStreamList[mPVWSourceIndex]){
		mStreamList[mPVWSourceIndex]->setVideoPVWSource(false);
	}

	mPVWSourceIndex = -1;
}

Stream* StreamManager::getStream(int index) {
    if (index < 0 || index >= mStreamList.size()) {
        return nullptr;
    }

    return mStreamList[index];
}

