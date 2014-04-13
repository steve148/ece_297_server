/**
 * @file
 * @brief This file implements the storage server.
 *
 * The storage server should be named "server" and should take a single
 * command line argument that refers to the configuration file.
 *
 * The storage server should be able to communicate with the client
 * library functions declared in storage.h and implemented in storage.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include "utils.h"
#include <time.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <pthread.h>

#define MAX_LISTENQUEUELEN 20   ///< The maximum number of queued connections.

// Global Variables
FILE *fserverOut;
struct server_record tables [MAX_TABLES][MAX_RECORDS_PER_TABLE];
int first_empty[MAX_TABLES];
struct config_params params;

/**
 * @brief Check if key exists in the server
 *
 * @param key_to_search key to search for in server
 * @param first_empty index of the first empty spot in keys & values
 * @param table_num the table to search the key for
 * @return returns index of record with matching key if it exists, false(-1) if it doesn't
 */
int key_exist(char key_to_search_for[MAX_KEY_LEN], int first_empty, int table_num)
{
    int i = 0;
    for (i = 0; i < first_empty; i++)
    {
        pthread_mutex_lock(&tables[table_num][i].lock);
        if (!strcmp(key_to_search_for, tables[table_num][i].key))
        {
            pthread_mutex_unlock(&tables[table_num][i].lock);
            return i;
        }
        pthread_mutex_unlock(&tables[table_num][i].lock);
    }
    return -1;
}

/**
 * @brief Get the name of the column based on name given to search for
 *
 * @param column_name column name to find the type for
 * @param num_columns number of columns for the table
 * @param table_num index of the table parsing
 * @param mycolumns names of the columns
 * @param column_types types of the columns
 * @return return string of type of the column
 */
char *get_column_type(char column_name[MAX_COLNAME_LEN], int num_columns, int table_num, char mycolumns[MAX_TABLES][MAX_COLUMNS_PER_TABLE][MAX_COLNAME_LEN], char column_types[MAX_TABLES][MAX_COLUMNS_PER_TABLE][10])
{
    int i;
    for (i = 0; i < num_columns; i ++)
    {
        pthread_mutex_lock(&params.lock);
        if (!strcmp(mycolumns[table_num][i], column_name))
        {
            if (strstr(column_types[table_num][i], "int"))
            {
                pthread_mutex_unlock(&params.lock);
                return "int";
            }
            else
            {
                pthread_mutex_unlock(&params.lock);
                return "string";
            }
        }
        pthread_mutex_unlock(&params.lock);
    }
}

/**
 * @brief Get the column name from a query predicate
 *
 * @param pred predicate to get the column out of
 * @param ret string to return the column name too
 * @return no return value
 */
void split_query_get_column(char pred[MAX_VALUE_LEN], char ret[MAX_VALUE_LEN])
{
    char strtoktemp[MAX_VALUE_LEN];
    strcpy(strtoktemp, pred);
    if (strstr(pred, ">"))
    {
        get_param(strtoktemp, ret, 0, ">\0");
    }
    else if (strstr(pred, "<"))
    {
        get_param(strtoktemp, ret, 0, "<\0");
    }
    else if (strstr(pred, "="))
    {
        get_param(strtoktemp, ret, 0, "=\0");
    }
    else
    {
        strcpy(ret, "");
    }
}

/**
 * @brief Get the comparison value from a query predicate
 *
 * @param pred predicate to get the value out of
 * @param ret string to return the column name too
 * @return no return value
 */
void split_query_get_value(char pred[MAX_VALUE_LEN], char ret[MAX_VALUE_LEN])
{
    char strtoktemp[MAX_VALUE_LEN];
    strcpy(strtoktemp, pred);
    if (strstr(pred, ">"))
    {
        get_param(strtoktemp, ret, 1, ">\0");
    }
    else if (strstr(pred, "<"))
    {
        get_param(strtoktemp, ret, 1, "<\0");
    }
    else if (strstr(pred, "="))
    {
        get_param(strtoktemp, ret, 1, "=\0");
    }
    else
    {
        strcpy(ret, "");
    }
}

/**
 * @brief Remove whitespace from beginning and end of string
 *
 * @param str string to remove leading and trailing whitespace from
 * @return trimmed version of the string
 */
char *trim(char *str)
{
    size_t len = 0;
    char *frontp = str - 1;
    char *endp = NULL;

    if ( str == NULL )
        return NULL;

    if ( str[0] == '\0' )
        return str;

    len = strlen(str);
    endp = str + len;

    /* Move the front and back pointers to address
     * the first non-whitespace characters from
     * each end.
     */
    while ( isspace(*(++frontp)) );
    while ( isspace(*(--endp)) && endp != frontp );

    if ( str + len - 1 != endp )
        *(endp + 1) = '\0';
    else if ( frontp != str &&  endp == frontp )
        *str = '\0';

    /* Shift the string so that it starts at str so
     * that if it's dynamically allocated, we can
     * still free it on the returned pointer.  Note
     * the reuse of endp to mean the front of the
     * string buffer now.
     */
    endp = str;
    if ( frontp != str )
    {
        while ( *frontp ) *endp++ = *frontp++;
        *endp = '\0';
    }


    return str;
}

/**
 * @brief Check if table exists in the server
 *
 * @param table_name table name to search for
 * @return returns table index if it exists, false(0) if it doesn't
 */
int has_table(char table_name[MAX_TABLE_LEN])
{
    int i;
    pthread_mutex_lock(&params.lock);
    for (i = 0; i < params.tablecount; i++)
    {
        if (!strcmp(table_name, params.mytables[i]))
        {
            pthread_mutex_unlock(&params.lock);
            return i;
        }
    }
    pthread_mutex_unlock(&params.lock);
    return -1;
}

/**
 * @brief Check if table exists in the server
 *
 * @param col_to_search column name to search for
 * @param mycolumns names of all the columns of all the tables
 * @param num_columns number of columns for the table
 * @param table_num index of the table that is being parsed
 * @return returns column index if it exists, -1 if it doesn't
 */
int has_column(char col_to_search[MAX_COLNAME_LEN], char mycolumns[MAX_TABLES][MAX_COLUMNS_PER_TABLE][MAX_COLNAME_LEN], int num_columns, int table_num)
{
    int i;
    for (i = 0; i < num_columns; i++ )
    {
        pthread_mutex_lock(&params.lock);
        if (!strcmp(col_to_search, mycolumns[table_num][i]))
        {
            pthread_mutex_unlock(&params.lock);
            return i;
        }
        pthread_mutex_unlock(&params.lock);
    }
    return -1;
}

/**
 * @brief Check if the column types match the values given
 *
 * @param val_to_set value to parse
 * @param num_columns number of columns for the table
 * @param table_num index of the table parsing
 * @param column_types types of the columns
 * @return returns true(1) if all column types match, false(0) if it doesn't
 */
int check_column_types(char val_to_set[MAX_VALUE_LEN], int num_columns, int table_num, char column_types[MAX_TABLES][MAX_COLUMNS_PER_TABLE][10])
{
    int i, j, len_str, str_modifier;
    char val[MAX_VALUE_LEN], strtoktemp[MAX_VALUE_LEN], temp[MAX_VALUE_LEN];

    for (i = 0; i < num_columns; i++)
    {

        strcpy(strtoktemp, val_to_set);
        get_param(strtoktemp, val, i, ",\0");

        strcpy(strtoktemp, val);
        get_param(strtoktemp, val, 1, " \0");

        len_str = strlen(val);

        if (val[0] == '-' || val[0] == '+')
            len_str--;

        pthread_mutex_lock(&params.lock);
        if (strstr(column_types[table_num][i], "int"))
        {
            pthread_mutex_unlock(&params.lock);
            strcpy(temp, val);

            str_modifier = 0;

            for (j = 0; temp[j] != NULL; j++)
            {
                if (temp[j] != '0')
                {
                    break;
                }
                str_modifier++;
            }

            if (len_str != j)
            {
                // val is not just all zeroes
                // remove the number of leading zeroes from len_str
                len_str = len_str - str_modifier;

            }

            if (atoi(val) == 0 || len_str != (floor(log10(abs(atoi(val)))) + 1))
            {
                // val is an integer but has alpha characters in it
                if ((strcmp(val, "0")))
                {
                    return -1;
                }

            }
        }
        else
        {
            pthread_mutex_unlock(&params.lock);
            // column is of type string
            char temp[MAX_VALUE_LEN];
            // Get the max size of the records in the string column
            int start_recording = 0;
            int index = 0, j;
            pthread_mutex_lock(&params.lock);
            for (j = 0; column_types[table_num][i][j] != '\0'; j++ )
            {
                if (column_types[table_num][i][j] == '[')
                {
                    start_recording = 1;
                }
                else if (column_types[table_num][i][j] == ']')
                {
                    start_recording = 0;
                }
                else if (start_recording)
                {
                    temp[index] = column_types[table_num][i][j];
                    index++;
                }
            }
            pthread_mutex_unlock(&params.lock);
            temp[index] = '\0';
            int col_size = atoi(temp);
            if (col_size < len_str)
            {
                return -1;
            }
        }
    }
    return 1;
}

