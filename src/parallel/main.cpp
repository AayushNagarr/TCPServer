/* 
 * tcpserver.c - A multithreaded TCP echo server 
 * usage: tcpserver <port>
 * 
 * Testing : 
 * nc localhost <port> < input.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <bits/stdc++.h>
#include <string>
#include <cstring>
#include <pthread.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <netinet/in.h>

using namespace std;

unordered_map<string,string> mymap;



void error(const char* msg)
{
  perror(msg);
  exit(1);
}
void* handleClient(void* acceptfd){

  int clientfd =  *(int*) acceptfd;
  char buffer[256];

  int n = read(clientfd, buffer, 255);
  if(string(buffer).find("END") == -1) error("No end statement. Broken Code");

  // cout << buffer << endl;

  stringstream inputstr;
  inputstr <<  string(buffer);

  string input_command;
  string input_key;
  string input_value;
  while(true)
  {
  inputstr >> input_command;


  if(input_command == "WRITE"){
    inputstr >> input_key;
    inputstr >> input_value;

    cout << "Input key is on write " << input_key << " and input_value be " << input_value << endl;
    input_value = input_value.substr(1, sizeof(input_value) - 1);
    mymap[input_key] = input_value;
    write(clientfd, "FIN\n", 4);
  }
  else if(input_command == "READ")
  {
    inputstr >> input_key;
    cout << "Input key is on read " << input_key << endl;
    if(mymap.find(input_key) == mymap.end())
      write(clientfd, "NULL\n", 5);
    else{
      write(clientfd, (mymap[input_key] + "\n").c_str(), (mymap[input_key]).size() + 1);
    }
  }
  else if(input_command == "COUNT")
  {
      string size_str = to_string(mymap.size());
      write(clientfd, (size_str + "\n").c_str(), (size_str).size() + 1);
  }
  else if(input_command == "DELETE")
  {
    inputstr >> input_key;
    cout << "Input key is on delete" << input_key << endl;
    if(mymap.erase(input_key))
      write(clientfd, "FIN\n", 4);
    else{
      write(clientfd, "NULL\n", 5);
    }
  }
  else if(input_command == "END")
  {
      break;
  }
  else{
      error("Invalid Input");
  }
  }
  
  pthread_exit(NULL);
  
}

int main(int argc, char ** argv) {
  
  int portno = 8080; /* port to listen on */
  struct sockaddr_in server_sockaddr, client_sockaddr;
  bzero((char *) &server_sockaddr, (socklen_t)sizeof(server_sockaddr));
  server_sockaddr.sin_family = AF_INET;
  server_sockaddr.sin_port = htons(portno);
  server_sockaddr.sin_addr.s_addr = INADDR_ANY;
  
  /* 
   * check command line arguments 
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  // DONE: Server port number taken as command line argument
  portno = atoi(argv[1]);



  //1) Create a Socket for server, which will return a file descriptor


  int serverfd = socket(AF_INET, SOCK_STREAM, 0);
  if(serverfd < 0)
  {
    error("Error at socket creation");
  }


  int option = 1;
  setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

  //2) Bind the created socket to a port
  
  int binderr = bind(serverfd, (struct sockaddr*)&server_sockaddr, sizeof(server_sockaddr));

  if(binderr < 0)
  {
    error("Error at binding to port");
  }

  //3) Listen for connections from some remote client
  int listenerr = listen(serverfd, 5);

  if(listenerr<0)
  {
    error("Error at listening");
  }

socklen_t acceptsize = sizeof(client_sockaddr);
//4) Accept connection from a client
 while(1)
 { 
  int acceptfd = accept(serverfd, (struct sockaddr*)&client_sockaddr, &acceptsize);
  if(acceptfd < 0)
  {
    error("Error at accepting connection");
  }

  pthread_t new_client;
  int * clientfd = new int(acceptfd);
  printf("Creating new thread \n");

  pthread_create(&new_client, NULL, handleClient, (void*)clientfd);
  // pthread_join(new_client, NULL);
  printf("After joining back \n");
}
}