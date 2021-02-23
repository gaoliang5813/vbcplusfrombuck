//
// Created by jackie on 1/22/21.
//

#ifndef VBCPLUS_MERGESTREAM_H
#define VBCPLUS_MERGESTREAM_H

#include <vector>

class AVFrame;
class XRtmp;
class Stream;
class PVWStream;
class PGMStream;
class XVideoSplitScreen;

typedef std::vector<Stream*> StreamList;

class MergeStream {
public:
    MergeStream();
    virtual ~MergeStream();

    int work();

    void setPGMStream(PGMStream* pgmStream);
    void setPVWStream(PVWStream* pvwStream);
    void setStreamList(const StreamList& streamList);
private:

    void cycle();
private:
    PGMStream* mPGMStream = nullptr;
    PVWStream* mPVWStream = nullptr;
    StreamList mStreamList;
    XRtmp* mXRtmp = nullptr;
    XVideoSplitScreen* mXVideoSplitScreen = nullptr;
};

#endif//VBCPLUS_MERGESTREAM_H
