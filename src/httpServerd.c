#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/httpServerd.h"

//Line length
#define MAX 128

//Typical Max server http message
#define MSGMAX 8192

extern int threadnum = 0;

pthread_mutex_t countlock;

//Set up file pointers for logs
FILE *errlog, *acclog, *cfile;



//Generate Response Headers


int main (int argc, char* argv[])
{
	//Open log file, append log data if exists
	errlog = fopen("../logs/error.log", "a+");

	if (errlog == '\0')
    {
		perror("Failed to open logfile.\n");
		fclose(errlog);
	}

	//Open log file, append log data if exists
	acclog = fopen("../logs/access.log", "a+");

	if (acclog == '\0')
    {
		perror("Failed to open access logfile.\n");
		fclose(acclog);
	}

	//Open configuration file
	cfile = fopen("../conf/example.conf", "r");

	if (cfile == '\0')
    {
		fprintf(errlog, "Failed to open config file, exiting server...\n");
		fclose(cfile);
	}
	
	//Setup config file variables
	char connections[MAX];
	char rootDir[MAX];
	char indexHTML[MAX];
	char portNumber[MAX];

	//Ensure config file is read properly
	if (fgets(connections, MAX, cfile) == NULL)
    {
		fprintf(errlog, "Failed to aquire # of default connections from configuration file\n");
	}

	if (fgets(rootDir, MAX, cfile) == NULL)
    {
		fprintf(errlog, "Failed to read default directory from configuration file\n");
	}

	if (fgets(indexHTML, MAX, cfile) == NULL)
    {
		fprintf(errlog, "Failed to read default directory from configuration file\n");
	}

	if (fgets(portNumber, MAX, cfile) == NULL)
    {
		fprintf(errlog, "Failed to read default directory from configuration file\n");
    }

	int pconnections = atoi(connections);
	short pportNumber = atoi(portNumber);
	
	//Set up passive server socket to handle HTTP requests
	
	struct sockaddr_in sin; //Address endpoint
	int masterSock, slaveSocket;

	//Clear and initialize address endpoint
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(pportNumber);

	//Create socket to listen on the default port number
	//from any interface on the internet
	masterSock = socket(PF_INET, SOCK_STREAM, 6);

	if (masterSock < 0)
    {
		fprintf(errlog, "Failed to create socket!\n");
		//close open files
		fclose(cfile);
		fclose(acclog);
		fclose(errlog);
		//exit with error
		exit(1);
	}

    int optval = 1;
	setsockopt(masterSock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	//Bind the socket to the servers network card
	if (bind(masterSock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
		fprintf(errlog, "Failed to bind socket to network card!\n");
		//close open files
		fclose(cfile);
		fclose(acclog);
		fclose(errlog);
		//exit with error
		exit(1);
    }

	if (listen(masterSock, 32) < 0)
    {
		fprintf(errlog, "Failed to listen on the socket!\n");
		//close open files
		fclose(cfile);
		fclose(acclog);
		fclose(errlog);
		//exit with error
		exit(1);
    }

	//Init variables for threading
	int addlen;
	pthread_t	tid;
	pthread_attr_t	ta;
	(void) pthread_attr_init(&ta);
	//Detach threads so they dont wait for each other to end
	(void) pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);
	addlen = sizeof(sin);
	//Accept connections and spawn a thread for each request

    int portInt;

    if (sscanf(portNumber, "%d", &portInt)<1)
    {
        fprintf(errlog, "Incorrect port number\n");
        exit(1);
    }

    fprintf(stderr, "Listening on port %d \n", portInt);

	while(1)
    {
		if( threadnum > pconnections+1)
        {
			continue;
		}

	    while((((slaveSocket = accept(masterSock, (struct sockaddr*)&sin, (socklen_t*)&addlen)) != -1)) &&
                (threadnum < pconnections +1))
        {

            //Check if socket accepted a connection
            if(slaveSocket < 0)
            {
                if (errno == EINTR)
                    continue;
                fprintf(errlog, "Failed to accept incoming connection\n");
            }
            //Create thread for each request


            if (pthread_create(&tid, NULL, reply, (void*)slaveSocket) < 0)
            {
                fprintf(errlog, "Failed to create new thread!\n");
            }

            pthread_mutex_lock(&countlock);
            threadnum++;
            pthread_mutex_unlock(&countlock);
        }
    }
}

char* img_header(char *msg, int content_len, char *content_type)
{
    sprintf(msg,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n",content_type, content_len);
    return msg;
}

char* get_headers(char *msg, char *contentType)
{
    strcat(msg, "HTTP/1.0 200 OK\r\n");
    strcat(msg, "Content-Type: ");
    //MUST PASS CONTENT TYPE WITH \r\n
    strcat(msg, contentType);
    return msg;
}

//Error 404 response message
char* err404(char * msg)
{
    strcat(msg, "HTTP/1.0 404 Not Found\r\n");
    strcat(msg, "Content-Type: text/html\r\n\r\n");
    strcat(msg, "<html><head><title>Not Found</title></head><body>Sorry, the object you requested was not found.</body></html>\n\n");
    return msg;
}

//Error 500 Internal Server Error
char *err500(char * msg)
{
    strcat(msg, "HTTP/1.0 500 Server Error\r\n");
    strcat(msg, "Content-Type: text/html\r\n\r\n");
    strcat(msg, "<html><head><title>Internal Serve Error</title></head><body><h1><b>Internal Server Error: BROKEN CGI </b></h1></body></html>\n\n");
    return msg;
}

//Function to handle replys in threads
void* reply(void* fd)
{
    int fd_sock = (int)fd;
    //Buffer to hold HTTP messages for tokenization
    char buf[MSGMAX];
    //Thread safety saveptr for strtok_r
    char* saveptr;
    //filepath from request
    char* filepath;
    int incBytes;

    //Recieve message
    incBytes = (recv(fd_sock, buf, MSGMAX, 0) > 0);
    fwrite(buf, sizeof(buf), 1, acclog);

    //Check for errors
    if (incBytes == 0 || incBytes == -1)
    {
        fprintf(errlog, "Failed to recieve HTTP message, or no data was recieved from host\n");
    }

    char buf2[MSGMAX];
    strcpy(buf2, buf);
    //Tokenize message

    const char delim = '\n';
    char* request_with_file = strtok_r(buf, &delim, &saveptr);
    char* request = strtok_r(request_with_file, " ", &saveptr);

    //If GET request, handle as follows
    if ((request != NULL) && strcmp("GET", request) == 0)
    {
        //tokenize request
        char* file = strtok_r(NULL, " ", &saveptr);
        filepath = file;
        //allocate ptr for get_headers func call
        char* hdr;
        //Allocate filepath string 256 is UNIX max
        char filep[MAX * 2];
        //Get out of /src
        strcat(filep,"../site");
        //Add filepath to ..
        strcat(filep,filepath);

        fprintf(stderr, "%s %s %s %d %s","New thread spawned. Serving:", filepath,"Thread count:", threadnum, "\n");

        int is_img = 0;
        int is_gif = 0;

        if ((strstr(filepath, ".jpg") != NULL) || (strstr(filepath, ".jpeg") != NULL) ||
            (strstr(filepath, ".JPG") != NULL) || (strstr(filepath, ".JPEG") != NULL))
        {
            is_img = 1;
        }

        else if ((strstr(filepath, ".gif") != NULL) || (strstr(filepath, ".GIF") != NULL))
        {
            is_gif = 1;
        }


        if (is_gif || is_img)
        {
            //Found .jpg file extensions, prepare to serve JPG
            //Set up file pointer for img
            FILE *fp;
            //Open file and reset to see length
            fp = fopen(filep, "rb");

            if (fp == NULL)
            {
                //SEND 404
                char * err;
                char msgbuff[MSGMAX];

                err = err404(msgbuff);
                write(fd_sock,err,strlen(err)+1);
                fclose(fp);
            }

            fseek(fp, 0L, SEEK_END);
            int sz = ftell(fp);
            fclose(fp);

            //Re-open to read for sending
            fp = fopen(filep, "rb");

            //convert size of file in int to char *
            char str[MAX];
            sprintf(str,"%d", sz);

            //Generate image headers
            char msgbuff[MSGMAX];
            char *newhdr;

            if (is_img)
            {
                newhdr = img_header(msgbuff, sz, "image/jpeg");
            }

            else if (is_gif)
            {
                newhdr = img_header(msgbuff, sz, "image/gif");
            }

            write(fd_sock, newhdr, strlen(newhdr));

            //If file exists
            if(fp)
            {
                //File offset
                int counter;
                //Temp character holder for writing to the socket binary data
                int ch;
                //Wrap sock descriptor into a FILE * for easier writes
                FILE* sock = fdopen(fd_sock, "wb+");

                for (counter = 0; counter<sz; counter++)
                {
                    //Put each character into a stream to the sock
                    fputc((ch = fgetc(fp)), sock);
                }

                //Close all open descriptors and exit from the thread
                fclose(sock);
                pthread_mutex_lock(&countlock);
                threadnum--;
                pthread_mutex_unlock(&countlock);
                pthread_exit(NULL);
            }

            else
            {
                //SEND 404
                char* err;
                char msgbuff[MSGMAX];
                err = err404(msgbuff);

                write(fd_sock,err,strlen(err)+1);
            }
            fclose(fp);
        }


        if((strstr(filepath, ".cgi") != NULL) || (strstr(filepath, ".CGI") != NULL))
        {
            //Open cgi file for reading to send into socket
            FILE* fp;
            //Open file, if no exist return 404

            //Create argv array to pass to script
            char* argz[100];
            //holds path ./<script-name>
            char script[MAX*2];
            char filepc[2*MAX];
            strcpy(filepc, filep);
            const char del = '?';
            const char del2 = '&';
            char* tok;

            tok = strtok_r(filepc, &del, &saveptr);
            strcpy(filep, tok);
            fp = fopen(filep, "r");

            if (fp == NULL)
            {
                //SEND 500 exec not found
                char * err;
                char msgbuff[MSGMAX];
                err = err500(msgbuff);
                write(fd_sock,err,strlen(err)+1);
            }

            fclose(fp);
            tok = strtok_r(NULL, &del2, &saveptr);
            if (tok != NULL)
            {
                argz[1] = tok;
                int count = 2;
                while((tok = strtok_r(NULL, &del2, &saveptr)) != NULL)
                {
                    argz[count] = tok;
                    count++;
                }
            }

            //fprintf(stderr, "%s\n", tok);

            sprintf(script, "./%s",filep);
            argz[0]= script;

            //MUST HAVE NULL to indicate end of ENV vars
            //Print out env vars passed
            /*for (int i = 0; i <10; i++){
                fprintf(stderr, "%s\n", argz[i]);
                }
                */

            argz[100] = NULL;

            //Write response header
            char * hdrs = "HTTP/1.0 200 OK\n";
            write(fd_sock, hdrs, strlen(hdrs)+1);

            //Fork process to prevent execvp from closing master threat prematurely
            int pid = fork();

            //Child CGI code
            if (pid == 0)
            {
                close(1);
                dup2(fd_sock, 1);

                // Of course this is not save and only used for work demonstration purpose
                execvp(script, argz );
            }

        }

        else
        {
            FILE* fp;
            //getline variables
            char* line = NULL;
            size_t len = 0;
            ssize_t read;

            //Open file, if NULL is returned, return HTTP 404 response
            fp = fopen(filep, "r");

            if (fp == NULL)
            {
                //SEND 404 not found
                char* err;
                char msgbuff[MSGMAX];
                err = err404(msgbuff);
                write(fd_sock,err,strlen(err)+1);

                pthread_mutex_lock(&countlock);
                threadnum--;
                pthread_mutex_unlock(&countlock);
                pthread_exit(NULL);
            }

            //Setup HTML content type for non-images
            char msgbuff[MSGMAX];

            if(strstr(filepath, ".html") != NULL)
            {
                hdr = get_headers(msgbuff, "text/html\r\n\r\n");
            }

            else
            {
                if(strstr(filepath, ".css") != NULL)
                {
                    hdr = get_headers(msgbuff, "text/css\r\n\r\n");
                }

                else
                {
                    if(strstr(filepath, ".js") != NULL)
                    {
                        hdr = get_headers(msgbuff, "text/javascript\r\n\r\n");
                    }

                    else
                    {
                        pthread_mutex_lock(&countlock);
                        threadnum--;
                        pthread_mutex_unlock(&countlock);
                        pthread_exit(NULL);
                    }
                }
            }


            //Write header to socket
            write(fd_sock,hdr, strlen(hdr)+1);

            //Read through the file line by line, writing each to the socket
            while ((read = getline(&line, &len, fp)) != -1)
            {
                write(fd_sock, line, strlen(line));
            }


            if (fp)
            {
                //Close the opened file if it was opened
                fclose(fp);
            }

            //Free the memory that getline allocated with malloc
            free(line);
        }

        close(fd_sock);
    }

    // handle POST request
    if ((request != NULL) && strcmp("POST", request) == 0)
    {
        //tokenize request
        request = strtok_r(NULL, " ", &saveptr);
        filepath = request;
        //allocate ptr for get_headers func call
        char * hdr;
        //Allocate filepath string 256 is UNIX max
        char filep[MAX * 2];
        //Get out of /src
        strcat(filep,"..");
        //Add filepath to ..
        strcat(filep,filepath);

        if((strstr(filepath, ".cgi") != NULL) ||(strstr(filepath, ".CGI") != NULL))
        {
            //Open cgi file for reading to send into socket
            FILE * fp;
            //Open file, if no exist return 404
            fp = fopen(filep, "r");

            if (fp == NULL)
            {
                //SEND 500 exec not found
                char * err;
                char msgbuff[MSGMAX];
                err = err500(msgbuff);
                write(fd_sock,err,strlen(err)+1);
                pthread_exit(NULL);
            }

            fclose(fp);
            //Create argv array to pass to script
            char * argz[100];
            //holds path ./<script-name>
            char script[MAX*2];
            const char del2 = '&';
            //searches for the last occurence of new line character
            char* query;
            query = strrchr(buf2, '\n');
            query++;
            query = strtok_r(query, &del2, &saveptr);

            if (query != NULL)
            {
                argz[1] = query;
                int count = 2;
                while((query = strtok_r(NULL, &del2, &saveptr)) != NULL)
                {
                    argz[count] = query;
                    count++;
                }
            }

            sprintf(script, "./%s",filepath);
            argz[0]= script;
            //MUST HAVE NULL to indicate end of ENV vars
            int i=0;

            for (i = 0; i <10; i++)
            {
                fprintf(stderr, "%s\n", argz[i]);
            }

            argz[100] = NULL;
            //Write response header
            char * hdrs = "HTTP/1.0 200 OK\n";
            write(fd_sock, hdrs, strlen(hdrs)+1);
            //Fork process to prevent execvp from closing master threat prematurely
            int pid = fork();

            //Child CGI code
            if (pid == 0)
            {
                close(1);
                dup2(fd_sock, 1);
                execvp(script, argz );
            }

            close(fd_sock);
        }
    }

    pthread_mutex_lock(&countlock);
    threadnum--;
    pthread_mutex_unlock(&countlock);
    pthread_exit(NULL);
}
