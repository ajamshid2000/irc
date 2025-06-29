#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sstream>

struct Client
{
    int fd;
    std::string nickname;
    std::string username;
    bool pass_ok;
    bool registered;
    std::string channels;
    std::string recieve_buffer;
    std::string send_buffer;
};


class Clients
{
private:
// when deleting (after desconnection) it should be started from the 1 to 4
    std::vector<int> fds; // 4
    std::vector<pollfd> pollfds; // 3
    std::map<int, Client> clients; // 2
    std::map<std::string, int> nick_to_fd; // 1  
public:
    Client * get_client(int fd) const;
    std::vector<pollfd> *get_pollfd(int index) const;
    int get_index(int fd) const;
    int get_fd(std::string nick_name);
    int poll_all();
    int remove_client(std::string nick);
    int remove_client(int fd);
    int add_to_client_recieve_buffer(std::string nick);
    int add_to_client_recieve_buffer(int fd);
    int add_to_client_send_buffer(std::string nick);
    int add_to_client_send_buffer(int fd);
    int change_client_nick(std::string new_nick, std::string old_nick);
    int change_client_nick(std::string new_nick, int fd);
    int add_to_channel(std::string channel, std::string nick);
    int add_to_channel(std::string channel, int fd);
    int get_fd_of_client(Client &client);
    int push_back_pollfd(int fd, int event)

};


#endif