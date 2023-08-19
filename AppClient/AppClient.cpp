// AppClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream> 
#include <cstring>
#include <ws2tcpip.h> 
#include <winsock2.h>
#include <string>

#define TRUE 1
#define FALSE 0

int main(int argc, char* argv[])
{
    WSADATA wsaData;
    int r, done; 
    struct addrinfo hints, *server;
    const char* port = "65001"; //65001                    //make sure port is available
    char* host;
    std::string LOCALHOST = "127.0.0.1";
    const int BUFSIZE = 2048;
    char buffer[BUFSIZE] = {};
    fd_set read_fd;

    if (argc < 2)
    {
        host = const_cast<char*>(LOCALHOST.c_str());      //run the server on localhost 
    }
    else
    {
        host = argv[1];
    }

    //----------------------------------------------------------------------------------------

    r = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (r != 0) {
        std::cerr << "Handle initialization error!!\n";
        exit(FALSE);
    }

    memset(&hints, 0, sizeof(hints));  // initialize all the datamember to zero
    hints.ai_family = AF_INET;         // IPv4 connection 
    hints.ai_socktype = SOCK_STREAM;   // TCP streaming
    hints.ai_protocol = 0;             // 0 or IPPROTO_xxx for IPv4 and IPv6
    r = getaddrinfo(host, port, &hints, &server);
    if (r != 0) {
        std::cerr << "Failed getaddrinfo server" << std::endl;
        exit(FALSE);
    }

    /* create a socket */
    SOCKET clientfd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if (clientfd == -1) {
        std::cerr << "Faild to create client socket\n";
        exit(FALSE);
    }

    printf("Connecting to the server...%s\n", host);
    /* connect to the socket */
    r = connect(clientfd, server->ai_addr, server->ai_addrlen);
    if (r == -1) {
        std::cerr << "Faild to connect server\n";
        closesocket(clientfd);
        exit(FALSE);
    }

    /* loop to interact with the server */
    done = FALSE;
    while (!done)
    {
        // initialize file descriptor set
        FD_ZERO(&read_fd);

        /*add client socket fd to fd_set*/
        FD_SET(clientfd, &read_fd);  // add the socket

        /*
        By using FD_SET(0, &read_fd), you're indicating that you want to monitor the standard input for read availability using the select() function. 
        This can be useful in situations where you want to wait until there's data available to read from the standard input before proceeding.*/
        FD_SET(0, &read_fd);    /* add standard input */

        r = select(clientfd + 1, &read_fd, NULL, NULL, 0);
         
        /*if (r == SOCKET_ERROR)
        {
            std::cerr << "Failed\n";
            exit(FALSE);
        }*/

        /* from server */
        if (FD_ISSET(clientfd, &read_fd))
        {
            memset(buffer, 0, BUFSIZE);
            r = recv(clientfd, buffer, BUFSIZE, 0);
            //if recieved byte 0, disconnect
            if (r < 1)
            {
                std::cout << "Connection closed\n";
                break;
            }

            std::cout << buffer << std::endl; 
        }
        
        /* client input */
        if (FD_ISSET(0, &read_fd))
        {
            std::string msg_client;
            std::getline(std::cin, msg_client);
            // do not send empty message
            if (msg_client == "close")
            {
                done = TRUE;
                msg_client.clear();
                std::cout << "Disconected... See Ya!!\n";
            }
            // send the message to server
            send(clientfd, msg_client.c_str(), sizeof(msg_client), 0);
        }
    } 

    // Close the socket
    closesocket(clientfd);

    return 0;
}
