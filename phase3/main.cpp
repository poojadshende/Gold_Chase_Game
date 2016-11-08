#include "goldchase.h"
#include "Map.h"
#include "daemon.h"
using namespace std;

int readFile();
int getRandomNum();
void sendRefresh(pid_t );
void getRefresh(int );
void writeQueue();
void readQueue(string );
void createQueue(string );
void readText(int );
void broadcast(int );
int exit();
void cleanup(int );
void player();
void initializeSignals();
void createDaemon(int, char *);

GameBoard *gb;
char *map;
Map *goldMine;
unsigned char currentPlayer;
int currentPosition, pidIndex;
pid_t currentPID;
string currentMQ;
mqd_t readQueue_fd;
string mqName[5];
const char* semName;
int rowCount, columnCount;
const char* shmName;
sem_t *goldSem;
int flag=0;

int main(int argc, char *argv[])
{
   semName="PDSGoldSem";
   shmName="PDSGoldMem";
   mqName[0]="/player1_mq";
   mqName[1]="/player2_mq";
   mqName[2]="/player3_mq";
   mqName[3]="/player4_mq";
   mqName[4]="/player5_mq";
   string serverIP= "";
   if(argc == 1)
   {	
      player();// call sevrver function
      if(gb->daemonID==0)	//daemon is not created
      {
         createDaemon(1, (char*)serverIP.c_str());
      }
   }
   else if(argc == 2)
   {
      goldSem=sem_open(
            semName, 
            O_RDWR); 

      if(goldSem==SEM_FAILED)
      {
         createDaemon(0, argv[1]);
         player();
      }
      else
      {
         sem_close(goldSem);
         player();
      }
   }   // call client function
   else
   {
      cout << "Error: invalid argument" << endl;
      return 0;
   }
   initializeSignals();
   goldMine=new Map((unsigned char *)gb->boardMap,gb->rows,gb->cols);  
   goldMine->postNotice("This is a notice");
   int a=0;
   while(1)
   {
      a=goldMine->getKey();
      if(a=='Q')
      {        
         exit();
         return 1;
      }
      switch(a)
      {	
         case 'h': 
            if(!(currentPosition%columnCount==0))
            {
               sem_wait(goldSem);
               if(!((gb->boardMap[currentPosition-1])&G_WALL))
               {

                  gb->boardMap[currentPosition-1]|=currentPlayer;
                  gb->boardMap[currentPosition]&=~(currentPlayer);
                  currentPosition=currentPosition-1;
                  goldMine->drawMap();
                  sendRefresh(currentPID);
                  if(gb->boardMap[currentPosition]==(gb->boardMap[currentPosition]|G_GOLD))
                  {goldMine->postNotice("Congrats..You won!");
                     flag=1;
                     gb->boardMap[currentPosition]=gb->boardMap[currentPosition]&(~G_GOLD);
                  }
                  else if(gb->boardMap[currentPosition]==(gb->boardMap[currentPosition]|G_FOOL))
                     goldMine->postNotice("Sorry..This is fool's gold!");
               }	
               sem_post(goldSem); 	
            }
            else
            {
               if(flag==1)
               {
                  broadcast(0);
                  exit();
                  return 1;
               }  
            }		
            break;

         case 'j':
            if(!((currentPosition>=columnCount*(rowCount-1)) && (currentPosition<=rowCount*columnCount)))
            {
               sem_wait(goldSem);
               if(!((gb->boardMap[currentPosition+columnCount])&G_WALL))
               {
                  gb->boardMap[currentPosition+columnCount]|=currentPlayer;
                  gb->boardMap[currentPosition]&=~(currentPlayer);
                  currentPosition=currentPosition+columnCount;
                  goldMine->drawMap();
                  sendRefresh(currentPID);
                  if(gb->boardMap[currentPosition]==(gb->boardMap[currentPosition]|G_GOLD))
                  {goldMine->postNotice("Congrats..You won!");
                     flag=1;
                     gb->boardMap[currentPosition]=gb->boardMap[currentPosition]&(~G_GOLD);
                  }		
                  else if(gb->boardMap[currentPosition]==(gb->boardMap[currentPosition]|G_FOOL))
                     goldMine->postNotice("Sorry..This is fool's gold!");
               }
               sem_post(goldSem); 
            }
            else
            {
               if(flag==1)
               {
                  broadcast(0);
                  exit();
                  return 1;
               }
            }
            break;

         case 'k':
            if(!((currentPosition>=0) && (currentPosition<columnCount)))
            {
               sem_wait(goldSem);
               if(!((gb->boardMap[currentPosition-columnCount])&G_WALL))
               {
                  gb->boardMap[currentPosition-columnCount]|=currentPlayer;
                  gb->boardMap[currentPosition]&=~(currentPlayer);
                  currentPosition=currentPosition-columnCount;
                  goldMine->drawMap();
                  sendRefresh(currentPID);
                  if(gb->boardMap[currentPosition]==(gb->boardMap[currentPosition]|G_GOLD))
                  {goldMine->postNotice("Congrats..You won!");
                     flag=1;
                     gb->boardMap[currentPosition]=gb->boardMap[currentPosition]&(~G_GOLD);

                  }
                  else if(gb->boardMap[currentPosition]==(gb->boardMap[currentPosition]|G_FOOL))
                     goldMine->postNotice("Sorry..This is fool's gold!");
               }	
               sem_post(goldSem);
            }
            else
            {
               if(flag==1)
               {
                  broadcast(0);
                  exit();
                  return 1;
               }
            }
            break;

         case 'l':
            if(!(((currentPosition+1)%columnCount)==0))
            {
               sem_wait(goldSem);
               if(!((gb->boardMap[currentPosition+1])&G_WALL))
               {
                  gb->boardMap[currentPosition+1]|=currentPlayer;
                  gb->boardMap[currentPosition]&=~(currentPlayer);
                  currentPosition=currentPosition+1;
                  goldMine->drawMap();
                  sendRefresh(currentPID);
                  if(gb->boardMap[currentPosition]==(gb->boardMap[currentPosition]|G_GOLD))
                  {
                     goldMine->postNotice("Congrats..You won!");
                     flag=1;
                     gb->boardMap[currentPosition]=gb->boardMap[currentPosition]&(~G_GOLD);
                  }
                  else if(gb->boardMap[currentPosition]==(gb->boardMap[currentPosition]|G_FOOL))
                     goldMine->postNotice("Sorry..This is fool's gold!");
               }
               sem_post(goldSem);
            }
            else
            {
               if(flag==1)
               {
                  broadcast(0);
                  exit();
                  return 1;
               }
            }
            break;

         case 'm':	
            writeQueue();
            break;

         case 'b':
            broadcast(1);
            break;

      }//switch end*/
   }//end of while loop
   return 0;
}

