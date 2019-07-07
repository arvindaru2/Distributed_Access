
#pragma once

#include <vector>
#include <string>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#define GREP_NUM_SERVERS 10

struct theParams {
  const char * hostName;
  std::vector<std::string> theArgs;
  int id;
};

namespace grep {
  void connectToAllServersAsync(std::vector<std::string> args);

  void * doServerTransaction(void* arg);

  int connectToServer(const char *hostname);

  void SendToServer(std::vector<std::string> theArgs, int sockfd);

  std::string receiveFromServer(int sockfd);

  void writeToFile(std::string Results, int id);


  /**
   * Create a vector containing the hostname of every valid hostname of a server
   * that might reasonably be running this program.
   *
   * @returns a vector of strings. These strings must be freed by the caller.
   */
  std::vector<const char *> generateServerHostnames();
}

