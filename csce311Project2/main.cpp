#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <regex>

using std::cout;
using std::endl;
using std::ifstream;
using std::regex;
using std::regex_match;
using std::string;
using std::regex_constants::icase;

constexpr auto SERVER_PATH = "unix_sock.server";
constexpr auto CLIENT_PATH = "unix_sock.client";
constexpr auto BUFFER_SIZE = 256;

/***************************************************************************
 * Author/copyright:  Christopher Moyer.  All rights reserved.
 * Date: 7 October 2019
 *
 * This program takes in a text file and a word and outputs any matches to 
 * stdout. Parent process handles all input/output and child process
 * handles parsing the file for matches. All information is passed
 * between processes using UNIX sockets.
 *
 **/

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
    char buffer[BUFFER_SIZE];
    memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(buffer, 0, sizeof(buffer));

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

    // Connect to the server and read in a file until there are no more
    // lines. After each line, send the line to server for processing.
    // Wait for the server to respond and then either output the line
    // to stdout or continue processing as appropriate.
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
    file.close();
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "EOF");
    rc = send(client_sock, buffer, strlen(buffer), 0);
    close(client_sock);
    return 0;
  } else if (n1 == 0) {
    // Child Process
    int server_sock, client_sock, rc;
    socklen_t length;
    int byte_rec = 0;
    struct sockaddr_un server_sockaddr;
    struct sockaddr_un client_sockaddr;
    char buffer[BUFFER_SIZE];
    int backlog = 10;
    bool run = true;
    memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(buffer, 0, sizeof(buffer));

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

    // Wait for incoming client connections. Once a client connects,
    // continue processing and sending the line of text when a match
    // is found or "NO MATCH FOUND" if a match is not found. Close all
    // connections once client sends "EOF" signaling the end of the file.
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
        close(client_sock);
        close(server_sock);
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
