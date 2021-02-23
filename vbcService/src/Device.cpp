//
// Created by joey on 12/23/20.
//
#include "Device.h"

Device::Device(	string deviceIP, int devicePort, int deviceIndex)
	:mDeviceIP(deviceIP),mDevicePort(devicePort), mDeviceIndex(deviceIndex)
{

}

Device::~Device(){

}

void Device::setDeviceIP(string deviceIP)
{
	mDeviceIP = deviceIP;
}

void Device::setDevicePort(int devicePort)
{
	mDevicePort = devicePort;
}

void Device::setDeviceIndex(int deviceIndex)
{
	mDeviceIndex = deviceIndex;
}