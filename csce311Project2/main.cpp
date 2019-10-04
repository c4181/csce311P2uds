#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <regex>

using std::cout;
using std::endl;
using std::ifstream;
using std::regex;
using std::regex_match;
using std::regex_constants::icase;
using std::string;

constexpr auto SERVER_PATH = "unix_sock.server";
constexpr auto CLIENT_PATH = "unix_sock.client";

int main(int argc, char *argv[]) {
  int n1 = fork();

  // Parent Process
  if (n1 > 0) {
    string line;
    ifstream file(argv[1]);

    int client_sock, rc;
    socklen_t length;
    struct sockaddr_un server_sockaddr;
    struct sockaddr_un client_sockaddr;
    char buffer[256];
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
    rc = connect(client_sock, (struct sockaddr *)&server_sockaddr, length);
    while (file.eof() == false) {
      getline(file, line);
      if (line.empty()) {
        continue;
      }
      memset(buffer, 0, sizeof(buffer));
      strcpy(buffer, line.c_str());
      rc = send(client_sock, buffer, strlen(buffer), 0);
      if (rc == -1) {
        cout << "Client Error Sending: " << errno << endl;
        exit(1);
        // TODO Handle Error
      }

      memset(buffer, 0, sizeof(buffer));
      rc = recv(client_sock, buffer, sizeof(buffer), 0);
      if (rc == -1) {
        cout << "Error Recieving: " << errno << endl;
        // TODO Handle Error
      }
      string rec_data(buffer);
      if (rec_data != "NO MATCH FOUND") {
        cout << rec_data << endl;
      }
    }
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "EOF");
    rc = send(client_sock, buffer, strlen(buffer), 0);
    close(client_sock);
    file.close();
    return 0;
  }
  // Child Process
  else if (n1 == 0) {
    int server_sock, client_sock, rc;
    socklen_t length;
    int byte_rec = 0;
    struct sockaddr_un server_sockaddr;
    struct sockaddr_un client_sockaddr;
    char buffer[256];
    int backlog = 10;
    bool run = true;
    memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(buffer, 0, 256);

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
    while (run) {
      memset(buffer, 0, sizeof(buffer));
      byte_rec = recv(client_sock, buffer, sizeof(buffer), 0);
      if (byte_rec == -1) {
        cout << "Recieve Error: " << errno << endl;
        // TODO: Handle the Recieve
      }

      string rec_data(buffer);
      string word = argv[2];
      if (rec_data == "EOF") {
        close(server_sock);
        close(client_sock);
        run = false;
        break;
      }

      string regex_build = "\\b" + word + "\\b";
      regex e(regex_build, icase);
      if (regex_search(rec_data, e)) {
        rc = send(client_sock, buffer, strlen(buffer), 0);
        if (rc == -1) {
          cout << "Server Error Sending: " << errno << endl;
        }
      } else {
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, "NO MATCH FOUND");
        rc = send(client_sock, buffer, strlen(buffer), 0);
        if (rc == -1) {
          cout << "Server Error Sending: " << errno << endl;
        }
      }
    }
    return (0);
  }
}