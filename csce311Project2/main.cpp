#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <fstream>
#include <iostream>

using std::cout;
using std::endl;
using std::ifstream;
using std::string;

constexpr auto SOCK_PATH = "unix_sock.server";
constexpr auto CLIENT_PATH = "unix_sock.client";
constexpr auto DATA = "";

int main(int argc, char *argv[]) {
  int n1 = fork();

  // Parent Process
  if (n1 > 0) {
    string line;
    ifstream file(argv[0]);

    int server_sock, client_sock, rc;
    socklen_t len;
    int bytes_rec = 0;
    struct sockaddr_un server_sockaddr;
    struct sockaddr_un client_sockaddr;
    char buffer[256];
    int backlog = 10;
    memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(buffer, 0, 256);

    // Create the socket
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock == -1) {
      cout << "Error opening server socket" << endl;
      exit(1);
    }

    // Setup the socket and bind it to a file path
    server_sockaddr.sun_family = AF_UNIX;
    strcpy(server_sockaddr.sun_path, SOCK_PATH);
    len = sizeof(server_sockaddr);

    unlink(SOCK_PATH);
    rc = bind(server_sock, (struct sockaddr *)&server_sockaddr, len);
    if (rc == -1) {
      cout << "Bind Error" << endl;
      close(server_sock);
      exit(1);
    }

    // Listen for incoming connections
    rc = listen(server_sock, backlog);
    if (rc == -1) {
      cout << "Listen Error" << endl;
      close(server_sock);
      exit(1);
    }

    // Accept incoming connection
    client_sock =
        accept(server_sock, (struct sockaddr *)&client_sockaddr, &len);
    if (client_sock == -1) {
      cout << "Accept Error" << endl;
      close(server_sock);
      close(client_sock);
      exit(1);
    } else {
      cout << "Server Connection Successful" << endl;
    }

    while (getline(file, line)) {
      strcpy(buffer, DATA);
      rc = send(client_sock, buffer, strlen(buffer), 0);
      if (rc == -1) {
        cout << "Send Error" << endl;
        close(server_sock);
        close(client_sock);
        exit(1);
      }
    }
  }
  // Child Process
  else if (n1 == 0) {
    int client_sock, rc;
    socklen_t len;
    struct sockaddr_un server_sockaddr;
    struct sockaddr_un client_sockaddr;
    char buffer[256];
    memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));

    // Create socket
    client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_sock == -1) {
      cout << "Socket Error" << endl;
      exit(1);
    }

    // Setup socket
    client_sockaddr.sun_family = AF_UNIX;
    strcpy(client_sockaddr.sun_path, CLIENT_PATH);
    len = sizeof(client_sockaddr);

    unlink(CLIENT_PATH);
    rc = bind(client_sock, (struct sockaddr *)&client_sockaddr, len);
    if (rc == -1) {
      cout << "Bind Error" << endl;
      close(client_sock);
      exit(1);
    }

    // Connect
    server_sockaddr.sun_family = AF_UNIX;
    strcpy(server_sockaddr.sun_path, SOCK_PATH);
    rc = connect(client_sock, (struct sockaddr *)&server_sockaddr, len);
    if (rc == -1) {
      cout << "Connection Error: " << errno << endl;
      close(client_sock);
      exit(1);
    } else {
      cout << "Client Connection Successful" << endl;
    }
  }
}
