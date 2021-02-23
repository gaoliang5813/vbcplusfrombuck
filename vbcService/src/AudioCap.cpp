//
// Created by xyhz on 2/2/21.
//

#include "AudioCap.h"

#include "CommonFunction.h"
#include "Logger.h"
#include "opencv2/opencv.hpp"
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
#include "libavdevice/avdevice.h"
#include "libavutil/time.h"
}

using namespace std;

AudioCap::AudioCap( const char* audio)  {
    avdevice_register_all();
    mCapturedMicFrame = av_frame_alloc();
    mCapturedMicPacket = new AVPacket();
    memset(mCapturedMicPacket, 0, sizeof(*mCapturedMicPacket));
    open(audio);
}

AudioCap::~AudioCap() {
    if (mCapturedMicFrame != nullptr) {
        av_frame_free(&mCapturedMicFrame);
        mCapturedMicFrame = nullptr;
    }

    if (mCapturedMicPacket != nullptr) {
        av_packet_free(&mCapturedMicPacket);
        mCapturedMicPacket = nullptr;
    }
}

int AudioCap::open(const char* audioDev) {
    // 打开麦克风.
    AVInputFormat* audioInputFormat = av_find_input_format("alsa");
    mAudioDevice = audioDev;

    int rv = avformat_open_input(&mAudioInputFormatContext, audioDev, audioInputFormat, NULL);
    if (rv != 0) {
        LOG_ERROR << "无法打开" << audioDev;
        return printFFMPEGError(rv);
    }

    if ((rv = avformat_find_stream_info(mAudioInputFormatContext, NULL)) != 0) {
        LOG_ERROR << "在" << audioDev << "中找不到音频流!";
        return printFFMPEGError(rv);
    }

    if (initAudioDecodeEnv(mAudioInputFormatContext) != 0) {
        LOG_ERROR << "为" << audioDev << "初始化音频解码环境失败！";
        return -1;
    }

    mOpened = true;
    return 0;
}

int AudioCap::initAudioDecodeEnv(AVFormatContext *audioFormatContext) {
    if (audioFormatContext == nullptr) {
        LOG_ERROR << mAudioDevice << ":非法的音频格式上下文!";
        return -1;
    }
    for (unsigned int i = 0; i < audioFormatContext->nb_streams; ++i) {
        if (audioFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if ((mAudioStream = audioFormatContext->streams[i]) == nullptr) {
                LOG_ERROR << "在" << mAudioDevice << "中找不到音频流！";
                return -1;
            }
            if ((mAudioDecoder = avcodec_find_decoder(mAudioStream->codecpar->codec_id)) == nullptr) {
                LOG_ERROR << "无法为" << mAudioDevice << "找到音频解码器";
                return -1;
            }
            if ((mAudioDecodeContext = avcodec_alloc_context3(mAudioDecoder)) == nullptr) {
                LOG_ERROR << "无法为" << mAudioDevice << "的音频解码器分配解码上下文";
                return -1;
            }
            avcodec_parameters_to_context(mAudioDecodeContext, mAudioStream->codecpar);
            if (avcodec_open2(mAudioDecodeContext, mAudioDecoder, NULL) < 0) {
                LOG_ERROR << "在" << mAudioDevice << "中打开音频解码器上下文失败!";
                return -1;
            }
            mAudioIndex = i;
            //if (!isCameraType()) {
            initAudioResampleEnv();
            //}
            return 0;
        }
    }
    return -1;
}

