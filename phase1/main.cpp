#include <sys/types.h>
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
#include "goldchase.h"
#include "Map.h"
#include "Screen.h"

using namespace std;

struct GameBoard {
   int rows; //4 bytes
   int cols; //4 bytes
   unsigned char players;
   pid_t pid[5];//
   unsigned char boardMap[0];
}*gb;

int rowCount, columnCount;
char *map;
int readFile();
int getRandomNum();
void sendRefresh(pid_t );
void getRefresh(int );
Map *goldMine;
unsigned char currentPlayer;
int currentPosition, pidIndex;
pid_t currentPID;
//GameBoard *gb;
int main()
{
   sem_t *goldSem;
   int flag=0;
   const char* semName="PDSGoldSem";
   const char* shmName="PDSGoldMem";
   struct sigaction refreshAction;
   refreshAction.sa_handler=getRefresh;
   sigemptyset(&refreshAction.sa_mask);
   refreshAction.sa_flags=0;
   refreshAction.sa_restorer=NULL;

   sigaction(SIGUSR1, &refreshAction, NULL);  

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
   
	sendRefresh(currentPID);
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
	 gb->pid[0]=getpid();//
	 currentPID=gb->pid[0];
	 pidIndex=0;

      }
      else if(!(gb->players&G_PLR1))
      {
         gb->players=gb->players|G_PLR1;
         currentPlayer=G_PLR1;
	 gb->pid[1]=getpid();//
	 currentPID=gb->pid[1];
	 pidIndex=1;
	
	
      }
      else if((gb->players&G_PLR2)!=G_PLR2)
      {
         gb->players=gb->players|G_PLR2;
         currentPlayer=G_PLR2;
	 gb->pid[2]=getpid();//
   	 currentPID=gb->pid[2];
	 pidIndex=2;
	
	
      }
      else if((gb->players&G_PLR3)!=G_PLR3)
      {
         gb->players=gb->players|G_PLR3;
         currentPlayer=G_PLR3;
	 gb->pid[3]=getpid();//
	 currentPID=gb->pid[3];
	 pidIndex=3;
	
	 
      }
      else if((gb->players&G_PLR4)!=G_PLR4)
      {
         gb->players=gb->players|G_PLR4;
         currentPlayer=G_PLR4;
	 gb->pid[4]=getpid();//
	 currentPID=gb->pid[4];
	 pidIndex=4;
	
      }
      else
      {
         cout<<"Can't add next player.."<<endl;
         sem_post(goldSem);
         return 0;
      }
      randomNum=getRandomNum();
      currentPosition=randomNum;
      gb->boardMap[randomNum]|=currentPlayer;
	 sendRefresh(currentPID);
      sem_post(goldSem);
   }
   goldMine=new Map((char *)gb->boardMap,gb->rows,gb->cols);
   int a=0;
   goldMine->postNotice("This is a notice");  
   while(1)
   {
      a=goldMine->getKey();
      if(a=='Q')
      {        
         sem_wait(goldSem);
         gb->players=gb->players&(~currentPlayer); //delete current player
	 gb->boardMap[currentPosition]=gb->boardMap[currentPosition]&(~currentPlayer);
         gb->pid[pidIndex]=0;
	 currentPID=0;
	 sendRefresh(currentPID);
         sem_post(goldSem);
         if(gb->players==0)
         {
            if(sem_unlink(semName)==-1)
               perror("Error in unlinking semaphore");
            if(shm_unlink(shmName)==-1)
               perror("Error in unlinking shared memory");
         }
	 delete goldMine;
         return 1;
      }
      switch(a)
      {	
         case 'h': 
            sem_wait(goldSem);
            if(!(currentPosition%columnCount==0))
            {
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
            }
            else
            {
               if(flag==1)
               {
                  
                  gb->players=gb->players&(~currentPlayer); //delete current player
                  gb->boardMap[currentPosition]=gb->boardMap[currentPosition]&(~currentPlayer);
                  sem_post(goldSem);
                  if(gb->players==0)
                  {
                     if(sem_unlink(semName)==-1)
                        perror("Error in unlinking semaphore");
                     if(shm_unlink(shmName)==-1)
                        perror("Error in unlinking shared memory");

                  }
                  return 1;
               }  
            }		
            sem_post(goldSem); 
            break;
         case 'j':
            sem_wait(goldSem);
            if(!((currentPosition>=columnCount*(rowCount-1)) && (currentPosition<=rowCount*columnCount)))
            {
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
            }
            else
            {
               if(flag==1)
               {
                 
                  gb->players=gb->players&(~currentPlayer); //delete current player
                  gb->boardMap[currentPosition]=gb->boardMap[currentPosition]&(~currentPlayer);
                  sem_post(goldSem);
                  if(gb->players==0)
                  {
                     if(sem_unlink(semName)==-1)
                        perror("Error in unlinking semaphore");
                     if(shm_unlink(shmName)==-1)
                        perror("Error in unlinking shared memory");
                  }
                  return 1;
               }
            }
            sem_post(goldSem);  
            break;
         case 'k':
            sem_wait(goldSem);
            if(!((currentPosition>=0) && (currentPosition<columnCount)))
            {
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
            }
            else
            {
               if(flag==1)
               {
               
                  gb->players=gb->players&(~currentPlayer); //delete current player
                  gb->boardMap[currentPosition]=gb->boardMap[currentPosition]&(~currentPlayer);
                  sem_post(goldSem);
                  if(gb->players==0)
                  {
                     if(sem_unlink(semName)==-1)
                        perror("Error in unlinking semaphore");
                     if(shm_unlink(shmName)==-1)
                        perror("Error in unlinking shared memory");
                  }
                  return 1;
               }
            }
            sem_post(goldSem);
            break;
         case 'l':
            sem_wait(goldSem);
            if(!(((currentPosition+1)%columnCount)==0))
            {
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
              
            }
            else
            {
               if(flag==1)
               {
                  //sem_wait(goldSem);
                  gb->players=gb->players&(~currentPlayer); //delete current player
                  gb->boardMap[currentPosition]=gb->boardMap[currentPosition]&(~currentPlayer);
                  sem_post(goldSem);
                  if(gb->players==0)
                  {
                     if(sem_unlink(semName)==-1)
                        perror("Error in unlinking semaphore");
                     if(shm_unlink(shmName)==-1)
                        perror("Error in unlinking shared memory");
                  }
                  return 1;
               }
            }
            sem_post(goldSem);
            break;
	    case 'm':
		unsigned char mask=0;
		for(int i=0; i<5; i++)
       		   {
			if(gbp->pid[i]!=0)
        		switch(i)
       			{
          			case 0:
            mask|=G_PLR0;
            break;
          case 1:
            mask|=G_PLR1;
        }
// 4. 
    unsigned char answer=your_map_variable->getPlayer(mask);


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
      else if(totalLine[i]=='*')   map[i]=G_WALL; //A wall
      else if(totalLine[i]=='1')   map[i]=G_PLR0; //The first player
      else if(totalLine[i]=='2')   map[i]=G_PLR1; //The second player
      else if(totalLine[i]=='G')   map[i]=G_GOLD; //Real gold!
      else if(totalLine[i]=='F')   map[i]=G_FOOL; //Fool's gold
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

void getRefresh(int )
{
	 goldMine->drawMap();
}

