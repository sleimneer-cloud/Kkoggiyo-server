#pragma once

#include <string>
#include "handlers/AuthHandler.hpp"
#include "handlers/ChatHandler.hpp"

class MessageRouter
{
public:
    void route(int client_fd, const std::string &jsonStr) const;

private:
    AuthHandler authHandler_;
    ChatHandler chatHandler_;
};