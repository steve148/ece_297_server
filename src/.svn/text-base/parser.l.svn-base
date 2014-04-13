%{
#include <stdio.h>
#include <string.h>
#include "parser.tab.h"
%}

%%
server_host               return HOSTTOK;
server_port               return PORTTOK;
username                  return USERNAMETOK;
password                  return PASSWORDTOK;
table                     return TABLETOK;
storage_policy            return STORAGEPOLICYTOK;
data_directory			  return DATADIRECTORYTOK;
concurrency				  return CONCURRENCYTOK;
in-memory				  return INMEMORYTOK;
on-disk					  return ONDISKTOK;
"int"                     return INTTOK;
"char["                   return CHARTOK;
-						  return DASH;
[a-zA-Z][a-zA-Z0-9]*      yylval.stringVal=strdup(yytext); return STRING;
[0-9][0-9]*               yylval.intVal=atoi(yytext); return INTEGERTOK;
[a-zA-Z0-9/\.!@><?"=+-][a-zA-Z0-9/\.!@><?"=+-]*  yylval.passwordVal=strdup(yytext); return PASSWORD; 
[a-zA-Z0-9/\.'-]*  		  yylval.dataVal=strdup(yytext); return DATA; 
[1-9][.0-9]+             yylval.hostVal=strdup(yytext); return HOSTNAME;
,					      return COMMA;
:					      return COLON;
[#][a-zA-Z0-9/\.-]*	      /* ignore */
[ \t]+ 				      /* ignore */
[\n]+                     return NEWLINE;
\]						  return CBRACKET;
<<EOF>>                   return END_OF_FILE;

%%
