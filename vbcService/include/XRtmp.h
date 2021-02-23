//
// Created by jackie on 12/15/20.
//

#ifndef __VBCSERVICE_XRTMP_H__
#define __VBCSERVICE_XRTMP_H__

#include <string>

struct AVPacket;
struct AVFrame;
struct AVStream;
struct AVCodec;
struct AVFormatContext;
struct AVCodecContext;

class XRtmp {
public:
    XRtmp(const char* url);
    ~XRtmp();

    void close();

    int addVideoStreamLowRes(int fps);
    int addVideoStreamHighRes(int fps);
    int addAudioStream();

    int sendHeader();

    int encodeVideoAndSend(AVFrame* frame);
    int encodeAudioAndSend(AVFrame* frame);

    uint64_t getNextVideoPts() const;
    uint64_t getNextAudioPts() const;
    bool isVideoNextToSend();
    bool isAudioNextToSend();

    const char* getURL() const;
private:
    XRtmp() = delete;

    int initConnection(const char* url);

    int addVideoStream(int fps, bool highRes);

    AVPacket* encodeVideoFrame(AVFrame* frame);
    AVPacket* encodeAudioFrame(AVFrame* frame);

    int initVideoEncodeEnv(int width, int height, int fps);
    int initAudioEncodeEnv();

private:
	//rtmp flv 封装器
	AVFormatContext* mOutputContext = nullptr;
    //视频编码器流
    AVCodecContext* mVideoDecodeContext = nullptr;
    AVCodecContext* mAudioDecodeContext = nullptr;
    AVStream* mVideoStream = nullptr;
    AVStream* mAudioStream = nullptr;

    AVCodec* mVideoDecoder = nullptr;
    AVCodec* mAudioDecoder = nullptr;

    AVCodecContext* mVideoEncodeContext = nullptr;
    AVCodecContext* mAudioEncodeContext = nullptr;
    AVCodec* mVideoEncoder = nullptr;
    AVCodec* mAudioEncoder = nullptr;

    std::string mUrl = "";
    uint64_t mNextVideoPts = 0;
    uint64_t mNextAudioPts = 0;
};


#endif //__VBCSERVICE_XRTMP_H__
