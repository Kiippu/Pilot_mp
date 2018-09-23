#include "stdafx.h"
#include <iostream>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <ws2tcpip.h>
#include <sstream>


#include "Client.h"
#include "Timer.h"
#include "Room.h"
#include "NetworkProtocols.h"
#include "Ship.h"


const int gamePort = 33303;

void Client::Initclient(char * serverIP, char * clientID)
{

	std::cin >> m_usersName;

	QuickDraw window;
	View & view = (View &)window;
	Controller & controller = (Controller &)window;

	m_controller = &controller;

	m_isClient = true;

	/// game enviroment
	//Room m_model(-400, 400, 100, -500);
	//m_ship = new Ship(controller, Ship::INPLAY, m_usersName, (int)(*clientID - '0'));
	//m_model.addActor(m_ship);

	m_networkMovement = new PlayerMovement((int)(*clientID - '0'));

	//// Add some opponents. These are computer controlled - for the moment...
	//Ship * opponent;
	//opponent = new Ship(controller, Ship::AUTO, "Bob");
	//m_model.addActor(opponent);
	//opponent = new Ship(controller, Ship::AUTO, "Fred");
	//m_model.addActor(opponent);
	//opponent = new Ship(controller, Ship::AUTO, "Joe");
	//m_model.addActor(opponent);

	// Create a timer to measure the real time since the previous game cycle.
	Timer timer;
	timer.mark(); // zero the timer.
	double lasttime = timer.interval();
	double avgdeltat = 0.0;

	double scale = 1.0;

	m_gameID = atoi(clientID);

	// Set up the socket.
	// Argument is an IP address to send packets to. Multicast allowed.
	m_socket_d = socket(AF_INET, SOCK_DGRAM, 0);

	m_client_addr.sin_family = AF_INET;		 // host byte order
	m_client_addr.sin_port = htons(gamePort + 1 + m_gameID); // short, network byte order
	m_client_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
	memset(&(m_client_addr.sin_zero), '\0', 8); // zero the rest of the struct

	if (bind(m_socket_d, (struct sockaddr *) &m_client_addr, sizeof(struct sockaddr)) == -1)
	{
		std::cerr << "Bind error: " << WSAGetLastError() << "\n";
		return;
	}

	u_long iMode = 1;
	ioctlsocket(m_socket_d, FIONBIO, &iMode); // put the socket into non-blocking mode.

	m_server_addr.sin_family = AF_INET;
	inet_pton(AF_INET, serverIP, &m_server_addr.sin_addr.s_addr);
	m_server_addr.sin_port = htons(gamePort);


	// Send a welcome message to server saying this client is 'alive'
	// Will get this client added to server list...
	serialize(ALIVE);
	//sendto(m_socket_d, (const char *)&aliveMsg, sizeof(aliveMsg), 0, (const sockaddr *) &(m_server_addr), sizeof(m_server_addr));

	// Very similar to the single player version - spot the difference.
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

		// Check for updates from the server.
		int addr_len = sizeof(struct sockaddr);
		char buf[50000];
		int numbytes = 50000;
		int n;

		// Receive requests from server.
		if ((n = recvfrom(m_socket_d, buf, numbytes, 0, (struct sockaddr *)&m_server_addr, &addr_len)) == -1)
		{
			std::cout << "Nothing recevied..." << "\n";
			if (WSAGetLastError() != WSAEWOULDBLOCK) // A real problem - not just avoiding blocking.
			{
				std::cerr << "Recv error: " << WSAGetLastError() << "\n";
			}
		}
		else
		{
			/// get incomming data and sendit to the clients enviroment
			deserialize(buf, n);

		}

		// Schedule a screen update event.
		//view.clearScreen();
		//m_model.display(view, offsetx, offsety, scale, player);
		//view.swapBuffer();

		if (m_roomReady && m_model != nullptr)
		{
			// Allow the environment to update.
			m_model->update(deltat);
			//m_model->update(0.016);

			//std::cout << "DT: " << std::to_string(deltat) << std::endl;

			// Schedule a screen update event.
			view.clearScreen();
			double offsetx = 0.0;
			double offsety = 0.0;
			(*m_ship).getPosition(offsetx, offsety);
			m_model->display(view, offsetx, offsety, scale);

			std::ostringstream score;
			score << "Score: " << m_ship->getScore();
			view.drawText(20, 20, score.str());
			view.swapBuffer();

			// send current data to server after revieving current game state
			//PlayerStats stats(player);
			//sendto(m_socket_d, (const char *)&stats, sizeof(stats), 0, (const sockaddr *) &(m_server_addr), sizeof(m_server_addr));
			//serialize(PLAYER_STATS);
		}
		else
			std::cout << "m_roomReady == false or m_model == nullptr" << "                                 \r";
	}
}



