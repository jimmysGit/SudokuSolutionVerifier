//@author: James Barrington
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
/*
This is a small program to validate sudoku solutions using multiple proccesses

*/
//create shared memory for proccesses to share
struct sharedMem{
  int buffer1[9][9];
  int buffer2[11];
  int counter;
} sharedMem;

//Locks
sem_t buffer2Lock;
sem_t logLock;
sem_t counterLock;

//CheckRow: takes in a pointer to the row and the row number.
//It will validate the row ensuring it meets the criteria to be a valid sudoku solution
int checkRow(int* inRow, int rowNumber)
{
  int isValid =1;
  int j=0;
  int inValidRow = 0;
  int testArray[10] = {0,0,0,0,0,0,0,0,0,0};

  for(j=0;j<9;j++)
  {
	 //check numbers are between 1-9
    if(inRow[j]<1 ||inRow[j]>9)
    {
      inValidRow=1;
    }
    else
    {
	 //check that only one of the numbers between 1-9 inclusive exist
      testArray[inRow[j]]++;
      if(testArray[inRow[j]]>1)
      {
        inValidRow=1;
      }
    }
  }
  //access shared memory
  if(inValidRow==1)
  {
    FILE * log;
    sem_wait(&logLock);
    log = fopen("log4.txt", "a");
    rowNumber++;
    fprintf(log,"Proccess ID %d: row %d is invalid \n", getpid(), rowNumber);
    fclose(log);
    sem_post(&logLock);
    isValid=0;

  }
  return isValid;
}
//CheckCollumn: Takes in a 2d array represeting the sudoko solution
int checkCollumn(int mem[9][9])
{
  int inValidCollumn = 0;
  int validCollumns = 9;
  int validCol[9] = {0,0,0,0,0,0,0,0,0};
  int testNum =0;
  int testArray[10] = {0,0,0,0,0,0,0,0,0,0};
  int f = 0;
  int y = 0;

  for(testNum=0;testNum < 9;testNum++)
  {
      for(f=0;f<9;f++)
      {
		//Ensure numbers are between 1-9
        if(mem[f][testNum]<1 ||mem[f][testNum]>9 )
        {
          inValidCollumn = 1;
          validCol[testNum] = 1;
        }
        else
        {
		//check that only one of the numbers between 1-9 inclusive exist
          testArray[mem[f][testNum]]++;
          if(testArray[mem[f][testNum]]>1)
          {
            inValidCollumn = 1;
            validCol[testNum] = 1;

          }
        }
      }
      for(y =0; y< 10; y++)
      {
        testArray[y] = 0;
      }
    }

    if(inValidCollumn==1)
    {
      FILE * log;
      sem_wait(&logLock);
      log = fopen("log4.txt", "a");
      fprintf(log,"Proccess ID %d: ", getpid());
      fprintf(log,"%s ", "collumn ");
      int checkCol;
      for(checkCol=0; checkCol < 9; checkCol++)
      {
        if(validCol[checkCol] ==1)
        {
          int actualCol = checkCol +1;
          fprintf(log,"%d ", actualCol);
          validCollumns--;
        }
      }
      fprintf(log,"are invalid \n");
      fclose(log);
      sem_post(&logLock);
    }
    return validCollumns;
}
//checkSub: Takes in a 2d array of integers representing the solution
int checkSub(int mem[9][9])
{
  int invalidSub =0;
  int i;
  int collums=0;
  int rows=0;
  int l;
  int m;
  int j;
  int testArray[10] = {0,0,0,0,0,0,0,0,0,0};
  int validSub[10] = {0,0,0,0,0,0,0,0,0,0};
  int numSubs =9;
  int subNo = 0;

  for(m=0;m<9;m++)
  {
    if(m%3==0)
    {
      for(i=0;i<9;i++)
      {
        if(i%3==0)
        {
          for(collums = m;collums<m +3;collums++)
          {
            for(rows = i;rows<i+3;rows++)
            {
              if(mem[rows][collums]<1 ||mem[rows][collums]>9)
              {
                validSub[subNo]=1;
                invalidSub =1;
              }
              else
              {
                testArray[mem[rows][collums]]++;
                if(testArray[mem[rows][collums]]>1)
                {
                  validSub[subNo]=1;
                  invalidSub =1;
                }
              }
            }
          }
          subNo++;
          for(j=0;j<10;j++)
          {
            testArray[j]=0;
          }
        }
      }
    }

  }

  if(invalidSub==1)
  {
    FILE * log;
    sem_wait(&logLock);
    log = fopen("log4.txt", "a");
    fprintf(log,"Thread ID %d: ", getpid());
    fprintf(log,"%s ", "sub grids ");
    for(l=0;l<9;l++)
    {
      if(validSub[l]==1)
      {
            numSubs--;
            int actualSub = l +1;
            fprintf(log,"%d ", actualSub);
      }

    }
    fprintf(log,"are invalid \n");
    fclose(log);
    sem_post(&logLock);
  }
  return numSubs;
}

