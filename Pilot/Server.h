#pragma once
#ifndef SERVER_SINGLE
#define SERVER_SINGLE

#include <vector>

#include "Room.h"

struct ClientAddr
{
	int ip;
	int port;

	ClientAddr(int IP, int PORT) : ip(IP), port(PORT) {}
};

class Server
{
public:
	static Server& getInstance()
	{
		static Server    instance;
		return instance;
	}

private:


	// Address info 
	int m_portNum;
	char * m_ipAddress;
	int m_socket_d;
	struct sockaddr_in m_server_addr;	// my address information

	std::vector<ClientAddr> m_clientList;

	std::shared_ptr<Room> model = std::make_shared<Room>(-400, 400, 100, -500);
	

	Server() {};

public:
	Server(Server const&) = delete;
	void operator=(Server const&) = delete;

	/// Class Methods
	void InitServer(char* ipAddress = nullptr);

	// Serialise the data
	char * serialize(int code, int & size);

	// deserialize the environment from a block.
	void deserialize(char * data, int size);

};



#endif // !SERVER_SINGLE

