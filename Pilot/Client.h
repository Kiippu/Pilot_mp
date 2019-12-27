#pragma once
#ifndef CLIENT
#define CLIENT

#include <vector>
#include <mutex>

// Forward declaration
class Ship;
class PlayerMovement;
class Room;
class Controller;
class PlayerDetails;
class NewPlayer;

class Client
{
public:
	static Client& getInstance()
	{
		static Client    instance;
		return instance;
	}


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

	bool m_roomReady = false;

	Room * m_model = nullptr;

	Controller * m_controller = nullptr;

	std::vector<PlayerMovement> m_serverPlayerState;
	std::mutex m_serverPlayerState_mutex;

	bool m_isClient = false;
	bool m_autoConnect = false;
	double m_autoConnectTimer = 0;
	double m_autoConnectTimerMax = 10.0;

	double m_ping = 0.0;
	double m_pingTimer = 0.0;
	double m_pingTimerMax = 1.0;
	bool m_registerPing = true;

	std::shared_ptr<std::vector<std::shared_ptr<PlayerDetails>>> m_playerList = std::make_shared<std::vector<std::shared_ptr<PlayerDetails>>>();

	void networkHandler();

	int m_mutexBlockCounter = 0;

	Client() {};

public:
	Client(Client const&) = delete;
	void operator=(Client const&) = delete;

	/// Class Methods
	void Initclient(char * serverIP, char * clientID);

	// Serialise the data
	void serialize(int code);

	// deserialize the environment from a block.
	void deserialize(char * data, int size);

	//get current server player details
	std::shared_ptr<std::vector<std::shared_ptr<PlayerDetails>>> & getPlayerDetails() { return  m_playerList; }

	// add player to severs stack
	void addPlayer(NewPlayer & np);

	//is this actively a client
	bool isClient() { return m_isClient; }

	PlayerMovement * m_networkMovement;

};


#endif // !CLIENT