/**
 * @brief Check if the value has the correct matching column names
 *
 * @param val_to_set value to parse
 * @param num_columns number of columns for the table
 * @param table_num index of the table parsing
 * @param mycolumns names of the columns
 * @return returns true(1) if all column names match, false(-1) if it doesn't
 */
int check_mycolumns(char val_to_set[MAX_VALUE_LEN], int num_columns, int table_num, char mycolumns[MAX_TABLES][MAX_COLUMNS_PER_TABLE][MAX_COLNAME_LEN])
{
    int i;
    char val[MAX_VALUE_LEN], val_temp[MAX_VALUE_LEN], strtoktemp[MAX_VALUE_LEN];

    for (i = 0; i < num_columns; i++)
    {
        strcpy(strtoktemp, val_to_set);
        get_param(strtoktemp, val_temp, i, ",\0");

        strcpy(strtoktemp, val_temp);
        get_param(strtoktemp, val, 0, " \0");

        pthread_mutex_lock(&params.lock);
        if (strcmp(val, mycolumns[table_num][i]))
        {
            pthread_mutex_unlock(&params.lock);
            return -1;
        }
        pthread_mutex_unlock(&params.lock);
    }
    return 1;
}

/**
 * @brief Check if the value has the correct number of columns
 *
 * @param val_to_set value to parse
 * @param num_columns number of columns for the table
 * @return returns true(1) if right number of columns, false(-1) if it doesn't
 */
int num_col_val(char val_to_set[MAX_VALUE_LEN], int num_columns)
{
    int i;
    char val[MAX_VALUE_LEN], old_val[MAX_VALUE_LEN], strtoktemp[MAX_VALUE_LEN];
    strcpy(old_val, "");

    for (i = 0; i < num_columns; i++)
    {
        strcpy(strtoktemp, val_to_set);
        get_param(strtoktemp, val, i, ",\0");
        if (!strcmp(val, old_val))
        {
            //too few columns in val_to_set
            return -1;
        }
        strcpy(old_val, val);
    }
    strcpy(strtoktemp, val_to_set);
    get_param(strtoktemp, val, i, ",\0");

    if (!strcmp(val, old_val))
    {
        // correct number of columns in val_to_set
        return 1;
    }
    // too many columns in val_to_set
    return -1;
}

/**
 * @brief Checks if the value passes all parsing
 *
 * @param val_to_set value to parse
 * @param table_num table index for the parsing
 * @return returns true(1) if matches all parsing, false(-1) if it doesn't
 */
int parse_value(char val_to_set[MAX_VALUE_LEN], int table_num)
{
    if (num_col_val(val_to_set, params.numcolumnspertable[table_num]) == -1)
    {
        return -1;
    }
    else if (check_mycolumns(val_to_set, params.numcolumnspertable[table_num], table_num, params.mycolumns) == -1 )
    {
        return -1;
    }
    else if (check_column_types(val_to_set, params.numcolumnspertable[table_num], table_num, params.column_types) == -1)
    {
        return -1;
    }
    return 1;
}

/**
 * @brief Check if all the predicates in the string are formatted correctly
 *
 * @param predicates predicates to parse
 * @param num_columns number of columns for the table
 * @param table_num index of the table parsing
 * @param mycolumns names of the columns
 * @param column_types types of the columns
 * @param num_pred number of predicates to parse
 * @return returns true(1) if parses correctly, false(-1) if it doesn't
 */
int check_predicates(char predicates[MAX_VALUE_LEN], int num_columns, int table_num, char mycolumns[MAX_TABLES][MAX_COLUMNS_PER_TABLE][MAX_COLNAME_LEN], char column_types[MAX_TABLES][MAX_COLUMNS_PER_TABLE][10], int num_pred)
{
    int i, len_str;
    char strtoktemp[MAX_VALUE_LEN], pred[MAX_VALUE_LEN], old_pred[MAX_VALUE_LEN], operator[MAX_VALUE_LEN], comparator[MAX_VALUE_LEN], column_name[MAX_COLNAME_LEN], temp[MAX_COLNAME_LEN];
    strcpy(old_pred, "");
    for (i = 0; i < num_pred; i++)
    {
        strcpy(strtoktemp, predicates);
        get_param(strtoktemp, pred, i, ",\0");

        split_query_get_column(pred, column_name);
        strcpy(column_name, trim(column_name));
        strcpy(temp, get_column_type(column_name, num_columns, table_num, mycolumns, column_types));

        if (!strcmp(temp, "int"))
        {
            if (!(strstr(pred, "=") || strstr(pred, ">") || strstr(pred, "<")))
            {
                // Uses an illegal operator
                return -1;
            }
            // check that the comparing value is a int
            split_query_get_value(pred, comparator);
            strcpy(comparator, trim(comparator));
            len_str = strlen(comparator);
            if (comparator[0] == '-' || comparator[0] == '+')
                len_str--;

            if (atoi(comparator) == 0 || len_str != (floor(log10(abs(atoi(comparator)))) + 1))
            {
                if (atoi(comparator) >= 0 && len_str != (floor(log10(abs(atoi(comparator)))) + 2))
                {
                    if (strcmp(comparator, "0"))
                        return -1;
                }
            }
        }
        else
        {
            if (!strstr(pred, "="))
            {
                // Uses an illegal operator
                return -1;
            }
        }
        strcpy(old_pred, pred);
    }
    return 1;
}

/**
 * @brief Check if the predicate has the correct matching column names
 *
 * @param pred_to_set predicate to parse
 * @param num_columns number of columns for the table
 * @param table_num index of the table parsing
 * @param mycolumns names of the columns
 * @return returns true(1) if all column names match, false(-1) if it doesn't
 */
int check_mycolumns_pred(char pred_to_set[MAX_VALUE_LEN], int num_columns, int table_num, char mycolumns[MAX_TABLES][MAX_COLUMNS_PER_TABLE][MAX_COLNAME_LEN], int num_pred)
{
    int i, col_index;
    char pred[MAX_VALUE_LEN], pred_temp[MAX_VALUE_LEN], strtoktemp[MAX_VALUE_LEN];
    int column_has_pred[num_columns];

    // Use array to keep track of which columns have a predicate assigned already
    for (i = 0; i < num_columns; i++)
    {
        column_has_pred[i] = 0;
    }

    for (i = 0; i < num_pred; i++)
    {
        strcpy(strtoktemp, pred_to_set);
        get_param(strtoktemp, pred_temp, i, ",\0");

        split_query_get_column(pred_temp, pred);
        strcpy(pred, trim(pred));

        col_index = has_column(pred, mycolumns, num_columns, table_num);

        if (col_index == -1)
        {
            //column name doesn't exist
            return -1;
        }
        if (column_has_pred[col_index] == 1)
        {
            // column name has already been used
            return -1;
        }
        column_has_pred[col_index] = 1;
    }
    return 1;
}

/**
 * @brief Check if the predicate string has the correct number of predicates
 *
 * @param predicates predicates to parse
 * @param num_columns number of columns for the table
 * @return returns number of predicates if right number of predicates, false(-1) if there are too many
 */
