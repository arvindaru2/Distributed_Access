
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "client.hpp"
#include "server.hpp"

static void printUsageAndExit(const char *argv0)
{
  printf("Usage:\n");
  printf("\t%s (-c | --client) args [...]\n", argv0);
  printf("\t%s (-s | --server) [file1 [...]]\n", argv0);
  exit(1);
}

int main(int argc, const char *argv[])
{
  // Check the args to figure out which mode to run in
  if (argc < 2) {
    printUsageAndExit(argv[0]);
  }
  const char *modeStr = argv[1];
  if (strcmp("-c", modeStr) == 0 || strcmp("--client", modeStr) == 0) {
    // Client mode
    if (argc <= 2) {
      printUsageAndExit(argv[0]);
    }
    // Pick out the args to send to the remote greps
    std::vector<std::string> args;
    for (int i = 2; i < argc; i++) {
      args.push_back(std::string(argv[i]));
    }
    grep::connectToAllServersAsync(args);
  } else if (strcmp("-s", modeStr) == 0 || strcmp("--server", modeStr) == 0) {
    // Server mode
    if (argc < 2) {
      printUsageAndExit(argv[0]);
    }
    // Parse out the files to be searched
    std::vector<std::string> files;
    for (int i = 2; i < argc; i++) {
      files.push_back(std::string(argv[i]));
    }
    grep::setFileNamesToSearch(files);
    // Start up the server
    grep::openSocketAndWait();
  } else {
    // No mode specified
    printUsageAndExit(argv[0]);
  }
}