void Client::serialize(int code)
{
	switch (code)
	{
	case ALIVE: {
		Alive aliveMsg(m_gameID);
		sendto(m_socket_d, (const char *)&aliveMsg, sizeof(aliveMsg), 0, (const sockaddr *) &(m_server_addr), sizeof(m_server_addr));
		break;
	}
	case DEAD: {
		Dead deadMsg(m_gameID);
		sendto(m_socket_d, (const char *)&deadMsg, sizeof(deadMsg), 0, (const sockaddr *) &(m_server_addr), sizeof(m_server_addr));
		break;
	}
	case REVIVE: {
		Revive reviveMsg(m_gameID);
		sendto(m_socket_d, (const char *)&reviveMsg, sizeof(reviveMsg), 0, (const sockaddr *) &(m_server_addr), sizeof(m_server_addr));
		break;
	}
	case NEW_PLAYER: {
		NewPlayer addMePlease(m_gameID, m_usersName);
		sendto(m_socket_d, (const char *)&addMePlease, sizeof(addMePlease), 0, (const sockaddr *) &(m_server_addr), sizeof(m_server_addr));
		break;
	}
	case PLAYER_MOVEMENT: {
		int size = sizeof(int) + sizeof(int) + ((sizeof(bool) * 4) + sizeof(int));

		char * data = new char[size];
		*(int*)data = code;
		(*(int*)(data + sizeof(int))) = m_gameID;
		(*(bool*)(data + sizeof(int) + sizeof(int))) = m_networkMovement->forward;
		(*(bool*)(data + sizeof(int) + sizeof(int) + (sizeof(bool) * 1))) = m_networkMovement->left;
		(*(bool*)(data + sizeof(int) + sizeof(int) + (sizeof(bool) * 2))) = m_networkMovement->right;
		(*(bool*)(data + sizeof(int) + sizeof(int) + (sizeof(bool) * 3))) = m_networkMovement->fire;

		sendto(m_socket_d, data, size, 0, (const sockaddr *) &(m_server_addr), sizeof(m_server_addr));
		delete data;
		break;
	}
	case PLAYER_STATS:
		{
			int size = sizeof(int) + sizeof(int) + (sizeof(double) * 5);

			char * data = new char[size];
			//std::cout << "player ID: " << std::to_string(m_gameID);
			*(int*)data = code;
			(*(int*)(data + sizeof(int))) = m_gameID;
			(*(double*)(data + sizeof(int) + sizeof(int))) = m_ship->getShipNetworkStats().posx;
			(*(double*)(data + sizeof(int) + sizeof(int) + (sizeof(double) * 1))) = m_ship->getShipNetworkStats().posy;
			(*(double*)(data + sizeof(int) + sizeof(int) + (sizeof(double) * 2))) = m_ship->getShipNetworkStats().speed;
			(*(double*)(data + sizeof(int) + sizeof(int) + (sizeof(double) * 3))) = m_ship->getShipNetworkStats().vx;
			(*(double*)(data + sizeof(int) + sizeof(int) + (sizeof(double) * 4))) = m_ship->getShipNetworkStats().vy;

			//PlayerStats stats = m_ship->getShipNetworkStats();
			sendto(m_socket_d, data, size, 0, (const sockaddr *) &(m_server_addr), sizeof(m_server_addr));
			delete data;
			break;
		}
	case SHOOT_BULLET:
		break;
	default:
		break;
	}
}

