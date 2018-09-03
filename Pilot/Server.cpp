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


void Server::InitServer(unsigned short port, char * ipAddress)
{
	m_portNum = port;
	if (ipAddress != nullptr)
	{
		m_ipAddress = ipAddress;
	}
	else 
	{
		m_ipAddress = INADDR_ANY;
	}
	

	// Create a timer to measure the real time since the previous game cycle.
	Timer timer;
	timer.mark(); // zero the timer.
	double lasttime = timer.interval();
	double avgdeltat = 0.0;

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

	Room model(-400, 400, 100, -500);

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

		// Update the model - the representation of the game environment.
		model.update(deltat);

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

			// Process any player to server messages.
			switch (*(int *)buf) // First int of each packet is the message code.
			{
			case ALIVE:
			{	
			}
			break;
			case ADDLINK:
			{
			}
			break;
			case REMOVELINKS:
			{
			}
			break;
			default:
			{
				//				cout << "Unknown message code: " << * (int *) buf << "\n";
			}
			break;
			}
		}

		// Server to player - send environment updates. Very clunky - try to do better.
		for (unsigned int i = 0; i < m_clientList.size(); i++)
		{
			// Send environment updates to clients.
			int addr = m_clientList[i].ip;
			int port = m_clientList[i].port;
			int modelsize;
			char * modeldata = model.serialize(WORLDUPDATE, modelsize);
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

		std::cout << "Server run: " << deltat << "             \r";
		//cout << "Server running: " << deltat << "\n";
	}
}

