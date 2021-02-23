//
// Created by joey on 12/28/20.
//

#undef main
#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>

#define BOOST_TEST_MODULE MasterTestSuite
//using namespace boost::unit_test;

/*
test_suite*
init_unit_test_suite( int argc, char* argv[] ) {
	test_suite *ts1 = BOOST_TEST_SUITE("testDeviceManager");
	ts1->add(BOOST_TEST_CASE(&testGetInstance));
	ts1->add(BOOST_TEST_CASE(&testDeviceManager));

	test_suite *ts2 = BOOST_TEST_SUITE("testTCPConnection");
	ts2->add(BOOST_TEST_CASE(&testSendCommand));

	test_suite *ts3 = BOOST_TEST_SUITE("testOrderControl");
	ts3->add(BOOST_TEST_CASE(&testOnParse));

	framework::master_test_suite().add(ts1);
	framework::master_test_suite().add(ts2);
	framework::master_test_suite().add(ts3);

	return 0;
}
 */