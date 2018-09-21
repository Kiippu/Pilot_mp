#pragma once
#ifndef CLIENT
#define CLIENT

#include <WinSock2.h>

class Ship;

class Client
{
public:
	Client();
	~Client();


private:

	// Address info 
	int m_portNum;
	char * m_ipAddress;
	int m_socket_d;
	struct sockaddr_in m_client_addr;	// my address information
	struct sockaddr_in m_server_addr;	// server address information

	int m_gameID = -1;

	std::string m_usersName = "no_name";

	Ship * m_ship = nullptr;


public:

	/// Class Methods
	void Initclient(char * serverIP, char * clientID);

	// Serialise the data
	void serialize(int code);

	// deserialize the environment from a block.
	void deserialize(char * data, int size);

};


#endif // !CLIENT



