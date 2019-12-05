//Example code: A simple server side code, which echos back the received input_id_msg.
//Handle multiple socket connections with select and fd_set on Linux
#include <arpa/inet.h>  //close
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  //strlen
#include <sys/socket.h>
#include <sys/time.h>  //FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/types.h>
#include <unistd.h>  //close
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

struct client_data {
    std::string name;
    int id;
};

#define TRUE 1
#define FALSE 0
#define PORT 8711
// #define client_socket_list .size() 30
#define BUFFER_SIZE 1024

/* Messages */
const char input_id_msg[] = "Please input your ID: ";
const std::string welcome_msg = "Hello, ";
const std::string exit_msg = " left the chat QAQ";
const std::string enter_msg = " enter the chat room";
const std::string DELIMETER = "|@|";

int opt = TRUE, send_exit_msg = FALSE;
std::string exit_client_name = "";
// client_data client_socket[client_socket_list.size()];
std::vector<client_data> client_socket_list;
int master_socket, addrlen, new_socket, activity, i, valread, sd;
int max_sd;
struct sockaddr_in address;

void print_msg(std::string tag, std::string content) {
    std::cout << "[ " << tag << " ] " << content << std::endl;
}

void send_to_all_client(char *buf) {
    for (int i = 0; i < client_socket_list.size(); i++) {
        sd = client_socket_list[i].id;
        send(sd, buf, strlen(buf), 0);
    }
}

int main(int argc, char *argv[]) {
    char buffer[BUFFER_SIZE + 1];

    // Set of socket descriptors
    fd_set readfds;

    // Initialise all client_socket[] to 0 so not checked
    client_socket_list.clear();

    // Create a master socket
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set master socket to allow multiple connections, this is just a good habit, it will work without this
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to localhost port 8711
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    print_msg("SERVER", "Listening on port " + std::to_string(PORT));

    // Try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Accept the incoming connection
    addrlen = sizeof(address);
    print_msg("SERVER", "Waiting for connections");

    while (TRUE) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        // Add child sockets to set
        for (int i = 0; i < client_socket_list.size(); i++) {
            // Socket descriptor
            sd = client_socket_list[i].id;

            // If valid socket descriptor then add to read list
            if (sd > 0)
                FD_SET(sd, &readfds);

            // Highest file descriptor number, need it for the select function
            if (sd > max_sd)
                max_sd = sd;
        }

        if (send_exit_msg) {
            std::string msg = "ADMIN" + DELIMETER + exit_client_name + exit_msg + "\n";
            char c_msg[msg.size() + 1];
            strcpy(c_msg, msg.c_str());
            send_to_all_client(c_msg);
            send_exit_msg = FALSE;
        }

        // Wait for an activity on one of the sockets , timeout is NULL, so wait indefinitely
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            printf("select error");
        }

        // If something happened on the master socket, then its an incoming connection
        if (FD_ISSET(master_socket, &readfds)) {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            // Send new connection greeting input_id_msg
            if (send(new_socket, input_id_msg, sizeof(input_id_msg), 0) != sizeof(input_id_msg)) {
                perror("send");
            }

            // Add new socket to array of sockets
            bool list_is_full = true;
            for (int i = 0; i < client_socket_list.size(); i++) {
                // If position is empty
                if (client_socket_list[i].id == 0) {
                    list_is_full = false;
                    client_socket_list[i].id = new_socket;
                    print_msg("CONNECT", "fd: " + std::to_string(new_socket) + ", ip: " + inet_ntoa(address.sin_addr) + ", port: " + std::to_string(ntohs(address.sin_port)) + ", socket_list: " + std::to_string(i));
                    break;
                }
            }
            if (list_is_full) {
                client_data cd;
                cd.id = new_socket;
                cd.name = "";
                client_socket_list.push_back(cd);
                print_msg("CONNECT", "fd: " + std::to_string(new_socket) + ", ip: " + inet_ntoa(address.sin_addr) + ", port: " + std::to_string(ntohs(address.sin_port)) + ", socket_list: " + std::to_string(client_socket_list.size() - 1));
            }
        }

        // Else its some IO operation on some other socket
        for (int i = 0; i < client_socket_list.size(); i++) {
            sd = client_socket_list[i].id;

            if (FD_ISSET(sd, &readfds)) {
                // Check if it was for closing , and also read the incoming message
                if ((valread = read(sd, buffer, BUFFER_SIZE)) == 0) {
                    //Somebody disconnected , get his details and print
                    getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

                    // If the user is registered, send exit_msg to everyone.
                    if (client_socket_list[i].name != "") {
                        exit_client_name = client_socket_list[i].name;
                        send_exit_msg = TRUE;
                        print_msg("DISCONNECT", "ID: " + exit_client_name + ", ip: " + inet_ntoa(address.sin_addr) + ", port: " + std::to_string(ntohs(address.sin_port)));
                    } else {
                        print_msg("DISCONNECT", std::string("ip: ") + inet_ntoa(address.sin_addr) + ", port: " + std::to_string(ntohs(address.sin_port)));
                    }

                    // Close the socket and mark as 0 in list for reuse
                    close(sd);
                    client_socket_list[i].id = 0;
                    client_socket_list[i].name = "";
                }

                // Echo back the input_id_msg that came in
                else {
                    buffer[valread] = '\0';
                    // The first input from client is their id
                    if (client_socket_list[i].name == "") {
                        std::string name = buffer;
                        name = name.substr(0, name.length() - 1);
                        client_socket_list[i].name = name;

                        std::string msg = "ADMIN" + DELIMETER + client_socket_list[i].name + enter_msg + "\n";
                        char c_msg[msg.size() + 1];
                        strcpy(c_msg, msg.c_str());
                        send_to_all_client(c_msg);
                        print_msg("REGISTER", name);
                    } else {
                        std::string msg = client_socket_list[i].name + DELIMETER + buffer;
                        print_msg("MESSAGE", client_socket_list[i].name + " > " + std::string(buffer).substr(0, std::string(buffer).length() - 1));
                        char c_msg[msg.size() + 1];
                        strcpy(c_msg, msg.c_str());
                        send_to_all_client(c_msg);
                    }
                }
            }
        }
    }

    return 0;
}
