#include <sstream>
#include <cerrno>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "server.hpp"
#include "utils.hpp"

static const std::vector<std::string> *fileNamesToSearch;

void grep::openSocketAndWait()
{
  // Based off:
  // http://beej.us/guide/bgnet/output/html/multipage/syscalls.html#connect
  int status;
  struct addrinfo hints;
  struct addrinfo *servinfo;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC; // don't care IPv4/v6
  hints.ai_socktype = SOCK_STREAM; // TCP stream socket
  hints.ai_flags = AI_PASSIVE; // fill in my IP automatically

  status = getaddrinfo(NULL, GREP_LISTEN_PORT_STR, &hints, &servinfo);
  if (status != 0) {
    MPLOG("getaddrinfo error: %s", gai_strerror(status));
    exit(1);
  }

  // Open a socket
  int sockfd = socket(servinfo->ai_family, servinfo->ai_socktype,
      servinfo->ai_protocol);
  if (sockfd == -1) {
    MPLOG("Error opening socket: %s", strerror(errno));
    exit(1);
  }

  // Bind to the socket
  if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
    MPLOG("Error on bind: %s", strerror(errno));
    exit(1);
  }

  // Listen on the socket
  int backlog = 20;
  if (listen(sockfd, backlog) == -1) {
    MPLOG("Error on listen: %s", strerror(errno));
    exit(1);
  }
  MPLOG("Waiting for connection on port %s", GREP_LISTEN_PORT_STR);

  // Accept incoming connections ad infinitum
  while (1) {
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof(their_addr);
    int new_connfd = accept(sockfd, (struct sockaddr *)&their_addr,
            &addr_size);
    if (new_connfd == -1) {
      MPLOG("Error accepting connection: %s", strerror(errno));
      exit(1);
    }
    MPLOG("Got new connection");
    handleNewConnection(new_connfd);
    close(new_connfd);
    MPLOG("Reply sent successfully. Waiting for next connection...");
  }

  freeaddrinfo(servinfo);
}

void grep::handleNewConnection(int connfd)
{
  int err;
  // Wait for the client to send a command
  std::string query = receiveCommand(connfd);
  MPLOG("Received command: %s", query.c_str());
  // Disable receiving on the socket
  err = shutdown(connfd, SHUT_RD);
  if (err != 0) {
    MPLOG("Error in shutdown: %s", strerror(errno));
  }

  // Parse the command
  grep::Command cmd = grep::Command(query);

  // Call out to grep and pipe the results back over the network
  callOutToTheRealGrep(cmd, connfd);

  // Send a null byte to the client to signify that we're done
  char nul = '\0';
  ssize_t bytesSent = send(connfd, &nul, sizeof(nul), 0);
  if (bytesSent == -1) {
    MPLOG("Encountered error while sending. Assuming the connection with "
          "the client has been closed and giving up.");
  }

  // Disable sending and receiving on the socket
  err = shutdown(connfd, SHUT_RDWR);
  if (err != 0) {
    MPLOG("Error in shutdown: %s", strerror(errno));
  }
}

std::string grep::receiveCommand(int connfd)
{
  // Get the command
  std::stringstream commandStr;
  char buf[1025];
  int buf_size; // Num bytes of valid data
  bool done = false;
  while (!done) {
    buf_size = recv(connfd, &buf[0], sizeof(buf) - 1, 0);
    if (buf_size == -1) {
      MPLOG("Receive error; continuing. %s", strerror(errno));
      continue;
    } else if (buf_size == 0) {
      MPLOG("Client either closed the connection or netcat is done sending");
      break;
    }
    // Check if we've received a null byte
    if (buf[buf_size - 1] == '\0') {
      done = true;
    }
    // Append the bytes received to the command string
    buf[buf_size] = '\0';
    commandStr << buf;
  }
  return commandStr.str();
}

void grep::callOutToTheRealGrep(const grep::Command &cmd, int connfd)
{
  // Concat the actual command to be run
  std::stringstream sstr;
  // Start with the program and our default args
  sstr << "grep";
  sstr << " -H"; // Print the filename
  // Add on the search terms
  for (auto it = cmd.argList.begin(); it != cmd.argList.end(); ++it) {
    sstr << " " << *it;
  }
  // Add on the files to be searched
  const std::vector<std::string> &files = generateFileNamesToSearch();
  for (auto it = files.begin(); it != files.end(); ++it) {
    const std::string &file = *it;
    sstr << " " << file;
  }
  const char *commandStr = sstr.str().c_str();
  MPLOG("%s", commandStr);

  // Run the command and open a pipe to it
  FILE *pipefp = popen(commandStr, "r");
  if (pipefp == NULL) {
    throw std::runtime_error("popen failed");
  }

  size_t totalBytesSent = 0;
  while (!feof(pipefp)) {
    // Read from the pipe
    char buf[1024];
    size_t buf_size = fread(buf, 1, sizeof(buf), pipefp);
    // if (buf_size == 0) {
    //   MPLOG("Read 0 bytes. That's not right... continuing anyway: %s",
    //         strerror(errno));
    // }

    // Send the data back over the network
    size_t bytesSentForBuf = 0;
    while (bytesSentForBuf < buf_size) {
      ssize_t bytesSent = send(connfd, &buf[bytesSentForBuf],
                               buf_size - bytesSentForBuf, 0);
      if (bytesSent == -1) {
        MPLOG("Encountered error while sending. Assuming the connection with "
              "the client has been closed and giving up.");
        goto pipeDone;
      }
      bytesSentForBuf += bytesSent;
      totalBytesSent += bytesSent;
    }
  }

pipeDone:
  // Clean up after the pipe
  int exitStatus = pclose(pipefp);
  if (exitStatus != 0 && exitStatus != 256) {
    MPLOG("The command exited with a non-zero status: %d", exitStatus);
  }
  MPLOG("Sent %zu bytes", totalBytesSent);
}

const std::vector<std::string> & grep::generateFileNamesToSearch()
{
  if (fileNamesToSearch == NULL || fileNamesToSearch->size() == 0) {
    // No file names given
    // Set the default file name and try again
    std::vector<std::string> fileNames;
    fileNames.push_back(getDefaultFileNameToSearch());
    setFileNamesToSearch(fileNames);
    return generateFileNamesToSearch();
  } else {
    // Use the file names given
    return *fileNamesToSearch;
  }
}

std::string grep::getDefaultFileNameToSearch()
{
  int servNum = getServerNumber();
  std::stringstream sstr;
  sstr << "vm" << servNum << ".log";
  return sstr.str();
}

int grep::getServerNumber()
{
  char name[1024];
  if (gethostname(&name[0], sizeof(name)) != 0) {
    // Failure
    MPLOG("Unable to get hostname: %s", strerror(errno));
    return -1;
  }
  name[sizeof(name) - 1] = '\0';

  // Parse our hostname to get our server number
  unsigned servNum;
  if (sscanf(name, "fa17-cs425-g18-%02u.cs.illinois.edu", &servNum) == 1) {
    // Success
    MPLOG("Got hostname %s", name);
    return static_cast<int>(servNum);
  }

  // Failure
  MPLOG("Error parsing the server number from our hostname");
  return -1;
}

void grep::setFileNamesToSearch(const std::vector<std::string> &fileNames)
{
  if (fileNamesToSearch != NULL) {
    delete fileNamesToSearch;
  }
  fileNamesToSearch = new std::vector<std::string>(fileNames);
}

