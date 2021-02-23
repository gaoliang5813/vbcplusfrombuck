//
// Created by jackie on 1/8/21.
//

#ifndef VBCSERVICE_COMMONFUNCTION_H
#define VBCSERVICE_COMMONFUNCTION_H

#define PGM_URL "rtmp://192.168.0.56/live/pgm"
#define MERGE_URL "rtmp://192.168.0.56/live/merge"
#define DEFAULT_AUDIO_FILE "../resources/sound.aac"

#define MAX_PVW_SOURCE 4

#include "opencv2/opencv.hpp"
using namespace cv;

struct AVFrame;

const int PGM_WIDTH = 1920;
const int PGM_HEIGHT = 1080;

const int MONITOR_WINDOW_WIDTH = 640;
const int MONITOR_WINDOW_HEIGHT = 360;

const int MERGE_WIDTH = 640;
const int MERGE_HEIGHT = 270;

const int ENCODE_BIT_RATE = 500 * 1024 * 8;
const int GOP_SIZE = 1;
const int MAX_B_FRAMES = 0;
const int FPS = 30;

const int AAC_SAMPLE_RATE = 44100;

const int BUFFER_FRAME_NUM = 3600 * FPS;

int printFFMPEGError(int errorCode);

void* getDefaultImageLowResolution();
void* getDefaultImageHighResolution();

void* getSilentAudioFrame(int sampleRate);

#endif //VBCSERVICE_COMMONFUNCTION_H
