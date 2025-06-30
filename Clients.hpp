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

#define MAX_CLIENTS 10
#define BUFFER_SIZE 512

struct Client
{
    int fd = 0;
    std::string nickname;
    std::string username;
    bool pass_ok = 0;
    bool registered = 0;
    std::string channels;
    std::string recieve_buffer;
    std::string send_buffer;
};

class Clients
{
private:
    // when deleting (after desconnection) it should be started from the 1 to 4
    //std::vector<int> fds;                  // 4
    std::vector<pollfd> pollfds;           // 3
    std::map<int, Client> client;          // 2
    std::map<std::string, int> nick_to_fd; // 1
public:
    void add_client(int fd);
    Client &get_client(int fd);
    std::map<int, Client> &get_clients();
    std::vector<pollfd> &get_pollfds();
    std::string get_nick(int fd);
    int remove_client(std::string nick);
    int remove_client(int fd);

    int get_index(int fd) const;
    int get_fd(std::string nick_name);
    int poll_all();

    int add_to_client_recieve_buffer(std::string nick);
    int add_to_client_recieve_buffer(int fd);
    int add_to_client_send_buffer(std::string nick);
    int add_to_client_send_buffer(int fd);
    int change_client_nick(std::string new_nick, std::string old_nick);
    int change_client_nick(std::string new_nick, int fd);
    int add_to_channel(std::string channel, std::string nick);
    int add_to_channel(std::string channel, int fd);
    int get_fd_of_client(Client &client);
    int push_back_pollfd(int fd, int event);
};

extern Clients clients_bj;
extern std::string server_password;
extern std::map<std::string, int> nick_to_fd;

int is_all_digits(std::string str);
int pars_args_and_port(int argc, char **argv);
int socket_prep_and_binding(int port);

void send_msg(int fd, const std::string &msg);
void disconnect_client(int fd);
void pass(Client &client, std::string pass);
void nick(Client &client, std::string nick);
void user(Client &client, std::string user);
void privmsg(Client &client, std::string message);

#endif