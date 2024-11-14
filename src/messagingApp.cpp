#include <atomic>
#include <cerrno>   // For errno
#include <fstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "Terminal++.hpp"

int EXIT_STATUS = 1;

class NetworkHandler {
    int sockfd;
    sockaddr_in serverAddr{};

public:
    NetworkHandler(const std::string& ip, const int& port) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

        if (connect(sockfd, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
            Terminal(Color::Red).println("failed to connect to server").flush();
            exit(1);
        }
    }

    // Send a message to the server
    void sendMessage(const std::string& message) const {
        send(sockfd, message.c_str(), message.size(), 0);
    }

    // Receive a message from the server
    [[nodiscard]] std::string receiveMessage() const {
        char buffer[1024];

        const int bytesRead = static_cast<int>(recv(sockfd, buffer, sizeof(buffer) - 1, 0));
        if (bytesRead <= 0) { // Check for disconnection or error
            close(sockfd);

            Terminal::clearScreen();
            Terminal(Color::Red).println("Disconnected from server").flush();

            exit(EXIT_STATUS); // Exit the program if disconnected
        }
        buffer[bytesRead] = '\0';
        std::string message(buffer, bytesRead);

        return message;
    }

    // Clean up resources
    ~NetworkHandler() {
        close(sockfd);
    }
};

std::atomic<bool> messagesUpdated = true; // init as true for initial ui update

void readNewMessages(std::vector<std::pair<std::string, std::string>>& messages, const NetworkHandler& client) {
    std::string message = client.receiveMessage();

    if (const auto index = message.find(','); index != std::string::npos) {
        messages.emplace_back(message.substr(0, index), message.substr(index + 1));
        messagesUpdated.store(true);
    }
}

void writeMessage(const std::pair<std::string, std::string>& message, const NetworkHandler& client) {
    client.sendMessage(message.first + ',' + message.second);
    messagesUpdated = true;
}

int main(const int argc, char** argv) {
    Terminal::enableAlternateScreen(); // alternate screen buffer

    std::string ip = "127.0.0.1";
    int port = 8080;

    if (argc > 1) {
        if (argc <= 3) {
            try {
                port = std::stoi(argv[1]);
            } catch (const std::invalid_argument&) {
                std::cerr << "Invalid port number." << std::endl;
                exit(1);
            } catch (const std::out_of_range&) {
                std::cerr << "Port number out of range." << std::endl;
                exit(1);
            }

            if (argc == 3)
                ip = argv[2];
        } else {
            std::cout << "Usage: " << argv[0] << " <Port> (default: 8080) <IP> (default: 127.0.0.1)" << std::endl;
            exit(1);
        }
    }

    Terminal::clearScreen();
    std::string inputBuffer;
    std::vector<std::pair<std::string, std::string>> messages;
    std::atomic<bool> isRunning = true;

    Terminal ui;
    const auto* client = new NetworkHandler(ip, port);
    Terminal::clearScreen();

    std::string userName = Terminal::getString("Enter username: ");
    writeMessage({userName, "joined the chat!"}, *client);

    auto closeApp = [&]() {
        writeMessage({userName, "disconnected!"}, *client);
        inputBuffer.clear();
        isRunning = false;
        EXIT_STATUS = 0;
        delete client;
        Terminal::disableAlternateScreen(); // disabling back the screen alternate screen buffer
        exit(EXIT_STATUS);
    };

    // input loop
    ui.nonBlock(
        [&]() {
            while (isRunning.load()) {
                switch (const char ch = Terminal::getChar()) {
                    case keyCode::Esc:
                        closeApp();
                        break;
                    case keyCode::Enter: {
                        if (inputBuffer.empty())
                            break;

                        if (inputBuffer.front() == '/') {
                            if (inputBuffer == "/clear") {
                                messages.clear();
                                Terminal::hideCursor();
                                Terminal::clearScreen();

                                auto [width,height] = Terminal::size();
                                std::string message{"Local conversation cleared!"};

                                Terminal::moveTo((width - static_cast<int>(message.length())) / 2, height / 3);
                                Terminal(Color::Green).println(message);
                                Terminal::sleep(1000);

                                Terminal::showCursor();
                            } else if (inputBuffer == "/exit") {
                                closeApp();
                            }
                        } else {
                            messages.emplace_back(userName, inputBuffer);
                            writeMessage(messages.back(), *client);
                        }

                        inputBuffer.clear();
                    }
                    break;
                    case keyCode::Backspace:
                        if (!inputBuffer.empty())
                            inputBuffer.erase(inputBuffer.length() - 1);
                        break;
                    default:
                        inputBuffer.push_back(ch);
                }
            }
        }
    );

    // reading messages loop
    ui.nonBlock(
        [&]() {
            while (isRunning.load()) {
                readNewMessages(messages, *client);
                Terminal::sleep(500);
            }
        }
      , true
    );

    Terminal separatingLine(Color::Blue), thisUser(Color::Cyan), otherUser(Color::Magenta), command(Color::Red);
    const std::string prompt{"> "};

    auto prevInputSize = inputBuffer.size();
    auto inputUpdated = [&]() {
        const bool updated = prevInputSize != inputBuffer.size();
        prevInputSize = inputBuffer.size();
        return updated;
    };

    // UI loop
    while (isRunning.load()) {
        Terminal::sleep(16); // some delay

        if (!(messagesUpdated.load() or inputUpdated() or ui.isResized())) // checks for ui updates
            continue;

        messagesUpdated.store(false);
        Terminal::clearScreen();
        auto [width,height] = Terminal::size();

        // printing previous messages
        Terminal::moveTo(1, 1);
        const int startingIdx{messages.size() >= height - 2 ? static_cast<int>(messages.size()) - (height - 3) : 0};

        // messages
        for (int i = startingIdx; i < messages.size(); ++i) {
            (messages[i].first == userName ? thisUser : otherUser).print(messages[i].first, ": ");
            ui.println(messages[i].second);
        }

        // separating line
        Terminal::moveTo(1, height - 2);
        for (int i = 1; i <= width; ++i)
            separatingLine.print("─");

        // prompt
        separatingLine.print(prompt);

        // user input
        const int limit = static_cast<int>(std::min(inputBuffer.size(), width - prompt.size()));
        for (int i = 0; i < limit; ++i)
            (!inputBuffer.empty() and inputBuffer.front() == '/' ? command : ui).print(inputBuffer[i]);

        Terminal::flush(); // flushing the output
    }

    Terminal::disableAlternateScreen(); // disabling back the screen alternate screen buffer
}
