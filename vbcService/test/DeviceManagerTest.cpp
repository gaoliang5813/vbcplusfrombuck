//
// Created by joey on 12/24/20.
//

//#define BOOST_TEST_INCLUDED

#undef main
#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>
#include "../include/DeviceManager.h"

BOOST_AUTO_TEST_SUITE (testDeviceManager) // name of the test suite is stringtest

	BOOST_AUTO_TEST_CASE (testGetInstance)
	{
		DeviceManager* deviceManager = DeviceManager::getInstance();
		BOOST_CHECK(deviceManager != nullptr);
	}

	BOOST_AUTO_TEST_CASE (testDeviceManager)
	{
		DeviceManager* deviceManager = DeviceManager::getInstance();
		deviceManager->addDevice("192.168.9.12", 5006, 1);
		BOOST_REQUIRE_EQUAL (1, deviceManager->getDeviceList().size());
		deviceManager->addDevice("192.168.9.12", 8080, 2);
		vector<Device> myDevices = deviceManager->getDeviceList();
		BOOST_REQUIRE_EQUAL (2, myDevices.size());
		BOOST_REQUIRE_EQUAL (2, deviceManager->getDeviceList().size());
		deviceManager->removeDevicebyIndex(2);
		BOOST_REQUIRE_EQUAL (1, deviceManager->getDeviceList().size());
		deviceManager->removeDevicebyIndex(1);
		BOOST_REQUIRE_EQUAL (0, deviceManager->getDeviceList().size());

		deviceManager->clearDevice();
		BOOST_REQUIRE_EQUAL (2, myDevices.size());
		BOOST_REQUIRE_EQUAL (0, deviceManager->getDeviceList().size());
	}

	BOOST_AUTO_TEST_CASE (testSendCommand)
	{
		DeviceManager* deviceManager = DeviceManager::getInstance();
		deviceManager->initialize();

		sleep(2);
		int ret = deviceManager->sendDeviceCmdbyIndex("#01O1\r\n", 3);
		BOOST_REQUIRE_EQUAL (0, ret);

		sleep(2);
		ret = deviceManager->sendDeviceCmdbyIndex("#01P01\r\n", 3);
		BOOST_REQUIRE_EQUAL (0, ret);
	}

BOOST_AUTO_TEST_SUITE_END( )
