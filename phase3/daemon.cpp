#include"daemon.h"
using namespace std;
GameBoard *gbPtr;
const char* shmName1="PDSGoldMem";
const char* semName1="PDSGoldSem";
int totalRows, totalColumns;
sem_t *goldSem1;
int clientfd; //file descriptor for the socket client;
int fdMem;
char *localMap;
int pipeDiscriptor;
string msgQueue[5];
mqd_t mqFileDescriptor[5];

void serverDaemon()
{
   clientfd=-1;
   pipeDiscriptor=open("/home/pooja/Documents/611/project3/gamePipe", O_RDWR);
   signalInitialize();
   msgQueue[0]="/player1_mq";
   msgQueue[1]="/player2_mq";
   msgQueue[2]="/player3_mq";
   msgQueue[3]="/player4_mq";
   msgQueue[4]="/player5_mq";

   mqFileDescriptor[0]=-1;
   mqFileDescriptor[1]=-1;
   mqFileDescriptor[2]=-1;
   mqFileDescriptor[3]=-1;
   mqFileDescriptor[4]=-1;

   goldSem1=sem_open(
         semName1, 
         O_RDWR); 
   if(goldSem1==SEM_FAILED)
   {
      close(pipeDiscriptor);
      exit(1);
   }
   sem_wait(goldSem1);

   fdMem=shm_open(shmName1, O_RDWR, S_IROTH| S_IWOTH| S_IRGRP| S_IWGRP| S_IRUSR| S_IWUSR);
   if(fdMem==-1)
   {
      close(pipeDiscriptor);
      exit(0);
   }
   read(fdMem,&totalRows,sizeof(totalRows));
   read(fdMem,&totalColumns,sizeof(totalColumns));
   gbPtr=(GameBoard*)mmap(NULL, sizeof(GameBoard)+(totalRows*totalColumns), PROT_WRITE|PROT_READ, MAP_SHARED, fdMem, 0);

   localMap=new char [totalRows*totalColumns];
   for(int i=0; i<=(totalRows*totalColumns)-1; i++)
      localMap[i]=gbPtr->boardMap[i];
   gbPtr->daemonID=getpid(); //for server
   sem_post(goldSem1);
}

void writePlayer(int) // SIGHUP handler
{
   unsigned char playerInfo;
   playerInfo=G_SOCKPLR|gbPtr->players;
   if(clientfd!=-1)
      WRITE(clientfd, &playerInfo, 1);
   if(!gbPtr->players)
   {
      close(fdMem);
      if(shm_unlink(shmName1)==-1)
         perror("Error in unlinking shared memory");
      sem_close(goldSem1);
      if(sem_unlink(semName1)==-1)
         perror("Error in unlinking semaphore");
      exit(1);
   }
}

void writeMap(int )
{ 
   int vectorSize;
   unsigned char mapTypeVal=0;
   vector < pair<short , unsigned char >> v;
   short offset;
   unsigned char value;	
   for(int i=0; i<=(totalRows*totalColumns)-1; i++)
   {
      if(localMap[i]!=gbPtr->boardMap[i])
      {
         localMap[i]=gbPtr->boardMap[i];	
         v.push_back(pair<short, unsigned char> (i, localMap[i]));
      }
   }
   vectorSize=v.size();
   if(vectorSize>0)
   {
      WRITE(clientfd, &mapTypeVal, 1);
      WRITE(clientfd, &vectorSize, sizeof(vectorSize));
      for(int i=0; i<vectorSize; i++)
      {
         WRITE(clientfd, &v[i].first, sizeof(v[i].first));
         WRITE(clientfd, &v[i].second, sizeof(v[i].second));
      }

   }
}

