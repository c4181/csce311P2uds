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

constexpr auto SERVER_PATH = "unix_sock.server";
constexpr auto CLIENT_PATH = "unix_sock.client";
constexpr auto DATA = "";

int main(int argc, char *argv[]) {
  int n1 = fork();

  // Parent Process
  if (n1 > 0) {
    string line;
    string all_lines;
    ifstream file(argv[1]);

    int client_sock, rc;
    socklen_t length;
    struct sockaddr_un server_sockaddr;
    struct sockaddr_un client_sockaddr;

    memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));

    // Create Socket
    client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_sock == -1) {
      cout << "Error Creating Socket: " << errno << endl;
      exit(1);
    }

    // Setup Socket
    client_sockaddr.sun_family = AF_UNIX;
    strcpy(client_sockaddr.sun_path, CLIENT_PATH);
    length = sizeof(client_sockaddr);

    unlink(CLIENT_PATH);
    rc = bind(client_sock, (struct sockaddr *)&client_sockaddr, length);
    if (rc == -1) {
      cout << "Error Binding Socket: " << errno << endl;
      exit(1);
    }

    server_sockaddr.sun_family = AF_UNIX;
    strcpy(server_sockaddr.sun_path, SERVER_PATH);

    // Read the entire file into a string
    if (file.is_open()) {
      while (getline(file, line)) {
        all_lines = all_lines + line + "\n";
      }
    }
    file.close();

    // Convert to a char[] and send to server for processing
    char buffer[all_lines.size() + 1];
    strcpy(buffer, all_lines.c_str());
    size_t buffer_length = strlen(buffer) + 1;
    cout << "Client Buffer Length: " << buffer_length << endl;
    rc = connect(client_sock, (struct sockaddr *)&server_sockaddr, length);
    if (rc == -1) {
      cout << "Connection Error: " << errno << endl;
    }
    rc = send(client_sock, &buffer_length, sizeof(buffer_length), 0);
    rc = send(client_sock, buffer, strlen(buffer), 0);

    // Recieve the result from the server and print to stdout
    memset(buffer, 0, sizeof(buffer));
    rc = recv(client_sock, buffer, sizeof(buffer), 0);
    if (rc == -1) {
      cout << "Recieve Error: " << errno << endl;
    }
    close(client_sock);
    string result(buffer);
    cout << result << endl;
    return 0;
  }
  // Child Process
  else if (n1 == 0) {
    int server_sock, client_sock, rc;
    socklen_t length;
    int byte_rec = 0;
    struct sockaddr_un server_sockaddr;
    struct sockaddr_un client_sockaddr;
    int backlog = 10;
    bool run = true;
    size_t buffer_length;
    memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));

    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock == -1) {
      cout << "Error Creating Socket: " << errno << endl;
    }

    server_sockaddr.sun_family = AF_UNIX;
    strcpy(server_sockaddr.sun_path, SERVER_PATH);
    length = sizeof(server_sockaddr);

    unlink(SERVER_PATH);
    rc = bind(server_sock, (struct sockaddr *)&server_sockaddr, length);
    if (rc == -1) {
      cout << "Error Binding Socket: " << errno << endl;
    }

    rc = listen(server_sock, backlog);
    if (rc == -1) {
      cout << "Error Listening: " << errno << endl;
    }

    client_sock =
        accept(server_sock, (struct sockaddr *)&client_sockaddr, &length);
    if (client_sock == -1) {
      cout << "Error Accepting: " << errno << endl;
    }
    byte_rec = recv(client_sock, &buffer_length, sizeof(buffer_length), 0);
    char buffer[buffer_length];
    cout << "Server Buffer Length " << buffer_length << endl;
    byte_rec = recv(client_sock, buffer, sizeof(buffer), 0);
    if (byte_rec == -1) {
      cout << "Recieve Error: " << errno << endl;
      // TODO: Handle the Recieve
    }
    string rec_data(buffer);
    string word = argv[2];
    string ret_data;

    size_t pos = 0;
    size_t beg_line;
    size_t end_line;
    for (size_t i = 0; i < rec_data.size(); ++i) {
    }
    return (0);
  }
}
