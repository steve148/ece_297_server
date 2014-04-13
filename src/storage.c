/**
 * @file
 * @brief This file contains the implementation of the storage server
 * interface as specified in storage.h.
 */
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "storage.h"
#include "utils.h"
#include <stdbool.h>

FILE *fclientOut;

/**
 * @brief Connects the client to the server
 *
 * @param hostname hostname of the server
 * @param port port number of the server
 */

void *storage_connect(const char *hostname, const int port)
{

    if (hostname == NULL)
    {
        errno = ERR_INVALID_PARAM;
        return NULL;
    }
    // Create a socket.
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        errno = ERR_UNKNOWN;
        return NULL;
    }

    // Get info about the server.
    struct addrinfo serveraddr, *res;
    memset(&serveraddr, 0, sizeof serveraddr); // Memory management, useful to log this event as it can potentially crash
    // the system or cause problems in the future

    char log_message[100];
    sprintf(log_message, "Connect: memset complete\n"); // Specifies it is the storage_connect function
    logger(fclientOut, log_message, LOGGING_CLIENT);

    serveraddr.ai_family = AF_UNSPEC;
    serveraddr.ai_socktype = SOCK_STREAM;
    char portstr[MAX_PORT_LEN];
    snprintf(portstr, sizeof portstr, "%d", port);
    int status = getaddrinfo(hostname, portstr, &serveraddr, &res);

    if (status != 0)
    {
        errno = ERR_UNKNOWN;
        return NULL;
    }

    // Connect to the server.
    status = connect(sock, res->ai_addr, res->ai_addrlen);
    if (status != 0)
    {
        errno = ERR_CONNECTION_FAIL;
        return NULL;
    }

    return (void *) sock;
}


/**
 * @brief Authenticate the client via the server
 *
 * @param username username of client
 * @param passwd password of client
 * @param conn represents connection to the server
 * @return returns 0 if successful / -1 if unsuccessfull
 */
int storage_auth(const char *username, const char *passwd, void *conn)
{

    if ((username == NULL) || (passwd == NULL) || (conn == NULL))
    {
        errno = ERR_INVALID_PARAM;
        return -1;
    }
    // Connection is really just a socket file descriptor.
    int sock = (int)conn;

    // Send some data.
    char buf[MAX_CMD_LEN];
    memset(buf, 0, sizeof buf); // Memory management, useful to log this event as it can potentially crash
    // the system or cause problems in the future

    char log_message[100];
    sprintf(log_message, "Authorizing: memset complete\n"); // Specifies it is the storage_auth function
    logger(fclientOut, log_message, LOGGING_CLIENT);

    char *encrypted_passwd = generate_encrypted_password(passwd, NULL);
    snprintf(buf, sizeof buf, "AUTH;%s;%s\n", username, encrypted_passwd);
    if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0)
    {
        char *if_auth = strstr(buf, "SUCCESS");
        if (if_auth)
        {
            return 0;
        }
        else
        {
            errno = ERR_AUTHENTICATION_FAILED;
            return -1;
        }
    }
    errno = ERR_CONNECTION_FAIL;
    return -1;
}

/**
 * @brief Get a record from the server
 *
 * @param *table name of table being searched
 * @param *key key of the record being search
 * @param *record struct to store the value of *key
 * @param conn represents connection to the server
 * @return returns 0 if successful / -1 if unsuccessfull
 */
int storage_get(const char *table, const char *key, struct storage_record *record, void *conn)
{
    if (!table || !key || !conn || !record)
    {
        errno = ERR_INVALID_PARAM;
        return -1;
    }

    int x = 0;
    while (table[x] != NULL)
    {
        if (!((table[x] >= 'a') && (table[x] <= 'z')))
        {
            if (!((table[x] >= '0') && (table[x] <= '9')))
            {
                if (!((table[x] >= 'A') && (table[x] <= 'Z')))
                {
                    errno = ERR_INVALID_PARAM;
                    return -1;
                }
            }
        }
        x = x + 1;
    }

    x = 0;
    while (key[x] != NULL)
    {
        if (!((key[x] >= 'a') && (key[x] <= 'z')))
        {
            if (!((key[x] >= '0') && (key[x] <= '9')))
            {
                if (!((key[x] >= 'A') && (key[x] <= 'Z')))
                {
                    errno = ERR_INVALID_PARAM;
                    return -1;
                }
            }
        }
        x = x + 1;
    }

    if (!strcmp(key, ""))
    {
        errno = ERR_INVALID_PARAM;
        return -1;
    }

    if (!strcmp(table, ""))
    {
        errno = ERR_INVALID_PARAM;
        return -1;
    }

    // Connection is really just a socket file descriptor.
    int sock = (int)conn;

    // Send some data.
    char buf[MAX_CMD_LEN], strtoktemp[MAX_CMD_LEN], value[MAX_CMD_LEN], metadata[MAX_CMD_LEN];
    memset(buf, 0, sizeof buf); // Memory management, useful to log this event as it can potentially crash
    // the system or cause problems in the future

    char log_message[100];
    sprintf(log_message, "GET: memset complete\n"); // Specifies it is the storage_get function
    logger(fclientOut, log_message, LOGGING_CLIENT);

    snprintf(buf, sizeof buf, "GET;%s;%s\n", table, key);
    if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0)
    {
        char *if_getfail1 = strstr(buf, "ERR_KEY_NOT_FOUND");
        char *if_getfail2 = strstr(buf, "ERR_TABLE_NOT_FOUND");
        char *if_authfail = strstr(buf, "ERR_NOT_AUTHENTICATED");

        if (if_getfail1)
        {
            errno = ERR_KEY_NOT_FOUND;
            return -1;
        }
        else if (if_authfail)
        {
            errno = ERR_NOT_AUTHENTICATED;
            return -1;
        }
        else if (if_getfail2)
        {
            errno = ERR_TABLE_NOT_FOUND;
            return -1;
        }
        else
        {
            strcpy(strtoktemp, buf);
            get_param(strtoktemp, value, 0, ";\0");
            strcpy(strtoktemp, buf);
            get_param(strtoktemp, metadata, 1, ";\0");
            strncpy(record->value, value, sizeof record->value);
            record->metadata[0] = atoi(metadata);
            return 0;
        }
    }
    errno = ERR_CONNECTION_FAIL;
    return -1;
}