int num_of_predicates(char predicates[MAX_VALUE_LEN], int num_columns)
{
    int i;
    char pred[MAX_VALUE_LEN], old_pred[MAX_VALUE_LEN], strtoktemp[MAX_VALUE_LEN];
    strcpy(old_pred, "");

    for (i = 0; i < num_columns; i++)
    {
        strcpy(strtoktemp, predicates);
        get_param(strtoktemp, pred, i, ",\0");

        if (!strcmp(pred, old_pred))
        {
            // num of predicates is less than num_columns
            return i;
        }
        strcpy(old_pred, pred);
    }
    strcpy(strtoktemp, predicates);
    get_param(strtoktemp, pred, i, ",\0");
    if (!strcmp(pred, old_pred))
    {
        // num of predicates is equal to num_columns
        return num_columns;
    }
    // too many columns in val_to_set
    return -1;
}

/**
 * @brief Umbrella function for all predicate parsing
 *
 * @param predicates predicates to parse
 * @param table_num index of the table parsing
 * @param params config parameters from config file
 * @return returns true(1) if matches all parsing, false(0) if it doesn't
 */
int parse_predicates(char predicates[MAX_VALUE_LEN], int table_num)
{
    int i = num_of_predicates(predicates, params.numcolumnspertable[table_num]);
    if (i == -1)
    {
        return -1;
    }
    if (check_mycolumns_pred(predicates, params.numcolumnspertable[table_num], table_num, params.mycolumns, i) == -1 )
    {
        return -1;
    }
    if (check_predicates(predicates, params.numcolumnspertable[table_num], table_num, params.mycolumns, params.column_types, i) == -1 )
    {
        return -1;
    }
    return i;
}

/**
 * @brief Get the specified value based on the key_to_get
 *
 * @param key_to_get key to serach for in keys
 * @param value_to_get value to get
 * @param first_empty index of the first empty spot in keys & values
 * @param table_num which table the get is being preformed on
 * @return returns the value of the search (ERR_KEY_NOT_FOUND if it DNE)
 */
void get_command(char key_to_get[MAX_KEY_LEN], char value_to_get[MAX_VALUE_LEN], int first_empty, int table_num)
{
    int index = 0, start = 0, i;
    char val_to_ret[MAX_VALUE_LEN], strtoktemp[MAX_CMD_LEN];
    strcpy(val_to_ret, "");

    if (first_empty == 0)
    {
        strcpy(value_to_get, "ERR_KEY_NOT_FOUND");
        return;
    }
    while (index < first_empty)
    {
        pthread_mutex_lock(&tables[table_num][index].lock);
        if (!strcmp(tables[table_num][index].key, key_to_get))
        {
            pthread_mutex_unlock(&tables[table_num][index].lock);
            break;
        }
        pthread_mutex_unlock(&tables[table_num][index].lock);
        index++;
    }
    pthread_mutex_lock(&tables[table_num][index].lock);
    if (index == first_empty && strcmp(tables[table_num][index].key, key_to_get))
    {
        pthread_mutex_unlock(&tables[table_num][index].lock);
        strcpy(value_to_get, "ERR_KEY_NOT_FOUND");
        return;
    }
    strcpy(value_to_get, tables[table_num][index].value);
    strcat(value_to_get, ";");
    snprintf(strtoktemp, MAX_CMD_LEN, "%d", tables[table_num][index].metadata);
    strcat(value_to_get, strtoktemp);
    pthread_mutex_unlock(&tables[table_num][index].lock);
}

void get_command_perm(char key_to_get[MAX_KEY_LEN], FILE *fileLoadData, char value_to_get[MAX_VALUE_LEN])
{
    char loadLine[MAX_VALUE_LEN];
    char loadLineTemp1[MAX_VALUE_LEN];
    char loadlineTemp2[MAX_VALUE_LEN];
    char *pch = NULL;

    bool stopLoad;
    bool firstPass;
    bool found;
    size_t lengthString;

    firstPass = true;
    stopLoad = false;
    found = false;

    strcpy(loadLine, "");
    strcpy(loadLineTemp1, "");
    strcpy(loadlineTemp2, "");
    strcpy(value_to_get, "");

    if (fileLoadData)
        do
        {
            strcpy (loadLineTemp1, loadLine);
            fgets(loadLine, MAX_VALUE_LEN, fileLoadData);

            lengthString = strlen(loadLine) - 1;
            if (loadLine[lengthString] == '\n')
                loadLine[lengthString] = '\0';
            strcpy (loadlineTemp2, loadLine);
            pch = strtok (loadlineTemp2, ":");

            if ( (firstPass == false) && (strcmp(loadLineTemp1, loadLine) == 0) )
                stopLoad = true;
            else
            {
                if (strcmp(loadlineTemp2, key_to_get) == 0)
                {
                    found = true;
                    pch = strtok (NULL, ":");
                    strcpy(value_to_get, pch);
                }
            }

            if (firstPass == true)
                firstPass = false;

        }
        while (stopLoad == false);

    if (found == false)
        strcpy(value_to_get, "ERR_KEY_NOT_FOUND");

}

void get_command_specific_perm(int lineNumber, FILE *fileLoadData, char value_to_get[MAX_VALUE_LEN])
{

    char loadLine[MAX_VALUE_LEN];
    char loadLineTemp1[MAX_VALUE_LEN];
    char loadlineTemp2[MAX_VALUE_LEN];
    char *pch2 = NULL;
    int i;

    bool stopLoad;
    bool firstPass;
    bool found;
    size_t lengthString;

    firstPass = true;
    stopLoad = false;
    found = false;

    strcpy(loadLine, "");
    strcpy(loadLineTemp1, "");
    strcpy(loadlineTemp2, "");
    strcpy(value_to_get, "");

    for (i = 0; i <= lineNumber; i++)
    {
        strcpy (loadLineTemp1, loadLine);
        fgets(loadLine, MAX_VALUE_LEN, fileLoadData);

        lengthString = strlen(loadLine) - 1;
        if (loadLine[lengthString] == '\n')
            loadLine[lengthString] = '\0';

        strcpy (loadlineTemp2, loadLine);
        if ( (firstPass == false) && (strcmp(loadLineTemp1, loadLine) == 0) )
            stopLoad = true;
        else
        {
            found = true;
            get_param(loadlineTemp2, value_to_get, 1, ":\0");
        }
        if (firstPass == true)
            firstPass = false;

    }

    if (stopLoad == true)
        strcpy(value_to_get, "STOP_LOADING");

}

/**
 * @brief Set the item using key_to_set and value_to_set
 *
 * @param key_to_set key to set
 * @param value_to_set value to set
 * @param first_empty index of the first empty spot in keys & values
 * @param table_num index of the table parsing
 * @return returns success string if it works (ERR_UNKNOWN if table already at max)
 */
char *set_command(char key_to_set[MAX_KEY_LEN], char value_to_set[MAX_VALUE_LEN],  int first_empty, int table_num)
{
    if (first_empty > MAX_RECORDS_PER_TABLE)
    {
        return "ERR_UNKNOWN";
    }
    pthread_mutex_lock(&tables[table_num][first_empty].lock);
    strcpy(tables[table_num][first_empty].key, key_to_set);
    strcpy(tables[table_num][first_empty].value, value_to_set);
    tables[table_num][first_empty].metadata = (unsigned long)time(NULL);
    pthread_mutex_unlock(&tables[table_num][first_empty].lock);
    return "SUCCESS";
}

