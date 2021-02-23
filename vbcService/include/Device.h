//
// Created by joey on 12/23/20.
//

#ifndef VBCSERVICE_DEVICE_H
#define VBCSERVICE_DEVICE_H

#include "TCPConnection.h"

class Device:public TCPConnection{
public:
	Device(	string deviceIP, int devicePort, int deviceIndex);
	~Device();
	void setDeviceIP(string deviceIP);
	void setDevicePort(int devicePort);
	void setDeviceIndex(int deviceIndex);
	string getDeviceIP() const {return mDeviceIP;}
	int getDevicePort() const {return mDevicePort;}
	int getDeviceIndex() const {return mDeviceIndex;}

private:
	string mDeviceIP;
	int mDevicePort;
	int mDeviceIndex;
};
#endif //VBCSERVICE_DEVICE_H
