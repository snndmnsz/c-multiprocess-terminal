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



/////////////// PART A:  BASH /////////////////
// In this function we tried to implement a simple Bash 
// It takes the argumenets and background variable then it creates a cild fork to 
// simulate real life bash using execv
// we put our argumets inside of execv and then the PATH 
// after child executed we implemeted a background and foreground 
// And also we handle the errors
void executeBash(char *args[],int background){
  if(!strcmp(args[0], "exit")){ // if user enters exit
        if(hasBackground==0){  // if no background process running
          return exit(0);
        }else{
          printf("A background process running CANNOT EXIT.\n");
          return;
        }
    }
    int pid = fork(); // fork child
    int status;

    if(pid<0){ // if pid less then 0 means that child is not created
      printf("ERROR, child cannot be created\n");
    }

    if (pid==0){ // pid 0 is child 
        int ChildID = getpid();
        args[argumentLenght]= NULL; // setting last argument to null 
        char *concated = getPath(args[0]);
        strcat(strcat(concated,"/"),args[0]);

        if(strcmp(args[argumentLenght-1],"&")==0){
            args[argumentLenght-1] = NULL;  // to handle background process
         }
        int execCall = execv(concated, args); // execv 
        printf("\n");
        if(execCall == -1){// if execv cannot execute, it prints an error
          printf("error in terminal,pls try again.\n");
          }
      }
    else if(pid > 0){
      if(background==1){ // if process is background then we send it to waitpid call
          currentBackgroundProcess = pid;// sending background pid to global variable 
          signal (SIGCHLD, exit_handler);
          waitpid(pid,&status, WNOHANG);// sending child to background
          // with this waitpid we didnt wait child to finish its execution
          hasBackground = 1; 
      }else if(background==0){
          printf("----------------------------------------------------\n");
          currentForegroundProcess=pid;// sending foreground pid to global variable 
          ForegroundProcessCheck = 1;
          waitpid(ChildID, &status, 0); // if it foreground we send it to normal waitpid 
          ForegroundProcessCheck = 0;
          currentForegroundProcess=0;
          printf("----------------------------------------------------\n");
      }
    }
}

// structs for the Holders
struct Holder{
  char key[20];//keys
  char value[80];//values
};


/////////////// ALLIEASES PRINTER /////////////////
// In this fucntion we printed all the alieses 
//that we created in our Bash
void printAlliases(struct Holder **storage){
  printf("\n");
  printf("All Alliases:\n");
  for (int i=0; i<storageLenght; i++){ // for loop to get all of them one by one
    printf("%s %s\n", storage[i]->key, storage[i]->value);
    // prints all the values inside allies
  }
}

/////////////// ALLIAS FINDER /////////////////
// This function for the find Alieses that we created 
// it loops over all the allieses and then returns the specific allies
struct Holder* findAllias(char* key, struct Holder **storage){
  // it takes key elemet and then all Holder array 
  for (int i=0; i<storageLenght; i++){
    if (!strcmp(storage[i]->key, key)){
      return storage[i];// when it finds it returns the value that we want
    }
  }
  return NULL; //if it cannot find it returnes to NULL
}

/////////////// ALLIAS STORE /////////////////
// this part is for storing the alias. 
//However this is not working very well due to unknown reasons
// This only 4 aliases can ve stored.
void storeAllias(char* key, char* value, struct Holder **storage){
  struct Holder *valueHolder = malloc(sizeof(struct Holder));
  
  strcpy(valueHolder->key, key);
  strcpy(valueHolder->value, value);

  storageLenght += 1;
  printf("%d\n", storageLenght);
  storage = realloc(storage, storageLenght * 8);
  //if (new_storage != 0)
  //  storage = new_storage;
  storage[storageLenght-1] = valueHolder;
}