void Client::deserialize(char * data, int size)
{

	MESSAGECODES msgType = (*(MESSAGECODES*)(data));

	switch (msgType)
	{
	case CREATE_ROOM: {
		if (m_model == nullptr)
		{


			std::cout << "client - CREATE_ROOM" << std::endl;
			int left =		(*(int*)(data + (sizeof(int) * 1)));
			int right =		(*(int*)(data + (sizeof(int) * 2)));
			int top =		(*(int*)(data + (sizeof(int) * 3)));
			int bottom =	(*(int*)(data + (sizeof(int) * 4)));

			m_model = new Room(left, right, top, bottom);

			std::cout << "Right = " << std::to_string(right) << " -- Left = " << std::to_string(left) << " -- top = " << std::to_string(top) << " -- bottom = " << std::to_string(bottom) << std::endl;
			
			// add this client as an active player on the server.
			serialize(NEW_PLAYER);

			std::shared_ptr<PlayerDetails> newPlayer = std::make_shared<PlayerDetails>();
			newPlayer->m_playerGameID = m_gameID;
			newPlayer->m_playerName = m_usersName;
			m_playerList->push_back(newPlayer);

			// add ship to enviroment
			m_ship = new Ship(*m_controller, Ship::NETWORKPLAYER, m_usersName, m_gameID);
			m_model->addActor(m_ship);

			m_roomReady = true;
		}

		break;
	}
	case WORLDUPDATE: {

		int elementSize = (sizeof(int) * 3) + ((sizeof(bool) * 4));// +sizeof(std::string);
		size_t arraySize = (*(int*)(data + sizeof(int)));

		auto list = Client::getInstance().getPlayerDetails();

		for (size_t i = 0; i < list->size(); i++)
		{
			//std::vector<Type> v = ....;
			//std::string myString = ....;
			int id = (*(int*)(data + (sizeof(int) * 2) + (elementSize * i)));
			auto it = std::find_if(list->begin(), list->end(), [&id](std::shared_ptr<PlayerDetails> obj) {return obj->getID() == id; });

			if (it != list->end())
			{
				auto index = std::distance(list->begin(), it);

				list->at(index)->forward = (*(bool*)(data + (sizeof(int) * 2) + sizeof(int) + (elementSize * i)));
				list->at(index)->left = (*(bool*)(data + (sizeof(int) * 2) + sizeof(int) + (sizeof(bool) * 1) + (elementSize * i)));
				list->at(index)->right = (*(bool*)(data + (sizeof(int) * 2) + sizeof(int) + (sizeof(bool) * 2) + (elementSize * i)));
				list->at(index)->fire = (*(bool*)(data + (sizeof(int) * 2) + sizeof(int) + (sizeof(bool) * 3) + (elementSize * i)));

				//std::cout << "Forward: " << std::to_string(list->at(index)->forward) << "Left: " << std::to_string(list->at(index)->left) << "Right: " << std::to_string(list->at(index)->right) << "Fire: " << std::to_string(list->at(index)->fire) << std::endl;
			}
			else
			{
				std::shared_ptr<PlayerDetails> temp = std::make_shared<PlayerDetails>();
				temp->forward = (*(bool*)(data + (sizeof(int) * 2) + sizeof(int) + (elementSize * i)));
				temp->left = (*(bool*)(data + (sizeof(int) * 2) + sizeof(int) + (sizeof(bool) * 1) + (elementSize * i)));
				temp->right = (*(bool*)(data + (sizeof(int) * 2) + sizeof(int) + (sizeof(bool) * 2) + (elementSize * i)));
				temp->fire = (*(bool*)(data + (sizeof(int) * 2) + sizeof(int) + (sizeof(bool) * 3) + (elementSize * i)));
					
				temp->m_playerGameID = (*(int*)(data + (sizeof(int) * 2) + (elementSize * i)));
				//temp->m_playerName = (*(std::string*)(data + (sizeof(int) * 2) + sizeof(int) + (sizeof(bool) * 4) + (elementSize * i)));

				list->push_back(temp);
				std::cout << "Client - added new player!" << std::endl;
			}
		}


		//m_serverPlayerState.resize(arraySize);
		////std::cout << "CLIENT RECIEVED - MESSAGE: WORLDUPDATE - array size = " << std::to_string(arraySize) << std::endl;
		//for (size_t i = 0; i < arraySize; i++)
		//{

		//	


		//	PlayerMovement temp((*(int*)(data + (sizeof(int) * 2) + (elementSize * i))));
		//	m_serverPlayerState.push_back(temp);
		//	m_serverPlayerState[i] = temp;
		//	m_serverPlayerState[i].forward = (*(bool*)(data + (sizeof(int) * 2) + sizeof(int) + (elementSize * i)));
		//	m_serverPlayerState[i].left = (*(bool*)(data + (sizeof(int) * 2) + sizeof(int) + (sizeof(bool) * 1) + (elementSize * i)));
		//	m_serverPlayerState[i].right = (*(bool*)(data + (sizeof(int) * 2) + sizeof(int) + (sizeof(bool) * 2) + (elementSize * i)));
		//	m_serverPlayerState[i].fire = (*(bool*)(data + (sizeof(int) * 2) + sizeof(int) + (sizeof(bool) * 3) + (elementSize * i)));

		//	//std::cout << "Right = " << std::to_string(m_serverPlayerState[i].right) << " -- Left = " << std::to_string(m_serverPlayerState[i].left) << " -- fire = " << std::to_string(m_serverPlayerState[i].fire) << " -- forward = " << std::to_string(m_serverPlayerState[i].forward) << std::endl;
		//}


		break;
	}
	case NO_PLAYERS:
		std::cout << "CLIENT RECIEVED - MESSAGE: NO_PLAYERS" << std::endl;
	default:
		std::cout << "CLIENT RECIEVED -  ERROR MSG = " << std::to_string(msgType) << std::endl;
		break;
	}


}
