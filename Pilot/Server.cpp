#include "stdafx.h"
#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <thread>

#include "Server.h"
#include "Timer.h"
#include "Room.h"
#include "NetworkProtocols.h"
#include "Ship.h"



void Server::InitServer(char * ipAddress)
{
	m_portNum = 33303;
	if (ipAddress != nullptr)
	{
		m_ipAddress = ipAddress;
	}
	else 
	{
		m_ipAddress = INADDR_ANY;
	}


	m_isServer = true;

	QuickDraw window;
	View & view = (View &)window;
	m_controller = &(Controller &)window;

	/// game enviroment
	m_room = new Room(-400, 400, 100, -500);

	// Create a timer to measure the real time since the previous game cycle.
	Timer timer;
	timer.mark(); // zero the timer.
	double lasttime = timer.interval();
	double avgdeltat = 0.0;

	double scale = 1.0;

	// Set up the socket.
	m_socket_d = socket(AF_INET, SOCK_DGRAM, 0);

	m_server_addr.sin_family = AF_INET;		 // host byte order
	m_server_addr.sin_port = htons(m_portNum); // short, network byte order
	inet_pton(AF_INET, m_ipAddress, &m_server_addr.sin_addr.s_addr);
	//inet_pton(AF_INET, "127.0.0.1", &m_server_addr.sin_addr.s_addr);
	memset(&(m_server_addr.sin_zero), '\0', 8); // zero the rest of the struct

	if (bind(m_socket_d, (struct sockaddr *) &m_server_addr, sizeof(struct sockaddr)) == -1)
	{
		std::cerr << "Bind error: " << WSAGetLastError() << ", Line " << __LINE__ <<"\n";
		return;
	}

	u_long iMode = 1;
	ioctlsocket(m_socket_d, FIONBIO, &iMode); // put the socket into non-blocking mode.

	/// Network handler
	std::cout << "Making network thread" << std::endl;
	std::thread networkHandler(&Server::clientHandler,this);

	

	std::cout << "waiting for connections..." << std::endl;
	while (true)
	{
		// Calculate the time since the last iteration.
		double currtime = timer.interval();
		double deltat = currtime - lasttime;

		// Run a smoothing step on the time change, to overcome some of the
		// limitations with timer accuracy.
		avgdeltat = 0.2 * deltat + 0.8 * avgdeltat;
		deltat = avgdeltat;
		lasttime = lasttime + deltat;

		// Update the m_model - the representation of the game environment.
		//m_model->update(deltat);
		//m_model->update(0.016);

		//std::cout << "Server run: " << deltat << "             \r";
		//cout << "Server running: " << deltat << "\n";

		// Allow the environment to update.
		m_model->update(deltat);

		//std::cout << "DT: " << std::to_string(deltat) << std::endl;

		// Schedule a screen update event.
		view.clearScreen();
		double offsetx = 0.0;
		double offsety = 0.0;
		m_model->display(view, offsetx, offsety, scale);

		//std::ostringstream score;
		//score << "SERVER ";
		//view.drawText(20, 20, score.str());
		view.swapBuffer();
	}
}


char * Server::serialize(int code, int & size)
{

	char * data;

	switch (code)
	{
	case CREATE_ROOM: {

		int modelsize = sizeof(int) * 5;
		data = new char[size];

		// TODO: change variables into server vars not hardcoded
		*(int *)data = code;
		(*(int*)(data + (sizeof(int) * 1))) = -400;
		(*(int*)(data + (sizeof(int) * 2))) = 400;
		(*(int*)(data + (sizeof(int) * 3))) = 100;
		(*(int*)(data + (sizeof(int) * 4))) = -500;

		for (unsigned int i = 0; i < m_clientList.size(); i++)
		{
			// Send environment updates to clients.
			int addr = m_clientList[i].ip;
			int port = m_clientList[i].port;
			struct sockaddr_in dest_addr;
			dest_addr.sin_family = AF_INET;
			dest_addr.sin_addr.s_addr = addr;
			dest_addr.sin_port = port;
			
			sendto(m_socket_d, data, modelsize, 0, (const sockaddr *) &(dest_addr), sizeof(dest_addr));

		}

		delete[] data;
		std::cout << "CREATE_ROOM" << std::endl;
		break;
	}
	case NO_PLAYERS: {
		size = sizeof(int);
		data = new char[size];
		*(int *)data = code;
		break;
	}
	case WORLDUPDATE: {
		// length of each data element
		int elementSize = (sizeof(int) * 1) + ((sizeof(bool) * 4));
		size = sizeof(int) + (elementSize * m_playerList->size());
		// var to pack server update into
		data = new char[size];
		*(int *)data = code;
		for (size_t i = 0; i < m_playerList->size(); i++)
		{
			(*(int*)(data + (sizeof(int)) + (elementSize * i))) = m_playerList->at(i)->m_playerGameID;
			(*(bool*)(data + (sizeof(int)) + sizeof(int) + (elementSize * i))) = m_playerList->at(i)->forward;
			(*(bool*)(data + (sizeof(int)) + sizeof(int) + (sizeof(bool) * 1) + (elementSize * i))) = m_playerList->at(i)->left;
			(*(bool*)(data + (sizeof(int)) + sizeof(int) + (sizeof(bool) * 2) + (elementSize * i))) = m_playerList->at(i)->right;
			(*(bool*)(data + (sizeof(int)) + sizeof(int) + (sizeof(bool) * 3) + (elementSize * i))) = m_playerList->at(i)->fire;
			}

		break;
	}
	case REVIVE:
		data = new char[1];
		break;
	default:
		data = new char[1];
		break;
	}

	return data;
}

