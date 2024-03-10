#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>

#define MAX_LINES 200000
#define MAX_LEN 100
#define BUFLENGTH 16

#ifndef DEBUG
#define DEBUG 0
#endif

//struct to represent a dictionary word object (more in write-up)
typedef struct {
    char lowerword[MAX_LEN];
    char origword[MAX_LEN];
} dictword;



// Dictionary file array (size of 200,000)
dictword dict[MAX_LINES];
//Text file array (size of 100)
char txt_files[100][MAX_LEN];
int txtcount = 0;



// Struct to be used for next_line function
typedef struct {
    int fd;
    int pos;
    int len;
    char buf[BUFLENGTH];
} lines_t;

// initializes struct to be used in next_line
void fdinit(lines_t *L, int fd) {
    L->fd = fd;
    L->pos = 0;
    L->len = 0;
}



// Struct for word objects in txt files
// each word object contains its word as a character array (max length of 100 characters), 
//line and column number in the text file
typedef struct {
    char word[MAX_LEN];
    int line;
    int number;     // column number
} word;


// Returns the next_line of a file using it's struct pointer
//Used to iterate through each line of a text file and extract the characters in it
char *next_line(lines_t *L) {
    // if fd isn't valid returns NULL
    if (L->fd < 0) return NULL;
    char *line = NULL;
    int line_length = 0;
    int segment_start = L->pos;

    while (1) {
        if (L->pos == L->len) {
            if (segment_start < L->pos) {
                int segment_length = L->pos - segment_start;
                line = realloc(line, line_length + segment_length + 1);
                memcpy(line + line_length, L->buf + segment_start, segment_length);
                line_length = line_length + segment_length;
                line[line_length] = '\0';
            }
            L->len = read(L->fd, L->buf, BUFLENGTH);
            if (L->len < 1) {
                close(L->fd);
                L->fd = -1;
                return line;
            }
            L->pos = 0;
            segment_start = 0;
        }
        while (L->pos < L->len) {
            if (L->buf[L->pos] == '\n') {
                int segment_length = L->pos - segment_start;
                line = realloc(line, line_length + segment_length + 1);
                memcpy(line + line_length, L->buf + segment_start, segment_length);
                line[line_length + segment_length] = '\0';
                L->pos++;
                return line;
            }
            L->pos++;
        }
    }
    return NULL;
}


// counts all the words in a file
//Used to determine the size of our words array later in the code
int wordCounter(char *file) {
    char *line;

    int count = 0;
    int fd = open(file, O_RDONLY);
    
    lines_t filetxt;
    fdinit(&filetxt, fd);

    while ((line = next_line(&filetxt))) {
        int index = 0;
        while (line[index] != '\n' && index < strlen(line)) {
            if (index < strlen(line) && line[index] != 32) {
                count++;
                index++;
                while (index < strlen(line) && line[index] != 32) {
                    index++;
                }
            }
            index++;
        }
    }

    return count;
}


// comparison function so qsort can sort based on the lowercase words of the dictionary
int compare_dictwords(const void *a, const void *b) {
    dictword *wordA = (dictword*)a;
    dictword *wordB = (dictword*)b;

    return strcmp(wordA->lowerword, wordB->lowerword);
}


// Populates the dictionary array
// Returns the number of words in the dictionary to use in binary search iteration
int populate_dict(lines_t *L) {
    char *line;
    int n = 0;

    //while there is a non-null space of characters 
    while ((line = next_line(L))) {
        //copy the word into original word of the dictionary object
        strcpy(dict[n].origword, line);
        for (int i = 0; line[i]; i++) {
            line[i] = tolower(line[i]);
        }
        //convert to lowercase and copy into lowercase word of the dictionary objecct
        strcpy(dict[n].lowerword, line);
        n++;
        free(line);
    }

    //qsort the dictionary to ensure it is sorted in ASCII order
    qsort(dict, n, sizeof(dictword), compare_dictwords);

    return n;
}



// gets the names of all txt files from the inputted directories
void populate_txts(char *handle) {
    //open provided directory
    DIR *d = opendir(handle);

    struct dirent *dir;
    
    if (d) {
        //while there is a valid directory entry
        while ((dir = readdir(d))) {
            //get the filename from that entry
            char *currname = dir->d_name;
            char *lastfour = &currname[strlen(currname)-4];
                
            struct stat sbuf;
            char temppath[100];
            strcpy(temppath, handle);
            strcat(temppath, "/");
            //assign a path to the directory into temppath
            strcat(temppath, currname);

            int x = stat(temppath, &sbuf);
            //close entry if there is no characters in the file
            if (x < 0) {
                closedir(d);
                return;
            }

            //only add the file into the txt_files array if it ends in .txt
            if (strlen(currname) >= 4) {
                if (!strcmp(lastfour, ".txt") && S_ISREG(sbuf.st_mode)) {
                    strcpy(txt_files[txtcount], temppath);
                    txtcount++;
                }
            }
            if (((S_ISDIR(sbuf.st_mode) && (currname[0] != '.') && (strcmp(lastfour, ".txt") != 0)))) {
                populate_txts(temppath);
            }
        }
    } else {
        return;
    }
    closedir(d);
}



