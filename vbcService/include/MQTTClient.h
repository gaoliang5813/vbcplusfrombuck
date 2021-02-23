//
// Created by jackie on 12/15/20.
//

#ifndef __VBSSERVICE_MQTTCLIENT_H__
#define __VBSSERVICE_MQTTCLIENT_H__

#include <string>
#include "mqtt/delivery_token.h"
#include "mqtt/callback.h"
#include "mqtt/async_client.h"
#include "IOrderControl.h"

class MQTTClient: public virtual mqtt::callback,
                   public virtual mqtt::iaction_listener
{
public:
    MQTTClient(string address, string clientID, IOrderControl& callbackOrderControl);
    ~MQTTClient();
    int connect();
    int publish(string topic, string payload, int QoS);
    int subscribe(string topic, int QoS);
    int disconnect();

private:
    int initialize();
    int uninitialize();
	string getActionType(mqtt::token::Type actionType);

    //callbacks
    void connected(const string& cause) override;
    void connection_lost(const string& cause) override;
    void delivery_complete(mqtt::delivery_token_ptr tok) override;
    void message_arrived(mqtt::const_message_ptr msg) override;

    //connection listener callbacks
    void on_failure(const mqtt::token& tok) override;
    void on_success(const mqtt::token& tok) override;

    mqtt::async_client *mPahoClient;
    mqtt::connect_options* mConnOpts;
    mqtt::token_ptr mConnTok;
    IOrderControl& mIOrderControl;
    string	mBrokerAddress;
    string	mClientID;
    int mRetryTimes;
};
#endif //__VBSSERVICE_MQTTCLIENT_H__
