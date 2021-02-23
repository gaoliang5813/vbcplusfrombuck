//
// Created by joey on 12/27/20.
//

#ifndef VBCSERVICE_TCPCONNECTION_H
#define VBCSERVICE_TCPCONNECTION_H

#include <string>
#include "Logger.h"

using namespace std;

class TCPConnection{
public:
	TCPConnection();
	~TCPConnection();
	int connectWithTCP(string deviceIP, int devicePort);
	int sendCommand(const char* command, int length) const;
	void receive();
	int closeTCPSocket() const;
	int getSocketFD() const {return mSocketFD;}

private:
	void setSocketFD(int socket);
	int setNonBlocking(int socket) const;

private:
	int mSocketFD;
};

#endif //VBCSERVICE_TCPCONNECTION_H