int readFile()
{
   int goldCount;
   string fileName= "mymap.txt";
   string line, totalLine;
   char ch;
   int totalChar;
   ifstream infile;
   infile.open(fileName.c_str());
   if(infile.is_open()==false)
   {
      cout<<"Could not open file"<<endl;
      exit(-1);
   }
   getline(infile, line);
   goldCount=atoi(line.c_str());
   while(getline(infile, line))
   {
      rowCount++;
      columnCount=line.length();
      totalLine.append(line);
   }
   infile.close();
   totalChar=rowCount*columnCount;
   map=new char[totalChar];
   char *mapptr=map;
   for(int i=0; i<totalChar; i++)
   {
      if(totalLine[i]==' ')   map[i]=0;
      else if(totalLine[i]=='*')   map[i]=G_WALL; 
      else if(totalLine[i]=='1')   map[i]=G_PLR0; 
      else if(totalLine[i]=='2')   map[i]=G_PLR1;
      else if(totalLine[i]=='G')   map[i]=G_GOLD; 
      else if(totalLine[i]=='F')   map[i]=G_FOOL;
   }
   return goldCount;
}

int getRandomNum()
{
   int randomNum;
   srand(time(0));
   int totalChars=rowCount*columnCount;
   randomNum=rand()%totalChars;
   while(gb->boardMap[randomNum])
   {
      randomNum=rand()%totalChars;
   }
   return randomNum;
}

void sendRefresh(pid_t currentPID)
{
   for(int i=0; i<5; i++)
   {
      if(currentPID==gb->pid[i])
      {}
      else if(gb->pid[i]!=0 && gb->pid[i]!=currentPID)
         kill(gb->pid[i], SIGUSR1);
   }
}

