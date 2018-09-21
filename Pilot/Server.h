#pragma once
#ifndef SERVER_H
#define SERVER_H

#include <vector>

#include "Room.h"

class NewPlayer;

struct ClientAddr
{
	int ip;
	int port;
	
	ClientAddr(int IP, int PORT) : ip(IP), port(PORT) {}
};

struct PlayerDetails
{
	int m_playerGameID;
	std::string m_playerName;

	// Position of the actor.
	double posx;
	double posy;

	// Magnitude of velocity.
	double speed;
	// Unit velocity vector.
	double vx;
	double vy;
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

	Controller * m_controller;
	Room * m_room;

	std::vector<ClientAddr> m_clientList;

	std::shared_ptr<std::vector<std::shared_ptr<PlayerDetails>>> m_playerList = std::make_shared<std::vector<std::shared_ptr<PlayerDetails>>>();

	std::shared_ptr<Room> m_model = std::make_shared<Room>(-400, 400, 100, -500);
	

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

	void addPlayer( NewPlayer & np);

};



#endif // !SERVER_H