char *set_command_perm(char key_to_set[MAX_KEY_LEN], char value_to_set[MAX_VALUE_LEN], FILE *fileLoadData, FILE *fileWriteData)
{
    // Set, Update, Delete Functionality

    char loadLine[MAX_VALUE_LEN];
    char loadLineTemp1[MAX_VALUE_LEN];
    char loadLineTemp2[MAX_VALUE_LEN];
    char *pch = NULL;

    bool stopLoad;
    bool firstPass;
    bool found;
    bool insert;
    bool deletef;

    size_t lengthString;

    strcpy(loadLine, "");
    strcpy(loadLineTemp1, "");
    strcpy(loadLineTemp2, "");

    firstPass = true;
    stopLoad = false;
    insert = true;
    deletef = false;

    if (fileLoadData != NULL)
    {
        do
        {
            strcpy (loadLineTemp1, loadLine);
            fgets(loadLine, 100, fileLoadData);

            lengthString = strlen(loadLine) - 1;
            if (loadLine[lengthString] == '\n')
                loadLine[lengthString] = '\0';
            strcpy (loadLineTemp2, loadLine);
            pch = strtok (loadLineTemp2, ":");

            if ( (firstPass == false) && (strcmp(loadLineTemp1, loadLine) == 0) )
                stopLoad = true;
            else
            {
                if (strcmp(loadLineTemp2, key_to_set) == 0)
                {
                    insert = false;
                    if (deletef == false)
                    {
                        fprintf(fileWriteData, "%s:", key_to_set);
                        fprintf(fileWriteData, "%s", value_to_set);
                        fprintf(fileWriteData, "\n");
                    }
                }
                else
                {
                    if (strcmp(loadLine, "") != 0)
                        fprintf(fileWriteData, "%s\n", loadLine);
                }
            }

            if (firstPass == true)
                firstPass = false;

        }
        while (stopLoad == false);

    }

    if (insert == true)
    {
        fprintf(fileWriteData, "%s:", key_to_set);
        fprintf(fileWriteData, "%s", value_to_set);
        fprintf(fileWriteData, "\n");
    }
    return "SUCCESS";
}

/**
 * @brief Set the item using key_to_set and value_to_set
 *
 * @param key_to_update key to set
 * @param value_to_update value to set
 * @param record_loc index of the record to update
 * @param table_num index of the table parsing
 * @param meta_data_recieved meta data recieved from client
 * @return returns success string if it works (always successful, due to key_exist() check)
 */
char *update_command(char key_to_update[MAX_KEY_LEN], char value_to_update[MAX_VALUE_LEN], int record_loc, int table_num, unsigned long int meta_data_recieved)
{
    pthread_mutex_lock(&tables[table_num][record_loc].lock);
    if ((tables[table_num][record_loc].metadata != meta_data_recieved) && (meta_data_recieved != 0))
    {
        pthread_mutex_unlock(&tables[table_num][record_loc].lock);
        strcpy(value_to_update, "ERR_TRANSACTION_ABORT");
        return "ERR_TRANSACTION_ABORT";
    }
    strcpy(tables[table_num][record_loc].key, key_to_update);
    strcpy(tables[table_num][record_loc].value, value_to_update);
    unsigned long new_meta = (unsigned long)time(NULL);
    if (tables[table_num][record_loc].metadata >= new_meta)
    {
        tables[table_num][record_loc].metadata = tables[table_num][record_loc].metadata + 1;
    }
    else
    {
        tables[table_num][record_loc].metadata = new_meta;
    }
    pthread_mutex_unlock(&tables[table_num][record_loc].lock);
    return "SUCCESS";
}

/**
 * @brief Get the index of the column based on the column name given
 *
 * @param column_name column name to find the type for
 * @param mycolumns names of the columns
 * @param table_num index of the table parsing
 * @return return the index of the column name given
 */
int get_column_index(char column_name[MAX_COLNAME_LEN], char mycolumns[MAX_TABLES][MAX_COLUMNS_PER_TABLE][MAX_COLNAME_LEN], int table_num)
{
    int i;
    for (i = 0; i < MAX_COLUMNS_PER_TABLE; i++)
    {
        if (!strcmp(column_name, mycolumns[table_num][i]))
        {
            return i;
        }
    }
}

/**
 * @brief Check if the value in the row index and column index passes the predicate
 *
 * @param predicate predicate to test for true or false
 * @param table_num index of the table parsing
 * @param column_index index of the column to check the predicate for
 * @param row_index index of the row to check the predicate for
 * @param column_types types of the columns
 * @return returns true(1) if the predicate is true, false(0) if it doesn't
 */
int predicate_true(char predicate[MAX_VALUE_LEN],  int table_num, int column_index, int row_index, char column_types[MAX_TABLES][MAX_COLUMNS_PER_TABLE][10])
{
    char strtoktemp[MAX_VALUE_LEN], operator[MAX_VALUE_LEN], val_to_measure[MAX_VALUE_LEN], col_str[MAX_VALUE_LEN], table_val[MAX_VALUE_LEN];
    int int_to_measure, val_in_table;

    if (strstr(column_types[table_num][column_index], "int"))
    {
        // column is of type int
        split_query_get_value(predicate, val_to_measure);

        int_to_measure = atoi(val_to_measure);

        pthread_mutex_lock(&tables[table_num][row_index].lock);
        strcpy(strtoktemp, tables[table_num][row_index].value);
        pthread_mutex_unlock(&tables[table_num][row_index].lock);
        get_param(strtoktemp, col_str, column_index, ",\0");

        strcpy(strtoktemp, col_str);
        get_param(strtoktemp, table_val, 1, " \0");

        val_in_table = atoi(table_val);

        if (strstr(predicate, ">"))
        {
            if (val_in_table > int_to_measure)
            {
                return 1;
            }
            return -1;
        }
        else if (strstr(predicate, "<"))
        {
            if (val_in_table < int_to_measure)
            {
                return 1;
            }
            return -1;
        }
        else if (strstr(predicate, "="))
        {
            if (val_in_table == int_to_measure)
            {
                return 1;
            }
            return -1;
        }
    }
    else
    {
        // column is of type int, only operator is '='
        split_query_get_value(predicate, val_to_measure);

        pthread_mutex_lock(&tables[table_num][row_index].lock);
        strcpy(strtoktemp, tables[table_num][row_index].value);
        pthread_mutex_unlock(&tables[table_num][row_index].lock);

        get_param(strtoktemp, col_str, column_index, ",\0");
        strcpy(strtoktemp, col_str);
        get_param(strtoktemp, table_val, 1, " \0");

        if (!strcmp(val_to_measure, table_val))
        {
            // String equals the predicate value
            return 1;
        }
        // String does not equal the predicate value
        return -1;
    }
}

int predicate_true_perm(char predicate[MAX_VALUE_LEN], char lineFromFile [MAX_VALUE_LEN], int table_num, int column_index, int row_index, char column_types[MAX_TABLES][MAX_COLUMNS_PER_TABLE][10])
{
    char strtoktemp[MAX_VALUE_LEN], operator[MAX_VALUE_LEN], val_to_measure[MAX_VALUE_LEN];
    int int_to_measure, val_in_table, i;
    char *pch;

    if (strstr(column_types[table_num][column_index], "int"))
    {
        // column is of type int
        split_query_get_value(predicate, val_to_measure);

        int_to_measure = atoi(val_to_measure);

        strcpy(strtoktemp, lineFromFile);
        pch = strtok (strtoktemp, ":");

        for (i = column_index; i > 0; i--)
        {
            pch = strtok (NULL, ",");
        }

        strcpy(strtoktemp, pch);
        pch = strtok (strtoktemp, " ");
        pch = strtok (NULL, " ");

        val_in_table = atoi(pch);


        if (strstr(predicate, ">"))
        {
            if (val_in_table > int_to_measure)
            {
                return 1;
            }
            return -1;
        }
        else if (operator[0] == '<')
        {
            if (val_in_table < int_to_measure)
            {
                return 1;
            }
            return -1;
        }
        else if (operator[0] == '=')
        {
            if (val_in_table == int_to_measure)
            {
                return 1;
            }
            return -1;
        }
    }
    else
    {
        // column is of type int, only operator is '='

        strcpy(strtoktemp, predicate);
        get_param(strtoktemp, val_to_measure, column_index, " \0");

        strcpy(strtoktemp, lineFromFile);

        get_param(strtoktemp, lineFromFile, 0, ":");

        if (!strcmp(val_to_measure, lineFromFile))
        {
            // String equals the predicate value
            return 1;
        }
        // String does not equal the predicate value
        return -1;
    }
}


/**
 * @brief Check if the value in the row index and column index passes the predicates
 *
 * @param predicates predicates to check if true or false
 * @param numcolumns number of columns in the table
 * @param table_num index of the table parsing
 * @param mycolumns names of the columns
 * @param column_types types of the columns
 * @param row_index index of the row to check the predicate for
 * @return returns true(1) if the predicate is true, false(0) if it doesn't
 */
