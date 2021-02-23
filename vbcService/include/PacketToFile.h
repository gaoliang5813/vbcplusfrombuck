/**
* Copyright(c) 2020, 合肥祥云汇智科技有限公司
* All rights reserved.
*
* 文件名称：PacketToFile.h
* 摘要:
*
* 作者: 梁元琨
* 完成日期: 2020-12-23
**/

#ifndef VBCSERVICE_PACKETTOFILE_H
#define VBCSERVICE_PACKETTOFILE_H

#include <string>
#include <memory>
#include <thread>
using namespace std;
class AVPacket;
class AVRational;
class AVFormatContext;

class PacketToFile {
public:
    int openOutput(string outUrl);
    void avPacketRescaleTs(AVPacket *pkt, AVRational srcTb, AVRational dstTb);
    int writePacket(shared_ptr<AVPacket> packet);
    void closeOutput();

private:
    AVFormatContext* mOutputContext;
};


#endif //VBCSERVICE_PACKETTOFILE_H
