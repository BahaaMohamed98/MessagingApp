#include <atomic>
#include <cerrno>       // For errno
#include <cstring>      // For strerror
#include <thread>
#include <unistd.h>
#include <unordered_set>
#include <netinet/in.h>
#include <sys/socket.h>
#include "Terminal++.hpp"

std::atomic<bool> running = true;

class ChatServer {
    int serverSock;
    sockaddr_in serverAddr{};
    std::unordered_set<int> clients; // Use a map to track client sockets and their threads
    Terminal logger;                 // Assuming you have a logger that uses println
    Terminal error;

public:
    explicit ChatServer(const int& port)
        : error(Color::Red) {
        serverSock = socket(AF_INET, SOCK_STREAM, 0);

        if (serverSock < 0) {
            error.println("Error: Failed to create server socket.");
            exit(1);
        }

        // Set the socket options to reuse the address
        constexpr int opt = 1;
        if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            error.println("Error: Failed to set socket options: ", std::strerror(errno));
            exit(1);
        }

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (bind(serverSock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
            error.println("Error: Failed to bind server to port ", port, ": ", std::strerror(errno)).flush();
            exit(1);
        }

        listen(serverSock, 5);
        logger.println("Server started...\nlistening on port ", port);
    }

    void acceptClients() {
        while (running) {
            if (int clientSock = accept(serverSock, nullptr, nullptr); clientSock >= 0) {
                // Create a new thread for each client and store it in the map
                clients.insert(clientSock);
                std::thread(&ChatServer::handleClient, this, clientSock).detach();

                logger.println("New client connected with socket ID: ", clientSock);
            } else {
                error.println("Error: Failed to accept client connection: ", std::strerror(errno));
            }
        }
    }

    void broadcastMessage(const int& clientSock, const char buffer[], const int& bytesRead) {
        for (const auto& sock : clients) {
            if (sock != clientSock) {
                if (send(sock, buffer, bytesRead, 0) < 0)
                    error.println("Error: Failed to send message to client ", sock, ": ", std::strerror(errno));
            }
        }
    }

    void handleClient(const int& clientSock) {
        char buffer[1024];
        while (running) {
            const int bytesRead = static_cast<int>(recv(clientSock, buffer, sizeof(buffer) - 1, 0));
            if (bytesRead <= 0) {
                close(clientSock);
                logger.println("Client with socket ID ", clientSock, " disconnected.");
                clients.erase(clientSock); // Remove client from the map
                return;
            }
            buffer[bytesRead] = '\0';

            // Log the received message
            logger.println("Received a message from client ", clientSock);

            // Broadcast message to all other clients
            broadcastMessage(clientSock, buffer, bytesRead);
        }
    }

    int clientCount() const {
        return static_cast<int>(clients.size());
    }

    ~ChatServer() {
        logger.println("Shutting down server and closing all connections.");
        close(serverSock);

        for (const int& clientSock : clients) {
            close(clientSock);
            logger.println("Closed connection for client socket ID: ", clientSock);
        }
    }
};

int main(const int argc, char** argv) {
    int port = 8080;

    if (argc > 1) {
        if (argc == 2)
            port = std::stoi(argv[1]);
        else {
            std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
            exit(1);
        }
    }

    Terminal app;
    Terminal::clearScreen();
    ChatServer server(port);

    // accepting connections
    std::thread(&ChatServer::acceptClients, &server).detach();

    // input loop
    while (running) {
        std::string line = Terminal::getLine();

        if (line.empty())
            continue;

        if (line.front() == '/') {
            if (line == "/clear")
                Terminal::clearScreen();
            else if (line == "/users")
                app.println("Current active users: ", server.clientCount()).flush();
            else if (line == "/exit")
                running = false;
        } else {
            std::string message = "Server," + line;
            server.broadcastMessage(-1, message.c_str(), static_cast<int>(message.size()));
        }
    }
}