int predicates_true(char predicates[MAX_VALUE_LEN],  int num_columns, int table_num, char mycolumns[MAX_TABLES][MAX_COLUMNS_PER_TABLE][MAX_COLNAME_LEN], char column_types[MAX_TABLES][MAX_COLUMNS_PER_TABLE][10], int row_index)
{
    char strtoktemp[MAX_VALUE_LEN], pred[MAX_VALUE_LEN], old_pred[MAX_VALUE_LEN], column_name[MAX_COLNAME_LEN];
    int index = 0, col_index;

    strcpy(old_pred, "");
    strcpy(strtoktemp, predicates);
    get_param(strtoktemp, pred, index, ",\0");

    while (strcmp(pred, old_pred) != 0)
    {
        split_query_get_column(pred, column_name);
        strcpy(column_name, trim(column_name));

        col_index = get_column_index(column_name, mycolumns, table_num);
        if (predicate_true(pred, table_num, col_index, row_index, column_types) == -1)
        {
            return -1;
        }
        index++;
        strcpy(old_pred, pred);

        strcpy(strtoktemp, predicates);
        get_param(strtoktemp, pred, index, ",\0");
    }
    return 1;
}

int predicates_true_perm(char predicates[MAX_VALUE_LEN], char lineFromFile [MAX_VALUE_LEN], int num_columns, int table_num, char mycolumns[MAX_TABLES][MAX_COLUMNS_PER_TABLE][MAX_COLNAME_LEN], char column_types[MAX_TABLES][MAX_COLUMNS_PER_TABLE][10], int row_index)
{
    char strtoktemp[MAX_VALUE_LEN], pred[MAX_VALUE_LEN], old_pred[MAX_VALUE_LEN], column_name[MAX_COLNAME_LEN];
    int index = 0, col_index;

    strcpy(old_pred, "");
    strcpy(strtoktemp, predicates);
    get_param(strtoktemp, pred, index, ",\0");

    while (strcmp(pred, old_pred) != 0)
    {
        strcpy(strtoktemp, pred);
        get_param(strtoktemp, column_name, 0, " \0");

        col_index = get_column_index(column_name, mycolumns, table_num);

        if (predicate_true_perm(pred, lineFromFile, table_num, col_index, row_index, column_types) == -1)
        {
            return -1;
        }
        index++;
        strcpy(old_pred, pred);

        strcpy(strtoktemp, predicates);
        get_param(strtoktemp, pred, index, ",\0");
    }
    return 1;
}


/**
 * @brief Query the table for matching values
 *
 * @param sock The socket connected to the client.
 * @param predicates predicates to check if true or false
 * @param first_empty index of the first empty spot in keys & values
 * @param table_num index of the table parsing
 * @param numcolumns number of columns in the table
 * @param mycolumns names of the columns
 * @param column_types types of the columns
 * @return returns the status for the server
 */
int query_command(int sock, char predicates[MAX_VALUE_LEN],  int first_empty, int table_num, int num_columns, char mycolumns[MAX_TABLES][MAX_COLUMNS_PER_TABLE][MAX_COLNAME_LEN], char column_types[MAX_TABLES][MAX_COLUMNS_PER_TABLE][10])
{
    int matched_lines[MAX_RECORDS_PER_TABLE];
    char comm_string[MAX_CMD_LEN];
    int i, index = 0;
    strcpy(comm_string, "");

    for (i = 0; i < first_empty; i++ )
    {
        if (predicates_true(predicates, num_columns, table_num, mycolumns, column_types, i) == 1)
        {
            matched_lines[index] = i;
            index++;
        }
    }

    if (index == 0)
    {
        // There are no records in the table that match the predicates
        strcpy(comm_string, "-1;0");
        sendall(sock, comm_string, strlen(comm_string));
        sendall(sock, "\n", 1);
        return 0;
    }

    for (i = 0; i < index; i++)
    {
        sprintf(comm_string, "%d", index - 1 - i);
        strcat(comm_string, ";");
        strcat(comm_string, tables[table_num][matched_lines[i]].key);
        strcat(comm_string, "\n\0");
        if (sendall(sock, comm_string, strlen(comm_string)) == 0 && recvline(sock, comm_string, MAX_CMD_LEN) == 0)
        {
            // Everything little thing is gonna be all right
        }
    }
    return 0;
}

int query_command_perm(int sock, char predicates[MAX_VALUE_LEN], int table_num, int num_columns, char mycolumns[MAX_TABLES][MAX_COLUMNS_PER_TABLE][MAX_COLNAME_LEN], char column_types[MAX_TABLES][MAX_COLUMNS_PER_TABLE][10], FILE *fileToLoad)
{
    int matched_lines[MAX_RECORDS_PER_TABLE];
    char comm_string[MAX_CMD_LEN], lineFromFile [MAX_VALUE_LEN];
    char *pch = NULL;
    int i, index = 0;
    bool stopLoop = false;
    strcpy(comm_string, "");
    i = 0;

    while (!stopLoop)
    {
        get_command_specific_perm(i, fileToLoad, lineFromFile);
        if (strcmp(lineFromFile, "STOP_LOADING") != 0)
        {
            if (predicates_true_perm(predicates, lineFromFile, num_columns, table_num, mycolumns, column_types, i) == 1)
            {
                matched_lines[index] = i;
                index++;
            }
        }
        else
        {
            stopLoop = true;
        }
        i++;
    }
    if (index == 0)
    {
        // There are no records in the table that match the predicates
        strcpy(comm_string, "-1;0");
        sendall(sock, comm_string, strlen(comm_string));
        sendall(sock, "\n", 1);
        return 0;
    }

    for (i = 0; i < index; i++)
    {
        sprintf(comm_string, "%d", index - 1 - i);
        strcat(comm_string, ";");
        get_command_specific_perm(matched_lines[index], fileToLoad, lineFromFile);
        strcat(comm_string, lineFromFile); // get'ed line
        sendall(sock, comm_string, strlen(comm_string));
        sendall(sock, "\n", 1);

        int wait_for_commands = 1;
        char cmd[MAX_CMD_LEN];
        int status = recvline(sock, cmd, MAX_CMD_LEN);
        if (status != 0)
        {
            // Either an error occurred or the client closed the connection.
            wait_for_commands = 0;
        }
        else
        {
            // Handle the command from the client.
            if (strstr(cmd, "SUCCESS"))
                wait_for_commands = 0; // Got the message, don't need to wait anymore
        }
    }
    return 0;

}

/**
 * @brief Delete the item in the server based on key_to_delete
 *
 * @param key_to_delete key to set
 * @param first_empty index of the first empty spot in keys & values
 * @param table_num index of the table parsing
 * @param num_columns number of columns in the table
 * @param mycolumns names of the columns
 * @return returns success string if it works (ERR_KEY_NOT_FOUND if key_to_delete DNE in keys)
 */
char *delete_command(char key_to_delete[MAX_KEY_LEN],  int first_empty, int table_num, int num_columns, char mycolumns[MAX_TABLES][MAX_COLUMNS_PER_TABLE][MAX_COLNAME_LEN] )
{
    char row[MAX_VALUE_LEN];
    int index = 0, i;
    if (first_empty == 0)
    {
        return "ERR_KEY_NOT_FOUND";
    }
    while (index < first_empty)
    {
        pthread_mutex_lock(&tables[table_num][index].lock);
        if (!strcmp(tables[table_num][index].key, key_to_delete))
        {
            pthread_mutex_unlock(&tables[table_num][index].lock);
            break;
        }
        pthread_mutex_unlock(&tables[table_num][index].lock);
        index++;
    }
    if (index == first_empty)
    {
        return "ERR_KEY_NOT_FOUND";
    }
    for (; index < first_empty - 1; index++)
    {
        pthread_mutex_lock(&tables[table_num][index].lock);
        strcpy(tables[table_num][index].key, tables[table_num][index + 1].key);
        strcpy(tables[table_num][index].value, tables[table_num][index + 1].value);
        tables[table_num][index].metadata = tables[table_num][index].metadata;
        pthread_mutex_unlock(&tables[table_num][index].lock);
    }
    return "SUCCESS";
}

