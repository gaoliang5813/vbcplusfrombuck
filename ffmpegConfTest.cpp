#include <iostream>
#include "ace/Log_Msg.h"
#include "opencv2/opencv.hpp"
extern "C" {
#include "libavcodec/avcodec.h"
}

using namespace std;
using namespace cv;

int main(int argc, char** argv)
{
    ACE_DEBUG((LM_DEBUG, "This is a log info.\n"));
    cout << "ffmpeg build info: "<< avcodec_configuration() << endl;
    cout << "============================" << endl;
    cout << "opencv version: " << CV_VERSION << endl;
    return 0;
}
