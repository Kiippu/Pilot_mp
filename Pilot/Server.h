#pragma once
#ifndef SERVER_SINGLE
#define SERVER_SINGLE

#pragma comment(lib, "ws2_32.lib")

#include <vector>



/// Server Messages
enum MESSAGECODES { ALIVE, ADDLINK, REMOVELINKS, WORLDUPDATE };

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

	Server() {};

public:
	Server(Server const&) = delete;
	void operator=(Server const&) = delete;

	/// Class Methods
	void InitServer(unsigned short port, char* ipAddress);




};



#endif // !SERVER_SINGLE

