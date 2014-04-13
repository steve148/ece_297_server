/**
 * @file
 * @brief This file declares various utility functions that are
 * can be used by the storage server and client library.
 */

#ifndef	UTILS_H
#define UTILS_H

#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "storage.h"

#define LOGGING_CLIENT 1
#define LOGGING_SERVER 1

/**
 * @brief Any lines in the config file that start with this character 
 * are treated as comments.
 */
static const char CONFIG_COMMENT_CHAR = '#';

/**
 * @brief The max length in bytes of a command from the client to the server.
 */
#define MAX_CMD_LEN (1024 * 8)

/**
 * @brief A macro to log some information.
 *
 * Use it like this:  LOG(("Hello %s", "world\n"))
 *
 * Don't forget the double parentheses, or you'll get weird errors!
 */
#define LOG(x)  {printf x; fflush(stdout);}

/**
 * @brief A macro to output debug information.
 * 
 * It is only enabled in debug builds.
 */
#ifdef NDEBUG
#define DBG(x)  {}
#else
#define DBG(x)  {printf x; fflush(stdout);}
#endif

/**
 * @brief A struct to store config parameters.
 */
struct config_params {
	/// The hostname of the server.
	char server_host[MAX_HOST_LEN];

	/// The listening port of the server.
	int server_port;

	/// The storage server's username
	char username[MAX_USERNAME_LEN];

	/// The storage server's encrypted password
	char password[MAX_ENC_PASSWORD_LEN];

	char mytables[MAX_TABLES][MAX_TABLE_LEN];

	//Column types for each the columns in the tables
	char mycolumns[MAX_TABLES][MAX_COLUMNS_PER_TABLE][MAX_COLNAME_LEN];	

	int tablecount;

	int numcolumnspertable[MAX_TABLES];

	char column_types[MAX_TABLES][MAX_COLUMNS_PER_TABLE][10];

	int storage_policy;

	// The directory where tables are stored.
  char data_directory[MAX_PATH_LEN];

  int concurrency;

  pthread_mutex_t lock;
};

/**
 * @brief Encapsulate the value associated with a key in a table.
 *
 * The metadata will be used later.
 */
struct server_record {
	/// This is where the actual value is stored.
	char value[MAX_VALUE_LEN];

	/// This is where the key is stored.
	char key[MAX_KEY_LEN];

	/// A place to put any extra data.
	unsigned long metadata;

	/// mutex variable for locking
	pthread_mutex_t lock;
};


/**
 * @brief Encapsulate the value associated with a key in a table.
 */
struct arguements {
	/// Communication port value
	int sock_;

	/// The client address information
	struct sockaddr_in clientaddr_;

	/// Socket length
	socklen_t clientaddrlen_;
};



/**
 * @brief Exit the program because a fatal error occured.
 *
 * @param msg The error message to print.
 * @param code The program exit return value.
 */
static inline void die(char *msg, int code)
{
	printf("%s\n", msg);
	exit(code);
}

/**
 * @brief Keep sending the contents of the buffer until complete.
 * @return Return 0 on success, -1 otherwise.
 *
 * The parameters mimic the send() function.
 */
int sendall(const int sock, const char *buf, const size_t len);

/**
 * @brief Receive an entire line from a socket.
 * @return Return 0 on success, -1 otherwise.
 */
int recvline(const int sock, char *buf, const size_t buflen);

/**
 * @brief Read and load configuration parameters.
 *
 * @param config_file The name of the configuration file.
 * @param params The structure where config parameters are loaded.
 * @return Return 0 on success, -1 otherwise.
 */
int read_config(const char *config_file, struct config_params *params);

/**
 * @brief Generates a log message.
 * 
 * @param file The output stream
 * @param message Message to log.
 */
void logger(FILE *file, char *message, int LOGGING);

/**
 * @brief Default two character salt used for password encryption.
 */
#define DEFAULT_CRYPT_SALT "xx"

/**
 * @brief Generates an encrypted password string using salt CRYPT_SALT.
 * 
 * @param passwd Password before encryption.
 * @param salt Salt used to encrypt the password. If NULL default value
 * DEFAULT_CRYPT_SALT is used.
 * @return Returns encrypted password.
 */
char *generate_encrypted_password(const char *passwd, const char *salt);

/**
 * @brief Split a string and return one of the split strings.
 *
 * @param str string to be split
 * @param to_return place to return the specified paramter in str
 * @param param_num which index of the split str to return
 * @param delims string of all characters used to deliminate string
 * @return no return value
 */
void get_param(char str[MAX_VALUE_LEN], char to_return[MAX_VALUE_LEN], int param_num, char *delims);

#endif
