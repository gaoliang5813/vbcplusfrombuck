//
// Created by joey on 12/11/20.
//

#include "json/json.h"
#include "OrderControl.h"
#include "StreamManager.h"
#include "DeviceManager.h"

COrderControl::COrderControl(string address, string clientID, string connName):
		mMQTTClient(nullptr), mBrokerAddress(address), mClientID(clientID), mConnName(connName),
		mMode(0), mPositionIndex(0), mScale(0), mVelocity(0)
{
	mStreamManager = StreamManager::getInstance();
	mDeviceManager = DeviceManager::getInstance();
}

COrderControl::~COrderControl()
{
	if(nullptr != mMQTTClient)
	{
		delete mMQTTClient;
		mMQTTClient = nullptr;
	}
}

int COrderControl::initialize()
{
	//Initialize DeviceManager to add devices and create TCP connection with them
	mDeviceManager->initialize();

	//create MQTT connection with Cloud Manager
	mMQTTClient = new MQTTClient( mBrokerAddress, mClientID, *this);
	if(nullptr == mMQTTClient){
		LOG_ERROR << "  ...Failed to Initialize vbcService MQTT Client!";
		return -1;
	}

	mMQTTClient->connect();
	mMQTTClient->subscribe(mConnName + "-c-c", 0); //device control command
	mMQTTClient->subscribe(mConnName + "-c-v", 0); //video stream command

	return 0;
}

void COrderControl::onParse(string topic, string payload)
{
	Json::Value jsonMessage;
	//jsonMessage.clear();
	Json::String err;
	Json::CharReaderBuilder myReader;
	std::unique_ptr<Json::CharReader>const json_read(myReader.newCharReader());
	if(nullptr != json_read){
		json_read->parse(payload.c_str(), payload.c_str() + payload.length(), &jsonMessage, &err);
		LOG_INFO << "COrderControl::onParse,  parse json! ";
	}

	//Parse the commands and parameters from MQTT messages
	mCommand = "", mSubtitleText = "", mEffectFileName = "";
	mMode = 0, mPositionIndex = 0, mScale = 0, mVelocity = 0;
	vector<int>().swap(mControlIndexList);
	if (jsonMessage.isMember("M") && jsonMessage["M"].isString())
	{
		mCommand = jsonMessage["M"].asString();
		LOG_INFO << "M is: " << mCommand;
	}
	if (jsonMessage.isMember("P") && jsonMessage["P"].isObject())
	{
		if (jsonMessage["P"].isMember("n") && jsonMessage["P"]["n"].isInt())
		{
			mPositionIndex = jsonMessage["P"]["n"].asInt();
			LOG_INFO << "mPositionIndex is: " << mPositionIndex;
		}
		if (jsonMessage["P"].isMember("m") && jsonMessage["P"]["m"].isInt())
		{
			mMode = jsonMessage["P"]["m"].asInt();
			LOG_INFO << "mMode is: " << mMode;
		}
		if (jsonMessage["P"].isMember("s") && jsonMessage["P"]["s"].isInt())
		{
			mScale = jsonMessage["P"]["s"].asInt();
			LOG_INFO << "mScale is: " << mScale;
		}
        if (jsonMessage["P"].isMember("t") && jsonMessage["P"]["t"].isString())
        {
	        mSubtitleText = jsonMessage["P"]["t"].asString();
	        LOG_INFO << "mSubtitleText is: " << mSubtitleText;
        }
		if (jsonMessage["P"].isMember("f") && jsonMessage["P"]["f"].isString())
		{
			mEffectFileName = jsonMessage["P"]["f"].asString();
			LOG_INFO << "mEffectFileName is: " << mEffectFileName;
		}
		if (jsonMessage["P"].isMember("v") && jsonMessage["P"]["v"].isInt())
		{
			mVelocity = jsonMessage["P"]["v"].asInt();
			LOG_INFO << "mVelocity is: " << mVelocity;
		}
		if (jsonMessage["P"].isMember("i") && jsonMessage["P"]["i"].isArray())
		{
			for (const auto& nDeviceIndex : jsonMessage["P"]["i"])
			{
				if (nDeviceIndex.isInt())
				{
					mControlIndexList.push_back(nDeviceIndex.asInt());
					LOG_INFO <<"Device index is:" << nDeviceIndex.asInt();
				}
			}
		}
	}
	//Parse the commands and parameters from MQTT messages, End!

	if(topic == (mConnName + "-c-c")){
		executeDeviceCommand();
	}
	else if(topic == (mConnName + "-c-v")){
		executeStreamCommand();
	}
}

