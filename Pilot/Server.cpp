#include "stdafx.h"
#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <algorithm>

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
	memset(&(m_server_addr.sin_zero), '\0', 8); // zero the rest of the struct

	if (bind(m_socket_d, (struct sockaddr *) &m_server_addr, sizeof(struct sockaddr)) == -1)
	{
		std::cerr << "Bind error: " << WSAGetLastError() << ", Line " << __LINE__ <<"\n";
		return;
	}

	u_long iMode = 1;
	ioctlsocket(m_socket_d, FIONBIO, &iMode); // put the socket into non-blocking mode.

	/// Game enviroment

	

	while (true)
	{
		int n;

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
		m_model->update(0.016);

		struct sockaddr_in aClientsAddress; // connector's address information
		int addr_len = sizeof(struct sockaddr);
		char buf[50000];
		int numbytes = 50000;

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
				m_clientList.push_back(ClientAddr(aClientsAddress.sin_addr.s_addr, aClientsAddress.sin_port));
				char their_source_addr[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &(aClientsAddress.sin_addr), their_source_addr, sizeof(their_source_addr));
				std::cout << "Adding client: " << their_source_addr << " " << ntohs(aClientsAddress.sin_port) << "\n";
			}
			
			deserialize(buf,n);

		}

		// Server to player - send environment updates. Very clunky - try to do better.
		for (unsigned int i = 0; i < m_clientList.size(); i++)
		{
			// Send environment updates to clients.
			int addr = m_clientList[i].ip;
			int port = m_clientList[i].port;
			int modelsize;
			char * modeldata = serialize(WORLDUPDATE, modelsize);
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
			} while (n <= 0); // retransmit if send fails. This server is capable of producing data faster than the machine can send it.
							  //			cout << "Sending world: " << n << " " << modelsize << " to " << hex << addr << dec << "\n";
			delete[] modeldata;
		}

		//std::cout << "Server run: " << deltat << "             \r";
		//cout << "Server running: " << deltat << "\n";

		// Allow the environment to update.
		m_model->update(deltat);

		std::cout << "DT: " << std::to_string(deltat) << std::endl;

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
	//EXAMPLE FROM BATTLEMULTI.sln
	//// used to turn game datain to bytes and send as packets
	//
	//int elementsize = sizeof(double) + sizeof(int) + sizeof(double);
	//size = sizeof(int) + sx * sy * elementsize;

	//char * data = new char[size];
	//*(int *)data = code;
	//for (int x = 0; x < sx; x++)
	//{
	//	for (int y = 0; y < sy; y++)
	//	{
	//		(*(double*)(data + sizeof(int) + (y * sx + x) * elementsize)) = (*environment[y * sx + x]).content;
	//		(*(int*)(data + sizeof(int) + (y * sx + x) * elementsize + sizeof(double))) = (*environment[y * sx + x]).owner;
	//		(*(double*)(data + sizeof(int) + (y * sx + x) * elementsize + sizeof(double) + sizeof(int))) = (*environment[y * sx + x]).production;
	//	}
	//}
	//return data;

	////

	int elementSize = -1;
	char * data;

	switch (code)
	{
	case ALIVE:
		data = new char[1];
		break;
	case WORLDUPDATE: {
		size = sizeof(MESSAGECODES); //+elementsize;
		data = new char[size];
		*(int *)data = code;



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
		addPlayer(*(NewPlayer*)(data));
		break;
	}
	case ALIVE:
		std::cout << " MESSAGE: ALIVE" << std::endl;
		break;
	case PLAYER_MOVEMENT: {

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
		break;
	}
	case PLAYER_STATS:
	{
		int playerID = (*(int*)(data + (sizeof(int))));

		for (size_t i = 0; i < m_playerList->size(); i++)
		{
			if (m_playerList->at(i)->m_playerGameID == playerID)
			{
				m_playerList->at(i)->posx =		(*(double*)(data + sizeof(int) + sizeof(int)));
				m_playerList->at(i)->posy =		(*(double*)(data + sizeof(int) + sizeof(int) + (sizeof(double) * 1)));
				m_playerList->at(i)->speed =	(*(double*)(data + sizeof(int) + sizeof(int) + (sizeof(double) * 2)));
				m_playerList->at(i)->vx =		(*(double*)(data + sizeof(int) + sizeof(int) + (sizeof(double) * 3)));
				m_playerList->at(i)->vy =		(*(double*)(data + sizeof(int) + sizeof(int) + (sizeof(double) * 4)));
			}
		}
		break;
	}
	default:
		std::cout << "SERVER RECIEVED -  ERROR: << " << std::to_string(msgType) << " >> message type is not recognised" << std::endl;
		break;
	}
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

