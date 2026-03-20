#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

// ANSI colour codes
#define RED   "\033[1;31m"
#define BLUE  "\033[1;34m"
#define RESET "\033[0m"

// Cursor movement
#define LEFT    "\033[1D"
#define RIGHT   "\033[1C"
#define SAVE    "\033[s"
#define RESTORE "\033[u"
#define HOME    "\033[H"

// Key codes
#define BACKSPACE 127
#define ESC       27

// Erase Functions
#define ERASE "\033[2J"

struct termios original_settings;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_settings);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &original_settings);

    atexit(disableRawMode);

    struct termios raw = original_settings;

    raw.c_lflag &= ~(ECHO | ICANON);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int readFile(char **buf) {
    FILE *input = fopen("example.txt", "r");
    if (!input) return 0;

    fseek(input, 0, SEEK_END);
    long size = ftell(input);
    rewind(input);
    
    *buf = malloc(size + 1);
    if (*buf == NULL) {
        fclose(input);
        return 0;
    }

    fread(*buf, 1, size, input);
    (*buf)[size] = '\0';

    fclose(input);
    return size;
}

void setup(char *gameText) {
    printf(ERASE);
    printf(SAVE "%s" RESET RESTORE, gameText);
    printf(RESTORE);
    fflush(stdout);
}

void result(int index, int errors, int words){
    double accuracy;
    
    if (index == 0 || errors == 0) {
        accuracy = 100.0;
    } else {
        accuracy = (double)(index - errors) / index;
        accuracy *= 100;
    }

    printf(ERASE HOME);
    printf("---- Results ----\n");
    printf("WPM: %d\n", words);
    printf("Accuracy: %.2f%c\n", accuracy, '%');
    printf("Errors: %d\n", errors);
    printf("\n\n");
}

void start(char* gameText, int fileSize) {
   long index = 0; // characters
   int errors = 0; 
   int words = 0;
    
   // Create timer - each game is 1 minute
   time_t now = time(NULL);
    
   struct tm *end_time = localtime(&now);
   //end_time->tm_min += 1;
   end_time->tm_sec += 20; // for quick tests
    
   time_t end = mktime(end_time); 
    
   // game will hang indefinitely if no user input - maybe try and improve later
   // would also be nice to display an elapsed time to the player 
   while(now < end) {
      now = time(NULL);
      char input = getchar();

      if (input == ESC) break;

      if (input == BACKSPACE) {
        if (index >= 0) { 
            index--;            
            printf(LEFT "%c" RESET LEFT, gameText[index]);
            index--;
        }
      } else {
          if (input == gameText[index]) {
              printf(BLUE "%c" RESET, input);              
          } else {
              printf(RED "%c" RESET, input);
              errors++;
          }
      }
              
      fflush(stdout);
      index++;
   }
   
   // Count words
   // Probably a more efficient way to do this by tracking 
   // words during the game loop but this method is simpler for now
   for (int i = 0; i < index; i++) {       
        if (gameText[i] == ' ') words++;
   }
   
   result(index, errors, words); 
}

int main(void) {
    char *gameText = NULL;
    int fileSize;
    
    
    fileSize= readFile(&gameText);
    
    if (!fileSize) {
        free(gameText);
        printf("File size is 0..\n");
        return 1;
    }
    
    enableRawMode();
    
    setup(gameText);
    
    start(gameText, fileSize); 

    free(gameText);
}
