#include "Channels.hpp"

bool Channels::channelExists(const std::string &name) const
{
    return _channels.find(name) != _channels.end();
}

bool Channels::createChannel(const std::string &name, int creatorFD, const std::string &password)
{
    if (this->channelExists(name))
        return false;
    
    Channel newChan;
    newChan.name = name;
    newChan.topic = "";
    newChan.password = password;
    std::cout << "passw de" << name << " = " << password << std::endl;

    newChan.clients.insert(creatorFD);
    newChan.operators.insert(creatorFD);
    newChan.modes.insert('t');

    if (!password.empty())
        newChan.modes.insert('k');

    _channels[name] = newChan;
    return true;
}

bool Channels::addClientToChannel(const std::string &name, int clientFd)
{
    std::map<std::string, Channel>::iterator it = _channels.find(name);
    if (it == _channels.end())
        return false;

    if (it->second.clients.count(clientFd))
        return false;

    it->second.clients.insert(clientFd);
    return true;
}

void Channels::removeClientFromChannel(const std::string &name, int clientFd)
{
    this->_channels[name].clients.erase(clientFd);
    this->_channels[name].operators.erase(clientFd);

    if (this->_channels[name].clients.empty())
        this->_channels.erase(name);
}

void Channels::addOperator(const std::string &name, int clientFd)
{
        this->_channels[name].operators.insert(clientFd);
}

void Channels::removeOperator(const std::string &name, int clientFd)
{
    this->_channels[name].operators.erase(clientFd);
}

bool Channels::isOperator(const std::string &name, int clientFd) const
{
    std::map<std::string, Channel>::const_iterator it = this->_channels.find(name);
    if (it ==  this->_channels.end())
        return false;
    return it->second.operators.count(clientFd) != 0;
}

bool Channels::checkPassword(const std::string &name, const std::string &pass) const
{
    std::map<std::string, Channel>::const_iterator it = this->_channels.find(name);
    if (it == _channels.end())
        return false;
    if (it->second.password == pass)
        return true;
    return false;
}

bool Channels::hasMode(const std::string &channelName, char mode) const
{
    std::map<std::string, Channel>::const_iterator it = this->_channels.find(channelName);
    if (it == this->_channels.end())
        return false;
    return it->second.modes.count(mode) != 0;
}

void Channels::addMode(const std::string &channelName, char mode)
{
    this->_channels[channelName].modes.insert(mode);
}

void Channels::removeMode(const std::string &channelName, char mode)
{
    this->_channels[channelName].modes.erase(mode);
}

void Channels::setPassword(const std::string &channelName, const std::string &newPass)
{
    this->_channels[channelName].password = newPass;
}

void Channels::removePassword(const std::string &channelName)
{
    this->_channels[channelName].password = "";
}

void Channels::setUserLimit(const std::string &channelName, int newUserLimit)
{
    this->_channels[channelName].user_limit = newUserLimit;
}

void Channels::removeUserLimit(const std::string &channelName)
{
    this->_channels[channelName].user_limit = -1;
}

int  Channels::getUserLimit(const std::string &channelName) const
{
    std::map<std::string, Channel>::const_iterator it = _channels.find(channelName);
    if (it != this->_channels.end())
        return it->second.user_limit;
    return INT_MIN;
}

const std::set<int> &Channels::getClientsInChannel(const std::string &name) const
{
    static const std::set<int> empty;
    std::map<std::string, Channel>::const_iterator it = this->_channels.find(name);
    if (it != this->_channels.end())
        return it->second.clients;
    return empty;
}

bool Channels::isClientInChannel(const std::string &channelName, int clientFd) const
{
    std::map<std::string, Channel>::const_iterator it = _channels.find(channelName);
    if (it == this->_channels.end())
        return false;
    return it->second.clients.count(clientFd) != 0;
}

bool Channels::hasTopic(const std::string &channelName) const
{
    std::map<std::string, Channel>::const_iterator it = this->_channels.find(channelName);
    if (it != this->_channels.end())
        return !it->second.topic.empty();
    return false;
}

const std::string &Channels::getTopic(const std::string &channelName) const
{
    static const std::string empty;
    std::map<std::string, Channel>::const_iterator it = this->_channels.find(channelName);
    if (it != this->_channels.end())
        return it->second.topic;
    return empty;
}

void Channels::setTopic(const std::string &channelName, const std::string &newTopic)
{
    this->_channels[channelName].topic = newTopic;
}

void Channels::inviteClient(const std::string &channelName, int clientFd)
{
    std::map<std::string, Channel>::iterator it = _channels.find(channelName);
    if (it == this->_channels.end())
        return;
    if (it->second.clients.count(clientFd) != 0)
        return;
    it->second.invited.insert(clientFd);
}

bool Channels::isInvited(const std::string &channelName, int clientFd) const
{
    std::map<std::string, Channel>::const_iterator it = _channels.find(channelName);
    if (it == _channels.end())
        return false;
    return it->second.invited.count(clientFd) != 0;
}

void Channels::removeInvite(const std::string &channelName, int clientFd)
{
    std::map<std::string, Channel>::iterator it = _channels.find(channelName);
    if (it != _channels.end())
        it->second.invited.erase(clientFd);
}

std::string Channels::getModeString(const std::string &channelName) const
{
    std::map<std::string, Channel>::const_iterator it = _channels.find(channelName);
    if (it == _channels.end())
        return "";

    const Channel &chan = it->second;
    std::string modeFlags = "+";
    std::vector<std::string> params;

    for (std::set<char>::const_iterator mit = chan.modes.begin(); mit != chan.modes.end(); ++mit) {
        char mode = *mit;
        modeFlags += mode;

        if (mode == 'k' && !chan.password.empty())
            params.push_back(chan.password);
        else if (mode == 'l') {
            params.push_back(std::to_string(chan.user_limit));
        }
    }

    std::ostringstream oss;
    oss << modeFlags;
    for (std::vector<std::string>::const_iterator pit = params.begin(); pit != params.end(); ++pit)
        oss << " " << *pit;

    return oss.str();
}