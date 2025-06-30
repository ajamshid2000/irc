/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ajamshid <ajamshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/27 15:16:19 by ajamshid          #+#    #+#             */
/*   Updated: 2025/06/30 17:46:49 by ajamshid         ###   ########.fr       */
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

void send_msg(int fd, const std::string &msg)
{
    // even must be set if it is not
    // revent check should be done before
    send(fd, msg.c_str(), msg.length(), 0);
    // if send was successfull even should be left for optemization.
}

bool is_valid_nick(const std::string &nick)
{
    return !nick.empty() && nick.length() < 10; // if nessessary mandatory length of nick could be changed!
}

void disconnect_client(int fd)
{
    std::cout << "Client disconnected (fd " << fd << ")\n";
    clients_bj.remove_client(fd);
}

void pass(Client &client, std::string pass)
{
    std::cout << pass << std::endl;
    if (pass == server_password)
        client.pass_ok = true;
    else
        disconnect_client(client.fd);
}
void nick(Client &client, std::string nick)
{
    std::cout << nick << std::endl;
    if (!client.pass_ok)
        disconnect_client(client.fd);
    else if (!is_valid_nick(nick) || nick_to_fd.count(nick))
    {
        send_msg(client.fd, ":server 433 * " + nick + " :Nickname is already in use\r\n");
    }
    else
    {
        if (!client.nickname.empty())
            nick_to_fd.erase(client.nickname);
        client.nickname = nick;
        nick_to_fd[nick] = client.fd;
    }
}
void user(Client &client, std::string user)
{
    std::cout << user << std::endl;
    if (!client.pass_ok)
        disconnect_client(client.fd);
    else
    {
        client.username = user;
    }
}

void privmsg(Client &client, std::string message)
{
    std::cout << "it does come inside privmsg \n";
    // message should be stored in sent_buffer of the recepient.
    // if message is sent to group it should be saved into sent buffer of all of recepients.
    // the messages should comply with tcp.
    send_msg(client.fd, message + "\r\n");
}