int AudioCap::initAudioResampleEnv() {
    if (mAudioStream == nullptr) {
        LOG_ERROR << "AudioCap::initAudioResampleEnv() failed, have no audio stream!";
        return -1;
    }
    const AVFilter* audioBufferSrc = avfilter_get_by_name("abuffer");
    const AVFilter* audioBufferSink = avfilter_get_by_name("abuffersink");
    mInputAudioFilter = avfilter_inout_alloc();
    mOutputAudioFilter = avfilter_inout_alloc();
    if (mInputAudioFilter == nullptr || mOutputAudioFilter == nullptr) {
        return -1;
    }

    if (mAudioDecodeContext->channel_layout == 0) {
        mAudioDecodeContext->channel_layout = av_get_default_channel_layout(mAudioDecodeContext->channels);
    }

    const enum AVSampleFormat OUTPUT_AUDIO_SAMPLE_FMTS[] = { AV_SAMPLE_FMT_FLTP, AV_SAMPLE_FMT_NONE };
    const int64_t OUTPUT_AUDIO_CHANNEL_LAYOUTS[] = { (int64_t)mAudioDecodeContext->channel_layout, -1 };
    const int OUTPUT_AUDIO_SAMPLE_RATES[] = { AAC_SAMPLE_RATE, -1 };
    if ((mAudioFilterGraph = avfilter_graph_alloc()) == nullptr) {
        return -1;
    }
    mAudioFilterGraph->nb_threads = 8;

    char args[512] = { 0 };
    snprintf(args, sizeof(args), "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%Id",
             mAudioStream->time_base.num, mAudioStream->time_base.den,
             mAudioDecodeContext->sample_rate, av_get_sample_fmt_name(mAudioDecodeContext->sample_fmt),
             mAudioDecodeContext->channel_layout);
    LOG_INFO << mAudioDevice << " initAudioResampleEnv argument: " << args;

    int rv = avfilter_graph_create_filter(&mAudioBufferSrcContext, audioBufferSrc, "in", args, NULL, mAudioFilterGraph);
    if (rv < 0) {
        return -1;
    }
    if ((rv = avfilter_graph_create_filter(&mAudioBufferSinkContext, audioBufferSink, "out", NULL, NULL, mAudioFilterGraph)) < 0) {
        return -1;
    }

    if ((rv = av_opt_set_int_list(mAudioBufferSinkContext, "sample_fmts", OUTPUT_AUDIO_SAMPLE_FMTS, -1, AV_OPT_SEARCH_CHILDREN)) < 0) {
        return -1;
    }
    if ((rv = av_opt_set_int_list(mAudioBufferSinkContext, "channel_layouts", OUTPUT_AUDIO_CHANNEL_LAYOUTS, -1, AV_OPT_SEARCH_CHILDREN)) < 0) {
        return -1;
    }
    if ((rv = av_opt_set_int_list(mAudioBufferSinkContext, "sample_rates", OUTPUT_AUDIO_SAMPLE_RATES, -1, AV_OPT_SEARCH_CHILDREN)) < 0) {
        return -1;
    }
    mOutputAudioFilter->name = av_strdup("in");
    mOutputAudioFilter->filter_ctx = mAudioBufferSrcContext;
    mOutputAudioFilter->pad_idx = 0;
    mOutputAudioFilter->next = NULL;

    mInputAudioFilter->name = av_strdup("out");
    mInputAudioFilter->filter_ctx = mAudioBufferSinkContext;
    mInputAudioFilter->pad_idx = 0;
    mInputAudioFilter->next = NULL;

    if ((rv = avfilter_graph_parse_ptr(mAudioFilterGraph, "anull", &mInputAudioFilter, &mOutputAudioFilter, nullptr)) < 0) {
        return -1;
    }
    if ((rv = avfilter_graph_config(mAudioFilterGraph, NULL)) < 0) {
        return -1;
    }
    av_buffersink_set_frame_size(mAudioBufferSinkContext, 1024);

    return 0;
}

int AudioCap::grab() {
    int ret = -1;
    av_packet_unref(mCapturedMicPacket);
    av_frame_unref(mCapturedMicFrame);
    if (haveAudio()) {
        return grabAudioPacket();
    }

    return -1;
}

int AudioCap::grabAudioPacket() {
    if (mAudioStream == nullptr) {
        LOG_ERROR << mAudioDevice << "中没有音频流，无法获取音频帧!";
        return -1;
    }

    int ret = -1;
    av_packet_unref(mCapturedMicPacket);
    if ((ret = av_read_frame(mAudioInputFormatContext, mCapturedMicPacket)) < 0) {
        return -1;
    }
    if ((ret = avcodec_send_packet(mAudioDecodeContext, mCapturedMicPacket)) < 0) {
        return -1;
    }
    if ((ret = avcodec_receive_frame(mAudioDecodeContext, mCapturedMicFrame)) < 0) {
        return -1;
    }
    mAudioPts += mCapturedMicFrame->nb_samples;

    return 1;
}

AVFrame* AudioCap::grabCapturedMicAudioFrame() const { return mCapturedMicFrame; }

AVFrame* AudioCap::resample(AVFrame* inFrame) {

    int ret = -1;
    uint64_t inT = av_gettime();
    uint64_t outT = 0;
    LOG_DEBUG << "Audio resample pts: " << inFrame->pts;
    if ((ret = av_buffersrc_add_frame_flags(mAudioBufferSrcContext, inFrame, AV_BUFFERSRC_FLAG_PUSH)) < 0) {
        LOG_ERROR << "Audio resample failed! add frame to buffer source error:" << ret;
        //printFFMPEGError(ret);
        outT = av_gettime();
        av_frame_free(&inFrame);
        LOG_INFO << "resample1 fail cost : "  <<outT-inT;
        return nullptr;
    }

    AVFrame* outFrame = av_frame_alloc();
    if ((ret = av_buffersink_get_frame_flags(mAudioBufferSinkContext, outFrame, AV_BUFFERSINK_FLAG_NO_REQUEST)) < 0) {
        LOG_ERROR << "Audio resample failed! get frame from buffer sink error:" << ret;
        av_frame_free(&outFrame);
        av_frame_unref(inFrame);
        outT = av_gettime();
        LOG_INFO << "resample1 fail cost : "  <<outT-inT;
        return nullptr;
    }
    outT = av_gettime();
    LOG_INFO << "resample333 success cost : "  <<outT-inT;
    av_frame_unref(inFrame);
    return outFrame;
}

bool AudioCap::isOpened() const { return mOpened; }
bool AudioCap::haveAudio() const { return mAudioStream != nullptr; }

const char* AudioCap::getAudioDeviceName() const { return mAudioDevice.c_str(); }

int AudioCap::getSampleRate() const {
    if (haveAudio()) {
        return mAudioDecodeContext->sample_rate;
    }
    else {
        return AAC_SAMPLE_RATE;
    }
}

AVStream* AudioCap::getAudioStream() const { return mAudioStream; }
