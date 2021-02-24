#include <thread>
#include "OrderControl.h"
#include "Logger.h"
#include "StreamManager.h"
#include <signal.h>
using namespace std;

int main(int argc, char** argv) {
    signal(SIGQUIT,exit);
    signal(SIGTERM,exit);

    // 设置每1分钟30秒更换一个日志文件
    int hour = 0, minute = 1, second = 30;
    logger::initLogStrategy(hour, minute, second);

    const int NUM_TEST_STREAMS = 4;
    //const char* cameras[NUM_TEST_STREAMS] = { "/dev/video0","../resources/a.flv", "../resources/b.flv", "../resources/c.flv"};
    const char* cameras[NUM_TEST_STREAMS] = { "../resources/d.flv","../resources/a.flv", "../resources/b.flv", "../resources/c.flv"};
    //const char* cameras[NUM_TEST_STREAMS] = { "../resources/4.flv","../resources/1.flv", "../resources/2.flv", "../resources/3.flv"};
   // const char* audioDevices[NUM_TEST_STREAMS] = { "hw:7", nullptr, nullptr, nullptr };
    const char* audioDevices[NUM_TEST_STREAMS] = { nullptr, nullptr, nullptr, nullptr };
    StreamManager* streamManager = StreamManager::getInstance();
    if (streamManager == nullptr ){
        LOG_ERROR << "StreamManager initTestStream() failed!";
        return -1;
    }
    if (streamManager->initStreams(cameras, audioDevices, NUM_TEST_STREAMS) != 0) {
        LOG_ERROR << "StreamManager initTestStream() failed!";
        return -1;
    }

    const char* brokerIP = "tcp://192.168.9.2:1883";
    const char* clientID = "mqttClientID";
    const char* connectionName = "videoBroadCast";

    COrderControl* orderControl = new COrderControl(brokerIP, clientID, connectionName);
    orderControl->initialize();

    //streamManager->startStreams();
    //usleep(1000);
    std::vector<int> v{1,2};
    streamManager->setStreamToPVW(1);
    streamManager->switchPVWToPGM();
    streamManager->setStreamToPVW(0);
    //streamManager->updateSplitScreen(v, 5);
    //v = {2, 3};
    //streamManager->updateSplitScreen(v, 5);
    //streamManager->setStreamToPVW(1);
    //streamManager->updateSplitScreen(v, 5);



    streamManager->startStreams();

    return 0;
}

