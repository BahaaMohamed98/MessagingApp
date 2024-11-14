# Terminal Messaging App

A terminal-based chat system in C++ that features a server capable of handling multiple client connections concurrently,
broadcasting messages between them. The client provides a real-time messaging experience with a simple and intuitive
terminal interface.

## Features

### Server

- Handles multiple clients concurrently using threads.
- Broadcasts incoming messages from clients to all other clients.
- Allows for basic administrative commands like /clear (clear chat), /users (display active users), and /exit (shutdown
  server).
- [Administrative commands](#server-commands)

### Client

- Connects to a server using TCP/IP sockets.
- Displays messages in a simple chat UI, with distinct colors for different users.
- Non-blocking message input and output.
- [User commands](#client-commands).

## Installation

1. Clone the repository:
    ```bash
    git clone https://github.com/BahaaMohamed98/MessagingApp.git
    cd MessagingApp/build
    ```

2. Run CMake to configure the project:
    ```bash
    cmake ..
    ```

3. Compile the project:
    ```bash
    cmake --build .
    ```

## Usage

1. **Start the server** with a port number as an argument:
    ```bash
    ./server 8080
    ```

2. Connect to the server with the client (from another terminal or machine):
    ```bash
    ./messagingApp 8080 127.0.0.1
    ```

## Command Line Arguments

### Server:

- `<port>`: Port number for the server to listen on (default: 8080).

### Client:

- `<port>`: Port number to connect to (default: 8080).
- `<ip>`: IP address of the server (default: 127.0.0.1).

## Commands

### Server Commands

- **/exit**: Shuts down the server.
- **/users**: Displays the number of connected clients.

### Client Commands

- **/exit**: Closes the app.
- **/clear**: Clears the messages' history.

## Architecture

### Server

- The server accepts incoming client connections and creates a new thread for each client using std::thread.
- It listens for incoming messages and broadcasts them to all connected clients.

### Client

- The client connects to the server and sends messages typed by the user.
- It also receives and displays messages from the server in real time.

## License

This project is licensed under the Apache 2.0 License - see the [LICENSE](LICENSE) file for details.
