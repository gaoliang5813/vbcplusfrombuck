//
// Created by jackie on 12/15/20.
//

#ifndef __VBCSERVICE_VIDEOCAP_H__
#define __VBCSERVICE_VIDEOCAP_H__

#include <string>
#include "network.h"
#include "mtcnn.h"

struct AVPacket;
struct AVFrame;
struct AVStream;
struct AVCodec;
struct SwsContext;
struct SwrContext;
struct AVInputFormat;
struct AVFormatContext;
struct AVCodecContext;
struct AVFilterContext;
struct AVFilterGraph;
struct AVFilterInOut;

class VideoCap {
    enum VideoCapType {
        VIDEO_CAP_TYPE_UNKNOWN = -1,
        VIDEO_CAP_TYPE_CAMERA = 0,
        VIDEO_CAP_TYPE_FILE = 1,
        VIDEO_CAP_TYPE_RTSP = 2,
    };

public:
    VideoCap(const char* url);
    VideoCap(const char* cam, const char* audio);
    virtual ~VideoCap();

    //判断摄像头是否打开.
    bool isOpened() const;
    bool isFileType() const;
    bool isCameraType() const;
    bool isRTSPType() const;

    bool haveVideo() const;
    bool haveAudio() const;

    bool isVideoNextToCap() const;

    // 如果打开的是文件，判断是否到达文件末尾.
    bool reachEOF() const;

    const char* getCameraUrl() const;
    const char* getAudioDeviceName() const;

    // 获取摄像头或文件的宽、高、帧率.
    int getWidth() const;
    int getHeight() const;
    int getFPS() const;
    int getSampleRate() const;
    mtcnn * getMtcnn() const;

    AVStream* getVideoStream() const;
    AVStream* getAudioStream() const;

    // 获取下一帧数据，对于视频文件和RTSP流，可能是视频帧，也可能是音频帧；
    // 对于摄像头和麦克风，音频和视频交替获取.
    // 成功：视频帧返回0，音频帧返回1；失败：返回-1.
    // 如果成功，则可以从mCapturedFrame取出解码后的YUV帧或PCM帧.
    int grab();

    AVFrame* grabCapturedVideoFrame() const;
    AVFrame* grabCapturedAudioFrame() const;

    // 音频重采样.
    // 成功：返回重采样后的帧，失败：返回nullptr.
    AVFrame* resample(AVFrame*);

    AVFrame* scale(AVFrame*);
    AVFrame* upscale(AVFrame*);

    mtcnn* mMtcnn;
private:
    VideoCap() = delete;

    //打开摄像头，支持网络摄像和本地视频.
    int open(const char* camUrl);
    // 打开摄像头和音频采集设备.
    int open(const char* camUrl, const char* audioDev);

    // 初始化音视频解码环境，用于打开视频文件和RTSP流.
    // 成功：返回0，失败：返回-1.
    int initVideoAudioDecodeEnv(AVFormatContext* videoAudioFormatContext);

    // 初始化视频解码环境.
    // 成功：返回0，失败：返回-1.
    int initVideoDecodeEnv(AVFormatContext* videoFormatContext);

    // 初始化音频解码环境.
    // 成功：返回0，失败：返回-1.
    int initAudioDecodeEnv(AVFormatContext* audioFormatContext);

    // 初始化视频缩放环境.
    // 成功：返回0，失败：返回-1.
    int initVideoScaleEnv();

    // 初始化音频重采样环境.
    // 成功：返回0，失败：返回-1.
    int initAudioResampleEnv();

    // 判断打开的摄像头或视频文件是否合法，如果合法，则将mType置为相应的类型，并返回true；
    // 否则返回false.
    bool isValidCameraInput(const char* url);

    // 读取视频数据，成功：返回0，失败：返回-1
    int grabVideoPacket();
    // 读取音频数据，成功：返回0，失败：返回-1
    int grabAudioPacket();

    //avframe转rgb
    cv::Mat convertYUVToRGB(AVFrame *yuvFrame);


    AVFrame* findFaceInYUVFrame(AVFrame* yuvFrame);
    AVFrame* convertMatToFaceFrame(cv::Mat imageMat);

private:
    std::string mCamUrl = "";
    std::string mAudioDevice = "";
    VideoCapType mType = VIDEO_CAP_TYPE_UNKNOWN;
    bool mOpened = false;
    bool mEOF = false; // 是否读到文件末尾.
    int mCapWidth = -1;
    int mCapHeight = -1;
    int mFPS = -1;

    int mVideoPts = 0;
    int mAudioPts = 0;

    AVStream* mVideoStream = nullptr;
    AVStream* mAudioStream = nullptr;
    int mVideoIndex = -1;
    int mAudioIndex = -1;

    AVFormatContext* mVideoInputFormatContext = nullptr;
    AVFormatContext* mAudioInputFormatContext = nullptr;

    AVCodecContext* mVideoDecodeContext = nullptr;
    AVCodecContext* mAudioDecodeContext = nullptr;

    AVCodec* mVideoDecoder = nullptr; // 将采集到的原始视频数据解码为YUV数据
    AVCodec* mAudioDecoder = nullptr; // 将采集的原始音频数据解码成PCM数据.

    AVPacket* mCapturedPacket = nullptr; // 采集到的原始数据包.
    AVFrame* mCapturedFrame = nullptr; // 原始数据包解码后的视频帧或音频帧.
    SwsContext* mScaleContext = nullptr; // 将采集到的视频数据转换成YUV数据.
    SwsContext* mUpscaleContext = nullptr; // 将采集到的视频数据放到成高清YUV数据.

    SwsContext* mAvFrametoRGBContext = nullptr; // avframe转rgb数据.
    SwsContext* mCameratoYUV420Context = nullptr; // 摄像头采集转YUV420数据.
    SwsContext* mYUV420toCameraContext = nullptr; // YUV420转摄像头采集数据.


    AVFilterContext* mAudioBufferSinkContext = nullptr;
    AVFilterContext* mAudioBufferSrcContext = nullptr;
    AVFilterGraph* mAudioFilterGraph = nullptr;
    AVFilterInOut* mInputAudioFilter = nullptr;
    AVFilterInOut* mOutputAudioFilter = nullptr;


    AVFrame* mframeWithFace;
};

#endif //__VBCSERVICE_VIDEOCAP_H__