// Populates the struct array with words from each file
// Returns number of words in the text file to use in binary search iteration
int getwords(char *txtfile, word words[]) {
    char *line;

    int wordcount = 0;
    int currfd = open(txtfile, O_RDONLY);
    int linenum = 1;
    int col = 1;

    lines_t currtxt;
    fdinit(&currtxt, currfd);

    // while there is a non-empty line in the file
    while ((line = next_line(&currtxt))) {
        //while you remain on the same line
        while (line[col-1] != '\n' && col-1 < strlen(line)) {
            //if haven't reached the end of the line and it is not a space character
            if ((col-1 < strlen(line)) && (line[col-1] != 32)) {
                char *currword = malloc(MAX_LEN);
                memset(currword, 0, MAX_LEN);
                
                int currlen = 0;
                //assign first character of the word into currword
                currword[currlen] = line[col-1];
                //assign line and column numbers into the word object
                words[wordcount].line = linenum;
                words[wordcount].number = col;
                
                col++;
                currlen++;
                while ((col-1 < strlen(line)) && (line[col-1] != 32)) {
                    currword[currlen] = line[col-1];
                    
                    col++;
                    currlen++;
                }
                if (DEBUG) printf("%s\n", currword);
              
                if (!isalpha(currword[0])) {
                    memmove(currword, currword+1, strlen(currword));
                    words[wordcount].number++;
                }

                if (!isalpha(currword[strlen(currword) - 1])) {
                    currword[strlen(currword) - 1] = '\0';
                }
                // continue traversing until you find the entire word and then copy it into the word of the word object
                strcpy(words[wordcount].word, currword);
                free(currword);

                wordcount++;
            }
            col++;
        }
        linenum++;
        col = 1;
        free(line);
    }

    close(currfd);
    return wordcount;
}



// clears all data from words so there's no overwriting issues
void clearwords (word words[], int wordslength) {
    for (int i = 0; i < wordslength; i++) {
        words[i].line = 0;
        words[i].number = 0;
        strcpy(words[i].word, "");
    }
}

//reports error message if not found in dictionary
void report_error(char *file, int line, int column_number, const char *word) {
    //has to be an actual character input
    if (strlen(word) > 0) {
        fprintf(stderr, "%s (%d,%d): %s\n", file, line, column_number, word);
    }
}


//converts the inputted word to all caps (used to check certain capitalization cases)
int allcaps(char *word) {
    for (int i = 0; word[i]; i++) {
        if (islower(word[i])) {
            return -1;
        }
    }
    return 1;
}


//binary searches the dictionary provided a inputted word object and the number of words in the dictionary
int binarySearchDict(dictword dictionary[MAX_LINES], word list, int dictionaryCount) {
    int min = 0;
    int max = dictionaryCount - 1;
    //converts the list word to lowercase and accounts for the null terminator
    char *lowercaseword = malloc(strlen(list.word)+1);
    strcpy(lowercaseword, list.word);

    for (int i = 0; lowercaseword[i]; i++) {
        lowercaseword[i] = tolower(lowercaseword[i]);
    }

    while (min <= max) {
        //obtains the mid value and mid dictionary word in lowercase
        int mid = min + (max-min) / 2;
        char *midword = dictionary[mid].lowerword;

        // calculates the difference between the two words
        int cmp = strcmp(lowercaseword, midword);
        
        if (DEBUG == 2) {
            printf("Current word: %s, dict word: %s, lowercase word: %s, lowercase dict: %s, cmp: %d\n", list.word, dictionary[mid].origword, lowercaseword, midword, cmp);
        }

        // if the dictionary and list word are equal to each other without case sensitivity
        if (cmp == 0) {
            //shifts to the left until we reach the capital version of the dictionary word (first occurance)
            while (mid > 0 && strcmp(lowercaseword, dictionary[mid-1].lowerword) == 0) {
                mid--;
            }

            while (strcmp(lowercaseword, dictionary[mid].lowerword) == 0) {
                //Checks the list word against the original dictionary word
                char *original = dictionary[mid].origword;
                int match = 1;
               // printf("Current dictionary word: %s\n", original);
                for (int i = 0; original[i]; i++) {
                   // printf("current i: %d, %c, %c, %d\n", i, original[i], list.word[i], allcaps(list.word));
                    if (!isalpha(original[i])) {
                        continue;
                    // if the dictionary word starts with a capital and the list word starts with a lowercase, 
                    //   reject the word
                    } else if (isupper(original[i]) && islower(list.word[i])) {
                        match = 0;
                        break;
                     // if the dictionary word starts with a lowercase and the list word starts with a capital,
                     // reject the word if all the letters aren't capital nor something other than the first letter is capital
                    } else if (islower(original[i]) && isupper(list.word[i]) && i!=0 && allcaps(list.word) != 1) {
                        match = 0;
                        break;
                    } 
                }
                //accept word if it passes and free the lowercase word created earlier
                if (match == 1) {
                    free(lowercaseword);
                    return 0;
                }

                mid++;
            }
            
            free(lowercaseword);
            return -1;
        } else if (cmp < 0) {
            max = mid-1;
        } else {
            min = mid+1;
        }                         
    }
    
    free(lowercaseword);
    return -1;    
}