void Server::deserialize(char * data, int size)
{

	MESSAGECODES msgType = (*(MESSAGECODES*)(data));
	int elementSize = -1;

	switch (msgType)
	{
	case NEW_PLAYER: {
		do
		{
			// make sure player is accepted
			if (m_clientList_mutex.try_lock())
			{
				addPlayer(*(NewPlayer*)(data));
				m_clientList_mutex.unlock();
				break;
			}
			else
				m_mutexBlockCounter++;
		} while (true);

		for (unsigned int i = 0; i < m_clientList.size(); i++)
		
{
			int n = 0;
			// Send environment updates to clients.
			int addr = m_clientList[i].ip;
			int port = m_clientList[i].port;
			int modelsize;
			char * modeldata;
			modeldata = serialize(NEW_PLAYER, modelsize);
			struct sockaddr_in dest_addr;
			dest_addr.sin_family = AF_INET;
			dest_addr.sin_addr.s_addr = addr;
			dest_addr.sin_port = port;
			NewPlayer temp((*(NewPlayer*)(data)).m_playerID, (*(NewPlayer*)(data)).m_playerName);
			do
			{
				n = sendto(m_socket_d, (const char *)&temp, sizeof(temp), 0, (const sockaddr *) &(dest_addr), sizeof(dest_addr));
				if (n <= 0)
				{
					std::cout << "Send failed: " << n << "\n";
				}
			} while (n <= 0); // retransmit if send fails. This server is capable of producing data faster than the machine can send it.
							  //			cout << "Sending world: " << n << " " << modelsize << " to " << hex << addr << dec << "\n";
			delete[] modeldata;
		}
		//NewPlayer addMePlease((*(NewPlayer*)(data)).m_playerID, (*(NewPlayer*)(data)).m_playerName);
		//sendto(m_socket_d, (const char *)&addMePlease, sizeof(addMePlease), 0, (const sockaddr *) &(dest_addr), sizeof(dest_addr));

		//std::cout << "Send NEW_PLAYER" << std::endl;
		//std::cout << "0" << std::endl;
		break;
	}
	case ALIVE: {
		//std::cout << " MESSAGE: ALIVE" << std::endl;
		int size = (sizeof(int) * 5);
		serialize(CREATE_ROOM, size);
		//std::cout << "0" << std::endl;
		break;
	}
	case PING: {
		//std::cout << " MESSAGE: PING" << std::endl;
		int size = (sizeof(int) * 2);
		serialize(PING, size);
		//std::cout << "0" << std::endl;
		break;
	}
	case PLAYER_MOVEMENT: {

		//std::cout << "PLAYER_MOVEMENT";
		int playerID = (*(int*)(data + (sizeof(int))));
		for (size_t i = 0; i < m_playerList->size(); i++)
		{
			if (m_playerList->at(i)->m_playerGameID == playerID)
			{
				m_playerList->at(i)->forward = (*(bool*)(data + sizeof(int) + sizeof(int)));
				m_playerList->at(i)->left = (*(bool*)(data + sizeof(int) + sizeof(int) + (sizeof(bool) * 1)));
				m_playerList->at(i)->right = (*(bool*)(data + sizeof(int) + sizeof(int) + (sizeof(bool) * 2)));
				m_playerList->at(i)->fire = (*(bool*)(data + sizeof(int) + sizeof(int) + (sizeof(bool) * 3)));
			}
		}
		//std::cout << "0" << std::endl;
		break;
	}
	default: {
		//std::cout << "SERVER RECIEVED -  ERROR: << " << std::to_string(msgType) << " >> message type is not recognised" << std::endl;
		m_packetLoss++;
		//std::cout << "1" << std::endl;
		break;
	}
	}
	m_packetTotal++;
}

