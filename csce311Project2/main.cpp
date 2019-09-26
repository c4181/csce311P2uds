#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fstream>
#include <iostream>

using std::cout;
using std::endl;
using std::ifstream;
using std::string;

constexpr auto SOCK_PATH = "tpf_unix_sock.server";
constexpr auto DATA = "";

int main(int argc, char *argv[]) {
  int n1 = fork();

  // Parent Process
  if (n1 > 0) {
    string line;
    ifstream file(argv[0]);

    int server_sock, client_sock, len, rc;
    int bytes_rec = 0;
    struct sockaddr_un server_sockaddr;
    struct sockaddr_un client_sockaddr;
    char buffer[256];
    int backlog = 1;
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
  }
}
 