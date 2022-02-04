#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>
#include <dirent.h>
#include<sys/types.h>
#include<sys/stat.h>
#include <fcntl.h>  
#include <stdbool.h>


//    KEREM KOSIF     
//   SINAN DUMANSIZ   


#define MAX_LINE 80 
/////////////// GLOBAL VARIABLES ///////////////
char fullCommand[80];
int argumentLenght = 0;
int storageLenght = 0;
int hasBackground = 0;
int ChildID = 0;
char data[80];
int currentForegroundProcess;
int ForegroundProcessCheck = 0;
int currentBackgroundProcess;


////////////// INPUT OUTPUT //////////////////
void takeInput(char* fileName, int toFile){
  strcpy(data, "");
  FILE *file;
  if ((file = fopen(fileName,"r")) == NULL){printf("Error! opening file");exit(1);}
  fscanf(file,"%[^\n]", data);
  fclose(file);
}

////////////// SETUP //////////////////
void setup(char inputBuffer[], char *args[],int *background)
{
  int length, /* # of characters in the command line */
      i,      /* loop index for accessing inputBuffer array */
      start,  /* index where beginning of next command parameter is */
      ct;     /* index of where to place the next parameter into args[] */
  
  ct = 0;
  length = read(STDIN_FILENO,inputBuffer,MAX_LINE);
  start = -1;
  if (length == 0)
      exit(0);

  if ( (length < 0) && (errno != EINTR) ) {
      perror("error reading the command");
      exit(-1);
  }

  //printf(">>%s\n",inputBuffer);
  strcpy(fullCommand, inputBuffer);
  for (i=0;i<length;i++){ /* examine every character in the inputBuffer */
    switch (inputBuffer[i]){
      case ' ':
      case '\t' :               /* argument separators */
      if(start != -1){
        args[ct] = &inputBuffer[start];    /* set up pointer */
        ct++;
      }

      inputBuffer[i] = '\0'; /* add a null char; make a C string */
      start = -1;
      break;

      case '\n':                 /* should be the final char examined */
      if (start != -1){
        args[ct] = &inputBuffer[start];     
        ct++;
      }
      inputBuffer[i] = '\0';
      args[ct] = NULL; /* no more arguments to this command */
      break;

      default :             /* some other character */
        if (start == -1) start = i;
          if (inputBuffer[i] == '&'){
            *background  = 1;
            inputBuffer[i-1] = '\0';
          }
      }
    }
    args[ct] = NULL;

  for (i = 0; i <= ct; i++){
    if (i > argumentLenght){
      argumentLenght = i;
    }
  }
}


/////////////// CTRL Z HANDLER /////////////////
// In here we tried to create a signal for ctrl+z 
// When ever user presses to button it otomaticly invokes
// and then if there is current process running it kills it with SIGKILL
// If there is not a process itsend a message to user
void ctrl_z_signalHandler(int signal){
    if(currentForegroundProcess!=0){ // if there is foreground process
        kill(currentForegroundProcess, SIGKILL); // kill command
        puts("\nForeground processes terminated."); //messages
        fflush(stdout);
    }else if(currentForegroundProcess){
        printf("There is nothing to kill."); //messages
        fflush(stdout);
    }
}


/////////////// EXIT HANDLER /////////////////
// This function is for the exit command line argument 
// Whenever user calls it, it exits from the main app
void exit_handler(int sig){
    int status;
    pid_t currentlRunningProcess = waitpid(currentBackgroundProcess, &status, WNOHANG);
    if (currentlRunningProcess == 0) {
      // If a process still running 
    } else if (currentlRunningProcess == -1) {
      // Error 
    } else {
      // if process finished its execution
      hasBackground=0;
    }
}


/////////////// PATH SEARCH /////////////////
// In this function we tried to implemet a PATH searcher
// it basiclly takes the path that we have to find it goes to all the path inside of it 
// searches and then retures to full path if its founded
char* getPath (char fileName[]) {
  char *pathvariable, *paths;
  char* foundedPathsInsidePATH = (char*)malloc(100); 
  int pathCounter = 0;
  pathvariable = getenv("PATH");   // get PATH
  paths = strtok (pathvariable,":"); // seperate with :
  while (paths != NULL) {   
    DIR *d;
    struct dirent *dir;
    int pathCounter = 0;
    d = opendir(paths);
      if (d) {
        while ((dir = readdir(d)) != NULL) {
          if(strcmp(dir->d_name,fileName) == 0)  {
            pathCounter = 1;
            break;
          }
        }
        closedir(d);
      }
    if(pathCounter) {   
    	strcpy(foundedPathsInsidePATH,paths);
      break;
    }
    paths = strtok (NULL, ":");
  }
  if (strcmp(foundedPathsInsidePATH,"") == 0)    
	  return "Sorry, path is not found. PLease try again.";
  else
	  return foundedPathsInsidePATH;  // return if its found
}


/////////////// TXT WRITER /////////////////
// This function written for redirecting the output from execv However due to the complications we have
// We just used system and didn't understand where to use this function
void WriteToTxt(char* fileName, char* args[], int addType, char* path){
  int file;
  char lastThree[] = {fileName[strlen(fileName)-4], fileName[strlen(fileName)-3], fileName[strlen(fileName)-2], fileName[strlen(fileName)-1]};

  if (addType == 0){
    file = open(fileName, O_CREAT | O_TRUNC | O_RDWR);  // this part is for reseting file
  }
  else{
    file = open(fileName, O_CREAT | O_APPEND | O_RDWR); // in this part it appends to the file.
  }
  if (strcmp(lastThree, ".txt")){
    printf("Use a name that ends with .txt!\n");
    return;
  }
  if (file < 0) {
      perror("open()");
      exit(EXIT_FAILURE);
  }
  dup2(file, 1); // redirecting to the file
  execv(path, args); 
  close(file);
}


/////////////// SLICER /////////////////
// This function slices string array to peices that we want 
//It basily takes the array as a paramamter and the indexes that we want to ignore
char** sliceListArray(char *args[], int ignore){
  char** argumentArray = malloc((argumentLenght-1)*sizeof(char*));// creating new memory
  for (int i=0; i<(argumentLenght-ignore); i++){
    argumentArray[i] = args[i];// replacing all the elemets exept ignore index
  }
  return argumentArray;// returnes the new sliced array 
}
