
#ifndef SERVER_HTTPSERVERD_H
#define SERVER_HTTPSERVERD_H


char* get_headers(char *msg, char *contentType);

char* err404(char * msg);

char *err500(char * msg);

void* reply(void* fd);


#endif