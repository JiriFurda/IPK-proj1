/**
 * @file ipk-server.c
 * @brief
 * @author Jiri Furda (xfurda00)
 */ 


#include <stdio.h>
#include <stdlib.h> // atoi

#include <unistd.h> // getopt
#include <string.h>	// bzero

#include <sys/socket.h> // socket
#include <netinet/in.h> // internet domain adresses

#define MSG_START "!OK!"
#define MSG_NOTFOUND "!NF!"
#define MSG_END "!EN!"

// === Prototypes ===
void createSocket(int* socketFD, int port);
void waitForClient(int socketFD, int* clientFD);
void getArguments(int argc, char** argv, int* port);
void sendMessage(int socket, char* msg);
void getUserInfo(int socket, char flag, char* login);
void errorExit(char* msg);


// === Functions ===
int main(int argc, char** argv)
{
	// --- Processing arguments ---
	int port;
	getArguments(argc, argv, &port);
	
	
	// --- Setting up socket ---
	int socketFD;
	createSocket(&socketFD, port);
	
	
	// --- Client connection ---
	int clientFD; // socklen_t
	waitForClient(socketFD, &clientFD);
	
	
	// --- Reading messages ---
	char buffer[256];
	memset(buffer, '\0', sizeof(buffer)); 	// Earse buffer
	int n;
	n = read(clientFD, buffer, sizeof(buffer)-1);	// Sleep untill message is sent
	if(n < 0)
		errorExit("Couldn't read from socket");
	
	int handshakeLength = strlen(buffer);
	char flag = buffer[handshakeLength-1];
	buffer[handshakeLength-2] = '\0';
	getUserInfo(clientFD, flag, buffer);

	return 0;
}


void createSocket(int* socketFD, int port)
{
	struct sockaddr_in serv_addr;
	
	
	// --- Creating socket ---
	*socketFD = socket(AF_INET, SOCK_STREAM, 0);

	if(*socketFD < 0)
		errorExit("ERROR: Couldn't create socket");
	
	
	// --- Setting up the socket ---
	memset(&serv_addr, '\0', sizeof(serv_addr));	// Earse adress buffer
	
	serv_addr.sin_family = AF_INET;	// Symbolic constant
	serv_addr.sin_port = htons(port);	// Convert to network byte order
	serv_addr.sin_addr.s_addr = INADDR_ANY;	// localost
	
	
	
	// --- Binding the socket ---
	int binded = bind(*socketFD, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if(binded < 0)
		errorExit("Couldn't bind socket");
}

void waitForClient(int socketFD, int* clientFD)
{
	
	listen(socketFD, 5);	// Expect client connection
	
	struct sockaddr_in clientAddr;
	socklen_t clientAddrSize = sizeof(clientAddr);
	
	*clientFD = accept(socketFD, (struct sockaddr *) &clientAddr, &clientAddrSize);	// Sleep untill connection
	if(*clientFD < 0) 
		errorExit("Couldn't accept connection");
}

void getUserInfo(int socket, char flag, char* login)
{
	FILE *file = fopen("/etc/passwd", "r");
	if(file == NULL)
		errorExit("Couldn't open file");	
	
	sendMessage(socket, MSG_START);
	
	int found = 0;	// Found result
	char line[1024];	// Buffer for reading file
	
	if(flag == 'l')
	{
		if(login == NULL)
		{
			// -- Browsing file --	
			while(fgets(line, sizeof(line), file) != NULL)
			{	
				char *token = strtok(line, ":");	// Get the first colon
				
				sendMessage(socket, token);
			}
		}
		else
		{
			// -- Browsing file --	
			while(fgets(line, sizeof(line), file) != NULL)
			{	
				if(!strncmp(login, line, strlen(login)))	// Matches the rule
				{
					char *token = strtok(line, ":");	// Get the first colon
					sendMessage(socket, token);
				}
			}			
		}
	}
	else
	{
		// -- Resolving flag --
		int skip; // How many colons should be skipped
		if(flag == 'n')
			skip = 4;
		else if(flag == 'h')
			skip = 5;
		else
		{
			fclose(file);
			errorExit("Invalid flag");
		}
		
		
		// -- Preparing login variable --
		char newLogin[strlen(login)+2];
		char suffix[2] = ":";
		strcpy(newLogin, login);
		strcat(newLogin, suffix);
		
		
		// -- Searching in file --	
		char line[1024];
		while(fgets(line, sizeof(line), file) != NULL)
		{
			if(!strncmp(newLogin, line, strlen(newLogin)))	// Found the specified login
			{
				found = 1;
				
				// - Skipping colons -
				char *token = strtok(line, ":");
				for(int i = 0; i != skip; i++)
					token = strtok(NULL, ":");
					
				// - Print result -
				sendMessage(socket, token);
			}
		}		
	}

	fclose(file);

	if(found == 0)
	{
		sendMessage(socket, MSG_NOTFOUND);
	}
	
	sendMessage(socket, MSG_END);
}

void sendMessage(int socket, char* msg)
{
	int n;
	if(!strcmp(msg, MSG_END))
	{
		n = write(socket, msg, strlen(msg));
	}
	else
	{
	int msgLength = strlen(msg);
	char msgSent[msgLength+2];
	strcpy(msgSent, msg);
	msgSent[msgLength] = '\n';
	msgSent[msgLength+1] = '\0';
	n = write(socket, msgSent, msgLength+1);
	}
	
	if(n < 0)
		errorExit("Couldn't write to socket");	
}


void getArguments(int argc, char** argv, int* port)
{
	int c;
	
	while((c = getopt (argc, argv, "p:")) != -1)
	{
		switch(c)
		{
			case 'p':
				*port = atoi(optarg);
				break;
			case '?':
				errorExit("Invalid arguments");
				break;
			default:
				errorExit("Strange error while loading arguments");
				break;
		}
	}
}


void errorExit(char* msg)
{
	fprintf(stderr,"ERROR: %s\n", msg);
	exit(1);	
}