void writeMessage(int )
{
   char msg[121];
   int msgSize, err;
   unsigned char players=0;
   struct sigevent mq_notification_event;
   mq_notification_event.sigev_notify=SIGEV_SIGNAL;
   mq_notification_event.sigev_signo=SIGUSR2;
   for(int i=0; i<5; i++)
   {
      if(mqFileDescriptor[i]!=-1)
         mq_notify(mqFileDescriptor[i], &mq_notification_event);
   }

   for(int i=0; i<5; i++)
   {
      memset(msg, 0, 121);
      while((err=mq_receive(mqFileDescriptor[i], msg, 120, NULL))!=-1)
      {
         msgSize=strlen(msg);
         if(i==0)
         {
            players=players|G_PLR0;
         }
         else if(i==1)
         {
            players=players|G_PLR1;
         }
         else if(i==2)
         {
            players=players|G_PLR2;
         }
         else if(i==3)
         {
            players=players|G_PLR3;
         }
         else if(i==4)
         {
            players=players|G_PLR4;
         }

         players=players|G_SOCKMSG;
         WRITE(clientfd, &players, sizeof(players));
         WRITE(clientfd, &msgSize, sizeof(msgSize));
         WRITE(clientfd, &msg, msgSize);
      }

   }
}

void serverSocket()
{
   int sockfd; //file descriptor for the socket
   int status; //for error checking

   //change this # between 2000-65k before using
   const char* portno="42424";
   struct addrinfo hints;
   memset(&hints, 0, sizeof(hints)); //zero out everything in structure
   hints.ai_family = AF_UNSPEC; //don't care. Either IPv4 or IPv6
   hints.ai_socktype=SOCK_STREAM; // TCP stream sockets
   hints.ai_flags=AI_PASSIVE; //file in the IP of the server

   struct addrinfo *servinfo;
   if((status=getaddrinfo(NULL, portno, &hints, &servinfo))==-1)
   {
      close(pipeDiscriptor);
      exit(1);
   }
   sockfd=socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

   /*avoid "Address already in use" error*/
   int yes=1;
   if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1)
   {
      close(pipeDiscriptor);
      exit(1);
   }

   //We need to "bind" the socket to the port number so that the kernel
   //can match an incoming packet on a port to the proper process
   if((status=bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen))==-1)
   {
      close(pipeDiscriptor);
      exit(1);
   }
   //when done, release dynamically allocated memory
   freeaddrinfo(servinfo);

   if(listen(sockfd,1)==-1)
   {
      close(pipeDiscriptor);
      exit(1);
   }

   struct sockaddr_in client_addr;
   socklen_t clientSize=sizeof(client_addr);
   do
      clientfd=accept(sockfd, (struct sockaddr*) &client_addr, &clientSize);
   while(clientfd==-1 && errno==EINTR);
   if(clientfd==-1)
   {
      close(pipeDiscriptor);
      exit(1);
   }
   WRITE(clientfd, &totalRows, sizeof(totalRows));         // rows sent
   WRITE(clientfd, &totalColumns, sizeof(totalColumns));   // columns sent
   WRITE(clientfd, &(gbPtr->players), 1);
   for(int i=0; i<=(totalRows*totalColumns)-1; i++)
   {
      if(localMap[i]!=gbPtr->boardMap[i])
      {
         localMap[i]=gbPtr->boardMap[i];
      }
   }	
   WRITE(clientfd, localMap, totalRows*totalColumns);
   while(1)
   {
      readFunction();
   }
}

