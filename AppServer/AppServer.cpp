#include <ws2tcpip.h>
#include <iostream>
#include <winsock2.h> 
#include <string>
#include <unordered_map>

#define TRUE 1;
#define FALSE 0;

int main(int argc, char* argv[])
{
	WSADATA wsaData;
	const int BUFSIZE = 2048;
	int hostname_size = 32;
	const char* connection_msg = "Type 'close' to disconnect; 'shutdown' to stop server\n";
	fd_set main_fd, read_fd;
	const int backlog = 10;
	struct addrinfo hints, *server;
	int r, run;
	char *host;
	const char* port = "65001"; //65001                    //make sure port is available
	std::string LOCALHOST = "127.0.0.1";
	char buffer[BUFSIZE] = {};
	const int MAX_CONNECT = 10;
	SOCKET serverfd, clientfd;
	struct sockaddr client_address;
	socklen_t client_len = sizeof(client_address);
	std::unordered_map<SOCKET, std::string> connections;
	

	if (argc < 2)
	{
		host = const_cast<char*>(LOCALHOST.c_str());      //run the server on localhost 
	}
	else
	{
		host = argv[1];
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr<<"Handle initialization error\n";
		exit(1);
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;			// IPv4 connection 
	hints.ai_socktype = SOCK_STREAM;	// TCP streaming
	hints.ai_protocol = 0;				// 0 or IPPROTO_xxx for IPv4 and IPv6
	//1. connection with localhost on port 8080
	if (getaddrinfo(host, port, &hints, &server) != 0) {
		std::cerr << "Failed getaddrinfo server" << std::endl;
		exit(1);
	}

	//2. Create a socket using the socket() function. Need to specify the address family, socket type, and protocol
	serverfd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
	if (serverfd == INVALID_SOCKET) {
		std::cerr << "Faield creating socket\n";
		WSACleanup();
		exit(1);
	}

	//3. Bind the socket to a specific IP address and port using bind(),
	r = bind(serverfd, server->ai_addr, server->ai_addrlen);
	if (r == SOCKET_ERROR) {
		closesocket(serverfd);
		WSACleanup();
		std::cerr << "Failed to bind the server\n";
		exit(1);
	}

	//4. start listening for incoming connections with listen():
	r = listen(serverfd, MAX_CONNECT);
	if (r == SOCKET_ERROR) {
		closesocket(serverfd);
		WSACleanup();
		std::cerr << "Failed to go live server!!\n";
		exit(1);
	}
	std::cout << "Server listening...at: "<<host<<':'<< port << "\n";

	/*create fd_sets for multiple connections*/
	FD_ZERO(&main_fd);
	FD_SET(serverfd, &main_fd);  // set the server's file descriptor

	run = FALSE;
	while (!run)
	{
		read_fd = main_fd;    // back up the main fd to read_fd
		r = select(MAX_CONNECT + 1, &read_fd, NULL, NULL, 0);
		
		if (r == -1)
		{
			std::cerr << "Failed\n";
			exit(1);
		}

		// process the connections
		for (int idx = 0; idx <= MAX_CONNECT && r > 0; idx++)
		{
			SOCKET fd = read_fd.fd_array[idx];
			// only active or new clients
			if (FD_ISSET(fd, &read_fd))
			{
				// if new client request
				if (fd == serverfd)
				{
					clientfd = accept(serverfd, &client_address, &client_len);
					if (clientfd == -1)
					{
						std::cerr << "Failed to accept new client request\n";
						exit(1);
					}
					
					r = getnameinfo(&client_address, client_len, buffer, BUFSIZE, 0, 0, NI_NUMERICHOST);
					connections[clientfd] = buffer;
					std::cout << "New connection: " << buffer << std::endl;

					FD_SET(clientfd, &main_fd);

					std::string str = "Hello, " + connections[clientfd] + "!\n" + connection_msg;
					r = send(clientfd, str.c_str(), str.size(), 0);

					// send message to all the user on the server and inform about new connection
					std::string msgtoall = "SERVER> " + connections[clientfd] + " has joined the chat\n";
					for (int x = idx + 1; idx <= MAX_CONNECT; idx++)
					{
						SOCKET fd = read_fd.fd_array[idx];
						if (FD_ISSET(read_fd.fd_array[x], &main_fd))
						{
							send(fd, msgtoall.c_str(), msgtoall.size(), 0);
						}
					}
					std::cout << msgtoall;
				}
				// update from current fd
				else
				{
					r = recv(fd, buffer, BUFSIZE, 0);
					if (r < 1)
					{
						// disconnect the client
						FD_CLR(fd, &main_fd);
						closesocket(fd);
						std::cout<< connections[fd] <<" Disconnected!!\n";
						connections.erase(fd);
					}
					else
					{
						buffer[r] = '\0'; // terminate the string
						if (strcmp(buffer, "shutdown") == 0)
						{
							run = TRUE; // terminate the loop
							std::cout << "Server shutdowned\n";
							break;
						}
						else
						{
							std::string msg = buffer;
							msg = "Echo: " + msg;

							for (int x = 0; x < main_fd.fd_count; x++)
							{
								SOCKET fd = main_fd.fd_array[x];
								if (FD_ISSET(fd, &main_fd))
								{
									send(fd, msg.c_str(), msg.size(), 0);
								}
							}
							//send(fd, msg.c_str(), msg.size(), 0);
						}
					}
				}
			}
		}
	} 

	closesocket(serverfd);
	freeaddrinfo(server);
	return 0;
}