void getRefresh(int)
{
   goldMine->drawMap();
}

void writeQueue()
{
   unsigned int playerNo;
   unsigned char mask=0;
   int index;
   mqd_t writeQueue_fd;
   for(int i=0; i<5; i++)
   {
      if(gb->pid[i]!=0 && currentPID!=gb->pid[i])
      {
         switch(i)
         {
            case 0:

               mask|=G_PLR0;
               break;
            case 1:
               mask|=G_PLR1;
               break;

            case 2:

               mask|=G_PLR2;
               break;

            case 3:

               mask|=G_PLR3;
               break;

            case 4:

               mask|=G_PLR4;
               break;
         }// end switch

      }//end if
   }//end for
   playerNo=goldMine->getPlayer(mask);
   if(playerNo==G_PLR0)
      index=0;
   else if(playerNo==G_PLR1)
      index=1;
   else if(playerNo==G_PLR2)
      index=2;
   else if(playerNo==G_PLR3)
      index=3;
   else if(playerNo==G_PLR4)
      index=4;
   else 
      return;
   if((writeQueue_fd=mq_open(mqName[index].c_str(), O_WRONLY|O_NONBLOCK))==-1)
   {
      perror("mq_open");
   }
   else
   {
      if(mq_send(writeQueue_fd, goldMine->getMessage().c_str(), 120, 0)==-1)
      {
         perror("mq_send");
      }
      mq_close(writeQueue_fd);
   }		
}

void readQueue(string mqName)
{
   struct mq_attr mq_attributes;
   mq_attributes.mq_flags=0;
   mq_attributes.mq_maxmsg=10;
   mq_attributes.mq_msgsize=120;
   if((readQueue_fd=mq_open(mqName.c_str(), O_RDONLY|O_CREAT|O_EXCL|O_NONBLOCK,
               S_IRUSR|S_IWUSR, &mq_attributes))==-1)
   {
      perror("mq_read");
   }
   else
   {
      struct sigevent mq_notification_event;
      mq_notification_event.sigev_notify=SIGEV_SIGNAL;
      mq_notification_event.sigev_signo=SIGUSR2;
      mq_notify(readQueue_fd, &mq_notification_event);
   }
}

void readText(int )
{
   struct sigevent mq_notification_event;
   mq_notification_event.sigev_notify=SIGEV_SIGNAL;
   mq_notification_event.sigev_signo=SIGUSR2;
   mq_notify(readQueue_fd, &mq_notification_event);
   int err;
   char msg[121];
   memset(msg, 0, 121);//set all characters to '\0'
   while((err=mq_receive(readQueue_fd, msg, 120, NULL))!=-1)
   {
      goldMine->postNotice(msg);
   }
   //we exit while-loop when mq_receive returns -1
   //if errno==EAGAIN that is normal: there is no message waiting
   if(errno!=EAGAIN)
   {
      perror("mq_receive");
   }
}

void broadcast(int broadcastValue)
{
   string message;	
   int playerNumber;
   if(broadcastValue==1)
   {
      message=goldMine->getMessage();			
   }
   else if(broadcastValue==0)
   {
      if(currentPlayer==G_PLR0)
      {
         playerNumber=1;
         message="First player has won the game..Cheers!";
      }
      else if(currentPlayer==G_PLR1)
      {
         playerNumber=2;
         message="Second player has won the game..Cheers!";
      }
      else if(currentPlayer==G_PLR2)
      {
         playerNumber=3;
         message="Third player has won the game..Cheers!";
      }
      else if(currentPlayer==G_PLR3)
      {
         playerNumber=4;
         message="Fourth player has won the game..Cheers!";
      }
      else if(currentPlayer==G_PLR4)
      {
         playerNumber=5;
         message="Fifth player has won the game..Cheers!";
      }
   }

   for(int i=0; i<5; i++)
   {
      if(currentPID==gb->pid[i])
      {}
      else if(gb->pid[i]!=0)
      {
         mqd_t broadcast_fd;
         if((broadcast_fd=mq_open(mqName[i].c_str(), O_WRONLY|O_NONBLOCK))==-1)
         {
            perror("mq_open");
         }
         else
         {
            if(mq_send(broadcast_fd, message.c_str(), 120, 0)==-1)
            {
               perror("mq_send");
            }
         }
         mq_close(broadcast_fd);
      }		
   }
}