void clientSocket(int pipefd, char *ipAddress)
{
   clientfd=-1;
   int pipeDiscriptor=open("/home/pooja/Documents/611/project3/gamePipe", O_RDWR);
   unsigned char players;
   int status; //for error checking
   msgQueue[0]="/player1_mq";
   msgQueue[1]="/player2_mq";
   msgQueue[2]="/player3_mq";
   msgQueue[3]="/player4_mq";
   msgQueue[4]="/player5_mq";

   mqFileDescriptor[0]=-1;
   mqFileDescriptor[1]=-1;
   mqFileDescriptor[2]=-1;
   mqFileDescriptor[3]=-1;
   mqFileDescriptor[4]=-1;

   //change this # between 2000-65k before using
   const char* portno="42424";
   // localMap=new char [totalRows, totalColumns];
   struct addrinfo hints;
   memset(&hints, 0, sizeof(hints)); //zero out everything in structure
   hints.ai_family = AF_UNSPEC; //don't care. Either IPv4 or IPv6
   hints.ai_socktype=SOCK_STREAM; // TCP stream sockets
   int val;
   struct addrinfo *servinfo;
   //instead of "localhost", it could by any domain name
   if((status=getaddrinfo(ipAddress, portno, &hints, &servinfo))==-1)
   {
      val=1;
      write(pipefd, &val, sizeof(val));
      exit(1);
   }
   clientfd=socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

   if((status=connect(clientfd, servinfo->ai_addr, servinfo->ai_addrlen))==-1)
   {
      val=1;
      write(pipefd, &val, sizeof(val));
      exit(1);
   }
   //release the information allocated by getaddrinfo()
   freeaddrinfo(servinfo);

   READ(clientfd, &totalRows, sizeof(totalRows));           // reading starts from this line
   READ(clientfd, &totalColumns, sizeof(totalColumns));

   localMap=new char [totalRows*totalColumns];
   READ(clientfd, &players, 1);
   READ(clientfd, localMap, totalRows*totalColumns);

   goldSem1=sem_open(
         semName1, 
         O_CREAT,
         S_IROTH| S_IWOTH| S_IRGRP| S_IWGRP| S_IRUSR| S_IWUSR,
         1
         ); 
   if(goldSem1==SEM_FAILED)
   {
      val=1;
      write(pipefd, &val, sizeof(val));
   }

   sem_wait(goldSem1);
   int fdMem=shm_open(shmName1, O_CREAT|O_RDWR, S_IROTH| S_IWOTH| S_IRGRP| S_IWGRP| S_IRUSR| S_IWUSR);
   if(fdMem==-1)
   {
      val=1;
      write(pipefd, &val, sizeof(val));
   }	

   ftruncate(fdMem, sizeof(GameBoard)+(totalRows*totalColumns));
   gbPtr=(GameBoard*)mmap(NULL, sizeof(GameBoard)+(totalRows*totalColumns), PROT_WRITE|PROT_READ, MAP_SHARED, fdMem, 0);
   gbPtr->rows=totalRows;       // initialization starts here
   gbPtr->cols=totalColumns;
   gbPtr->players=players;
   gbPtr->daemonID=getpid(); // for client
   memcpy(gbPtr->boardMap,localMap,totalRows*totalColumns);
   if((gbPtr->players&G_PLR0)==G_PLR0)
   {
      gbPtr->pid[0]=getpid();
      createMQ(msgQueue[0], 0);
   }	    
   if((gbPtr->players&G_PLR1)==G_PLR1)
   {
      gbPtr->pid[1]=getpid();
      createMQ(msgQueue[1], 1);
   }
   if((gbPtr->players&G_PLR2)==G_PLR2)
   {
      gbPtr->pid[2]=getpid();
      createMQ(msgQueue[2], 2);
   }
   if((gbPtr->players&G_PLR3)==G_PLR3)
   {
      gbPtr->pid[3]=getpid();
      createMQ(msgQueue[3], 3);
   }
   if((gbPtr->players&G_PLR4)==G_PLR4)
   {
      gbPtr->pid[4]=getpid();
      createMQ(msgQueue[4], 4);
   }
   sem_post(goldSem1);
   signalInitialize();
   val=0;
   write(pipefd, &val, sizeof(val));
   while(1)
   {
      readFunction();
   }
}

