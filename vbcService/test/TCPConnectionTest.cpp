//
// Created by joey on 12/28/20.
//

#define BOOST_TEST_INCLUDED

#include <thread>
#include "../include/TCPConnection.h"
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_SUITE (testTCPConnection) // name of the test suite is stringtest

	BOOST_AUTO_TEST_CASE (testSendCommand)
	{
		const char* TCPServerIP = "192.168.0.23";
		int port = 5006;

		TCPConnection* tcpConnection = new TCPConnection;
		BOOST_CHECK(tcpConnection != nullptr);
		int sock = tcpConnection->connectWithTCP(TCPServerIP, port);
		LOG_INFO <<"connectWithTCP return, sockfd is:" << to_string(sock);

		sleep(2);
		string cmd = "#01O1\r\n";

		LOG_INFO << cmd;
		tcpConnection->sendCommand(cmd.c_str(), cmd.length());

		//std::thread recThread(&TCPConnection::receive, tcpConnection);
		//recThread.join(); // 等待线程结束

		sleep(2);
		cmd = "#01P01\r\n";
		LOG_INFO << cmd;
		tcpConnection->sendCommand(cmd.c_str(), cmd.length());

		tcpConnection->closeTCPSocket();
		BOOST_CHECK(tcpConnection != nullptr);
	}



BOOST_AUTO_TEST_SUITE_END( )