%{
#include <stdio.h>
#include <string.h>
#include "utils.h"

int colnum=0;
int m;
extern int error_occurred;
extern int server_hostcount;
extern int server_portcount;
extern int usernamecount;
extern int passwordcount;
extern int tablecount;
extern int storagepolicycount;
extern int datadirectorycount;
extern struct config_params paramslex;


void yyerror(const char *str)
{
	error_occurred=1;
	fprintf(stderr,"error : %s\n",str);
}


int yywrap()
{
	return 1;
}

%}


%start lines

%union {
char *stringVal;
char *passwordVal;
char *hostVal;
char *dataVal;
int intVal;
}


%token HOSTTOK PORTTOK USERNAMETOK PASSWORDTOK TABLETOK DASH END_OF_FILE
%token STORAGEPOLICYTOK DATADIRECTORYTOK INMEMORYTOK ONDISKTOK CONCURRENCYTOK
%token COMMA COLON NEWLINE INTTOK CHARTOK CBRACKET
%token <stringVal> STRING
%token <intVal> INTEGERTOK
%token <passwordVal> PASSWORD
%token <hostVal> HOSTNAME
%token <dataVal> DATA
 
%%



lines: line
	|	 
	lines NEWLINE line
;


line:

STORAGEPOLICYTOK INMEMORYTOK {
paramslex.storage_policy=0; 
storagepolicycount=storagepolicycount+1;
}
|
STORAGEPOLICYTOK ONDISKTOK {
paramslex.storage_policy=1;
storagepolicycount=storagepolicycount+1;
}
|
DATADIRECTORYTOK DATA {
strncpy(paramslex.data_directory, $2, sizeof paramslex.data_directory);
datadirectorycount=datadirectorycount+1;
}
|
STORAGEPOLICYTOK INMEMORYTOK END_OF_FILE {
paramslex.storage_policy=0; 
storagepolicycount=storagepolicycount+1;
return;
}
|
STORAGEPOLICYTOK ONDISKTOK END_OF_FILE {
paramslex.storage_policy=1;
storagepolicycount=storagepolicycount+1;
return;
}
|
DATADIRECTORYTOK DATA END_OF_FILE {
strncpy(paramslex.data_directory, $2, sizeof paramslex.data_directory);
datadirectorycount=datadirectorycount+1;
return;
}
|
HOSTTOK STRING { 
strncpy(paramslex.server_host, $2, sizeof paramslex.server_host);
server_hostcount=server_hostcount+1; }
|
HOSTTOK HOSTNAME { 
strncpy(paramslex.server_host, $2, sizeof paramslex.server_host);
server_hostcount=server_hostcount+1; }
|
PORTTOK INTEGERTOK {
paramslex.server_port = $2; 
server_portcount=server_portcount+1; }
|
USERNAMETOK STRING {
strncpy(paramslex.username, $2, sizeof paramslex.username);
usernamecount=usernamecount+1; }
|
CONCURRENCYTOK INTEGERTOK{
paramslex.concurrency = $2; 
}
|
CONCURRENCYTOK INTEGERTOK END_OF_FILE{
paramslex.concurrency = $2; 
return;
}
|
PASSWORDTOK PASSWORD { 
strncpy(paramslex.password, $2, sizeof paramslex.password); 
passwordcount=passwordcount+1; }
|
PASSWORDTOK STRING { 
strncpy(paramslex.password, $2, sizeof paramslex.password); 
passwordcount=passwordcount+1; }
|
END_OF_FILE { return; }
|
{   }
|
TABLES
;

TABLES:
TABLETOK STRING COLUMNS { 
strncpy(paramslex.mytables[tablecount], $2, sizeof paramslex.mytables[tablecount]);
m=0;
for(m=0; m<tablecount; m++) {
 if(strcmp(paramslex.mytables[m], $2) == 0) {
  error_occurred=1;
 }
}
printf("tablename is %s\n", $2);
paramslex.numcolumnspertable[tablecount]=colnum; 
tablecount=tablecount+1;
if(tablecount==101){
error_occurred=1;
}
paramslex.tablecount=tablecount;
colnum=0;
}
|
TABLETOK STRING COLUMNS END_OF_FILE { 
strncpy(paramslex.mytables[tablecount], $2, sizeof paramslex.mytables[tablecount]);
m=0;
for(m=0; m<tablecount; m++) {
 if(strcmp(paramslex.mytables[m], $2) == 0) {
  error_occurred=1;
 }
}
paramslex.numcolumnspertable[tablecount]=colnum; 
tablecount=tablecount+1;
if(tablecount==101){
error_occurred=1;
}
paramslex.tablecount=tablecount;
colnum=0;
return; }
|
TABLETOK STRING { 
strncpy(paramslex.mytables[tablecount], $2, sizeof paramslex.mytables[tablecount]);
m=0;
for(m=0; m<tablecount; m++) {
 if(strcmp(paramslex.mytables[m], $2) == 0) {
  error_occurred=1;
 }
}
paramslex.numcolumnspertable[tablecount]=colnum; 
tablecount=tablecount+1;
if(tablecount==101){
error_occurred=1;
}
paramslex.tablecount=tablecount;
colnum=0;
}
|
TABLETOK STRING END_OF_FILE { 
strncpy(paramslex.mytables[tablecount], $2, sizeof paramslex.mytables[tablecount]);
m=0;
for(m=0; m<tablecount; m++) {
 if(strcmp(paramslex.mytables[m], $2) == 0) {
  error_occurred=1;
 }
}
paramslex.numcolumnspertable[tablecount]=colnum; 
tablecount=tablecount+1;
if(tablecount==101){
error_occurred=1;
}
paramslex.tablecount=tablecount;
colnum=0;
return; }
;


COLUMNS: COLUMN
      | COLUMNS COMMA COLUMN
      ;
      
COLUMN:

STRING COLON INTTOK { 
strncpy(paramslex.mycolumns[tablecount][colnum], $1 , sizeof paramslex.mycolumns[tablecount][colnum]);
sprintf (paramslex.column_types[tablecount][colnum], "int");
colnum=colnum+1;
m=0;
while($1[m]!='\0'){
 m=m+1;
 if(m==20){
  error_occurred=1;
  }
}
 }
|
STRING COLON CHARTOK INTEGERTOK CBRACKET { 
strncpy(paramslex.mycolumns[tablecount][colnum], $1 , sizeof paramslex.mycolumns[tablecount][colnum]);
snprintf(paramslex.column_types[tablecount][colnum], sizeof paramslex.column_types[tablecount][colnum], "char[%d]\n",$4 );
colnum=colnum+1;
m=0;
while($1[m]!='\0'){
 m=m+1;
 if((m==20)||($4>40)){
  error_occurred=1;
  }
}
 }
|
STRING COLON CHARTOK DASH INTEGERTOK CBRACKET{
error_occurred=1;
}
;









