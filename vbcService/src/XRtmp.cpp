#include "XRtmp.h"
#include "VideoCap.h"
#include "Logger.h"
#include "CommonFunction.h"
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/time.h"
}

XRtmp::XRtmp(const char* url) {
    initConnection(url);
}

XRtmp::~XRtmp() {
}

int XRtmp::initConnection(const char *url) {
    avformat_network_init();
    //a 创建输出封装器上下文
    if (url == nullptr) {
        LOG_ERROR << "No RTMP Stream Server URL given!";
        return -1;
    }
    mUrl = url;
    int ret = avformat_alloc_output_context2(&mOutputContext, 0, "flv", url);
    if (ret != 0) {
        printFFMPEGError(ret);
        return -1;
    }
    return 0;
}

const char* XRtmp::getURL() const { return mUrl.c_str(); }

void XRtmp::close() {
    if (mOutputContext != nullptr) {
        avformat_close_input(&mOutputContext);
        mVideoStream = nullptr;
        mAudioStream = nullptr;
    }
    mUrl = "";
}

int XRtmp::addVideoStreamLowRes(int fps) {
    return addVideoStream(fps, false);
}

int XRtmp::addVideoStreamHighRes(int fps) {
    return addVideoStream(fps, true);
}

int XRtmp::addVideoStream(int fps, bool highRes) {
    if (mVideoStream != nullptr) {
        LOG_INFO << "VideoStream already added!";
        return -1;
    }
    if (fps <= 0) {
        LOG_INFO << "无效的视频流帧率：" << fps << "，已恢复为默认值：" << FPS;
        fps = FPS;
    }
    if ((mVideoDecoder = avcodec_find_decoder(AV_CODEC_ID_H264)) == nullptr) {
        LOG_ERROR << "添加视频流失败：找不到H264编码器!";
        return -1;
    }
    if (highRes) {
        initVideoEncodeEnv(PGM_WIDTH, PGM_HEIGHT, fps);
    }
    else {
        initVideoEncodeEnv(MERGE_WIDTH, MERGE_HEIGHT, fps);
    }

    if ((mVideoStream = avformat_new_stream(mOutputContext, mVideoDecoder)) == nullptr) {
        LOG_ERROR << "Cannot add video stream!";
        return -1;
    }
    avcodec_parameters_from_context(mVideoStream->codecpar, mVideoEncodeContext);
    mVideoStream->codecpar->codec_tag = 0;
    return 0;
}

int XRtmp::initVideoEncodeEnv(int width, int height, int fps) {
    mVideoEncoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    mVideoEncodeContext = avcodec_alloc_context3(mVideoEncoder);
    mVideoEncodeContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    mVideoEncodeContext->codec_id = mVideoEncoder->id;
    mVideoEncodeContext->thread_count = 1;
    mVideoEncodeContext->bit_rate = ENCODE_BIT_RATE;
    mVideoEncodeContext->width = width;
    mVideoEncodeContext->height = height;
    mVideoEncodeContext->time_base = {1, fps};
    mVideoEncodeContext->framerate = {fps, 1};
    mVideoEncodeContext->gop_size = GOP_SIZE;
    mVideoEncodeContext->max_b_frames = MAX_B_FRAMES;
    mVideoEncodeContext->pix_fmt = AV_PIX_FMT_YUV420P;
    avcodec_open2(mVideoEncodeContext, mVideoEncoder, NULL);
    return 0;
}

int XRtmp::initAudioEncodeEnv() {
    //mAudioEncoder = avcodec_find_encoder_by_name("libmp3lame");
    mAudioEncoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
    mAudioEncodeContext = avcodec_alloc_context3(mAudioEncoder);
    mAudioEncodeContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    mAudioEncodeContext->codec_id = mAudioEncoder->id;
    mAudioEncodeContext->codec = mAudioEncoder;
    mAudioEncodeContext->sample_rate = AAC_SAMPLE_RATE;
    mAudioEncodeContext->channels = 2;
    mAudioEncodeContext->channel_layout = av_get_default_channel_layout(2);
    mAudioEncodeContext->sample_fmt = AV_SAMPLE_FMT_FLTP;
    mAudioEncodeContext->codec_tag = 0;
    avcodec_open2(mAudioEncodeContext, mAudioEncoder, NULL);
    return 0;
}

int XRtmp::addAudioStream() {
    if (mAudioStream != nullptr) {
        LOG_INFO << "AudioStream already added!";
        return -1;
    }
    if ((mAudioDecoder = avcodec_find_decoder(AV_CODEC_ID_AAC)) == nullptr) {
        LOG_ERROR << "添加音频流失败：找不到AAC编码器!";
        return -1;
    }
    if ((mAudioStream = avformat_new_stream(mOutputContext, mAudioDecoder)) == nullptr) {
        LOG_ERROR << "Cannot add audio stream!";
        return -1;
    }
    initAudioEncodeEnv();
    mAudioStream->codecpar->codec_tag = 0;
    avcodec_parameters_from_context(mAudioStream->codecpar, mAudioEncodeContext);

    return 0;
}

