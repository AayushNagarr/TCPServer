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
#define THREAD_LIMIT 128
using namespace std;


  // static pthread_cond_t queuecond;

struct threadArgs {
  int clientfd;
  static unordered_map<string, string> mymap;
  static pthread_rwlock_t mylock;
  static queue<int> thread_queue;
  static pthread_mutex_t queuelock;
  static pthread_cond_t queuecond;
  threadArgs(int clientfd){
    cout << "in constructor" << endl;
    this->clientfd = clientfd;
    pthread_rwlock_init(&(this->mylock), NULL);
    pthread_mutex_init(&(this->queuelock), NULL);
    pthread_cond_init(&(this->queuecond), NULL);
    
  }

};

unordered_map<string, string> threadArgs::mymap;
pthread_rwlock_t threadArgs::mylock;
pthread_mutex_t threadArgs::queuelock;
queue<int> threadArgs::thread_queue;
pthread_cond_t threadArgs::queuecond;

void error(const char* msg)
{
  perror(msg);
  exit(1);
}
void* handleClient(void* mythread_void){
while(true){
  pthread_mutex_lock(&(threadArgs::queuelock));
  while((threadArgs::thread_queue).empty()){
  pthread_cond_wait(&(threadArgs::queuecond), &(threadArgs::queuelock));
  cout << "Waiting empty" << endl;
  }
  threadArgs mythread = threadArgs((threadArgs::thread_queue).front());
  mythread.thread_queue.pop();
  pthread_mutex_unlock(&(threadArgs::queuelock));

  
  int clientfd = mythread.clientfd;
  char buffer[256];

  int n = read(clientfd, buffer, 255);
  if(string(buffer).find("END") == -1) 
  {
    close(clientfd);
    return NULL;
  }
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
    pthread_rwlock_wrlock(&(mythread.mylock));

    inputstr >> input_key;
    inputstr >> input_value;

    cout << "Input key is on write " << input_key << " and input_value be " << input_value << endl;
    input_value = input_value.substr(1, sizeof(input_value) - 1);
    mythread.mymap[input_key] = input_value;
    write(clientfd, "FIN\n", 4);
    pthread_rwlock_unlock(&(mythread.mylock));

  }
  else if(input_command == "READ")
  {
     cout << "LOCKING" << endl;
    pthread_rwlock_rdlock(&(mythread.mylock));
    inputstr >> input_key;
    cout << "Input key is on read " << input_key << endl;
    if(mythread.mymap.find(input_key) == mythread.mymap.end())
      write(clientfd, "NULL\n", 5);
    else{
      write(clientfd, (mythread.mymap[input_key] + "\n").c_str(), (mythread.mymap[input_key]).size() + 1);
    }
    pthread_rwlock_unlock(&(mythread.mylock));
     cout << "unLOCKING" << endl;
  }
  else if(input_command == "COUNT")
  {
    pthread_rwlock_rdlock(&(mythread.mylock));
      string size_str = to_string(mythread.mymap.size());
      write(clientfd, (size_str + "\n").c_str(), (size_str).size() + 1);
    pthread_rwlock_unlock(&(mythread.mylock));
  }
  else if(input_command == "DELETE")
  {
    pthread_rwlock_wrlock(&(mythread.mylock));
    inputstr >> input_key;
    cout << "Input key is on delete" << input_key << endl;
    if(mythread.mymap.erase(input_key))
      write(clientfd, "FIN\n", 4);
    else{
      write(clientfd, "NULL\n", 5);
    }
    pthread_rwlock_unlock(&(mythread.mylock));
  }
  else if(input_command == "END")
  {
      close(clientfd);
      break;
  }
  else{
      error("Invalid Input");
  }
  }
  }
  pthread_exit(NULL);
  
}

int main(int argc, char ** argv) {
  
    // pthread_cond_init(&(threadArgs::queuecond), NULL);

  threadArgs* test = new threadArgs(0);
  
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

  for(int i = 0; i< THREAD_LIMIT; i++)
  {
    pthread_t tid;
    pthread_create(&tid, NULL, handleClient, NULL);
  }
//4) Accept connection from a client
 while(1)
 { 
  int acceptfd = accept(serverfd, (struct sockaddr*)&client_sockaddr, &acceptsize);
  if(acceptfd < 0)
  {
    error("Error at accepting connection");
  
  }

  //Pushing acceptfd to the queue
  pthread_mutex_lock(&(threadArgs::queuelock));
  cout << "Locking queue" << endl;
  threadArgs::thread_queue.push(acceptfd);
  pthread_cond_signal(&(threadArgs::queuecond));
  pthread_mutex_unlock(&(threadArgs::queuelock));


  printf("Creating new thread \n");

}
}