char *delete_command_perm(char key_to_set[MAX_KEY_LEN], FILE *fileLoadData, FILE *fileWriteData)
{

    // Set, Update, Delete Functionality

    char loadLine[MAX_VALUE_LEN];
    char loadLineTemp1[MAX_VALUE_LEN];
    char loadLineTemp2[MAX_VALUE_LEN];
    char *pch = NULL;

    bool stopLoad;
    bool firstPass;
    bool found;
    bool insert;
    bool deletef;

    size_t lengthString;

    strcpy(loadLine, "");
    strcpy(loadLineTemp1, "");
    strcpy(loadLineTemp2, "");

    firstPass = true;
    stopLoad = false;
    insert = true;
    deletef = true;

    do
    {
        strcpy (loadLineTemp1, loadLine);
        fgets(loadLine, 100, fileLoadData);

        lengthString = strlen(loadLine) - 1;
        if (loadLine[lengthString] == '\n')
            loadLine[lengthString] = '\0';
        strcpy (loadLineTemp2, loadLine);
        pch = strtok (loadLineTemp2, ":");

        if ( (firstPass == false) && (strcmp(loadLineTemp1, loadLine) == 0) )
            stopLoad = true;
        else
        {
            if (strcmp(loadLineTemp2, key_to_set) == 0)
            {
                insert = false;
            }
        }

        if (firstPass == true)
            firstPass = false;

    }
    while (stopLoad == false);

    if (insert == true)
        return "ERR_KEY_NOT_FOUND";

    return "SUCCESS";
}

/**
 * @brief Process a command from the client.
 *
 * @param sock The socket connected to the client.
 * @param cmd The command received from the client.
 * @param *auth_var variable that keeps track if client is authorized or not
 * @return Returns 0 on success, -1 otherwise.
 */
