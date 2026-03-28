#pragma once

#include <string>
#include "handlers/AuthHandler.hpp"
#include "handlers/ChatHandler.hpp"
#include "handlers/OrderHandler.hpp"
#include "handlers/AdminHandler.hpp"
#include "handlers/ClientHandler.hpp"
#include "handlers/StoreHandler.hpp"
#include "handlers/MenuHandler.hpp"

class MessageRouter
{
public:
    void route(int client_fd, const std::string &jsonStr) const;

private:
    AuthHandler authHandler_;
    ChatHandler chatHandler_;
    OrderHandler orderHandler_;
    mutable AdminHandler adminHandler_;
    ClientHandler clientHandler_;
    mutable StoreHandler storeHandler_;
    mutable MenuHandler menuHandler_;
};