/**
 * @file
 * @brief This file implements various utility functions that are
 * can be used by the storage server and client library.
 */

#define _XOPEN_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "utils.h"
#include "parser.tab.h"

extern int yyparse();
extern FILE *yyin;

int error_occurred=0;
int server_hostcount=0;
int server_portcount=0;
int usernamecount=0;
int passwordcount=0;
int tablecount=0;
int storagepolicycount=0;
int datadirectorycount=0;
struct config_params paramslex;


int sendall(const int sock, const char *buf, const size_t len)
{
    size_t tosend = len;
    while (tosend > 0) {
        ssize_t bytes = send(sock, buf, tosend, 0);
        if (bytes <= 0)
            break; // send() was not successful, so stop.
        tosend -= (size_t) bytes;
        buf += bytes;
    };

    return tosend == 0 ? 0 : -1;
}

/**
 * In order to avoid reading more than a line from the stream,
 * this function only reads one byte at a time.  This is very
 * inefficient, and you are free to optimize it or implement your
 * own function.
 */
int recvline(const int sock, char *buf, const size_t buflen)
{
    int status = 0; // Return status.
    size_t bufleft = buflen;

    while (bufleft > 1) {
        // Read one byte from scoket.
        ssize_t bytes = recv(sock, buf, 1, 0);
        if (bytes <= 0) {
            // recv() was not successful, so stop.
            status = -1;
            break;
        } else if (*buf == '\n') {
            // Found end of line, so stop.
            *buf = 0; // Replace end of line with a null terminator.
            status = 0;
            break;
        } else {
            // Keep going.
            bufleft -= 1;
            buf += 1;
        }
    }
    *buf = 0; // add null terminator in case it's not already there.

    return status;
}


int read_config(const char *config_file, struct config_params *params)
{
	// Open file for reading.
    yyin = fopen(config_file, "r");
    yyparse();

    //If any of params is missing, give error
    if(!(server_hostcount&&server_portcount&&usernamecount&&passwordcount&&tablecount)) {
        error_occurred = 1;
    }

    if((server_hostcount>1)||(server_portcount>1)||(usernamecount>1)||(passwordcount>1)||(storagepolicycount>1)||(datadirectorycount>1)) {

    	error_occurred = 1;
        }



    strncpy(params->server_host, paramslex.server_host, sizeof params->server_host);
    params->server_port=paramslex.server_port;
    params->storage_policy=paramslex.storage_policy;
    params->tablecount=paramslex.tablecount;
    params->concurrency=paramslex.concurrency;
    strncpy(params->username, paramslex.username, sizeof params->username);
    strncpy(params->password, paramslex.password, sizeof params->password);
    strncpy(params->data_directory, paramslex.data_directory, sizeof params->data_directory);


    int x=0;
    int y=0;
    while(x<paramslex.tablecount){
    	y=0;
    	while(y<MAX_COLUMNS_PER_TABLE){
    		 strncpy(params->mycolumns[x][y], paramslex.mycolumns[x][y], sizeof params->mycolumns[x][y]);
    		 strncpy(params->column_types[x][y], paramslex.column_types[x][y], sizeof params->column_types[x][y]);
    	y=y+1;
    	}
    	strncpy(params->mytables[x], paramslex.mytables[x], sizeof params->mytables[x]);
    	params->numcolumnspertable[x]=paramslex.numcolumnspertable[x];
    	x=x+1;
    }

    if(storagepolicycount==0){
    	params->storage_policy=0;
    }


    return error_occurred ? -1 : 0;
}

void logger(FILE *file, char *message, int loggingConstant)
{
    if (loggingConstant==2) {
        fprintf(file,"%s",message);
        fflush(file);
    }
    else if (loggingConstant==1) {
        printf("%s\n",message);
    }
    else if (loggingConstant==0) {
        // Disabled
    }
    else {
        // Nothing
    }

    return;

}

char *generate_encrypted_password(const char *passwd, const char *salt)
{
    if(salt != NULL)
        return crypt(passwd, salt);
    else
        return crypt(passwd, DEFAULT_CRYPT_SALT);
}

/**
 * @brief Split a string and return one of the split strings.
 *
 * @param str string to be split
 * @param to_return place to return the specified paramter in str
 * @param param_num which index of the split str to return
 * @param delims string of all characters used to deliminate string
 * @return no return value
 */
void get_param(char str[MAX_VALUE_LEN], char to_return[MAX_VALUE_LEN], int param_num, char *delims)
{

    int i = 0;
    char *pch;
    pch = strtok (str, delims);

    while (pch != NULL)
    {
        if (param_num - i > 0)
        {
            pch = strtok (NULL, delims);
            i++;
        }
        else
        {
            break;
        }
    }
    if (pch == NULL)
    {
        // param doesn't exist, something messed up
        // don't need to catch it here, should be done in parsing
        // of command in client.c or storage.c
    }
    else
    {
        strcpy(to_return, pch);
    }
}

