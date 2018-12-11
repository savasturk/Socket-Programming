#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
/*--------------------------------README--------------------------------*/
/* Please run my code like this. */
/* gcc 2015510052.c -o 2015510052 -lpthread -std=c99 */
/* Not necassery change directory*/
/* Could you run firefox web browser like 127.0.0.1:8080/<file name> */
/* Only jpg and html file must be in same folder or directory with 2015510052.c . It is enough. */
/* If cause of error like this "bind: Address already in use" please use "fuser -k 8080/tcp" to kill.
/*-----------------------------------------------------------------------*/

#define MYPORT 8080 // the port users will be connecting to
#define BACKLOG 10  // how many pending connections queue will hold
#define MAXDATASIZE 1000
extern int serverAcceptNumber;
int serverAcceptNumber = 0;
/*********************************************************************/
void sigchld_handler(int s)
{
    while (wait(NULL) > 0)
        ;
}
/*********************************************************************/
/*---------------------------httpRequest Method------------------------*/
void *httpRequest(void *data)
{
    int me = *((int *)data);
    char buf[MAXDATASIZE], requestLine[100], tmp[20];
    int a = 0, k = 0, b = 0, received = -1;
    int indexOfNewLine = 0, indexOfSpace = 0;
    char *indexChar;
    /*Get the Header lines from the client*/
    if ((received = recv(me, buf, 100, 0)) < 0)
    {
        perror("Failed to receive initial bytes from client");
    }
    buf[received - 1] = '\0';
    //Extract the request line.
    indexChar = strchr(buf, '\n');
    indexOfNewLine = (int)(indexChar - buf);
    strncpy(requestLine, buf, indexOfNewLine * sizeof(char));
    indexChar = strchr(requestLine, ' ');
    indexOfSpace = (int)(indexChar - requestLine);
    strncpy(tmp, requestLine, indexOfSpace * sizeof(char));

    if (strcmp(tmp, "GET") == 0)
    {

        //Extract the file name.
        char *fileName, *fl1, *fl2;
        fl1 = strpbrk(requestLine, "/");
        fl2 = strtok(fl1, " ");
        fileName = strpbrk(fl2, "/");
        // printf("%s\n", fileName);
        int len = strlen(fileName);
        char newFileName[100];
        // printf("%c\n", fileName[1]);
        // strncpy(fileName, fileName + 1, len);
        memcpy(newFileName, &fileName[1], len-1);
        // printf("%s\n", requestLine);
        // printf("%s\n", newFileName);
        if (strcmp(newFileName, "favicon.ico") != 0)
        {
            FILE *fp;
            fp = fopen(newFileName, "r");
            fseek(fp, 0, SEEK_END);
            long fsize = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            int file_exist = 1;
            if (fp == NULL)
            {
                printf("closed\n");

                file_exist = 0;
            }
            //Extract the content type of that file.
            char *filetype;
            char *type;
            int s = '.';
            filetype = strrchr(newFileName, s);

            if ((strcmp(filetype, ".htm")) == 0 || (strcmp(filetype, ".html")) == 0)
            {
                type = "text/html";
            }
            else if ((strcmp(filetype, ".jpg")) == 0)
            {
                type = "image/jpeg";
            }

            char statusLine[100] = "HTTP/1.1";
            char contentTypeLine[100] = "Content-type: ";
            char entityBody[100] = "<html>";
            char contentLength[100] = "Content-length: ";
            char contentLengthNumber[4];
            sprintf(contentLengthNumber, "%ld", fsize);
            if (file_exist == 1)
            {
                //send status and the content type line
                strcpy(statusLine, strcat(statusLine, "200 OK"));
                strcpy(statusLine, strcat(statusLine, "\r\n"));
                strcpy(contentTypeLine, strcat(contentTypeLine, type));
                strcpy(contentTypeLine, strcat(contentTypeLine, "; charset= UTF-8"));
                strcpy(contentTypeLine, strcat(contentTypeLine, "\r\n"));
                strcpy(contentLength, strcat(contentLength, contentLengthNumber));
                strcpy(contentLength, strcat(contentLength, "\r\n"));
            }
            else
            {
                strcpy(statusLine, strcat(statusLine, "404 Not Found"));
                strcpy(statusLine, strcat(statusLine, "\r\n"));
                //send a blank line to indicate the end of the header lines
                strcpy(contentTypeLine, strcat(contentTypeLine, "NONE"));
                strcpy(contentTypeLine, strcat(contentTypeLine, "\r\n"));
                //send the entity body
                strcpy(entityBody, strcat(entityBody, "<HEAD><TITLE>404 Not Found</TITLE></HEAD>"));
                strcpy(entityBody, strcat(entityBody, "<BODY>Not Found</BODY></html>"));
                strcpy(entityBody, strcat(entityBody, "\r\n"));
            }

            if (file_exist)
            {
                char read_char[fsize + 1];
                int i = 0;
                while (i < fsize)
                {
                    read_char[i] = fgetc(fp);
                    i++;
                }
                read_char[i] = '\0';
                if (serverAcceptNumber < 10)
                {

                    if (((send(me, statusLine, strlen(statusLine), 0) == -1) ||
                         (send(me, contentTypeLine, strlen(contentTypeLine), 0) == -1) || (send(me, contentLength, strlen(contentLength), 0) == -1) ||
                         (send(me, "\r\n", strlen("\r\n"), 0) == -1)) &&
                        serverAcceptNumber <= 10)
                    {
                        printf(" enter\n");
                    }
                    else if ((send(me, read_char, fsize + 1, 0) != fsize + 1) && serverAcceptNumber <= 10)
                    {
                        perror("No Failed to send bytes to client");
                    }
                    else
                    {
                        serverAcceptNumber++;
                        printf("%d\n", serverAcceptNumber);
                    }
                }
                else if (serverAcceptNumber >= 10)
                {
                    printf("Server busy now!!\n");
                    send(me, "<HEAD><TITLE>Server busy now!!</TITLE></HEAD>", 45, 0);
                    pause();
                }
            }
            else
            {
                if (send(me, entityBody, 100, 0) == -1)
                    perror("Failed to send bytes to client");
            }
            close(me);          // Close the connection.
            pthread_exit(NULL); // Existing from the threads.
        }
    }
    else
    {
        printf("Warning message\n");
    }
}
/****************************************************************************************************************/

int main(void)
{
    int sockfd, new_fd;            // listen and new connection 
    struct sockaddr_in my_addr;    // my address information
    struct sockaddr_in their_addr; // connector's address information
    int sin_size;
    int yes = 1;
    pthread_t p_thread[3000];
    int thr_id, i = 0;
    //Create the parent socket.
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }
    //Handy debugging trick that lets
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        perror("setsockopt");
        exit(1);
    }
    //Build the server's Inernet address.
    // bzero((char *) &my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;         // host byte order
    my_addr.sin_port = htons(MYPORT);     // short, network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
    memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct
                                          //Binding
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }
    //Make this socket ready to accept connection requests.
    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }
    while (1)
    {
        sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
        {
            perror("accept");
            continue;
        }

        thr_id = pthread_create(&p_thread[i++], NULL, httpRequest, (void *)&new_fd);
    }
    return 0;
}