//splits words with hyphens into smaller chunks and writes it to an array
int splitHyphens(word input, word* words) {
    char* token;
    token = strtok(input.word, "-");
    int count = 0;  

    while (token != NULL) {
        word temp; // Create a new word structure for each token
        strcpy(temp.word, token);
        temp.line = input.line;
        temp.number = input.number;

        // Allocate memory for the word and copy the token into it
        words[count] = temp;

        token = strtok(NULL, "-");
        count++;
    }

    return count;
}

//checks if there is a hyphen in the word
//returns 1 if there is
int checkHyphen(char *word) {
    int i = 0;
    int check = 0;
    while (word[i] != '\0') {
        if (word[i] == '-') {
            check = 1;
        }
        i++;
    }
    return check;
}

//iterates through an array of word objects, binary searching each of them and checking seperately if they have a hyphen
void iterateFile(dictword dictionary[MAX_LINES], word* list, int dictionaryCount, int lengthOfFile, char* file) {

    for (int i = 0; i < lengthOfFile; i++) {

        if (checkHyphen(list[i].word) == 1) {
            word noHyphen[MAX_LEN];
            int count = splitHyphens(list[i], noHyphen);
           for (int j = 0; j < count; j++) {
               int state = binarySearchDict(dictionary, noHyphen[j], dictionaryCount);
               if (state == -1 && list[i].line != 0) {
                    report_error(file, list[i].line, list[i].number, list[i].word);
               }
           }
        } else  {
            int state = binarySearchDict(dictionary, list[i], dictionaryCount);
            if (state == -1 && list[i].line != 0) {
                report_error(file, list[i].line, list[i].number, list[i].word);
            }
   
        }
    }
}

int main(int argc, char **argv) {  
    if (argc < 2) {
      //  printf("No dictionary supplied.\n");
        return EXIT_FAILURE;
    }

    int dict_fd = open(argv[1], O_RDONLY);
    if (dict_fd < 0) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }

    lines_t dictL;
    fdinit(&dictL, dict_fd);
    //returns number of words in the dictionary
    int dictionaryCount = populate_dict(&dictL);

    
    for (int i = 2; i < argc; i++){
        char *curr = argv[i];
        if ((strlen(curr) > 1) && (curr[0] == '.')) {
            continue;
        } else if (strlen(curr) > 4) {
            char *lastfour = &curr[strlen(curr)-4];
            if (!strcmp(lastfour, ".txt")) {
                strcpy(txt_files[txtcount], curr);
                txtcount++;
            }
        } else {
            populate_txts(argv[i]);
        }
    }
     
    for (int i = 0; i < txtcount; i++) {
        int currcount = wordCounter(txt_files[i]);
        word words[currcount];
        //returns number of words in the struct array while populating it
        int wordCount = getwords(txt_files[i], words);
        
        if (DEBUG) {
            int j = 0;
            while (words[j].line != 0) {
                printf("File: %s\n", txt_files[i]);
                printf("Line Number: %d\n", words[j].line);
                printf("Column: %d\n", words[j].number);
                printf("Word: \"%s\"\n", words[j].word);
                j++;
            }
        }

        iterateFile(dict, words, dictionaryCount, wordCount, txt_files[i]);
        clearwords(words, currcount);
    }

    return EXIT_SUCCESS;
}
