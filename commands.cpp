/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ajamshid <ajamshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/27 15:16:19 by ajamshid          #+#    #+#             */
/*   Updated: 2025/07/30 19:59:31 by ajamshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// :yournickname!yourident@yourhost PRIVMSG otheruser :Hello
// 433 * NewNickname :Nickname is already in use
// client to ircserv
/*
NICK yournickname
USER yournick 0 * :Your Real Name
JOIN #channel
PRIVMSG #channel :hello everyone
PRIVMSG othernick :hello privately
*/
// ircserv to client
/*
:irc.example.com/ircserv/127.0.0.1 001 yournickname :Welcome to the IRC
:yournickname!user@host PRIVMSG #channel :hello everyone
:yournickname!user@host PRIVMSG othernick :hello privately
*/
// IRC SERVER TO CONNECT FOR CHECKING
// irssi -c Irc.oftc.net -p 6697 -n yournickname
// /dcc get sender_nick
// /set dcc_download_path on/off (leave empty if you want to knwo current settings)
// /set dcc_autoget on/off (leave empty if you want to knwo current settings)
// for file transfer (/dcc send reciever_nick ~/Desktop/...)

#include "Clients.hpp"

// modif mel-yand = //
void send_msg(int fd, const std::string &message)
{
	std::map<int, Client> cl = clients_bj.get_clients();
	std::cout << cl[fd].nickname << "message(" << message << ')' << std::endl;
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
	return (!nick.empty() && nick.size() < 9 && nick[0] <= 'z' && nick[0] >= 'a');
}

void disconnect_client(int fd)
{
	std::cout << "Client disconnected (fd " << fd << ")\n";
	clients_bj.remove_client(fd);
}

void pass(Client &client, std::string pass)
{
	if (client.registered) //
	{
		send_msg(client.fd, ":ircserv 462 * :You may not reregister\r\n"); //
		return;
	}
	if (pass == server_password)
	{
		std::cout << "Pass OK =" << pass << std::endl;
		client.pass_ok = true;
	}
	else
	{
		client.disconnect = 1;
		send_msg(client.fd, ":ircserv 464 * :Password required\r\n"); //
	}
}

void nick(Client &client, std::string nick)
{
	if (!client.pass_ok)
	{
		std::string error = ":ircserv 464 * :Password required\r\n";
		send(client.fd, error.c_str(), error.size(), 0);
		client.disconnect = 1;
	}
	else if (!is_valid_nick(nick) || clients_bj.get_nick_to_fd().count(nick))
	{
		if (client.registered)
			send_msg(client.fd, ":ircserv 433 * " + nick + " :Nickname is already in use\r\n");
		else
		{
			std::string error = ":ircserv 433 * " + nick + " :Nickname is already in use\r\n";
			send_msg(client.fd, error);
			client.disconnect = 1;
		}
	}
	else
	{
		std::string old_nick = client.nickname; //
		if (!old_nick.empty())					//
		{
			clients_bj.get_nick_to_fd().erase(client.nickname);
			send_msg(client.fd, ":" + old_nick + " NICK :" + nick + "\r\n"); // send the new name to the client to update it
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

	if (reciever.empty())
	{
		std::cout << ">> pas de destinataire" << std::endl;
		send_msg(client.fd, ":ircserv 411 " + client.nickname + " :No recipient given (PRIVMSG)\r\n");
		return;
	}
	if (rest.empty())
	{
		std::cout << ">> pas de message" << std::endl;
		send_msg(client.fd, ":ircserv 412 " + client.nickname + " :No text to send\r\n");
		return;
	}

	if (rest.size() >= 2 && rest[0] == ' ' && rest[1] == ':')
		rest = rest.substr(2);
	else if (!rest.empty() && rest[0] == ' ')
	{
		rest = rest.substr(1);
	}

	if (!reciever.empty() && reciever[0] == '#')
	{
		if (!g_channels.channelExists(reciever))
		{
			send_msg(client.fd, ":ircserv 403 " + client.nickname + " " + reciever + " :No such channel\r\n");
			return;
		}

		const std::set<int> &clients = g_channels.getClientsInChannel(reciever);
		if (clients.find(client.fd) == clients.end())
		{
			send_msg(client.fd, ":ircserv 442 " + client.nickname + " " + reciever + " :You're not on that channel\r\n");
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
		send_msg(client.fd, ":ircserv 401 " + client.nickname + " " + reciever + " :No such nick\r\n");
		return;
	}

	std::string msg = ":" + client.nickname + "!" + client.username + "@ircserv " + " PRIVMSG " + reciever + " :" + rest + "\r\n";
	send_msg(dest_fd, msg);
}

void notice(Client &client, std::string message)
{
	std::string reciever;
	std::string rest;
	std::string msg;
	std::istringstream iss(message);
	iss >> reciever;
	std::getline(iss, rest);
	rest = rest.substr(2);
	msg = ":" + client.nickname + "!" + client.username + "@ircserv " + " NOTICE " + reciever + " :" + rest + "\r\n";
	send_msg(clients_bj.get_fd_of(reciever), msg);
}