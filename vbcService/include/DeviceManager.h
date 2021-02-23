//
// Created by jackie on 12/15/20.
//

#ifndef __VBSSERVICE_DEVICEMANAGER_H__
#define __VBSSERVICE_DEVICEMANAGER_H__

#include <vector>
using namespace std;

#include "Device.h"

class DeviceManager {
public:
    static DeviceManager* getInstance();
	int initialize();
	void addDevice(string deviceIP, int devicePort, int deviceIndex);
	int removeDevicebyIndex(int index);
	int clearDevice();
	int sendDeviceCmdbyIndex(const string& command, const int& index);
	vector<Device> getDeviceList() const {return mDeviceList;}

private:
    DeviceManager();

private:
    static DeviceManager* mInstance;
    vector<Device> mDeviceList;
    int mEPollFD;
};


#endif //__VBSSERVICE_DEVICEMANAGER_H__
