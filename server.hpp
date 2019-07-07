
#pragma once

#include <map>
#include <string>
#include <vector>
#include "Command.hpp"

#define GREP_LISTEN_PORT_STR "5678"
#define UUID_STR_LENGTH 36

namespace grep {
  void openSocketAndWait() __attribute__((noreturn));

  /**
   * Set up our state once we get a new connection.
   */
  void handleNewConnection(int connfd);

  std::string receiveCommand(int connfd);

  void callOutToTheRealGrep(const grep::Command &cmd, int connfd);

  const std::vector<std::string> & generateFileNamesToSearch();

  std::string getDefaultFileNameToSearch();

  int getServerNumber();

  void setFileNamesToSearch(const std::vector<std::string> &fileNames);
}

//std::map<grep::UUID, grep::ConnectionState> connections;

