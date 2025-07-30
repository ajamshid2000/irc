/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Clients.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ajamshid <ajamshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/27 16:06:31 by ajamshid          #+#    #+#             */
/*   Updated: 2025/07/30 19:42:51 by ajamshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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
#include <fcntl.h>
#include <signal.h>

#include "Channels.hpp"

#define MAX_CLIENTS 10
#define BUFFER_SIZE 512
// #define SERVER_NAME "irc.42.fr"
// #define HOST_NAME "ircserv"

// // ----------- FORMAT UTILITAIRE POUR RÉPONSES IRC -----------
// #define REPLY_FORMAT(num_rply_numb, nickname) (std::string(":") + SERVER_NAME + " " + num_rply_numb + " " + nickname + " ")
// #define CLIENT_ID(nickname, username, command) (":" + nickname + "!" + username + "@" + SERVER_NAME + " " + command + " ")

// // ----------- RÉPONSES SERVEUR IRC -----------

// // 001 - Welcome
// #define RPL_WELCOME(nickname) (std::string(":") + SERVER_NAME + " 001 " + nickname + " :Welcome to the " + SERVER_NAME + " Server " + nickname + "\r\n")

// // 400+ - Erreurs essentielles
// #define ERR_NOSUCHNICK(nickname, other_nickname) (std::string(":") + SERVER_NAME + " 401 " + nickname + " " + other_nickname + " :No such nick\r\n")
// #define ERR_NOSUCHCHANNEL(nickname, channel) (std::string(":") + SERVER_NAME + " 403 " + nickname + " " + channel + " :No such channel\r\n")
// #define ERR_NONICKNAMEGIVEN(nickname) (std::string(":") + SERVER_NAME + " 431 *" + nickname + " :No nickname given\r\n")
// #define ERR_ERRONEUSNICKNAME(nickname) (std::string(":") + SERVER_NAME + " 432 *" + nickname + " Erroneus nickname\r\n")
// #define ERR_NICKNAMEINUSE(nickname) (std::string(":") + SERVER_NAME + " 433 * " + nickname + "\r\n")
// #define ERR_NOTREGISTERED(nickname, command) (std::string(":") + SERVER_NAME + " 451 " + nickname + " " + command + " :You have not registered\r\n")
// #define ERR_PASSWDMISMATCH(nickname) (std::string(":") + SERVER_NAME + " 464 " + nickname + " :Password incorrect\r\n")
// #define ERR_NEEDMOREPARAMS(nickname, command) (std::string(":") + SERVER_NAME + " 461 " + nickname + " " + command + " :Not enough parameters\r\n")

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
    bool disconnect = 0;
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
    int remove_client(int fd);
    void add_to_client_recieve_buffer(int fd, std::string data);
    std::string &get_client_recieve_buffer(int fd);
    void add_to_client_send_buffer(int fd, std::string data);
    std::string &get_client_send_buffer(int fd);
    std::map<std::string, int> &get_nick_to_fd();
    int get_fd_of(std::string nick);

    bool nickExists(const std::string &nick) const;
};

extern Clients clients_bj;
extern std::string server_password;

int is_all_digits(std::string str);
int pars_args_and_port(int argc, char **argv);
int socket_prep_and_binding(int port);

void send_msg(int fd, const std::string &msg);
void disconnect_client(int fd);
void pass(Client &client, std::string pass);
void nick(Client &client, std::string nick);
void user(Client &client, std::string user);
void privmsg(Client &client, std::string message);
void notice(Client &client, std::string message);

#endif