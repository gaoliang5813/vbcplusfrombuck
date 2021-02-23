//
// Created by joey on 12/28/20.
//

#define BOOST_TEST_INCLUDED

#include <boost/test/included/unit_test.hpp>
#include "../include/OrderControl.h"

BOOST_AUTO_TEST_SUITE (testOrderControl) // name of the test suite is stringtest

	BOOST_AUTO_TEST_CASE (testOnParse)
	{
		const char* brokerIP = "tcp://10.9.9.125:1883";
		const char* clientID = "mqttClientID";
		const char* connectionName = "videoBroadCast";

		COrderControl* orderControl = new COrderControl(brokerIP, clientID, connectionName);
		orderControl->onParse("videoBroadCast-c-c", "Hardcode for testing");
		BOOST_CHECK(orderControl != nullptr);
	}



BOOST_AUTO_TEST_SUITE_END( )