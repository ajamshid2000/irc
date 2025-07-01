/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abdul-rashed <abdul-rashed@student.42.f    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/27 15:16:19 by ajamshid          #+#    #+#             */
/*   Updated: 2025/07/02 01:39:26 by abdul-rashe      ###   ########.fr       */
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

void send_msg(int fd, const std::string &message)
{
	for (size_t i = 0; i < clients_bj.get_pollfds().size(); ++i)
		if (clients_bj.get_pollfds()[i].fd == fd)
		{
			clients_bj.get_pollfds()[i].events |= POLLOUT;
			break;
		}
	clients_bj.add_to_client_send_buffer(fd, "\r\n");
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
	if (pass == server_password)
		client.pass_ok = true;
	else
		disconnect_client(client.fd);
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
		if (!client.nickname.empty())
			clients_bj.get_nick_to_fd().erase(client.nickname);
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
	rest = rest.substr(2);
	std::string msg = ":" + client.nickname + " PRIVMSG " + reciever + " :" + rest + "\r\n";
	send_msg(clients_bj.get_fd_of(reciever), msg);
}