string COrderControl::translateDeviceCommand(const string& jsonCommand) const
{
	string cmd;
	char formatInt[3] = {0};
	if(jsonCommand == "ON"){
		LOG_INFO << "COrderControl::translateDeviceCommand, pan power on!";
		cmd = "#01O1\r\n";
	}
	else if(jsonCommand == "OF")
	{
		LOG_INFO << "COrderControl::translateDeviceCommand, pan power off!";
		cmd = "#01O0\r\n";
	}
	else if(jsonCommand == "P")
	{
		LOG_INFO << "COrderControl::translateDeviceCommand, pan turn left/right : " << mVelocity;
		snprintf(formatInt, sizeof(formatInt), "%02d", mVelocity);
		cmd = "#01P" + string(formatInt) + "\r\n";
	}
	else if(jsonCommand == "T")
	{
		LOG_INFO << "COrderControl::translateDeviceCommand, pan tilt up/down: " << mVelocity;
		snprintf(formatInt, sizeof(formatInt), "%02d", mVelocity);
		cmd = "#01T" + string(formatInt) + "\r\n";
	}
	else if(jsonCommand == "Z")
	{
		LOG_INFO << "COrderControl::translateDeviceCommand, pan zoom in/out: " << mVelocity;
		snprintf(formatInt, sizeof(formatInt), "%02d", mVelocity);
		cmd = "#01Z" + string(formatInt) + "\r\n";
	}
	else if(jsonCommand == "F")
	{
		LOG_INFO << "COrderControl::translateDeviceCommand, pan focus closely/far: " << mVelocity;
		snprintf(formatInt, sizeof(formatInt), "%02d", mVelocity);
		cmd = "#01F" + string(formatInt) + "\r\n";
	}
	else if(jsonCommand == "SP")
	{
		LOG_INFO << "COrderControl::translateDeviceCommand, pan store position: " << mPositionIndex;
		snprintf(formatInt, sizeof(formatInt), "%02d", mPositionIndex);
		cmd = "#01M" + string(formatInt) + "\r\n";
	}
	else if(jsonCommand == "GP")
	{
		LOG_INFO << "COrderControl::translateDeviceCommand, pan restore position: " << mPositionIndex;
		snprintf(formatInt, sizeof(formatInt), "%02d", mPositionIndex);
		cmd = "#01R" + string(formatInt) + "\r\n";
	}
	else if(jsonCommand == "IS")
	{
		LOG_INFO << "COrderControl::translateDeviceCommand, pan iris size: " << mScale;
		snprintf(formatInt, sizeof(formatInt), "%02d", mScale);
		cmd = "#01I" + string(formatInt) + "\r\n";
	}
	else if(jsonCommand == "IA")
	{
		LOG_INFO << "COrderControl::translateDeviceCommand, pan iris auto mode!";
		cmd = "#01D31\r\n";
	}
	else if(jsonCommand == "IM")
	{
		LOG_INFO << "COrderControl::translateDeviceCommand, pan iris manual mode!";
		cmd = "#01D30\r\n";
	}

	return cmd;
}

int COrderControl::executeDeviceCommand()
{
	LOG_INFO << "COrderControl::executeDeviceCommand!";

	for(const auto& deviceIndex : mControlIndexList) {
		mDeviceManager->sendDeviceCmdbyIndex(translateDeviceCommand(mCommand), deviceIndex);
		LOG_INFO << "COrderControl::executeDeviceCommand, send control command to device index:" << deviceIndex;
	}

	return 0;
}

int COrderControl::executeStreamCommand()
{
	LOG_INFO << "COrderControl::executeStreamCommand!";

	if(mCommand == "PG")
	{
		LOG_INFO << "COrderControl::executeStreamCommand, set device as PGM with mMode: " << mMode;
		// StreamManager starts to switch PVW as PGM
		StreamManager::getInstance()->switchPVWToPGM();
		//StreamManager::getInstance()->setStreamToPVW((StreamManager::getInstance()->mPGMSourceIndex + 1) % 4);
	}
	else if(mCommand == "PV")
	{
		LOG_INFO << "COrderControl::executeStreamCommand, set device as PVW with mMode: " << mMode;
		// StreamManager starts to set a device as PVW
		for(const auto& streamIndex : mControlIndexList) {
			StreamManager::getInstance()->setStreamToPVW(streamIndex-1);
		}
	}
	else if (mCommand == "TT") {
		LOG_INFO << "COrderControl::executeStreamCommand, add subtitles for PVW with mMode: " << mMode;
		// StreamManager starts to add subtitles for PVW
		StreamManager::getInstance()->updateSubtitle(mSubtitleText.c_str(), mMode);
	}
	else if (mCommand == "LG") {
		LOG_INFO << "COrderControl::executeStreamCommand, add logo for PVW with mMode: " << mMode;
		// StreamManager starts to add logo for PVW
        StreamManager::getInstance()->updateLogo(mEffectFileName.c_str(), mMode);
	}
	else if (mCommand == "XF") {
		LOG_INFO << "COrderControl::executeStreamCommand, xfade for PVW with mMode: " << mMode;
		// StreamManager starts to xfade for PVW
	}
	else if (mCommand == "SC") {
		LOG_INFO << "COrderControl::executeStreamCommand, mScale for PVW with mMode: " << mMode;
		// StreamManager starts to mScale for PVW
	}
	else if (mCommand == "SS") {
		LOG_INFO << "COrderControl::executeStreamCommand, split screen for PVW with mMode: " << mMode;
		// StreamManager starts to split screen for PVW
		for (int i = 0; i < mControlIndexList.size(); ++i) {
		    mControlIndexList.at(i) = mControlIndexList.at(i) - 1;
		}
		StreamManager::getInstance()->updateSplitScreen(mControlIndexList, mMode);
	}

	return 0;
}