//Main program
int main(int argc, char**argv)
{
  int* pid;
  int i;
  int k;
  int j;
  int m;
  int key= 6969;
  int segment_id;
  int maxWait = atoi(argv[2]);
  int pidKey = 6970;
  int pid_id;

  sem_init(&logLock,0,1);
  sem_init(&counterLock,0,1);
  sem_init(&buffer2Lock,0,1);
  //malloc pid array
  pid = (int*)malloc(sizeof(int)*11);
  if(pid ==NULL)
  {
    printf("Failed to malloc program will now exit");
    return -1;
  }
  //shared pid memory so they can all store thier pids
  pid_id = shmget(pidKey,sizeof(int[11]), IPC_CREAT|0666);
  pid = (int *) shmat(pid_id,NULL,0);

  if(pid_id < 0)
  {
    printf("shmget error\n");
    shmctl(segment_id,IPC_RMID,NULL);
    shmctl(pid_id,IPC_RMID,NULL);
    exit(-1);
  }

  struct sharedMem *mem;
  //Make the struct shared
  segment_id = shmget(key,sizeof(struct sharedMem), IPC_CREAT|0666);
  mem = (struct sharedMem *) shmat(segment_id,NULL,0);

  if(segment_id < 0)
  {
    printf("shmget error\n");
    shmctl(segment_id,IPC_RMID,NULL);
    shmctl(pid_id,IPC_RMID,NULL);
    exit(-1);
  }
  //default to 0
  for(m=0;m<11;m++)
  {
    mem->buffer2[m] =0;
  }
  
  //Ensure we have the required Command line arguments
  if (argc==3)
  {
    FILE *myFile = fopen(argv[1],"r");
    if(myFile == NULL)
    {
      perror("error in opening file!");
      shmctl(segment_id,IPC_RMID,NULL);
      shmctl(pid_id,IPC_RMID,NULL);
      return -1;
    }
    else
    {

      for(i=0; i<9;i++)
      {
        for(j=0;j<9;j++)
        {
          //printf("Got to here");
          if(fscanf(myFile, "%d", &mem->buffer1[i][j]) ==1)
          {
            //printf("%d", mem.mySudoku[i][j]);
          }
          else if(feof(myFile))
          {
            printf("Reached End of file!\n" );
            int amount = i * j;
            printf("Added %d numbers \n", amount);
          }
          else
          {
            perror("error reading file!");
            return -1;
          }
        }
      }
      fclose(myFile);
    }
  }
  else
  {
      printf("No filename or maxdelay provided! will now exit\n");
      shmctl(segment_id,IPC_RMID,NULL);
      shmctl(pid_id,IPC_RMID,NULL);
      return -1;
  }


//Start the first group of proccesses
  for(k=0;k<11;k++)
  {
    if(k <9)
    {
      //Create Group 1
      if(fork()==0)
      {
        //Store pid so it can be used later
        pid[k] = getpid();
        int valid = 0;
        //Check Row
        valid = checkRow(mem->buffer1[k],k);
        //Update buffer2
        srand(time(NULL));
        int wait = rand() % maxWait +1;
        //Sleep by maxdelay
        sleep(wait);

        sem_wait(&buffer2Lock);
        mem->buffer2[k] = valid;
        sem_post(&buffer2Lock);
        //Update counter
        sem_wait(&counterLock);
        mem->counter += valid;
        sem_post(&counterLock);

        exit(EXIT_SUCCESS);
      }
      else
      {
        //Wait for the child to finish
        wait(NULL);
      }
    }
    //Start Group 2
    else if(k==9)
    {
      if(fork()==0)
      {
        //Store pid for later use by parent
        pid[k] = getpid();
        int numberValid = 0;
        //Check collums
        numberValid = checkCollumn(mem->buffer1);
        //Update buffer 2
        srand(time(NULL));
        int wait = rand() % maxWait +1;
        //Sleep by maxdelay
        sleep(wait);

        sem_wait(&buffer2Lock);
        mem->buffer2[k] = numberValid;
        sem_post(&buffer2Lock);
        //update counter
        sem_wait(&counterLock);
        mem->counter += numberValid;
        sem_post(&counterLock);
        //Sleep random


        exit(EXIT_SUCCESS);
      }
      else
      {
        //Wait for child to finish and remove
        wait(NULL);
      }
    }
    //Start group 3
    else
    {
      if(fork()==0)
      {
        pid[k] = getpid();
        int numberSubs=0;
        //Check sub grids
        numberSubs = checkSub(mem->buffer1);
        //seed random and wait
        srand(time(NULL));
        int wait = rand() % maxWait +1;
        //Sleep by maxdelay
        sleep(wait);
        //Update buffer 2
        sem_wait(&buffer2Lock);
        mem->buffer2[k] = numberSubs;
        sem_post(&buffer2Lock);
        //update counter
        sem_wait(&counterLock);
        mem->counter += numberSubs;
        sem_post(&counterLock);
        exit(EXIT_SUCCESS);
      }
      else
      {
        //Wait for child to finish and remove
        wait(NULL);
      }
    }
  }
  //Wait for child to finish and remove
  wait(NULL);
  //
  //Print out buffer information and counter information
  int l ;
  int actual=0;
  for(l=8;l>-1;l--)
  {
    actual = l+1;
    if(mem->buffer2[l]==1)
    {
      printf("Validation result from proccess ID %d:",pid[l] );
      printf("row %d is valid\n",actual);
    }
    else
    {
      printf("Validation result from proccess ID %d:",pid[l] );
      printf("row %d is invalid\n",actual);
    }
  }

  printf("Validation result from proccess ID %d:",pid[9] );
  printf("%d of 9 collumns are valid\n", mem->buffer2[9] );
  printf("Validation result from proccess ID %d:",pid[10] );
  printf("%d of 9 subgrids are valid\n", mem->buffer2[10] );
  printf("There are %d valid sub-grids ", mem->counter);
  if(mem->counter==27)
  {
    printf(", and thus the solution is valid\n");
  }
  else
  {
    printf(", and thus the solution is invalid\n");
  }

  //Clean up memory
  shmctl(segment_id,IPC_RMID,NULL);
  sem_destroy(&buffer2Lock);
  sem_destroy(&counterLock);
  sem_destroy(&logLock);
  return 0;
}
