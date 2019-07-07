#include <iostream>
#include <cerrno>
#include <fstream>
#include <cstring>
#include <sstream>
#include <vector>
#include "client.hpp"
#include "server.hpp"
#include "utils.hpp"
#include <pthread.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>


void grep::connectToAllServersAsync(std::vector<std::string> args)
{

  std::vector<const char *> hostNames = generateServerHostnames();	//create the list of host server names
  pthread_t threads[GREP_NUM_SERVERS];  //create the list of threads
  theParams params[GREP_NUM_SERVERS]; //create the array of structs to be passed as out argument for our pthread function


  for(int i = 0; i < 10; i++){
    params[i].id = i+1;   //an id for each thread that matches the server's name
    params[i].hostName = hostNames[i];  //the name of the server to be handles by the thread
    params[i].theArgs = args; //the grep arguments to be passed
    pthread_create(&threads[i], NULL, grep::doServerTransaction, &params[i]); // creates the threads
  }

  for(int i = 0; i < 10; i++){
    pthread_join(threads[i], NULL); // join all threads
  }
}

void * grep::doServerTransaction(void* arg)
{
  int sockid;   // the socket file descriptor to be passed
  std::string results;  //the results of each grep
  theParams threadParam = *((theParams*)(arg)); // setting the pointer
  sockid = grep::connectToServer(threadParam.hostName); // gets the file descriptor and makes the connection to the server
  SendToServer(threadParam.theArgs, sockid);  // sends the command to the server
  results = receiveFromServer(sockid);  // receives the grep output from the server
  writeToFile(results, threadParam.id); // writes the received data into files
  pthread_exit(0);
}



int grep::connectToServer(const char *hostname)
{
  // Based off:
  // http://beej.us/guide/bgnet/output/html/multipage/syscalls.html#connect
  int status;
  struct addrinfo hints;
  struct addrinfo *ai_results, *servinfo;   //creates the necessary structs

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC; // don't care IPv4/v6
  hints.ai_socktype = SOCK_STREAM; // TCP stream socket
  hints.ai_flags = AI_PASSIVE; // fill in my IP automatically

  status = getaddrinfo(hostname, GREP_LISTEN_PORT_STR, &hints, &ai_results);  // sets up the struct in order to make connections
  if (status != 0) {
    MPLOG("getaddrinfo error: %s", gai_strerror(status));
    exit(1);
  }

  // Find the valid result entry.
  servinfo = ai_results; // we are preparing to loo through to get a sufficient struct
  int sockfd;
  // Open up a socket.
  for(ai_results = servinfo; ai_results != NULL; ai_results = ai_results->ai_next){	  // loops through to get a correct output
     sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sockfd == -1) {
      MPLOG("Error opening socket: %s", strerror(errno));
      continue;
    }

    // Connect to the server.
    if(connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {  // tries makes the socket connection
      MPLOG("Error connecting to server: %s", strerror(errno));
      continue;
    }

    break;  // if no error, break from the loop
  }

  if(ai_results == NULL){
    MPLOG("NO available socket: %s", strerror(errno));  // if we could not find any possible solutions, program exits
    pthread_exit(0);
  }

  freeaddrinfo(ai_results); // free the memory
  return sockfd;  // returns the file descriptor
}



void grep::SendToServer(std::vector<std::string> theArgs, int sockfd)
{

  int num = theArgs.size();
  char end = '\0';

  for (int i = 0; i < num; i++) {
    const char * test = theArgs[i].c_str();
    size_t length = strlen(test);
    // Send the data back over the network
    size_t bytesSentForBuf = 0;
    while (bytesSentForBuf < length) {
      ssize_t bytesSent = send(sockfd, &test[bytesSentForBuf],
                               length - bytesSentForBuf, 0);
      if (bytesSent == -1) {
        MPLOG("Encountered error while sending. Assuming the connection with "
              "the server has been closed and giving up.");
        close(sockfd);
        pthread_exit(0);
      }
      bytesSentForBuf += bytesSent;
    }
  }

  ssize_t sent = send(sockfd, &end, sizeof(end), 0);
  if (sent != sizeof(end)) {
    MPLOG("End was not sent successfully: %s", strerror(errno));
  	close(sockfd);
    pthread_exit(NULL);
   }
}

std::string grep::receiveFromServer(int sockfd)
{
  std::stringstream theStr;
  char buf[1024];
  int buf_size; // Num bytes of valid data
  bool done = false;
  while (!done) {
    buf_size = recv(sockfd, &buf[0], sizeof(buf), 0);
    if (buf_size == -1) {
      MPLOG("Receive error; continuing. %s", strerror(errno));
      continue;
    }
    else if (buf_size == 0) {
      MPLOG("Client closed the connection.");
      break;
    }

    // Check if we've received a null byte.
    if (buf[buf_size - 1] == '\0') {
      done = true;
    }

    buf[buf_size] = '\0';
    theStr << buf;
  }
  close(sockfd);
  return theStr.str();

}

void grep::writeToFile(std::string Results, int id)
{
  std::string ID = std::to_string(id);
  std::string Push;
  std::string grepOut = "grepOut";
  grepOut += ID;
  grepOut += ".txt";
  int numLines = std::count(Results.begin(), Results.end(), '\n');
  printf("Server %02d has %d lines\n",  id, numLines);
  std::ofstream out(grepOut);

      out << Results;

  out.close();
}



std::vector<const char *> grep::generateServerHostnames()
{
  std::vector<const char *> hostnames;
  for (int i = 1; i <= GREP_NUM_SERVERS; ++i) {
    char *hostname = NULL;
    if (asprintf(&hostname, "fa17-cs425-g18-%02d.cs.illinois.edu", i) == -1) {
      MPLOG("asprintf ran out of memory :(");
      exit(1);
    }
    hostnames.push_back(hostname);
  }
  return hostnames;
}

