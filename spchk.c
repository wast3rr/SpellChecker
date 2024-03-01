#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_LINES 200000
#define MAX_LEN 100
#define BUFLENGTH 16

#ifndef DEBUG
#define DEBUG 0
#endif



// Dictionary file array
char dict[MAX_LINES][MAX_LEN];
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



// Struct for words in txt files
typedef struct {
    char word[MAX_LEN];
    int line;
    int number;     // column number
} word;



// Returns the next_line of a file using it's struct pointer
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



// Populates the dictionary array
void populate_dict(lines_t *L) {
    char *line;
    int n = 0;

    while ((line = next_line(L))) {
        strcpy(dict[n], line);
        n++;
        free(line);
    }
}



// gets the names of all txt files
void populate_txts(char *handle) {
    char path[100];
    strcpy(path, handle);
    DIR *d = opendir(handle);

    struct dirent *dir;
    
    if (d) {
        while ((dir = readdir(d))) {
            char *currname = dir->d_name;
            char *lastfour = &currname[strlen(currname)-4];
                
            struct stat sbuf;
            char temppath[100];
            strcpy(temppath, path);
            strcat(temppath, "/");
            strcat(temppath, currname);

            int x = stat(temppath, &sbuf);

            if (x < 0) {
                closedir(d);
                return;
            }

            if (strlen(currname) >= 4) {
                if (!strcmp(lastfour, ".txt") && S_ISREG(sbuf.st_mode)) {
                    strcat(txt_files[txtcount], temppath);
                    txtcount++;
                }
            }
            if (((S_ISDIR(sbuf.st_mode) && (currname[0] != '.') && (strcmp(lastfour, ".txt") != 0)))) {
                strcat(path, "/");
                strcat(path, currname);
                populate_txts(path);
            }
        }
    } else {
        return;
    }
    closedir(d);
}



// Populates the struct array with words from each file
void getwords(char *txtfile, word words[]) {
    char *line;

    int wordcount = 0;
    int currfd = open(txtfile, O_RDONLY);
    int linenum = 1;
    int col = 1;

    lines_t currtxt;
    fdinit(&currtxt, currfd);

   
    while ((line = next_line(&currtxt))) {
        while (line[col-1] != '\n' && col-1 < strlen(line)) {
            if ((col-1 < strlen(line)) && ((line[col-1] >= 65 && line[col-1] <= 90) || (line[col-1] >= 97 && line[col-1] <= 122))) {
                char *currword = malloc(MAX_LEN);
                int currlen = 0;

                currword[currlen] = line[col-1];
                
                words[wordcount].line = linenum;
                words[wordcount].number = col;
                
                col++;
                currlen++;
                while ((col-1 < strlen(line)) && ((line[col-1] >= 65 && line[col-1] <= 90) || (line[col-1] >= 97 && line[col-1] <= 122) || (line[col-1] == '-') || (line[col-1] == 39))) {
                    currword[currlen] = line[col-1];
                    
                    col++;
                    currlen++;
                }
                if (DEBUG) printf("%s\n", currword);
                strcpy(words[wordcount].word, currword);
                wordcount++;
            }

            col++;
        }
        linenum++;
        col = 1;
        free(line);
    }

    close(currfd);
}



int main(int argc, char **argv) {  
    if (argc < 2) {
        printf("No dictionary supplied.\n");
        return EXIT_FAILURE;
    }

    int dict_fd = open(argv[1], O_RDONLY);
    if (dict_fd < 0) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }

    lines_t dictL;
    fdinit(&dictL, dict_fd);
    populate_dict(&dictL);

    
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
        word words[500];
        getwords(txt_files[i], words);
        
        for (int j = 0; j < 10; j++) {
            printf("File: %s\n", txt_files[i]);
            printf("Line Number: %d\n", words[j].line);
            printf("Column: %d\n", words[j].number);
            printf("Word: %s\n", words[j].word);
        }


        if (DEBUG) printf("%s\n", txt_files[i]);
    }

    return EXIT_SUCCESS;
}
