#include "stdafx.h"
#include <iostream>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <ws2tcpip.h>
#include <sstream>


//#include "Server.h"
#include "Client.h"
#include "Timer.h"
#include "Room.h"
#include "networkEnums.h"
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




	QuickDraw window;
	View & view = (View &)window;
	Controller & controller = (Controller &)window;

	/// game enviroment
	Room model(-400, 400, 100, -500);
	Ship * ship = new Ship(controller, Ship::INPLAY, "You");
	model.addActor(ship);

	//// Add some opponents. These are computer controlled - for the moment...
	//Ship * opponent;
	//opponent = new Ship(controller, Ship::AUTO, "Bob");
	//model.addActor(opponent);
	//opponent = new Ship(controller, Ship::AUTO, "Fred");
	//model.addActor(opponent);
	//opponent = new Ship(controller, Ship::AUTO, "Joe");
	//model.addActor(opponent);

	// Create a timer to measure the real time since the previous game cycle.
	Timer timer;
	timer.mark(); // zero the timer.
	double lasttime = timer.interval();
	double avgdeltat = 0.0;

	double scale = 1.0;

	int player = atoi(clientID);

	// Set up the socket.
	// Argument is an IP address to send packets to. Multicast allowed.
	m_socket_d = socket(AF_INET, SOCK_DGRAM, 0);

	m_client_addr.sin_family = AF_INET;		 // host byte order
	m_client_addr.sin_port = htons(gamePort + 1 + player); // short, network byte order
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
	Alive aliveMsg(player);
	
	sendto(m_socket_d, (const char *)&aliveMsg, sizeof(aliveMsg), 0, (const sockaddr *) &(m_server_addr), sizeof(m_server_addr));
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

		// Check for events from the player.
		int mx, my, mb;
		bool mset;
		controller.lastMouse(mx, my, mb, mset);
		if (mset)
		{
			// EXAMPLE use of finding regions in enviroment
			//int x = (int)(((mx / scale) + offsetx) / gameparameters.CellSize);
			//int y = (int)(((my / scale) + offsety) / gameparameters.CellSize);

			if (mb == WM_LBUTTONDOWN)
			{
				//startx = x;
				//starty = y;
				//startset = true;
			}
			if ((mb == WM_RBUTTONDOWN) /*&& (startset)*/)
			{
				/// EXAMPLE getting a class to do action and sending over network
				//model.addLink(player, startx, starty, x, y);
				//AddLink al(player, startx, starty, x, y);
				//sendto(m_socket_d, (const char *)&al, sizeof(al), 0, (const sockaddr *) &(m_server_addr), sizeof(m_server_addr));

				//startset = false;
			}
			if (mb == WM_MBUTTONDOWN)
			{
				//model.removeLinks(player, x, y);
				//RemoveLinks rl(player, x, y);
				//sendto(m_socket_d, (const char *)&rl, sizeof(rl), 0, (const sockaddr *) &(m_server_addr), sizeof(m_server_addr));
			}
		}

		PlayerStats stats(player, ship);
		sendto(m_socket_d, (const char *)&stats, sizeof(stats), 0, (const sockaddr *) &(m_server_addr), sizeof(m_server_addr));


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
		//model.display(view, offsetx, offsety, scale, player);
		//view.swapBuffer();



		// Allow the environment to update.
		model.update(deltat);

		// Schedule a screen update event.
		view.clearScreen();
		double offsetx = 0.0;
		double offsety = 0.0;
		(*ship).getPosition(offsetx, offsety);
		model.display(view, offsetx, offsety, scale);

		std::ostringstream score;
		score << "Score: " << ship->getScore();
		view.drawText(20, 20, score.str());
		view.swapBuffer();

	}
}



char * Client::serialize(int code, int & size)
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
	case WORLDUPDATE:
		size = sizeof(MESSAGECODES); //+elementsize;
		data = new char[size];
		*(int *)data = code;
		std::cout << "WORLD UPDATE!!!" << std::endl;

		break;
	case KILL:
		data = new char[1];
		break;
	case REVIVE:
		data = new char[1];
		break;
		//case PLAYER_STATS:
		//	// make size of element here
		//	elementSize = sizeof(int) + sizeof(int) + sizeof(Ship);
		//	size = sizeof(MESSAGECODES); + elementSize;

		//	PlayerStats bill();

		//	data = new char[size];
		//	*(int *)data = code;
		//	(*(int*)(data + sizeof(int))) = (*environment[y * sx + x]).owner;
		//	//*(int *)data + sizeof(msgType) = code;

		//	break;
	case POSITION_BULLET:
		data = new char[1];
		break;
	default:
		data = new char[1];
		break;
	}

	return data;
}

void Client::deserialize(char * data, int size)
{

	MESSAGECODES msgType = (*(MESSAGECODES*)(data));

	switch (msgType)
	{
	case ALIVE:
		std::cout << " MESSAGE: ALIVE" << std::endl;
		break;
	case WORLDUPDATE:
		std::cout << " MESSAGE: WORLDUPDATE" << std::endl;
		break;
	case KILL:
		std::cout << " MESSAGE: KILL" << std::endl;
		break;
	case REVIVE:
		std::cout << " MESSAGE: REVIVE" << std::endl;
		break;
	case PLAYER_STATS:
		std::cout << " MESSAGE: PLAYER_STATS" << std::endl;


		break;
	case POSITION_BULLET:
		std::cout << " MESSAGE: POSITION_BULLET" << std::endl;
		break;
	default:
		std::cout << " MESSAGE: DEFAULT = " << std::to_string(msgType) << std::endl;
		break;
	}


}
