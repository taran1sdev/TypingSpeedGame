#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

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

#define WORDLIST_SIZE 10000
#define NUM_WORDS     300

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

int readFile(char *gameText) {
    FILE *words = fopen("wordlist.txt", "r");
    if (!words) return 0;
    
    // index the file position for each word
    long offsets[WORDLIST_SIZE];
    int word_count = 0;

    offsets[word_count++] = 0;

    int ch;
    while ((ch = fgetc(words)) != EOF && word_count < WORDLIST_SIZE) {
        if (ch == '\n') {
            offsets[word_count++] = ftell(words);
        }
    } 
    
    // Randomly select 300 words from the file for the game 
    srand(time(NULL));
    int selection;

    char *ptr = gameText;
    char buf[128];
    for (int i = 0; i < NUM_WORDS; i++) {
        selection = rand() % WORDLIST_SIZE;
        fseek(words, offsets[selection], SEEK_SET); 
    
        if (fgets(buf, sizeof(buf), words)) {
            size_t len = strcspn(buf, "\r\n");
            buf[len] = '\0';

            memcpy(ptr, buf, len);
            ptr += len;               

            if (i < NUM_WORDS - 1) {
                *ptr = ' ';
                ptr++;
            }
        }
    }

    *ptr = '\0';
    
    size_t finalSize = (size_t)(ptr - gameText);
    
    fclose(words);   
    
    return finalSize;
}

void setup(char *gameText) {
    printf(ERASE HOME);
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

void start(char* gameText) {
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
    // Allocate space for 300 words
    char *gameText = malloc(WORDLIST_SIZE);
    if (!gameText) return 1;

    int textSize = readFile(gameText);    

    if (!textSize) {
        free(gameText);
        printf("File size is 0..\n");
        return 1;
    }
    
    // Reallocate buffer to correct size
    char *tmp = realloc(gameText, textSize + 1);
    if (!tmp) {
        free(gameText);
        printf("Unable to reallocate game text buffer\n");
        return 1;
    } 
    
    gameText = tmp;

    enableRawMode();
    
    setup(gameText);
    
    start(gameText); 

    free(gameText);

    return 0;
}
