#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void replaceAll(char *str, const char *oldWord, const char *newWord)
{
    char *pos, temp[100];
    int index = 0;
    int owlen;

    owlen = strlen(oldWord);

    if (!strcmp(oldWord, newWord))
    {
        return;
    }

    while ((pos = strstr(str, oldWord)) != NULL)
    {

        strcpy(temp, str);

        index = pos - str;

        str[index] = '\0';

        strcat(str, newWord);

        strcat(str, temp + index + owlen);
    }
}

int main(int argc, char **argv)
{
    FILE *fPtr, *fTemp;

    fPtr = fopen("passwd", "r");
    fTemp = fopen("passwd.tmp", "w");

    /* fopen() return NULL if unable to open file in given mode. */
    if (fPtr == NULL || fTemp == NULL)
    {
        /* Unable to open file hence exit */
        printf("\nUnable to open file.\n");
        printf("Please check whether file exists and you have read/write privilege.\n");
        exit(EXIT_SUCCESS);
    }

    /*
     * Read line from source file and write to destination
     * file after replacing given word.
     */
    char buffer[1000];
    while ((fgets(buffer, 1000, fPtr)) != NULL)
    {
        // Replace all occurrence of word from current line
        replaceAll(buffer, argv[1], argv[2]);

        // After replacing write it to temp file.
        fputs(buffer, fTemp);
    }

    /* Close all files to release resource */
    fclose(fPtr);
    fclose(fTemp);

    system("rm passwd");
    system("mv passwd.tmp passwd");

    return 0;
}