//
// Created by joey on 12/27/20.
//

#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/resource.h>
#include "TCPConnection.h"

TCPConnection::TCPConnection():mSocketFD(0){

}

TCPConnection::~TCPConnection(){
	/*if(mSocketFD > 0){
		close(mSocketFD);
	}*/
}

int TCPConnection::connectWithTCP(string deviceIP, int devicePort)
{
	mSocketFD = socket(AF_INET, SOCK_STREAM, 0);
	if(mSocketFD < 0)
	{
		LOG_ERROR <<"TCPConnection::connectWithTCP, Create socket failed!";
		return -1;
	}

	LOG_INFO <<"TCPConnection::connectWithTCP, socket FD is: " << mSocketFD;
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(devicePort);
	addr.sin_addr.s_addr = inet_addr(deviceIP.c_str());

	if(connect(mSocketFD, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		LOG_ERROR <<"TCPConnection::connectWithTCP, connect failed!";
		close(mSocketFD);
		return -1;
	}

	if(setNonBlocking(mSocketFD) < 0){
		LOG_ERROR << "TCPConnection::connectWithTCP, setNonBlocking failed!";
		return -1;
	}

	return mSocketFD;
}

int TCPConnection::closeTCPSocket() const{
	close(mSocketFD);
	return 0;
}

int TCPConnection::sendCommand(const char* command, int length) const{
	if(mSocketFD < 0){
		LOG_ERROR <<"TCPConnection::sendCommand, Invalid socket fd!";
		return -1;
	}

	send(mSocketFD, command, length,0);

	return 0;
}

void TCPConnection::receive(){
	char recvbuf[1024];
	while(1){
		recv(mSocketFD, recvbuf, sizeof(recvbuf),0); ///接收
		fputs(recvbuf, stdout);

		memset(recvbuf, 0, sizeof(recvbuf));
	}
}

int TCPConnection::setNonBlocking(int socket) const {
	int opts;
	opts=fcntl(socket,F_GETFL);
	if(opts<0)
	{
		LOG_ERROR <<"TCPConnection::setNonBlocking, getFL failed!";
		return -1;
	}
	opts = opts|O_NONBLOCK;
	if(fcntl(socket,F_SETFL,opts)<0)
	{
		LOG_ERROR <<"TCPConnection::setNonBlocking, setFL failed!";
		return -1;
	}

	return 0;
}