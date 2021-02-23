//
// Created by xyhz on 2/2/21.
//

#ifndef VBCSERVICE_AUDIOCAP_H
#define VBCSERVICE_AUDIOCAP_H

#include <string>

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

class AudioCap {
public:
    AudioCap(const char* audio);
    virtual ~AudioCap();

    //判断摄像头是否打开.
    bool isOpened() const;

    bool haveAudio() const;

    const char* getAudioDeviceName() const;

    // 获取摄像头或文件的宽、高、帧率.
    int getSampleRate() const;

    AVStream* getAudioStream() const;

    // 获取下一帧数据，对于视频文件和RTSP流，可能是视频帧，也可能是音频帧；
    // 对于摄像头和麦克风，音频和视频交替获取.
    // 成功：视频帧返回0，音频帧返回1；失败：返回-1.
    // 如果成功，则可以从mCapturedFrame取出解码后的YUV帧或PCM帧.
    int grab();

    AVFrame* grabCapturedMicAudioFrame() const;

    // 音频重采样.
    // 成功：返回重采样后的帧，失败：返回nullptr.
    AVFrame* resample(AVFrame*);

private:
    AudioCap() = delete;

    // 打开摄像头和音频采集设备.
    int open(const char* audioDev);

    // 初始化音频解码环境.
    // 成功：返回0，失败：返回-1.
    int initAudioDecodeEnv(AVFormatContext* audioFormatContext);

    // 初始化音频重采样环境.
    // 成功：返回0，失败：返回-1.
    int initAudioResampleEnv();

    // 读取音频数据，成功：返回0，失败：返回-1
    int grabAudioPacket();

private:
    std::string mCamUrl = "";
    std::string mAudioDevice = "";
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
    AVPacket* mCapturedMicPacket = nullptr; // 采集到mic的原始数据包.
    AVFrame* mCapturedFrame = nullptr; // 原始数据包解码后的视频帧或音频帧.
    AVFrame* mCapturedMicFrame = nullptr; // mic原始数据包解码后的视频帧或音频帧.
    AVFrame* mCapturedAudioFrame = nullptr; // 原始数据包解码后的视频帧或音频帧.
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
};


#endif //VBCSERVICE_AUDIOCAP_H