/**
 * @brief Insert or delete a record from the server
 *
 * @param *table name of table where record is being set
 * @param *key key of the record being set
 * @param *record struct to store the value of *key
 * @param conn represents connection to the server
 * @return returns 0 if successful / -1 if unsuccessfull
 */
int storage_set(const char *table, const char *key, struct storage_record *record, void *conn)
{
    bool boolnull = false;

    if ((table == NULL) || (key == NULL) || (conn == NULL))
    {
        errno = ERR_INVALID_PARAM;
        return -1;
    }


    int x = 0;
    while (table[x] != NULL)
    {
        if (!((table[x] >= 'a') && (table[x] <= 'z')))
        {
            if (!((table[x] >= '0') && (table[x] <= '9')))
            {
                if (!((table[x] >= 'A') && (table[x] <= 'Z')))
                {
                    errno = ERR_INVALID_PARAM;
                    return -1;
                }
            }
        }
        x = x + 1;
    }

    x = 0;
    while (key[x] != NULL)
    {
        if (!((key[x] >= 'a') && (key[x] <= 'z')))
        {
            if (!((key[x] >= '0') && (key[x] <= '9')))
            {
                if (!((key[x] >= 'A') && (key[x] <= 'Z')))
                {
                    errno = ERR_INVALID_PARAM;
                    return -1;
                }
            }
        }
        x = x + 1;
    }

    if (!strcmp(key, ""))
    {
        errno = ERR_INVALID_PARAM;
        return -1;
    }

    if (!strcmp(table, ""))
    {
        errno = ERR_INVALID_PARAM;
        return -1;
    }

    // Connection is really just a socket file descriptor.
    int sock = (int)conn;

    // Send some data.
    char buf[MAX_CMD_LEN];
    memset(buf, 0, sizeof buf); // Memory management, useful to log this event as it can potentially crash
    // the system or cause problems in the future

    char log_message[100];
    sprintf(log_message, "SET: memset complete\n"); // Specifies it is the storage_set function
    logger(fclientOut, log_message, LOGGING_CLIENT);

    char *if_NULLvalue;

    if (record != NULL && record->value != NULL)
        if_NULLvalue = strstr(record->value, "NULL");

    if (record == NULL || (record->value == NULL) || (if_NULLvalue))
    {
        if (record == NULL)
            snprintf(buf, sizeof buf, "DELETE;%s;%s;%s\n", table, key, "NULL");
        else if (record->value == NULL)
            snprintf(buf, sizeof buf, "DELETE;%s;%s;%s\n", table, key, "NULL");
        else
            snprintf(buf, sizeof buf, "DELETE;%s;%s;%s\n", table, key, record->value);
        if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0)
        {
            char *if_setfail = strstr(buf, "ERR_TABLE_NOT_FOUND");
            char *if_authfail = strstr(buf, "ERR_NOT_AUTHENTICATED");
            char *if_keynotfound = strstr(buf, "ERR_KEY_NOT_FOUND");
            char *if_invalidparam = strstr(buf, "ERR_INVALID_PARAM");
            if (if_setfail)
            {
                errno = ERR_TABLE_NOT_FOUND;
                return -1;
            }
            else if (if_authfail)
            {
                errno = ERR_NOT_AUTHENTICATED;
                return -1;
            }
            else if (if_keynotfound)
            {
                errno = ERR_KEY_NOT_FOUND;
                return -1;
            }
            else if (if_invalidparam)
            {
                errno = ERR_INVALID_PARAM;
                return -1;
            }
            else
                return 0;
        }
    }

    else
    {
        snprintf(buf, sizeof buf, "SET;%s;%s;%s;%d\n", table, key, record->value, record->metadata[0]);
        if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0)
        {
            char *if_setfail = strstr(buf, "ERR_TABLE_NOT_FOUND");
            char *if_authfail = strstr(buf, "ERR_NOT_AUTHENTICATED");
            char *if_invalidparam = strstr(buf, "ERR_INVALID_PARAM");
            char *if_transaction_error = strstr(buf, "ERR_TRANSACTION_ABORT");

            if (if_setfail)
            {
                errno = ERR_TABLE_NOT_FOUND;
                return -1;
            }
            else if (if_authfail)
            {
                errno = ERR_NOT_AUTHENTICATED;
                return -1;
            }
            else if (if_invalidparam)
            {
                errno = ERR_INVALID_PARAM;
                return -1;
            }
            else if (if_transaction_error)
            {
                errno = ERR_TRANSACTION_ABORT;
                return -1;
            }
            else
            {
                return 0;
            }
        }
    }
    errno = ERR_CONNECTION_FAIL;
    return -1;
}