void Server::addPlayer(NewPlayer & np)
{
	// add ship to server data
	std::shared_ptr<PlayerDetails> newPlayer = std::make_shared<PlayerDetails>();
	newPlayer->m_playerGameID = np.m_playerID;
	newPlayer->m_playerName = np.m_playerName;
	
			m_playerList->push_back(newPlayer);
	

	// add ship to enviroment
	Ship * s = new Ship(*m_controller, Ship::NETWORKPLAYER, np.m_playerName, np.m_playerID);
	m_model->addActor(s);
}


void Server::sendNetworkDataInThread(ClientAddr address)
{
	int modelsize;
	char * modeldata;
	if (m_playerList->size() > 0)
	{
		modeldata = serialize(WORLDUPDATE, modelsize);
	}
	else
		modeldata = serialize(NO_PLAYERS, modelsize);

	int n;
	int addr = address.ip;
	int port = address.port;
	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = addr;
	dest_addr.sin_port = port;
	do
	{
		n = sendto(m_socket_d, modeldata, modelsize, 0, (const sockaddr *) &(dest_addr), sizeof(dest_addr));
		if (n <= 0)
		{
			std::cout << "Send failed: " << n << "\n";
		}
	} while (n <= 0);

	delete[] modeldata;
}

void Server::clientHandler()
{
	while (true)
	{
		int n = 0;
		struct sockaddr_in aClientsAddress; // connector's address information
		int addr_len = sizeof(struct sockaddr);
		char buf[5000];
		int numbytes = 5000;

		// Receive requests from clients.
		if ((n = recvfrom(m_socket_d, buf, numbytes, 0, (struct sockaddr *)&aClientsAddress, &addr_len)) == -1)
		{
			if (WSAGetLastError() != WSAEWOULDBLOCK) // A real problem - not just avoiding blocking.
			{
				static int cnt = 0;
				std::string errMsg = "\nRecv error (" + std::to_string(cnt++) + "): " + std::to_string(WSAGetLastError()) + "\n";
				std::cerr << errMsg;		// Keep the error message together...
			}
		}
		else
		{
			//		cout << "Received: " << n << "\n";
			// Add any new clients provided they have not been added previously.
			bool found = false;
			
			//std::cout << "Client.size() == " << m_clientList.size() << std::endl;
			for (unsigned int i = 0; i < m_clientList.size(); i++)
			{
				if ((m_clientList[i].ip == aClientsAddress.sin_addr.s_addr) && (m_clientList[i].port == aClientsAddress.sin_port))
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				// if max client limit is not hit
				if (m_clientList.size() != m_maxClients)
				{
					m_clientList_mutex.lock();
					m_clientList.push_back(ClientAddr(aClientsAddress.sin_addr.s_addr, aClientsAddress.sin_port));
					m_clientList_mutex.unlock();
					char their_source_addr[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &(aClientsAddress.sin_addr), their_source_addr, sizeof(their_source_addr));
					std::cout << "Adding client: " << their_source_addr << " on port " << ntohs(aClientsAddress.sin_port) << std::endl;
				}
				// limit of client reached. new client is told to wait.
				else
				{
					int modelsize = sizeof(int);
					char * modeldata;
					modeldata = new char[modelsize];
					*(int *)modeldata = MATCH_FULL;
					sendto(m_socket_d, modeldata, modelsize, 0, (const sockaddr *) &(aClientsAddress.sin_addr), sizeof(aClientsAddress.sin_addr));
					delete[] modeldata;
				}
				
			}
			if ((*(MESSAGECODES*)(buf)) == PING)
			{
				//std::chrono::seconds timespan(2); // testing lag

				//std::this_thread::sleep_for(timespan);

				struct sockaddr_in dest_addr;
				dest_addr.sin_family = AF_INET;
				dest_addr.sin_addr.s_addr = aClientsAddress.sin_addr.s_addr;
				dest_addr.sin_port = aClientsAddress.sin_port;
				int modelsize = sizeof(int);
				char * modeldata;
				modeldata = new char[modelsize];
				*(int *)modeldata = PING;
				int n = sendto(m_socket_d, modeldata, modelsize, 0, (const sockaddr *) &(dest_addr), sizeof(dest_addr));
				if(n == -1)
				{
					std::cout << "<<error>> ping not sent";
				}
				delete[] modeldata;
				//std::cout << "0" << std::endl;
			}
			else {
				deserialize(buf, n);
			}

		}

		// Send environment updates to clients.
		

		//m_clientList_mutex.try_lock();
		for (unsigned int i = 0; i < m_clientList.size(); i++)
		{

			sendNetworkDataInThread(m_clientList[i]);
			
		}
		//m_clientList_mutex.unlock();

	}
}