int XRtmp::sendHeader() {
    ///打开rtmp 的网络输出IO
    int ret = avio_open(&mOutputContext->pb, mUrl.c_str(), AVIO_FLAG_WRITE);
    if (ret != 0) {
        printFFMPEGError(ret);
        return -1;
    }

    //写入封装头
    ret = avformat_write_header(mOutputContext, NULL);
    if (ret != 0) {
        printFFMPEGError(ret);
        return -1;
    }
    return 0;
}

uint64_t XRtmp::getNextVideoPts() const {
    return mNextVideoPts;
}

uint64_t XRtmp::getNextAudioPts() const {
    return mNextAudioPts;
}

bool XRtmp::isVideoNextToSend() {
    return av_compare_ts(mNextVideoPts, mVideoEncodeContext->time_base,
                         mNextAudioPts, mAudioEncodeContext->time_base) <= 0;
}

bool XRtmp::isAudioNextToSend() {
    return av_compare_ts(mNextVideoPts, mVideoEncodeContext->time_base,
                         mNextAudioPts, mAudioEncodeContext->time_base) > 0;
}

int XRtmp::encodeVideoAndSend(AVFrame *frame) {
    if (frame == nullptr) {
        return -1;
    }
    frame->pts = mNextVideoPts++;

    AVPacket* packet = encodeVideoFrame(frame);
    if (packet == nullptr) {
        return -1;
    }

    packet->pts = av_rescale_q(packet->pts, mVideoEncodeContext->time_base, mVideoStream->time_base);
    packet->dts = av_rescale_q(packet->dts, mVideoEncodeContext->time_base, mVideoStream->time_base);
    packet->duration = av_rescale_q(packet->duration, mVideoEncodeContext->time_base, mVideoStream->time_base);
    int ret = av_interleaved_write_frame(mOutputContext, packet);
    if (packet != nullptr) {
        delete packet;
    }
    if (ret != 0) {
        printFFMPEGError(ret);
        LOG_ERROR << "视频发送失败:av_interleaved_write_frame() failed! ret = " << ret;
        return -1;
    }
    return 0;
}

AVPacket* XRtmp::encodeVideoFrame(AVFrame *frame) {
    int ret = avcodec_send_frame(mVideoEncodeContext, frame);
    if (ret != 0) {
        printFFMPEGError(ret);
        LOG_ERROR << "视频编码失败:avcodec_send_frame() failed!";
        return nullptr;
    }
    AVPacket* packet = new AVPacket;
    memset(packet, 0, sizeof(*packet));

    ret = avcodec_receive_packet(mVideoEncodeContext, packet);
    if (ret != 0 && packet->size <= 0) {
        delete packet;
        printFFMPEGError(ret);
        LOG_ERROR << "视频编码失败:avcodec_receive_packet() failed!";
        return nullptr;
    }
    packet->stream_index = mVideoStream->index;

    return packet;
}

int XRtmp::encodeAudioAndSend(AVFrame *frame) {
    if (frame == nullptr) {
        return -1;
    }
    if (frame->nb_samples == 0) {
        return -1;
    }
    LOG_INFO << "frame nb_samples: " << frame->nb_samples;
    frame->pts = mNextAudioPts;
    //mNextAudioPts += av_rescale_q(frame->nb_samples, mAudioEncodeContext->time_base, mAudioStream->time_base);
    mNextAudioPts += frame->nb_samples;
    AVPacket* packet = encodeAudioFrame(frame);
    if (packet == nullptr) {
        LOG_ERROR << "音频发送失败:encodeAudioFrame() failed!";
        return -1;
    }

    packet->pts = av_rescale_q(packet->pts, mAudioEncodeContext->time_base, mAudioStream->time_base);
    packet->dts = av_rescale_q(packet->dts, mAudioEncodeContext->time_base, mAudioStream->time_base);
    packet->duration = av_rescale_q(packet->duration, mAudioEncodeContext->time_base, mAudioStream->time_base);
    int ret = av_interleaved_write_frame(mOutputContext, packet);
    if (packet != nullptr) {
        delete packet;
        packet = nullptr;
    }
    if (ret != 0) {
        printFFMPEGError(ret);
        LOG_ERROR << "音频发送失败:av_interleaved_write_frame() failed! ret: " << ret;
        return -1;
    }
    return 0;
}

AVPacket* XRtmp::encodeAudioFrame(AVFrame *frame) {
    int ret = avcodec_send_frame(mAudioEncodeContext, frame);
    if (ret != 0) {
        printFFMPEGError(ret);
        LOG_ERROR << "音频编码失败:avcodec_send_frame() failed!";
        return nullptr;
    }

    AVPacket* packet = new AVPacket;
    memset(packet, 0, sizeof(*packet));

    ret = avcodec_receive_packet(mAudioEncodeContext, packet);
    if (ret != 0 && packet->size <= 0) {
        delete packet;
        printFFMPEGError(ret);
        return nullptr;
    }
    packet->stream_index = mAudioStream->index;

    return packet;
}
