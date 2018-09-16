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

Client::Client()
{
}


Client::~Client()
{
}

void Client::Initclient(char * serverIP, char * clientID)
{

	std::string playersName;
	std::cin >> playersName;

	QuickDraw window;
	View & view = (View &)window;
	Controller & controller = (Controller &)window;

	/// game enviroment
	Room m_model(-400, 400, 100, -500);
	m_ship = new Ship(controller, Ship::INPLAY, "You");
	m_model.addActor(m_ship);

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
	Alive aliveMsg(m_gameID);
	sendto(m_socket_d, (const char *)&aliveMsg, sizeof(aliveMsg), 0, (const sockaddr *) &(m_server_addr), sizeof(m_server_addr));
	//sendto(m_socket_d, (const char *)&aliveMsg, sizeof(aliveMsg), 0, (const sockaddr *) &(m_server_addr), sizeof(m_server_addr));

	// add this client as an active player on the server.
	NewPlayer addMePlease(m_gameID,playersName);
	sendto(m_socket_d, (const char *)&addMePlease, sizeof(addMePlease), 0, (const sockaddr *) &(m_server_addr), sizeof(m_server_addr));


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


		// Allow the environment to update.
		m_model.update(deltat);

		// Schedule a screen update event.
		view.clearScreen();
		double offsetx = 0.0;
		double offsety = 0.0;
		(*m_ship).getPosition(offsetx, offsety);
		m_model.display(view, offsetx, offsety, scale);

		std::ostringstream score;
		score << "Score: " << m_ship->getScore();
		view.drawText(20, 20, score.str());
		view.swapBuffer();

		// send current data to server after revieving current game state
		//PlayerStats stats(player);
		//sendto(m_socket_d, (const char *)&stats, sizeof(stats), 0, (const sockaddr *) &(m_server_addr), sizeof(m_server_addr));
		serialize(PLAYER_STATS);
	}
}



void Client::serialize(int code)
{
	switch (code)
	{
	case ALIVE:
		break;
	case KILL:
		break;
	case REVIVE:
		break;
		case PLAYER_STATS:
		{
			PlayerStats stats = m_ship->getShipNetworkStats();
			sendto(m_socket_d, (const char *)&stats, sizeof(PlayerStats), 0, (const sockaddr *) &(m_server_addr), sizeof(m_server_addr));
			break;
		}
	case POSITION_BULLET:
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
	case WORLDUPDATE:
		std::cout << "CLIENT RECIEVED - MESSAGE: WORLDUPDATE" << std::endl;
		break;
	default:
		std::cout << "CLIENT RECIEVED -  MESSAGE: DEFAULT = " << std::to_string(msgType) << std::endl;
		break;
	}


}
