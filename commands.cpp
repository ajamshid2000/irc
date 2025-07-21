/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mel-yand <mel-yand@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/27 15:16:19 by ajamshid          #+#    #+#             */
/*   Updated: 2025/07/14 20:30:04 by mel-yand         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// :yournickname!yourident@yourhost PRIVMSG otheruser :Hello
// 433 * NewNickname :Nickname is already in use
// client to server
/*
NICK yournickname
USER yournick 0 * :Your Real Name
JOIN #channel
PRIVMSG #channel :hello everyone
PRIVMSG othernick :hello privately
*/
// server to client
/*
:irc.example.com/localhost/127.0.0.1 001 yournickname :Welcome to the IRC
:yournickname!user@host PRIVMSG #channel :hello everyone
:yournickname!user@host PRIVMSG othernick :hello privately
*/
// IRC SERVER TO CONNECT FOR CHECKING
// irssi -c Irc.oftc.net -p 6697 -n yournickname

#include "Clients.hpp"

// modif mel-yand = //
void send_msg(int fd, const std::string &message)
{
	for (size_t i = 0; i < clients_bj.get_pollfds().size(); ++i)
		if (clients_bj.get_pollfds()[i].fd == fd)
		{
			clients_bj.get_pollfds()[i].events |= POLLOUT;
			break;
		}
	clients_bj.add_to_client_send_buffer(fd, message);
	return;
}

bool is_valid_nick(const std::string &nick)
{
	return (!nick.empty());
	// if nessessary mandatory length of nick could be changed!
}

void disconnect_client(int fd)
{
	std::cout << "Client disconnected (fd " << fd << ")\n";
	clients_bj.remove_client(fd);
}

void pass(Client &client, std::string pass)
{
	if (client.registered)//
	{
		send_msg(client.fd, ":server 462 * :You may not reregister\r\n");//
		return;
	}
	if (pass == server_password)
	{
		std::cout << "Pass OK =" << pass << std::endl;
		client.pass_ok = true;
	}
	else
	{
		disconnect_client(client.fd);
		send_msg(client.fd, ":server 464 * :Password required\r\n");//
	}
}

void nick(Client &client, std::string nick)
{
	if (!client.pass_ok)
		disconnect_client(client.fd);
	else if (!is_valid_nick(nick) || clients_bj.get_nick_to_fd().count(nick))
	{
		send_msg(client.fd, ":server 433 * " + nick + " :Nickname is already in use\r\n");
	}
	else
	{
		std::string old_nick = client.nickname;//
		if (!old_nick.empty())//
		{
			clients_bj.get_nick_to_fd().erase(client.nickname);
			send_msg(client.fd, ":" + old_nick + " NICK :" + nick + "\r\n"); //send the new name to the client to update it
		}
		client.nickname = nick;
		clients_bj.get_nick_to_fd()[nick] = client.fd;
	}
}


void user(Client &client, std::string user)
{
	if (!client.pass_ok)
		disconnect_client(client.fd);
	else
	{
		std::cout << "User = " << user << std::endl;
		client.username = user;
	}
}

void privmsg(Client &client, std::string message)
{
	std::string reciever;
    std::string rest;
    std::istringstream iss(message);
    iss >> reciever;
    std::getline(iss, rest);
	
	if (reciever.empty()) {
		std::cout << ">> pas de destinataire" << std::endl;
		send_msg(client.fd, ":server 411 " + client.nickname + " :No recipient given (PRIVMSG)\r\n");
    	return;
	}
	if (rest.empty()) {
		std::cout << ">> pas de message" << std::endl;
    	send_msg(client.fd, ":server 412 " + client.nickname + " :No text to send\r\n");
    	return;
	}

    if (rest.size() >= 2 && rest[0] == ' ' && rest[1] == ':')
        rest = rest.substr(2);
    else if (!rest.empty() && rest[0] == ' ') {
        rest = rest.substr(1);
	}

	if (!reciever.empty() && reciever[0] == '#')
    {
        if (!g_channels.channelExists(reciever))
        {
            send_msg(client.fd, ":server 403 " + client.nickname + " " + reciever + " :No such channel\r\n");
            return;
        }

        const std::set<int> &clients = g_channels.getClientsInChannel(reciever);
        if (clients.find(client.fd) == clients.end())
        {
            send_msg(client.fd, ":server 442 " + client.nickname + " " + reciever + " :You're not on that channel\r\n");
            return;
        }

        std::string msg = ":" + client.nickname + " PRIVMSG " + reciever + " :" + rest + "\r\n";
        for (std::set<int>::const_iterator it = clients.begin(); it != clients.end(); ++it)
        {
            if (*it != client.fd)
                send_msg(*it, msg);
        }
        return;
    }
		
    int dest_fd = clients_bj.get_fd_of(reciever);
    if (dest_fd == -1)
    {
		std::cout << ">> pas trouver le  destinataire" << std::endl;
        send_msg(client.fd, ":server 401 " + client.nickname + " " + reciever + " :No such nick\r\n");
        return;
    }

    std::string msg = ":" + client.nickname + " PRIVMSG " + reciever + " :" + rest + "\r\n";
    send_msg(dest_fd, msg);
}