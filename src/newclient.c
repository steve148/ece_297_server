#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Add wait time
// Add input/output?
// fix formatting

// Saves the input of the user in a dynamically allocated string
void getString (char **stringS) {
	
	// Clear input stream
	fflush(stdin);
	
	char c;
	int stringLen=1;
	int charCount=0;
	
	if (*stringS) {
		free(*stringS);
	}
	
	// Allocate memory for one character and a NULL terminator
	(*stringS) = (char*)malloc( (stringLen+1));
	
	// Skip all newlines
	do {
		c = getc(stdin);
	} while (c=='\n');
	
	do {
		// If more space is needed in string, reallocate it
		if (charCount >= stringLen) {
			stringLen=stringLen*2;
			(*stringS) = realloc((*stringS),(stringLen+1));
		}
		// Add to string
		(*stringS)[charCount]=c;
		charCount++;
		c = getc(stdin);
	} while (c!='\n');
	
	(*stringS)[charCount]='\0';
	
}

// Checks if the string has characters other than integers
bool intCheck (char *stringS) {
	
	int i = 0;
	int temp;
	
	do {
		temp = stringS[i] - 48;
		if (temp>9 || temp < 0)
			return false;
		i++;
	} while (stringS[i]!='\0');
	
	return true;
	
}

	
int main () {
	
	FILE *ofp = NULL;
	ofp = fopen("logFile.txt", "a");
	
	int commandValue;
	int portNumber;
	
	bool exitClient=false;
	
	char *commandKey = NULL;
	char *hostnameString = NULL;
	char *portString = NULL;
	char *usernameString = NULL;
	char *passwordString = NULL;
	char *tableGetString = NULL;
	char *keyGetString = NULL;
	char *recordGetString = NULL;
	char *tableSetString = NULL;
	char *keySetString = NULL;
	char *recordSetString = NULL;
	char *confirmString = NULL;
	
	char *escapeSeq = "!";
	
	do {
		fflush(stdin);
		
		// Show Menu
		printf("> Menu: \n");
		printf("> --------------------- \n");
		printf("> 1) Connect \n");	
		printf("> 2) Authenticate \n");
		printf("> 3) Get \n");
		printf("> 4) Set \n");
		printf("> 5) Disconnect \n");
		printf("> 6) Exit \n");
		printf("> --------------------- \n");
		
		// Command Input
		printf("> Enter command key: ");
		getString(&commandKey);
		
		commandValue=(*commandKey)-48; // ASCII Value of '0' is 48

		switch (commandValue) {
		
			case 1: // Connect
			
				printf("> \n");
				printf("> Connect Selected \n");
				printf("> \n");

				printf("> Input the hostname: ");
				getString(&hostnameString);
				
				if ((*hostnameString)==(*escapeSeq)) {
					printf("> Escape Sequence Detected - Aborting Command \n");
					break;
				}
				
				printf("> Input the port: ");
				getString(&portString);
				
				if ((*portString)==(*escapeSeq)) {
					printf("> Escape Sequence Detected - Aborting Command \n");
					break;
				}
					
				while (!intCheck(portString)) { // &portString?
					printf("> Port is not a proper number, please reenter port: ");
					getString(&portString);
				}
				
				portNumber=atoi(portString); // Converts the string to an integer for proper function argument
				
				printf("> \n");
				printf("> Waiting for server response \n");
				
				sleep(1000);
				
				printf("> \n");
				
				break;
			case 2: // Authenticate
		
				printf("> \n");
				printf("> Authenticate Selected \n");
				printf("> \n");
				
				printf("> Input the username: ");
				getString(&usernameString);
				
				if ((*usernameString)==(*escapeSeq))
					break;
				
				printf("> Input the password: ");
				getString(&passwordString);
				
				if ((*passwordString)==(*escapeSeq))
					break;
				
				printf("> \n");
				break;
			case 3: // Get
			
				printf("> \n");
				printf("> Get Selected \n");
				printf("> \n");
				
				printf("> Input the table: ");
				getString(&tableGetString);
				
				if ((*tableGetString)==(*escapeSeq))
					break;
				
				printf("> Input the key: ");
				getString(&keyGetString);
				
				if ((*keyGetString)==(*escapeSeq))
					break;
				
				printf("> Input the record: ");
				getString(&recordGetString);
				
				if ((*recordGetString)==(*escapeSeq))
					break;

				printf("> \n");
				break;
			case 4: // Set
			
				printf("> \n");
				printf("> Set Selected \n");
				printf("> \n");
				
				printf("> Input the table: ");
				getString(&tableSetString);
				
				if ((*tableSetString)==(*escapeSeq))
					break;
				
				printf("> Input the key: ");
				getString(&keySetString);
				
				if ((*keySetString)==(*escapeSeq))
					break;
				
				printf("> Input the record: ");
				getString(&recordSetString);
				
				if ((*recordSetString)==(*escapeSeq))
					break;
				
				printf("> \n");
				break;
			case 5: // Disconnect
					
				printf("> \n");
				printf("> Disconnect Selected \n");
				printf("> \n");
				
				printf("> To confirm input any integer (to cancel input escape sequence): ");
				getString(&confirmString);
				
				if ((*confirmString)==(*escapeSeq))
					break;
				
				// uses the storage_connect return call
				
				printf("> \n");
				break;
			case 6: // Exit
			
				printf("> \n");
				printf("> Exit Selected \n");
				printf("> \n");
				
				printf("> To confirm input any integer (to cancel input escape sequence): ");
				getString(&confirmString);
				
				if ((*confirmString)==(*escapeSeq))
					break;
				
				exitClient=true;
				
				break;
			default: // Error
				printf("> --------------------- \n");
				printf("Error: invalid command \n");
				printf("> --------------------- \n");
				
		}

	} while (exitClient==false);
	
	if (commandKey)
		free(commandKey);
	if (hostnameString)
		free(hostnameString);
	if (portString)
		free(portString);
	if (usernameString)
		free(usernameString);
	if (passwordString)
		free(passwordString);
	if (tableGetString)
		free(tableGetString);
	if (keyGetString)
		free(keyGetString);
	if (recordGetString)
		free(recordGetString);
	if (tableSetString)
		free(tableSetString);
	if (keySetString)
		free(keySetString);
	if (recordSetString)
		free(recordSetString);
	if (confirmString)
		free(confirmString);

	return 0;		
}
