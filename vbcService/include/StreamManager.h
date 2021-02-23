//
// Created by jackie on 12/15/20.
//

#ifndef __VBSSERVICE_STREAMMANAGER_H__
#define __VBSSERVICE_STREAMMANAGER_H__

#include <vector>
#include <string>

using namespace std;

class Stream;
class PVWStream;
class PGMStream;
class MergeStream;

enum VideoEffectIndex { SPLIT_SCREEN = 0, TEXT, LOGO, XFADE };

class StreamManager {
public:
    static StreamManager* getInstance();

    int initStream(const char* camera, const char* audioDevice, int streamIndex);

    int initStreams(const char* cameras[], const char* audioDevices[], int numStreams);

    void startStreams();

    void updateSubtitle(const char* text, int pos);
    void updateLogo(const char *logoFile, int pos);
	void updateSplitScreen(const vector<int> streamSrcList, int mode);

	void switchPVWToPGM();
	void setStreamToPVW(int indexPVWSource);
    void setStreamsToPVW(const vector<int>& pvwSources);
	vector<Stream*> getStreamList() const {return mStreamList;}
	Stream* getStream(int index);

private:
    StreamManager();

private:
	PVWStream* mPVWStream = nullptr;
	PGMStream* mPGMStream = nullptr;
	MergeStream* mMergeStream = nullptr;
public:
    static StreamManager* mInstance;
    std::vector<Stream*> mStreamList;
    int mPVWSourceIndex = -1;
    int mPGMSourceIndex = -1;
    vector<int> mPVWSrcIndexList, mPGMSrcIndexList;
};


#endif //__VBSSERVICE_STREAMMANAGER_H__