int storage_query(const char *table, const char *predicates, char *keys[MAX_RECORDS_PER_TABLE], const int max_keys, void *conn)
{

    if ((table == NULL) || (predicates == NULL) || (keys == NULL))
    {
        errno = ERR_INVALID_PARAM;
        return -1;
    }

    int x = 0;
    while (table[x] != NULL)
    {
        if (!((table[x] >= 'a') && (table[x] <= 'z')))
        {
            if (!((table[x] >= '0') && (table[x] <= '9')))
            {
                if (!((table[x] >= 'A') && (table[x] <= 'Z')))
                {
                    errno = ERR_INVALID_PARAM;
                    return -1;
                }
            }
        }
        x = x + 1;
    }

    if (!strcmp(table, ""))
    {
        errno = ERR_INVALID_PARAM;
        return -1;
    }

    // Connection is really just a socket file descriptor.
    int sock = (int)conn;

    // Send some data.
    char buf[MAX_CMD_LEN], strtoktemp[MAX_CMD_LEN], temp[MAX_CMD_LEN], key[MAX_CMD_LEN];
    memset(buf, 0, sizeof buf);

    snprintf(buf, sizeof buf, "QUERY;%s;%s\n", table, predicates);
    if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0)
    {

        char *if_authfail = strstr(buf, "ERR_NOT_AUTHENTICATED");
        char *if_getfail1 = strstr(buf, "ERR_KEY_NOT_FOUND");
        char *if_getfail2 = strstr(buf, "ERR_TABLE_NOT_FOUND");
        char *if_getfail3 = strstr(buf, "ERR_INVALID_PARAM");
        if (if_authfail)
        {
            errno = ERR_NOT_AUTHENTICATED;
            return -1;
        }
        else if (if_getfail1)
        {
            errno = ERR_KEY_NOT_FOUND;
            return -1;
        }
        else if (if_getfail2)
        {
            errno = ERR_TABLE_NOT_FOUND;
            return -1;
        }
        else if (if_getfail3)
        {
            errno = ERR_INVALID_PARAM;
            return -1;
        }
        int returncount;

        strcpy(strtoktemp, buf);
        get_param(strtoktemp, temp, 0, ";\0");
        strcpy(strtoktemp, buf);
        get_param(strtoktemp, key, 1, ";\0");

        int count = atoi(temp);

        if (count == -1)
        {
            return 0;
        }

        if ((count + 1) > max_keys)
        {
            returncount = max_keys;
        }
        else
        {
            returncount = count + 1;
        }

        int keynumber = 0;
        sscanf(key, "%s\n", keys[keynumber]);
        keynumber = keynumber + 1;


        while (keynumber < (count + 1))
        {
            sprintf (buf, "SUCCESS\n");
            if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0)
            {
                strcpy(strtoktemp, buf);
                get_param(strtoktemp, key, 0, ";\0");
                strcpy(strtoktemp, buf);
                get_param(strtoktemp, key, 1, ";\0");
                if (keynumber < returncount)
                {
                    sscanf(key, "%s\n", keys[keynumber]);
                }
            }
            else
            {
                errno = ERR_CONNECTION_FAIL;
                return -1;
            }
            keynumber = keynumber + 1;
        }

        sprintf (buf, "SUCCESS\n");
        if (sendall(sock, buf, strlen(buf)) == 0)
        {
            return returncount;
        }
    }
    errno = ERR_CONNECTION_FAIL;
    return -1;
}

/**
 * @brief Connects the client to the server
 *
 * @param conn represents connection to the server
 * @return 0 if successful / -1 if unsuccessful
 */
int storage_disconnect(void *conn)
{
    if (conn == NULL)
    {
        errno = ERR_INVALID_PARAM;
        return -1;
    }
    // Cleanup
    int sock = (int)conn;


    close(sock);

    return 0;
}

