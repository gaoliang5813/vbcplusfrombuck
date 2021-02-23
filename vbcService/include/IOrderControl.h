//
// Created by joey on 12/14/20.
//

#ifndef MQTTPARSER_IORDERCONTROL_H
#define MQTTPARSER_IORDERCONTROL_H

#include <string>

using namespace std;

class IOrderControl
{
public:
	virtual void onParse(string topic, string payload)  = 0;
};


#endif //MQTTPARSER_IORDERCONTROL_H
