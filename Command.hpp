
#pragma once

#include <string>
#include <vector>

namespace grep {
  class Command {
  public:
    Command(const std::string &commandStr);

    std::string uuid;
    std::vector<std::string> argList;

  private:
    /**
     * Attempt to read a valid UUID string off the front of uuidStr. If the
     * string is invalid for any reason, an exception is raised.
     */
    std::string parseUUID(const std::string &uuidStr) const;

    /**
     * Make sure the args are valid and don't contain any potentially
     * problematic text from a security perspective.
     */
    std::vector<std::string> parseArgs(const std::string &argStr) const;
  };
}