int handle_command(int sock, char cmd[MAX_CMD_LEN], int *auth_var)
{
    char key_temp[MAX_KEY_LEN];
    char value_temp[MAX_VALUE_LEN];
    char strtok_temp[MAX_CMD_LEN];
    char username_temp[MAX_USERNAME_LEN];
    char passwd_temp[MAX_ENC_PASSWORD_LEN];
    char update_value_temp[MAX_VALUE_LEN];
    char table_temp[MAX_TABLE_LEN];
    char pred_temp[MAX_VALUE_LEN];
    char meta_temp[MAX_CMD_LEN];

    char *is_auth = strstr(cmd, "AUTH");
    char *is_get = strstr(cmd, "GET");
    char *is_set = strstr(cmd, "SET");
    char *is_delete = strstr(cmd, "DELETE");
    char *is_query = strstr(cmd, "QUERY");

    int table_index, return_val_query_perm, has_key;
    unsigned long int meta_temp_int;

    // AUTH comand called
    if (is_auth)
    {
        // Get the username and password from the communication string
        strcpy(strtok_temp, cmd);
        get_param(strtok_temp, username_temp, 1, ";\0");
        strcpy(strtok_temp, cmd);
        get_param(strtok_temp, passwd_temp, 2, ";\0");

        pthread_mutex_lock(&params.lock);
        if (!strcmp(params.username, username_temp) && !strcmp(params.password, passwd_temp))
        {
            pthread_mutex_unlock(&params.lock);
            // Username and password from client cmd are the same as in the config file
            *auth_var = 1;
            strcpy(value_temp, "SUCCESS");
            sendall(sock, value_temp, strlen(value_temp));
            sendall(sock, "\n", 1);
        }
        else
        {
            pthread_mutex_unlock(&params.lock);
            // Username and password from client cmd are not the same as in the config file
            strcpy(value_temp, "ERR_AUTHENTICATION_FAILED");
            sendall(sock, value_temp, strlen(value_temp));
            sendall(sock, "\n", 1);
            return -1;
        }
    }
    else if (is_get)
    {
        if (*auth_var)
        {
            // Get table name from cmd
            strcpy(strtok_temp, cmd);
            get_param(strtok_temp, table_temp, 1, ";\0");
            table_index = has_table(table_temp);
            if (table_index == -1)
            {
                // table name doesn't exist in the config file / server
                strcpy(value_temp, "ERR_TABLE_NOT_FOUND");
                sendall(sock, value_temp, strlen(value_temp));
                sendall(sock, "\n", 1);
                return -1;
            }
            // table exists in config file, continue

            // Get key name from cmd
            strcpy(strtok_temp, cmd);
            get_param(strtok_temp, key_temp, 2, ";\0");
            pthread_mutex_lock(&params.lock);
            if (params.storage_policy == 0)
            {
                pthread_mutex_unlock(&params.lock);
                // Use memory for server storage
                get_command(key_temp, value_temp, first_empty[table_index], table_index);
                sendall(sock, value_temp, strlen(value_temp));
                sendall(sock, "\n", 1);
            }
            else
            {
                pthread_mutex_unlock(&params.lock);
                FILE *fileLoadData;
                char tablenamestring[MAX_TABLE_LEN + 8];
                char datadirectory[MAX_PATH_LEN + MAX_TABLE_LEN + 8];

                char datadirectoryTEMP2[MAX_PATH_LEN + MAX_TABLE_LEN + 8];
                strcpy(datadirectoryTEMP2, params.data_directory);


                get_param(datadirectoryTEMP2, datadirectory, 0, ".\0");

                strcpy(datadirectoryTEMP2, datadirectory);
                strcpy(datadirectory, "");
                get_param(datadirectoryTEMP2, datadirectory, 0, "/\0");

                strcat(datadirectory, "/");
                struct stat st = {0};

                if (stat(datadirectory, &st) == -1)
                {
                    mkdir(datadirectory, 0700);
                }

                strcpy(tablenamestring, table_temp);
                strcat(tablenamestring, "_tbl.txt");
                strcat(datadirectory, tablenamestring);
                fileLoadData = fopen (datadirectory, "rt");
                get_command_perm(key_temp, fileLoadData, value_temp);
                if (fileLoadData)
                    fclose(fileLoadData);
                sendall(sock, value_temp, strlen(value_temp));
                sendall(sock, "\n", 1);
            }
        }
        else
        {
            // Client is not authenticated to the server yet
            strcpy(value_temp, "ERR_NOT_AUTHENTICATED");
            sendall(sock, value_temp, strlen(value_temp));
            sendall(sock, "\n", 1);
            return -1;
        }
    }
    else if (is_set)
    {
        if (*auth_var)
        {
            // Client is authenticated

            // Get the table name from cmd
            strcpy(strtok_temp, cmd);
            get_param(strtok_temp, table_temp, 1, ";\0");

            table_index = has_table(table_temp);
            if (table_index == -1)
            {
                // table name DNE in the config params
                strcpy(value_temp, "ERR_TABLE_NOT_FOUND");
                sendall(sock, value_temp, strlen(value_temp));
                sendall(sock, "\n", 1);
                return -1;
            }
            // table does exist in config params

            // Get the key and value to store from cmd
            strcpy(strtok_temp, cmd);
            get_param(strtok_temp, key_temp, 2, ";\0");
            strcpy(strtok_temp, cmd);
            get_param(strtok_temp, value_temp, 3, ";\0");

            pthread_mutex_lock(&params.lock);
            if (params.storage_policy == 0)
            {
                pthread_mutex_unlock(&params.lock);
                // Use memory for storing the server
                has_key = key_exist(key_temp, first_empty[table_index], table_index);
                if (has_key != -1)
                {
                    // Key exists, do an update
                    trim(value_temp);
                    if (parse_value(value_temp, table_index) == 1)
                    {
                        strcpy(strtok_temp, cmd);
                        get_param(strtok_temp, meta_temp, 4, ";\0");
                        meta_temp_int = atoi(meta_temp);
                        // value string matches the setup of the table
                        strcpy(update_value_temp, update_command(key_temp, value_temp, has_key, table_index, meta_temp_int));
                        sendall(sock, update_value_temp, strlen(value_temp));
                        sendall(sock, "\n", 1);
                    }
                    else
                    {
                        // value string does not match the setup of the table
                        strcpy(update_value_temp, "ERR_INVALID_PARAM");
                        sendall(sock, update_value_temp, strlen(update_value_temp));
                        sendall(sock, "\n", 1);
                        return -1;
                    }
                }
                else
                {
                    if (parse_value(value_temp, table_index) == 1)
                    {
                        // value string matches the setup of the table
                        strcpy(value_temp, set_command(key_temp, value_temp, first_empty[table_index], table_index));
                        sendall(sock, value_temp, strlen(value_temp));
                        sendall(sock, "\n", 1);
                        first_empty[table_index] = first_empty[table_index] + 1;
                    }
                    else
                    {
                        // value string does not match the setup of the table
                        strcpy(update_value_temp, "ERR_INVALID_PARAM");
                        sendall(sock, update_value_temp, strlen(update_value_temp));
                        sendall(sock, "\n", 1);
                        return -1;
                    }
                }
            }
            else
            {
                pthread_mutex_unlock(&params.lock);

                trim(value_temp);
                if (parse_value(value_temp, table_index) == 1)
                {

                    // check if matches set up
                    FILE *fileLoadData;
                    FILE *fileWriteData;

                    char tablenamestring[MAX_TABLE_LEN + 8];
                    char datadirectory[MAX_PATH_LEN + MAX_TABLE_LEN + 8];

                    char tablenamestringTEMP[MAX_TABLE_LEN + 13];
                    char datadirectoryTEMP[MAX_PATH_LEN + MAX_TABLE_LEN + 13];

                    char datadirectoryTEMP2[MAX_PATH_LEN + MAX_TABLE_LEN + 8];
                    strcpy(datadirectoryTEMP2, params.data_directory);


                    get_param(datadirectoryTEMP2, datadirectory, 0, ".\0");

                    strcpy(datadirectoryTEMP2, datadirectory);
                    strcpy(datadirectory, "");
                    get_param(datadirectoryTEMP2, datadirectory, 0, "/\0");

                    strcat(datadirectory, "/");
                    struct stat st = {0};

                    if (stat(datadirectory, &st) == -1)
                    {
                        mkdir(datadirectory, 0700);
                    }

                    strcpy(tablenamestring, table_temp);
                    strcpy (tablenamestringTEMP, tablenamestring);
                    strcat(tablenamestring, "_tbl.txt");
                    strcat(tablenamestringTEMP, "_tbl_TEMP.txt");
                    strcpy(datadirectoryTEMP, datadirectory);
                    strcat(datadirectory, tablenamestring);
                    strcat(datadirectoryTEMP, tablenamestringTEMP);
                    fileLoadData = fopen (datadirectory, "rt");
                    fileWriteData = fopen (datadirectoryTEMP, "w");
                    strcpy(update_value_temp, set_command_perm(key_temp, value_temp, fileLoadData, fileWriteData));
                    if (fileLoadData != NULL)
                        fclose(fileLoadData);
                    if (fileWriteData != NULL)
                        fclose(fileWriteData);

                    remove (datadirectory);
                    rename (datadirectoryTEMP, datadirectory);

                    sendall(sock, value_temp, strlen(value_temp));
                    sendall(sock, "\n", 1);
                }
                else
                {
                    // value string does not match the setup of the table
                    strcpy(update_value_temp, "ERR_INVALID_PARAM");
                    sendall(sock, update_value_temp, strlen(update_value_temp));
                    sendall(sock, "\n", 1);
                    return -1;
                }

            }
        }
        else
        {
            strcpy(value_temp, "ERR_NOT_AUTHENTICATED");
            sendall(sock, value_temp, strlen(value_temp));
            sendall(sock, "\n", 1);
            return -1;
        }
    }
    else if (is_query)
    {
        if (*auth_var)
        {
            // Client is authorized to access server

            // Get the table name from the command
            strcpy(strtok_temp, cmd);
            get_param(strtok_temp, table_temp, 1, ";\0");

            table_index = has_table(table_temp);
            if (table_index == -1)
            {
                // table name DNE in the config params
                strcpy(value_temp, "ERR_TABLE_NOT_FOUND");
                sendall(sock, value_temp, strlen(value_temp));
                sendall(sock, "\n", 1);
                return -1;
            }
            // Table does exist in the config_params
            strcpy(strtok_temp, cmd);
            get_param(strtok_temp, pred_temp, 2, ";\0");
            int num_pred = parse_predicates(pred_temp, table_index);
            if (num_pred != -1)
            {
                pthread_mutex_lock(&params.lock);
                if (params.storage_policy == 0)
                {
                    pthread_mutex_unlock(&params.lock);
                    return query_command(sock, pred_temp, first_empty[table_index], table_index, params.numcolumnspertable[table_index], params.mycolumns, params.column_types);
                }
                else
                {
                    pthread_mutex_unlock(&params.lock);
                    FILE *fileLoadData;
                    char tablenamestring[MAX_TABLE_LEN + 8];
                    char datadirectory[MAX_PATH_LEN + MAX_TABLE_LEN + 8];
                    char datadirectoryTEMP2[MAX_PATH_LEN + MAX_TABLE_LEN + 8];
                    strcpy(datadirectoryTEMP2, params.data_directory);


                    get_param(datadirectoryTEMP2, datadirectory, 0, ".\0");

                    strcpy(datadirectoryTEMP2, datadirectory);
                    strcpy(datadirectory, "");
                    get_param(datadirectoryTEMP2, datadirectory, 0, "/\0");

                    strcat(datadirectory, "/");
                    struct stat st = {0};

                    if (stat(datadirectory, &st) == -1)
                    {
                        mkdir(datadirectory, 0700);
                    }
                    strcpy(tablenamestring, table_temp);
                    strcat(tablenamestring, "_tbl.txt");
                    strcat(datadirectory, tablenamestring);
                    fileLoadData = fopen (datadirectory, "rt");
                    return_val_query_perm = query_command_perm(sock, pred_temp, table_index, params.numcolumnspertable[table_index], params.mycolumns, params.column_types, fileLoadData);

                    if (fileLoadData)
                        fclose(fileLoadData);

                    return return_val_query_perm;
                }
            }
            else
            {
                strcpy(pred_temp, "ERR_INVALID_PARAM");
                sendall(sock, pred_temp, strlen(pred_temp));
                sendall(sock, "\n", 1);
                return -1;
            }
        }
        else
        {
            strcpy(value_temp, "ERR_NOT_AUTHENTICATED");
            sendall(sock, value_temp, strlen(value_temp));
            sendall(sock, "\n", 1);
            return -1;
        }
    }
    else if (is_delete)
    {
        if (*auth_var)
        {
            // Client is authorized to access server

            // Get the table name from the command
            strcpy(strtok_temp, cmd);
            get_param(strtok_temp, table_temp, 1, ";\0");

            table_index = has_table(table_temp);
            if (table_index == -1)
            {
                // Table does not exist in the config_params
                strcpy(value_temp, "ERR_TABLE_NOT_FOUND");
                sendall(sock, value_temp, strlen(value_temp));
                sendall(sock, "\n", 1);
                return -1;
            }
            // Table does exist in the config_params

            // Get the key of the value to delete
            strcpy(strtok_temp, cmd);
            get_param(strtok_temp, key_temp, 2, ";\0");

            pthread_mutex_lock(&params.lock);
            if (params.storage_policy == 0)
            {
                pthread_mutex_unlock(&params.lock);
                // Use memory for storing the server
                strcpy(value_temp, delete_command(key_temp, first_empty[table_index], table_index, params.numcolumnspertable[table_index], params.mycolumns));
                first_empty[table_index] = first_empty[table_index] - 1;
                sendall(sock, value_temp, strlen(value_temp));
                sendall(sock, "\n", 1);
            }
            else
            {
                pthread_mutex_unlock(&params.lock);
                FILE *fileLoadData;
                FILE *fileWriteData;

                char tablenamestring[MAX_TABLE_LEN + 8];
                char datadirectory[MAX_PATH_LEN + MAX_TABLE_LEN + 8];

                char tablenamestringTEMP[MAX_TABLE_LEN + 13];
                char datadirectoryTEMP[MAX_PATH_LEN + MAX_TABLE_LEN + 13];

                char datadirectoryTEMP2[MAX_PATH_LEN + MAX_TABLE_LEN + 8];
                strcpy(datadirectoryTEMP2, params.data_directory);


                get_param(datadirectoryTEMP2, datadirectory, 0, ".\0");

                strcpy(datadirectoryTEMP2, datadirectory);
                strcpy(datadirectory, "");
                get_param(datadirectoryTEMP2, datadirectory, 0, "/\0");

                strcat(datadirectory, "/");
                struct stat st = {0};

                if (stat(datadirectory, &st) == -1)
                {
                    mkdir(datadirectory, 0700);
                }

                strcpy(tablenamestring, table_temp);
                strcpy (tablenamestringTEMP, tablenamestring);
                strcat(tablenamestring, "_tbl.txt");
                strcat(tablenamestringTEMP, "_tbl_TEMP.txt");
                strcpy(datadirectoryTEMP, datadirectory);
                strcat(datadirectory, tablenamestring);
                strcat(datadirectoryTEMP, tablenamestringTEMP);
                fileLoadData = fopen (datadirectory, "rt");
                fileWriteData = fopen (datadirectoryTEMP, "w");
                strcpy(value_temp, delete_command_perm(key_temp, fileLoadData, fileWriteData));
                fclose(fileLoadData);
                fclose(fileWriteData);

                remove (datadirectory);
                rename (datadirectoryTEMP, datadirectory);

                sendall(sock, value_temp, strlen(value_temp));
                sendall(sock, "\n", 1);
            }
        }
        else
        {
            strcpy(value_temp, "ERR_NOT_AUTHENTICATED");
            sendall(sock, value_temp, strlen(value_temp));
            sendall(sock, "\n", 1);
            return -1;
        }
    }
    else
    {
        strcpy(value_temp, "ERR_UNKNOWN");
        sendall(sock, value_temp, strlen(value_temp));
        return -1;
    }
    return 0;
}

