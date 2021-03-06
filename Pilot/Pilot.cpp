// Pilot.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <WinSock2.h>

#include "Client.h"
#include "Server.h"

#include <sstream>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char * argv[])
{
	// Messy process with windows networking - "start" the networking API.
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (argc == 3)			// 2 arguments
	{
		// client. First argumentL IP address of server, second argument: player ID (Name)
		Client::getInstance().Initclient(argv[1], argv[2]);
	}
	else if (argc == 2)		// Only one argument
	{
		Server::getInstance().InitServer(argv[1]);
	}

	else if (argc == 1)		// Only one argument
	{
		Server::getInstance().InitServer();
	}
	else
	{
		std::cout << "ERROR: Must run the applciaiton with either 0, 1 or 3 arguments..." << "\n";
		std::cout << "E.g. to run as server over whole region:        " << argv[0] << "\n";
		std::cout << "Or, to run as server just use one argument:     " << argv[0] << "<k-d bounds code>\n";
		std::cout << "Otherwise to run as a client use two arguments: " << argv[0] << " <ip-address of server>  <client number>\n";
		return -1;
	}

	WSACleanup();
	return 0;
}


//
//int main(int argc, char * argv[])
//{
//	QuickDraw window;
//	View & view = (View &)window;
//	Controller & controller = (Controller &)window;
//
//	Room m_model(-400, 400, 100, -500);
//	Ship * ship = new Ship(controller, Ship::INPLAY, "You");
//	m_model.addActor(ship);
//
//	// Add some opponents. These are computer controlled - for the moment...
//	Ship * opponent;
//	opponent = new Ship(controller, Ship::AUTO, "Bob");
//	m_model.addActor(opponent);
//	opponent = new Ship(controller, Ship::AUTO, "Fred");
//	m_model.addActor(opponent);
//	opponent = new Ship(controller, Ship::AUTO, "Joe");
//	m_model.addActor(opponent);
//
//	// Create a timer to measure the real time since the previous game cycle.
//	Timer timer;
//	timer.mark(); // zero the timer.
//	double lasttime = timer.interval();
//	double avgdeltat = 0.0;
//
//	double scale = 1.0;
//
//	while (true)
//	{
//		// Calculate the time since the last iteration.
//		double currtime = timer.interval();
//		double deltat = currtime - lasttime;
//
//		// Run a smoothing step on the time change, to overcome some of the
//		// limitations with timer accuracy.
//		avgdeltat = 0.2 * deltat + 0.8 * avgdeltat;
//		deltat = avgdeltat;
//		lasttime = lasttime + deltat;
//
//		// Allow the environment to update.
//		m_model.update(deltat);
//
//		// Schedule a screen update event.
//		view.clearScreen();
//		double offsetx = 0.0;
//		double offsety = 0.0;
//		(*ship).getPosition(offsetx, offsety);
//		m_model.display(view, offsetx, offsety, scale);
//
//		std::ostringstream score;
//		score << "Score: " << ship->getScore();
//		view.drawText(20, 20, score.str());
//		view.swapBuffer();
//	}
//
//	return 0;
//}