void readFunction()
{
   close(pipeDiscriptor);
   pipeDiscriptor=open("/home/pooja/Documents/611/project3/gamePipe", O_RDWR);

   char playerInfo, msg[121];
   int vecotrSize, msgSize;
   short location;
   mqd_t queueFD;
   unsigned char mapValue;
   READ(clientfd, &playerInfo, sizeof(playerInfo));
   write(pipeDiscriptor," read data\n",sizeof(" read data\n"));
   if(playerInfo&G_SOCKPLR)           //process for socket player
   { 
      if((gbPtr->players&G_PLR0)==0 && (playerInfo&G_PLR0)==G_PLR0)
      {
         gbPtr->players=gbPtr->players|G_PLR0;
         gbPtr->pid[0]=getpid();
         createMQ(msgQueue[0], 0);
      }
      else if((gbPtr->players&G_PLR0)==G_PLR0 && (playerInfo&G_PLR0)==0)
      {
         gbPtr->players=gbPtr->players&(~G_PLR0);
         gbPtr->pid[0]=0;
         deleteMQ(msgQueue[0], 0);    
      }
      if((gbPtr->players&G_PLR1)==0 && (playerInfo&G_PLR1)==G_PLR1)
      { 
         gbPtr->players=gbPtr->players|G_PLR1;
         gbPtr->pid[1]=getpid();
         createMQ(msgQueue[1], 1);
      }
      else if((gbPtr->players&G_PLR1)==G_PLR1 && (playerInfo&G_PLR1)==0)
      { 
         gbPtr->players=gbPtr->players&(~G_PLR1);
         gbPtr->pid[1]=0;
         deleteMQ(msgQueue[1], 1); 
      }
      if((gbPtr->players&G_PLR2)==0 && (playerInfo&G_PLR2)==G_PLR2)
      { 
         gbPtr->players=gbPtr->players|G_PLR2;
         gbPtr->pid[2]=getpid();
         createMQ(msgQueue[2], 2);
      }
      else if((gbPtr->players&G_PLR2)==G_PLR2 && (playerInfo&G_PLR2)==0)
      { 
         gbPtr->players=gbPtr->players&(~G_PLR2);
         gbPtr->pid[2]=0;
         deleteMQ(msgQueue[2], 2); 
      }
      if((gbPtr->players&G_PLR3)==0 && (playerInfo&G_PLR3)==G_PLR3)
      { 
         gbPtr->players=gbPtr->players|G_PLR3;
         gbPtr->pid[3]=getpid();
         createMQ(msgQueue[3], 3);
      }
      else if((gbPtr->players&G_PLR3)==G_PLR3 && (playerInfo&G_PLR3)==0)
      { 
         gbPtr->players=gbPtr->players&(~G_PLR3);
         gbPtr->pid[3]=0;
         deleteMQ(msgQueue[3], 3); 
      }
      if((gbPtr->players&G_PLR4)==0 && (playerInfo&G_PLR4)==G_PLR4)
      { 
         gbPtr->players=gbPtr->players|G_PLR4;
         gbPtr->pid[4]=getpid();
         createMQ(msgQueue[4], 4);
      }
      else if((gbPtr->players&G_PLR4)==G_PLR4 && (playerInfo&G_PLR4)==0)
      { 
         gbPtr->players=gbPtr->players&(~G_PLR4);
         gbPtr->pid[4]=0;
         deleteMQ(msgQueue[4], 4);
      } 
      if(!gbPtr->players)
      { 
         close(fdMem);
         if(shm_unlink(shmName1)==-1)
            perror("Error in unlinking shared memory");
         sem_close(goldSem1);
         if(sem_unlink(semName1)==-1)
            perror("Error in unlinking semaphore");
         exit(1);	
      }
   }   
   else if(playerInfo==0)
   {
      READ(clientfd, &vecotrSize, sizeof(vecotrSize));
      for(int i=0; i<vecotrSize; i++)
      {
         READ(clientfd, &location, sizeof(location));
         READ(clientfd, &mapValue, sizeof(mapValue));
         gbPtr->boardMap[location]=mapValue;
         localMap[location]=mapValue;
      }
      for(int i=0;i<5;i++)
      {
         if(gbPtr->pid[i]!=0 && gbPtr->pid[i]!=getpid())
         {
            kill(gbPtr->pid[i],SIGUSR1);
         }
      }
   }
   else if(playerInfo&G_SOCKMSG)
   {
      READ(clientfd, &msgSize, sizeof(msgSize));
      memset(msg, 0, 121);
      READ(clientfd, &msg, msgSize);
      if(playerInfo&G_PLR0)
      { 
         if((queueFD=mq_open(msgQueue[0].c_str(), O_WRONLY|O_NONBLOCK))!=-1)
         {
            mq_send(queueFD, (const char *)&msg, 120, 0);
            mq_close(queueFD);
         }	  
      }
      else if(playerInfo&G_PLR1)
      {
         if((queueFD=mq_open(msgQueue[1].c_str(), O_WRONLY|O_NONBLOCK))!=-1)
         {
            mq_send(queueFD, (const char *)&msg, 120, 0);
            mq_close(queueFD);
         }	  
      }
      else if(playerInfo&G_PLR2)
      {
         if((queueFD=mq_open(msgQueue[2].c_str(), O_WRONLY|O_NONBLOCK))!=-1)
         {
            mq_send(queueFD, (const char *)&msg, 120, 0);
            mq_close(queueFD);
         }	  
      }
      else if(playerInfo&G_PLR3)
      {
         if((queueFD=mq_open(msgQueue[3].c_str(), O_WRONLY|O_NONBLOCK))!=-1)
         {
            mq_send(queueFD, (const char *)&msg, 120, 0);
            mq_close(queueFD);
         }	  
      }
      else if(playerInfo&G_PLR4)
      {
         if((queueFD=mq_open(msgQueue[4].c_str(), O_WRONLY|O_NONBLOCK))!=-1)
         {
            mq_send(queueFD, (const char *)&msg, 120, 0);
            mq_close(queueFD);
         }	  
      }
   }
}