int exit()
{
   sem_wait(goldSem);
   gb->players=gb->players&(~currentPlayer); //delete current player
   gb->boardMap[currentPosition]=gb->boardMap[currentPosition]&(~currentPlayer);
   gb->pid[pidIndex]=0;
   currentPID=0;
   kill(gb->daemonID, SIGHUP);
   sendRefresh(currentPID);
   mq_close(readQueue_fd);
   mq_unlink(currentMQ.c_str());
   sem_post(goldSem);
   delete goldMine;
   return 1;	
}

void cleanup(int )
{
   exit();
   exit(0);
}

void player()
{
   int randomNum;
   goldSem=sem_open(
         semName, 
         O_RDWR); 

   if(goldSem==SEM_FAILED)
   {
      goldSem=sem_open(
            semName, 
            O_CREAT,
            S_IROTH| S_IWOTH| S_IRGRP| S_IWGRP| S_IRUSR| S_IWUSR,
            1
            ); 
      if(goldSem==SEM_FAILED)
      {
         perror("Error in creating semaphore");
         exit(1);
      }
      sem_wait(goldSem);
      int totalGold=readFile();
      int fdMem=shm_open(shmName, O_CREAT|O_RDWR, S_IROTH| S_IWOTH| S_IRGRP| S_IWGRP| S_IRUSR| S_IWUSR);
      ftruncate(fdMem, sizeof(GameBoard)+(rowCount*columnCount));
      gb=(GameBoard*)mmap(NULL, sizeof(GameBoard)+(rowCount*columnCount), PROT_WRITE|PROT_READ, MAP_SHARED, fdMem, 0);
      gb->rows=rowCount;
      gb->cols=columnCount;
      int totalChar=rowCount*columnCount;
      for(int i=0; i<totalChar; i++)
      {
         gb->boardMap[i]=map[i];
      }
      randomNum=getRandomNum();
      gb->boardMap[randomNum]|=G_PLR0;
      currentPosition=randomNum;
      gb->players=0|G_PLR0;
      randomNum=getRandomNum();
      gb->boardMap[randomNum]|=G_GOLD;
      for(int i=2; i<=totalGold; i++)
      {
         randomNum=getRandomNum();
         gb->boardMap[randomNum]|=G_FOOL;
      }
      currentPlayer=G_PLR0;
      gb->pid[0]=getpid();
      currentPID=gb->pid[0];
      pidIndex=0;

      gb->daemonID=0;

      currentMQ="/player1_mq";
      sendRefresh(currentPID);
      readQueue(mqName[0]);
      sem_post(goldSem);
   }
   else
   {  
      sem_wait(goldSem);
      int totalGold=readFile();
      int fdMem=shm_open(shmName, O_RDWR, S_IROTH| S_IWOTH| S_IRGRP| S_IWGRP| S_IRUSR| S_IWUSR);
      if(fdMem==-1)
         perror("Error in opening shared memory");
      gb=(GameBoard*)mmap(NULL, sizeof(GameBoard)+(rowCount*columnCount), PROT_WRITE|PROT_READ, MAP_SHARED, fdMem, 0);
      if((gb->players&G_PLR0)!=G_PLR0)
      {
         gb->players=gb->players|G_PLR0;
         currentPlayer=G_PLR0;
         gb->pid[0]=getpid();
         currentPID=gb->pid[0];
         pidIndex=0;

         currentMQ="/player1_mq";
         readQueue(mqName[0]);
      }
      else if(!(gb->players&G_PLR1))
      {
         gb->players=gb->players|G_PLR1;
         currentPlayer=G_PLR1;
         gb->pid[1]=getpid();
         currentPID=gb->pid[1];
         pidIndex=1;

         currentMQ="/player2_mq";
         readQueue(mqName[1]);
      }
      else if((gb->players&G_PLR2)!=G_PLR2)
      {
         gb->players=gb->players|G_PLR2;
         currentPlayer=G_PLR2;
         gb->pid[2]=getpid();
         currentPID=gb->pid[2];
         pidIndex=2;

         currentMQ="/player3_mq";
         readQueue(mqName[2]);
      }
      else if((gb->players&G_PLR3)!=G_PLR3)
      {
         gb->players=gb->players|G_PLR3;
         currentPlayer=G_PLR3;
         gb->pid[3]=getpid();
         currentPID=gb->pid[3];
         pidIndex=3;

         currentMQ="/player4_mq";
         readQueue(mqName[3]);
      }
      else if((gb->players&G_PLR4)!=G_PLR4)
      {
         gb->players=gb->players|G_PLR4;
         currentPlayer=G_PLR4;
         gb->pid[4]=getpid();
         currentPID=gb->pid[4];
         pidIndex=4;

         currentMQ="/player5_mq";
         readQueue(mqName[4]);
      }
      else
      {
         cout<<"Can't add next player.."<<endl;
         sem_post(goldSem);
         exit(0);
      }
      randomNum=getRandomNum();
      currentPosition=randomNum;
      gb->boardMap[randomNum]|=currentPlayer;
      sendRefresh(currentPID);
      kill(gb->daemonID, SIGHUP);
      sem_post(goldSem);
   }
}

