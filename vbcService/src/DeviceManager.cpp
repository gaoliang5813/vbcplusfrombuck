//
// Created by jackie on 12/15/20.
//

#include <pthread.h>
#include <sys/epoll.h>
#include "DeviceManager.h"


DeviceManager* DeviceManager::mInstance = nullptr;

//#define MAX_SOCKET_SIZE 1024

DeviceManager::DeviceManager():mEPollFD(-1)
{
}

DeviceManager* DeviceManager::getInstance() {
    if (nullptr == mInstance) {
        mInstance = new DeviceManager();
    }

    return mInstance;
}

int DeviceManager::initialize(){
	//TODO:May load from configure file and add devices
	//addDevice("192.168.0.23", 5006, 1);
	//Power ON the added devices
	sendDeviceCmdbyIndex("#01O1\r\n", 1);

	/*
	mEPollFD = epoll_create(MAX_SOCKET_SIZE);
	if(mEPollFD < 0)
	{
		LOG_ERROR << "DeviceManager::initialize, create EPOLL failed!";
		return -1;
	}

	struct epoll_event event;
	//struct epoll_event ev[MAX_SOCKET_SIZE];

	for(auto iter = mDeviceList.cbegin(); iter != mDeviceList.cend(); iter++){
		event.data.fd = (iter)->getSocketFD();
		//event.data.ptr =
		event.events = EPOLLIN|EPOLLET;
		LOG_INFO <<"DeviceManager::initialize, socket FD is: " << (iter)->getSocketFD();
		LOG_INFO <<"DeviceManager::initialize, EPOLL FD is: " << mEPollFD;
		int ret = epoll_ctl(mEPollFD,EPOLL_CTL_ADD,iter->getSocketFD(),&event);
		if(0 != ret)
		{
			perror("epoll_ctl");
			LOG_ERROR << "DeviceManager::initialize, register to EPOLL for device:" << iter->getDeviceIndex() << " failed!, return value is: " << ret;
			return -1;
		}
	}
	*/

	return 0;
}

void DeviceManager::addDevice(string deviceIP, int devicePort, int deviceIndex){
	Device device(deviceIP, devicePort, deviceIndex);
	int ret = device.connectWithTCP(deviceIP, devicePort);
	if(ret > 0) {
		if(mDeviceList.empty()){
			mDeviceList.push_back(device);
			LOG_INFO <<"DeviceManager::addDevice success, added the first device with index: " << deviceIndex;
		}
		else{
			auto iter = mDeviceList.cbegin();
			for(; iter != mDeviceList.cend(); iter++){
				//Already assigned the index
				if(iter->getDeviceIndex() == deviceIndex){
					LOG_ERROR << "DeviceManager::addDevice failed! Conflict for index: " << deviceIndex;
					break;
				}
			}
			if(iter == mDeviceList.cend()){
				mDeviceList.push_back(device);
				LOG_INFO <<"DeviceManager::addDevice success with index: " << deviceIndex;
			}
		}
	}
	else{
		LOG_ERROR <<"DeviceManager::addDevice failed, cannot connect to the device for index: " << deviceIndex;
	}
}

int DeviceManager::removeDevicebyIndex(int index){
	if(mDeviceList.empty()){
		LOG_ERROR << "DeviceManager::removeDevicebyIndex failed! Device list is empty!";
	}
	else{
		auto iter = mDeviceList.cbegin();
		for(iter; iter != mDeviceList.cend(); iter++){
			//Found the index
			if(iter->getDeviceIndex() == index){
				iter->closeTCPSocket();
				mDeviceList.erase(iter);
				LOG_INFO << "DeviceManager::removeDevicebyIndex Success for device index: " << index;
				return 0;
			}
		}

		if(iter == mDeviceList.cend()){
			LOG_ERROR << "DeviceManager::removeDevicebyIndex failed! Cannot find the device by the index: " << index;
		}
	}

	return -1;
}

int DeviceManager::clearDevice(){
	if(!mDeviceList.empty()) {
		vector<Device>().swap(mDeviceList);
	}

	return 0;
}

int DeviceManager::sendDeviceCmdbyIndex(const string& command, const int& index){
	if(mDeviceList.empty()){
		LOG_ERROR << "DeviceManager::sendDeviceCmdbyIndex failed! Device list is empty!";
	}
	else{
		for(const auto& device : mDeviceList){
			//Found the index
			if(device.getDeviceIndex() == index){
				device.sendCommand(command.c_str(), command.length());
				return 0;
			}
		}

		LOG_ERROR << "DeviceManager::sendDeviceCmdbyIndex failed! Cannot find the device by the index!";
	}

	return -1;
}