/////////////// DELETE ALLIAS  /////////////////
// This part is for deleting the desires alias from the storage struct pointer array.
// It takes the holder allias and storage list to search inside of it 
// After it finds the target allies it deletes from the array
void deleteAllias(struct Holder* allias, struct Holder **storage){
  for (int i=0; i<argumentLenght; i++){
    if (storage[i]->key == allias->key){
      if (storageLenght == 1){
        storageLenght-=1;
        free(allias);
        allias = NULL;
        return;
      }
      storageLenght-=1;
      // When we delete an element from the array this part moves the rest of the 
      // array to fill the hole. Realloc is to shrink back the array.
      memmove(&storage[i], &storage[i+1], storageLenght * sizeof(struct Holder*));
      struct Holder **new_storage = realloc(storage, storageLenght * sizeof(struct Holder*));
      if (new_storage != 0)
        storage = new_storage;
    }
  }
}

/////////////// HELPER FUCNTION SPLITTER /////////////////
// this part is for splitting string and returning splitted string in array form.
char** splitString(char* string){
  char* str2;
  strcpy(str2, string);
  
  // counting the future arrays length.
  int i = 0;
  char *p = strtok (string, " ");
  while (p != NULL){
    p = strtok (NULL, " ");
    i++;
  }

  // creating the array form.
  int j = 0;
  char *k = strtok (str2, " ");
  char** newArr = malloc(sizeof(char[20])* i);

  while (k != NULL){
    newArr[j] = k;
    j++;
    k = strtok (NULL, " ");
  }
  return newArr;
}
 

///////////////  MAIN PART  ////////////////
int main(void){
  char inputBuffer[MAX_LINE];
  int background;
  char *args[MAX_LINE/2 + 1];
  char arguments[80];
  char key[20];
  char temp[80];
  char command[80];
  struct Holder **storage = malloc(storageLenght * sizeof(struct Holder*));

  while (1){
    background = 0;
    printf("myshell>> ");
    fflush(stdout);
    setup(inputBuffer, args, &background);

    /////////// CTRL+Z SIGNAL //////////////
    signal(SIGTSTP, ctrl_z_signalHandler);

    /////////// ALLIAS //////////////
    // This function is activated when user types alias
    // if it inputs with -l then it prints all aliases.
    // 
    if (!strcmp(args[0], "alias") && args[1] != NULL){
      if (!strcmp(args[1], "-l")){
        printAlliases(storage);
        continue;
      }
      strcpy(key, args[argumentLenght-1]);

      // this part is for eliminating alias in the begining.
      for (int i=1; i<argumentLenght-1; i++){
        strcat(temp, args[i]);
        strcat(temp, " ");
      }      

      strcpy(temp, "");
      char *result = temp+1;
      result[strlen(result)-2] = '\0';
      strcpy(command, result);

      // checks if there is an alias with the same key.
      if (findAllias(key, storage) != NULL){
        printf("There is an allias with that name!\n");
        continue;
      }
      storeAllias(key, command, storage);
    }

    ///////// UNALLIAS //////////
    // This part is activated when user input unalias.
    // And first finds the alias than deletes it from the array.
    else if (!strcmp(args[0], "unalias")){
      struct Holder* found= findAllias(args[1], storage);
      deleteAllias(found, storage);
    }

    ////////// BASH AND EXECUTION /////////
    else {

      //PART B ==> FUNCTION
      // This is for executing an allias bookmark.
      // first it finds the allias from the array and executes it using system.
      struct Holder *alliasObject = findAllias(args[0], storage);
      if (alliasObject != NULL){
        system(alliasObject->value);
        continue;
      }

      //PART C ==> BUT ITS NOT WORKING BCS OF SEGMENTATION FAULT
      // if (!strcmp(args[argumentLenght-2], ">") || !strcmp(args[argumentLenght-2], ">>") || !strcmp(args[argumentLenght-2], "<")){
      //   printf(" ==> %s\n", fullCommand);
      //   system(fullCommand);
      //   printf("sıfırlayan\n");
      //   continue;
      // }

      // PART A ==> FUNCTION
      executeBash(args,background); 
    }
  }
}