void signalInitialize()
{
   struct sigaction socketPlayer;
   socketPlayer.sa_handler=writePlayer;
   sigemptyset(&socketPlayer.sa_mask);
   sigaddset(&socketPlayer.sa_mask, SIGUSR1);
   sigaddset(&socketPlayer.sa_mask, SIGUSR2);
   socketPlayer.sa_flags=0;
   socketPlayer.sa_restorer=NULL;
   sigaction(SIGHUP, &socketPlayer, NULL); 

   struct sigaction socketMap;
   socketMap.sa_handler=writeMap;
   sigemptyset(&socketMap.sa_mask);
   sigaddset(&socketMap.sa_mask, SIGUSR2);
   socketMap.sa_flags=0;
   socketMap.sa_restorer=NULL;
   sigaction(SIGUSR1, &socketMap, NULL);

   struct sigaction socketMessage;
   socketMessage.sa_handler=writeMessage;
   sigemptyset(&socketMessage.sa_mask);
   socketMessage.sa_flags=0;
   socketMessage.sa_restorer=NULL;
   sigaction(SIGUSR2, &socketMessage, NULL);  
}

void createMQ(string mqName, int forPlayer)
{
   struct mq_attr mq_attributes;
   mq_attributes.mq_flags=0;
   mq_attributes.mq_maxmsg=10;
   mq_attributes.mq_msgsize=120;

   if((mqFileDescriptor[forPlayer]=mq_open(mqName.c_str(), O_RDONLY|O_CREAT|O_EXCL|O_NONBLOCK,
               S_IRUSR|S_IWUSR, &mq_attributes))==-1)
   {
      perror("mq_read");
   }
   else
   {
      struct sigevent mq_notification_event;
      mq_notification_event.sigev_notify=SIGEV_SIGNAL;
      mq_notification_event.sigev_signo=SIGUSR2;
      mq_notify(mqFileDescriptor[forPlayer], &mq_notification_event);
   }
}

void deleteMQ(string mqName, int forPlayer)
{
   mq_close(mqFileDescriptor[forPlayer]);
   mqFileDescriptor[forPlayer]=-1;
   mq_unlink(mqName.c_str());

}

