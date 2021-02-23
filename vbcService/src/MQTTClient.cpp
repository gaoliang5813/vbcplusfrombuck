//
// Created by jackie on 12/15/20.
//

#include "MQTTClient.h"
#include <iostream>
#include <chrono>
#include "Logger.h"

#define TIMEOUT chrono::milliseconds(2500)
#define MAXRETRYTIMES 10

MQTTClient::MQTTClient(string address, string clientID, IOrderControl& callbackOrderControl):
		mPahoClient(nullptr), mConnOpts(nullptr), mConnTok(nullptr), mIOrderControl(callbackOrderControl),
		mBrokerAddress(address), mClientID(clientID), mRetryTimes(0)
{
    initialize();
}

MQTTClient::~MQTTClient()
{
    uninitialize();
}

int MQTTClient::initialize()
{
	LOG_INFO << "Initializing MQTT Client for server '" << mBrokerAddress << "'..." ;
	mPahoClient = new mqtt::async_client(mBrokerAddress, mClientID);
    if(nullptr == mPahoClient){
	    LOG_ERROR << "  ...Failed to Initialize Paho MQTT Client!";
	    return -1;
    }

	LOG_INFO << "  ...Initialized OK";
    mConnOpts = new mqtt::connect_options;
	if(nullptr == mConnOpts){
		LOG_ERROR << "  ...Failed to Initialize Paho MQTT connect options!";
		return -1;
	}

    mConnOpts->set_keep_alive_interval(20);
    mConnOpts->set_clean_session(true);

    mPahoClient->set_callback(*this);

    return 0;
}

int MQTTClient::uninitialize()
{
    if(nullptr != mPahoClient) {
        delete mPahoClient;
	    mPahoClient = nullptr;
    }

    if(nullptr != mConnOpts) {
        delete mConnOpts;
        mConnOpts = nullptr;
    }

	if(nullptr != mConnTok) {
		mConnTok = nullptr;
	}

    return 0;
}

int MQTTClient::connect()
{
    try {
	    LOG_INFO << "Connecting...";
        mConnTok = mPahoClient->connect(*mConnOpts, nullptr, *this);
	    LOG_INFO << "Waiting for the connection...";
	    mConnTok->wait();
	    LOG_INFO << "  ...OK" ;
    }
    catch (const mqtt::exception& exc) {
	    LOG_ERROR << exc.what() ;
        return -1;
    }

    return 0;
}

int MQTTClient::disconnect()
{
    // Double check that there are no pending tokens
    try{
        auto toks = mPahoClient->get_pending_delivery_tokens();
        if (!toks.empty())
	        LOG_ERROR << "Error: There are pending delivery tokens!";

        // Disconnect
	    LOG_INFO << "Disconnecting..." ;
        mConnTok = mPahoClient->disconnect();
        mConnTok->wait_for(TIMEOUT);
	    LOG_INFO << "  ...OK";
    }
    catch (const mqtt::exception& exc) {
	    LOG_ERROR << exc.what() ;
        return -1;
    }

    return 0;
}

int MQTTClient::publish(string topic, string payload, int QoS)
{
    try{
        // use a message pointer.
	    LOG_INFO << "Sending message...";
        mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
        pubmsg->set_qos(QoS);
	    mConnTok = mPahoClient->publish(pubmsg);
	    mConnTok ->wait_for(TIMEOUT);
	    LOG_INFO << "  ...OK";
    }
    catch (const mqtt::exception& exc) {
	    LOG_ERROR << exc.what();
        return -1;
    }

    return 0;
}

int MQTTClient::subscribe(string topic, int QoS)
{
	try{
		LOG_INFO << "Subscribing to topic '" << topic
	              << " ' for client " << mClientID
	              << " using QoS " << QoS;

		mConnTok = mPahoClient->subscribe(topic, QoS, nullptr, *this);
		mConnTok ->wait_for(TIMEOUT);
		LOG_INFO << "  ...OK";
	}
	catch (const mqtt::exception& exc) {
		LOG_ERROR << exc.what();
		return -1;
	}

    return 0;

}

/////////////////////////////////////////////////////////////////////////////
//get the string for request action type
string MQTTClient::getActionType(mqtt::token::Type actionType)
{
	switch (actionType) {
		case mqtt::token::CONNECT:
			return "CONNECT";
		case mqtt::token::SUBSCRIBE:
			return "SUBSCRIBE";
		case mqtt::token::PUBLISH:
			return "PUBLISH";
		case mqtt::token::UNSUBSCRIBE:
			return "UNSUBSCRIBE";
		case mqtt::token::DISCONNECT:
			return "DISCONNECT";
		default:
			break;
	}

	return "";
}

/**
 * Callback functions for use with the main MQTT client.
 */
void MQTTClient::on_failure(const mqtt::token& tok)
{
	LOG_ERROR << "Action type:" << getActionType(tok.get_type()) << " failure!";
    if (tok.get_message_id() != 0) {
	    LOG_ERROR << " for token: [" << tok.get_message_id() << "]" ;
    }

    //Try to reconnect
    if ((++mRetryTimes <= MAXRETRYTIMES) && (mqtt::token::CONNECT == tok.get_type())) {
	    connect();
    }
}

void MQTTClient::on_success(const mqtt::token& tok)
{
	LOG_INFO << "Action type:" << getActionType(tok.get_type()) << " success!";
    if (tok.get_message_id() != 0) {
	    LOG_INFO << " for token: [" << tok.get_message_id() << "]";
        auto top = tok.get_topics();
        if (top && !top->empty()) {
	        LOG_INFO << "token topic: '" << (*top)[0] << "', ...";
        }
    }
}

// (Re)connection success
void MQTTClient::connected(const string& cause)
{
	LOG_INFO << "Connection success";
}

void MQTTClient::connection_lost(const string& cause)
{
	LOG_ERROR << "Connection lost";
    if (!cause.empty()){
	    LOG_ERROR << "\tcause: " << cause;
    }

	LOG_INFO << "Reconnecting...";
    mRetryTimes = 0;
    connect();
}

void MQTTClient::delivery_complete(mqtt::delivery_token_ptr tok)
{
	LOG_INFO << "Delivery complete for token: "
         << (tok ? tok->get_message_id() : -1) ;
}

// Callback for when a message arrives.
void MQTTClient::message_arrived(mqtt::const_message_ptr msg)
{
	string topic = msg->get_topic();
	string payload = msg->to_string();
	LOG_INFO << "Message arrived!";
	LOG_INFO << "topic: '" << topic << "'";
	LOG_INFO << "payload: '" << payload << "'";

    //Callback to upper layer to parse the message
    mIOrderControl.onParse(topic, payload);

}
/////////////////////////////////////////////////////////////////////////////