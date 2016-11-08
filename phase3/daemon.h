#ifndef DAEMON_H
#define DAEMON_H

#include <sys/types.h>
#include<sys/socket.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <iomanip> 
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include<signal.h>
#include<netdb.h>
#include <mqueue.h>
#include<string.h> //for memset
#include<vector>
#include<cmath>
#include<utility>
#include<string.h>
#include "Map.h"
#include "fancyRW.h"
using namespace std;
struct GameBoard {
   int rows; //4 bytes
   int cols; //4 bytes
   int daemonID;
   unsigned char players;
   pid_t pid[5];
   unsigned char boardMap[0]; // aray bound check is not done in C++
};


void serverDaemon();
void writePlayer(int );
void writeMap(int );
void writeMessage(int );
void serverSocket();
void clientSocket(int , char *arr);
void readFunction();
void signalInitialize();
void createMQ(string, int);
void deleteMQ(string, int);

#endif