void *handle_client(void *arg)
{
    int is_auth = 0;
    pthread_t pth;

    struct arguements *args = arg;;
    int clientsock = args->sock_;
    struct sockaddr_in clientaddr = args->clientaddr_;
    socklen_t clientaddrlen = args->clientaddrlen_;


    // Get commands from client.edit
    int wait_for_commands = 1;
    do
    {
        // Read a line from the client.
        char cmd[MAX_CMD_LEN];
        int status = recvline(clientsock, cmd, MAX_CMD_LEN);
        if (status != 0)
        {
            // Either an error occurred or the client closed the connection.
            wait_for_commands = 0;
        }
        else
        {
            // Handle the command from the client.
            int status = handle_command(clientsock, cmd, &is_auth);
            if (status != 0)
                wait_for_commands = 0; // Oops.  An error occured.
        }
    }
    while (wait_for_commands);

    // Close the connection with the client.
    close(clientsock);
    is_auth = 0;

    char log_message_closeconnection[150];
    sprintf(log_message_closeconnection, "Closed connection from %s:%d.\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);
    logger(fserverOut, log_message_closeconnection, LOGGING_SERVER);
}

/**
 * @brief Start the storage server.
 *
 * This is the main entry point for the storage server.  It reads the
 * configuration file, starts listening on a port, and proccesses
 * commands from clients.
 */
int main(int argc, char *argv[])
{
    int is_auth = 0, i, j;
    pthread_t pth;

    for (i = 0; i < MAX_TABLES; i++)
    {
        for (j = 0; j < MAX_RECORDS_PER_TABLE; j++)
        {
            pthread_mutex_init(&tables[i][j].lock, NULL);
            pthread_mutex_unlock(&tables[i][j].lock);
        }
    }

    // Initialize the number of elements in each table to 0
    for (i = 0; i < MAX_TABLES; i++)
    {
        first_empty[i] = 0;
    }

    time_t rawtime;
    struct tm *timeinfo;
    char timeStamp [50];

    time (&rawtime);
    timeinfo = localtime (&rawtime);

    strftime (timeStamp, 50, "Server-%F-%H-%M-%S.log", timeinfo);
    if (LOGGING_SERVER == 2)
    {
        fserverOut = fopen (timeStamp, "w+");
    }
    // Process command line arguments.
    // This program expects exactly one argument: the config file name.
    assert(argc > 0);
    if (argc != 2)
    {
        printf("Usage %s <config_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char *config_file = argv[1];

    // Read the config file.

    int status = read_config(config_file, &params);
    if (status != 0)
    {
        printf("Error processing config file.\n");
        exit(EXIT_FAILURE);
    }

    char log_message_serveron[150];
    sprintf(log_message_serveron, "Server on %s:%d\n", params.server_host, params.server_port);
    logger(fserverOut, log_message_serveron, LOGGING_SERVER);

    // Create a socket.
    int listensock = socket(PF_INET, SOCK_STREAM, 0);
    if (listensock < 0)
    {
        printf("Error creating socket.\n");
        exit(EXIT_FAILURE);
    }

    // Allow listening port to be reused if defunct.
    int yes = 1;
    status = setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    if (status != 0)
    {
        printf("Error configuring socket.\n");
        exit(EXIT_FAILURE);
    }

    // Bind it to the listening port.
    struct sockaddr_in listenaddr;
    memset(&listenaddr, 0, sizeof listenaddr);
    listenaddr.sin_family = AF_INET;
    listenaddr.sin_port = htons(params.server_port);
    inet_pton(AF_INET, params.server_host, &(listenaddr.sin_addr)); // bind to local IP address
    status = bind(listensock, (struct sockaddr *) &listenaddr, sizeof listenaddr);
    if (status != 0)
    {
        printf("Error binding socket.\n");
        exit(EXIT_FAILURE);
    }

    // Listen for connections.
    status = listen(listensock, MAX_LISTENQUEUELEN);
    if (status != 0)
    {
        printf("Error listening on socket.\n");
        exit(EXIT_FAILURE);
    }

    if (params.concurrency == 1)
    {
        // Multithreading / multiple clients
        // Listen loop.
        int wait_for_connections = 1;
        while (wait_for_connections)
        {
            // Wait for a connection.
            struct sockaddr_in clientaddr;
            socklen_t clientaddrlen = sizeof clientaddr;
            int clientsock = accept(listensock, (struct sockaddr *)&clientaddr, &clientaddrlen);
            if (clientsock < 0)
            {
                printf("Error accepting a connection.\n");
                exit(EXIT_FAILURE);
            }

            char log_message_getconnection[150];
            printf("Got a connection from %s:%d.\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);
            sprintf(log_message_getconnection, "Got a connection from %s:%d.\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);
            logger(fserverOut, log_message_getconnection, LOGGING_SERVER);

            struct arguements args;
            args.clientaddr_ = clientaddr;
            args.sock_ = clientsock;
            args.clientaddrlen_ = clientaddrlen;

            pthread_create(&pth, NULL, handle_client, (void *)&args);
        }

        // Stop listening for connections.
        close(listensock);

        return EXIT_SUCCESS;
    }
    else
    {
        // Listen loop.
        int wait_for_connections = 1;
        while (wait_for_connections)
        {
            // Wait for a connection.
            struct sockaddr_in clientaddr;
            socklen_t clientaddrlen = sizeof clientaddr;
            int clientsock = accept(listensock, (struct sockaddr *)&clientaddr, &clientaddrlen);
            if (clientsock < 0)
            {
                printf("Error accepting a connection.\n");
                exit(EXIT_FAILURE);
            }

            char log_message_getconnection[150];
            printf("Got a connection from %s:%d.\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);
            sprintf(log_message_getconnection, "Got a connection from %s:%d.\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);
            logger(fserverOut, log_message_getconnection, LOGGING_SERVER);

            // Get commands from client.edit
            int wait_for_commands = 1;
            do
            {
                // Read a line from the client.
                char cmd[MAX_CMD_LEN];
                int status = recvline(clientsock, cmd, MAX_CMD_LEN);
                if (status != 0)
                {
                    // Either an error occurred or the client closed the connection.
                    wait_for_commands = 0;
                }
                else
                {
                    // Handle the command from the client.
                    int status = handle_command(clientsock, cmd, &is_auth);
                    if (status != 0)
                        wait_for_commands = 0; // Oops.  An error occured.
                }
            }
            while (wait_for_commands);

            // Close the connection with the client.
            close(clientsock);
            is_auth = 0;

            char log_message_closeconnection[150];
            sprintf(log_message_closeconnection, "Closed connection from %s:%d.\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);
            logger(fserverOut, log_message_closeconnection, LOGGING_SERVER);
        }

        // Stop listening for connections.
        close(listensock);

        return EXIT_SUCCESS;
    }
}