void initializeSignals()
{
   struct sigaction refreshAction;
   refreshAction.sa_handler=getRefresh;
   sigemptyset(&refreshAction.sa_mask);
   refreshAction.sa_flags=0;
   refreshAction.sa_restorer=NULL;
   sigaction(SIGUSR1, &refreshAction, NULL);  

   struct sigaction exitAction;
   exitAction.sa_handler=cleanup;
   sigemptyset(&exitAction.sa_mask);
   exitAction.sa_flags=0;
   exitAction.sa_restorer=NULL;
   sigaction(SIGINT, &exitAction, NULL);
   sigaction(SIGHUP, &exitAction, NULL);
   sigaction(SIGTERM, &exitAction, NULL);

   struct sigaction readAction;
   readAction.sa_handler=readText; 
   sigemptyset(&readAction.sa_mask); 
   readAction.sa_flags=0;
   sigaction(SIGUSR2, &readAction, NULL); 
}

void createDaemon(int isServer, char *ipAddress)
{
   if(isServer)                    //for server
   {
      pid_t daemonPID;
      daemonPID = fork();
      if(daemonPID>0)
         return;
      daemonPID = fork();
      if(daemonPID>0)
         exit(0);

      pid_t sid;
      sid=setsid();
      if(sid==-1)
         exit(1);
      for(int i; i< sysconf(_SC_OPEN_MAX); ++i)  //The maximum number of files that a process can have open at any time
         close(i);
      open("/dev/null", O_RDWR); //fd 0
      open("/dev/null", O_RDWR); //fd 1
      open("/dev/null", O_RDWR); //fd 2
      umask(0);
      chdir("/");

      int pipeDiscriptor=open("/home/pooja/Documents/611/project3/gamePipe", O_RDWR);

      serverDaemon();
      serverSocket();
   }
   else                             //for client
   {
      pid_t PID;
      int pipefd[2];
      pipe(pipefd);
      PID = fork();
      if(PID>0)
      {
         close(pipefd[1]); //close write, parent only needs read
         int val;
         read(pipefd[0], &val, sizeof(val)); //read file descriptor
         if(val==0)
         {
            write(1, "Success!\n", sizeof("Success!\n")); // 1=stdout file descriptor value
            return;
         }
         else
         {
            write(1, "Failure!\n", sizeof("Failure!\n"));
            exit(1);
         }
      }
      PID = fork();
      if(PID>0)
         exit(0);

      pid_t sid;
      sid=setsid();
      if(sid==-1)
         exit(1);
      for(int i; i< sysconf(_SC_OPEN_MAX); ++i)  //The maximum number of files that a process can have open at any time
      {
         if(i!=pipefd[1])//close everything, except write
            close(i);
      }
      open("/dev/null", O_RDWR); //fd 0
      open("/dev/null", O_RDWR); //fd 1
      open("/dev/null", O_RDWR); //fd 2
      umask(0);
      chdir("/");
      int pipeDiscriptor=open("/home/pooja/Documents/611/project3/gamePipe", O_RDWR);
      clientSocket(pipefd[1], ipAddress);
   }
}


