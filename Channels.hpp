#ifndef CHANNELS_HPP
#define CHANNELS_HPP

#include "Clients.hpp"
#include <set>
#include <climits>
#include <cctype>

struct Client;

struct Channel
{
    std::string name;
    std::string topic;
    std::set<int> clients;
    std::set<int> operators;
    std::set<int> invited;
    std::string password;
    std::set<char> modes;
    int user_limit = -1;
};

class Channels
{
    private:
        std::map<std::string, Channel> _channels;

    public:
        bool channelExists(const std::string &name) const;
        bool createChannel(const std::string &name, int creatorFD, const std::string &password = "");
        bool addClientToChannel(const std::string &name, int clientFd);
        void removeClientFromChannel(const std::string &name, int clientFd);
        void addOperator(const std::string &name, int clientFd);
        void removeOperator(const std::string &name, int clientFd);
        bool isOperator(const std::string &name, int clientFd) const;
        bool checkPassword(const std::string &name, const std::string &pass) const;
        bool hasMode(const std::string &channelName, char mode) const;
        void addMode(const std::string &channelName, char mode);
        void removeMode(const std::string &channelName, char mode);
        void setPassword(const std::string &channelName, const std::string &newPass);
        void removePassword(const std::string &channelName);
        void setUserLimit(const std::string &channelName, int newUserLimit);
        void removeUserLimit(const std::string &channelName);
        int  getUserLimit(const std::string &channelName) const;

        const std::set<int> &getClientsInChannel(const std::string &name) const;
        bool isClientInChannel(const std::string &channelName, int clientFd) const;
        bool hasTopic(const std::string &channelName) const;
        const std::string &getTopic(const std::string &channelName) const;
        void setTopic(const std::string &channelName, const std::string &newTopic);
        void inviteClient(const std::string &channelName, int clientFd);
        bool isInvited(const std::string &channelName, int clientFd) const;
        void removeInvite(const std::string &channelName, int clientFd);
        std::string getModeString(const std::string &channelName) const;
};

extern Channels g_channels;

void join(Client &client, std::string args);
void topic(Client &client, std::string args);
void invite(Client &client, std::string args);
void kick(Client &client, std::string args);
void mode(Client &client, std::string args);

#endif