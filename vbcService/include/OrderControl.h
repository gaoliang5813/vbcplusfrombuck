//
// Created by joey on 12/11/20.
//

#ifndef MQTTPARSER_CORDERCONTROL_H
#define MQTTPARSER_CORDERCONTROL_H

#include "MQTTClient.h"

class StreamManager;
class DeviceManager;

class COrderControl: public IOrderControl{
public:
	COrderControl(string address, string clientID, string connName);
	~COrderControl();
	int initialize();

	//callback from MQTT Client
	void onParse(string topic, string payload) override;

	string translateDeviceCommand(const string& jsonCommand) const;
	int executeDeviceCommand();
	int executeStreamCommand();

private:
	MQTTClient * mMQTTClient;
	string	mBrokerAddress;
	string	mClientID;
	string mConnName;
	string mCommand, mSubtitleText, mEffectFileName;
	int mMode, mPositionIndex, mScale, mVelocity;
	vector<int> mControlIndexList;
	StreamManager* mStreamManager;
	DeviceManager* mDeviceManager;
};


#endif //MQTTPARSER_CORDERCONTROL_H
