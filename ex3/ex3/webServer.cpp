#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <sys/stat.h>
#include <stdio.h>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <string>


struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	char buffer[2048];
	int len;
	string header;
	string body;
	time_t time;
};

const int HTTP_PORT = 8080;
const int MAX_SOCKETS = 60;
const int EMPTY = 0;
const int LISTEN  = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;

const string PATH = "C:\\temp\\";
const string STATUS_OK = "HTTP/1.1 200 OK";
const string STATUS_NO_CONTENT = "HTTP/1.1 204 No Content";
const string STATUS_ERROR = "HTTP/1.1 500 Internal Server Error";
const string STATUS_CREATED = "HTTP/1.1 201 Created";

bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);

void processReq(SocketState& socketState);
string getReqType(string header);
void doGetOrHead(SocketState& socketState, char* sendBuff, string reqType);
string getLang(string header);
string getFileName(string header);
void doPost(SocketState& socketState, char* sendBuff);
void doPut(SocketState& socketState, char* sendBuff, string& status);
void doOptions(char* sendBuff);
string extractFileContent(string fileName);
void doDelete(SocketState& socketState, string& status);
string setHeader(string body, string status,string add);
void createNewFile(string fullPath, string body);
void editFile(string fullPath, string body);
string getFileNameToCreate(string header);

struct SocketState sockets[MAX_SOCKETS]={0};
int socketsCount = 0;

void main() 
{
    // Initialize Winsock (Windows Sockets).

	// Create a WSADATA object called wsaData.
    // The WSADATA structure contains information about the Windows 
	// Sockets implementation.
	WSAData wsaData; 
    	
	// Call WSAStartup and return its value as an integer and check for errors.
	// The WSAStartup function initiates the use of WS2_32.DLL by a process.
	// First parameter is the version number 2.2.
	// The WSACleanup function destructs the use of WS2_32.DLL by a process.
	if (NO_ERROR != WSAStartup(MAKEWORD(2,2), &wsaData))
	{
        cout<<"Time Server: Error at WSAStartup()\n";
		return;
	}

	// Server side:
	// Create and bind a socket to an internet address.
	// Listen through the socket for incoming connections.

    // After initialization, a SOCKET object is ready to be instantiated.
	
	// Create a SOCKET object called listenSocket. 
	// For this application:	use the Internet address family (AF_INET), 
	//							streaming sockets (SOCK_STREAM), 
	//							and the TCP/IP protocol (IPPROTO_TCP).
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for errors to ensure that the socket is a valid socket.
	// Error detection is a key part of successful networking code. 
	// If the socket call fails, it returns INVALID_SOCKET. 
	// The if statement in the previous code is used to catch any errors that
	// may have occurred while creating the socket. WSAGetLastError returns an 
	// error number associated with the last error that occurred.
	if (INVALID_SOCKET == listenSocket)
	{
        cout<<"Time Server: Error at socket(): "<<WSAGetLastError()<<endl;
        WSACleanup();
        return;
	}

	// For a server to communicate on a network, it must bind the socket to 
	// a network address.

	// Need to assemble the required data for connection in sockaddr structure.

    // Create a sockaddr_in object called serverService. 
	sockaddr_in serverService;
	// Address family (must be AF_INET - Internet address family).
    serverService.sin_family = AF_INET; 
	// IP address. The sin_addr is a union (s_addr is a unsigned long 
	// (4 bytes) data type).
	// inet_addr (Iternet address) is used to convert a string (char *) 
	// into unsigned long.
	// The IP address is INADDR_ANY to accept connections on all interfaces.
	serverService.sin_addr.s_addr = INADDR_ANY;
	// IP Port. The htons (host to network - short) function converts an
	// unsigned short from host to TCP/IP network byte order 
	// (which is big-endian).
	serverService.sin_port = htons(HTTP_PORT);

	// Bind the socket for client's requests.

    // The bind function establishes a connection to a specified socket.
	// The function uses the socket handler, the sockaddr structure (which
	// defines properties of the desired connection) and the length of the
	// sockaddr structure (in bytes).
    if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR *) &serverService, sizeof(serverService))) 
	{
		cout<<"Time Server: Error at bind(): "<<WSAGetLastError()<<endl;
        closesocket(listenSocket);
		WSACleanup();
        return;
    }

    // Listen on the Socket for incoming connections.
	// This socket accepts only one connection (no pending connections 
	// from other clients). This sets the backlog parameter.
    if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "Time Server: Error at listen(): " << WSAGetLastError() << endl;
        closesocket(listenSocket);
		WSACleanup();
        return;
	}
	addSocket(listenSocket, LISTEN);

    // Accept connections and handles them one by one.
	while (true)
	{
		// The select function determines the status of one or more sockets,
		// waiting if necessary, to perform asynchronous I/O. Use fd_sets for
		// sets of handles for reading, writing and exceptions. select gets "timeout" for waiting
		// and still performing other operations (Use NULL for blocking). Finally,
		// select returns the number of descriptors which are ready for use (use FD_ISSET
		// macro to check which descriptor in each set is ready to be used).
		fd_set waitRecv;
		FD_ZERO(&waitRecv);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		//
		// Wait for interesting event.
		// Note: First argument is ignored. The fourth is for exceptions.
		// And as written above the last is a timeout, hence we are blocked if nothing happens.
		//
		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, NULL);
		if (nfd == SOCKET_ERROR)
		{
			cout <<"Time Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i);
					break;

				case RECEIVE:
					receiveMessage(i);
					break;
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				switch (sockets[i].send)
				{
				case SEND:
					sendMessage(i);
					break;
				}
			}
		}
	}

	// Closing connections and Winsock.
	cout << "Time Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}

bool addSocket(SOCKET id, int what)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			socketsCount++;
			return (true);
		}
	}
	return (false);
}

void removeSocket(int index)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	socketsCount--;
}

void acceptConnection(int index)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr *)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{ 
		 cout << "Time Server: Error at accept(): " << WSAGetLastError() << endl; 		 
		 return;
	}
	cout << "Time Server: Client "<<inet_ntoa(from.sin_addr)<<":"<<ntohs(from.sin_port)<<" is connected." << endl;

	//
	// Set the socket to be in non-blocking mode.
	//
	unsigned long flag=1;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
	{
		cout<<"Time Server: Error at ioctlsocket(): "<<WSAGetLastError()<<endl;
	}

	if (addSocket(msgSocket, RECEIVE) == false)
	{
		cout<<"\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

void receiveMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;

	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);
	
	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "Time Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);			
		removeSocket(index);
		return;
	}
	if (bytesRecv == 0)
	{
		closesocket(msgSocket);			
		removeSocket(index);
		return;
	}
	else
	{
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		cout<<"Time Server: Recieved: "<<bytesRecv<<" bytes of \""<<&sockets[index].buffer[len]<<"\" message.\n";
		
		sockets[index].len += bytesRecv;

		if (sockets[index].len > 0)
		{			
			processReq(sockets[index]);
			sockets[index].send  = SEND;
			memcpy(sockets[index].buffer, &sockets[index].buffer[bytesRecv], sockets[index].len - bytesRecv);
			sockets[index].len -= bytesRecv;			
		}
	}
}

void sendMessage(int index)
{
	int bytesSent = 0;
	char sendBuff[5000];
	SOCKET msgSocket = sockets[index].id;
	string reqType = getReqType(sockets[index].header);
	string status;

	if (reqType.compare("GET") == 0 || reqType.compare("HEAD"))
		 doGetOrHead(sockets[index], sendBuff, reqType);
	else if (reqType.compare("POST") == 0)
		doPost(sockets[index], sendBuff);
	else if (reqType.compare("OPTIONS") == 0)
		doOptions(sendBuff);
	else if (reqType.compare("PUT") == 0)
		doPut(sockets[index], sendBuff, status);
	/*else if (reqType.compare("TRACE") == 0)
		doTrace(sockets[index]);*/
	else if (reqType.compare("DELETE") == 0)
		doDelete(sockets[index],status);
	/*else if (reqType.compare("HEAD") == 0)
		doHead(sockets[index]);*/
	
	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "Time Server: Error at send(): " << WSAGetLastError() << endl;	
		return;
	}

	sockets[index].send = IDLE;
}

void processReq(SocketState& socketState)
{
	string buffer(socketState.buffer);
	int posSplit = buffer.find("\r\n\r\n") + 4;
	socketState.header = buffer.substr(0, posSplit);
	socketState.body = buffer.substr(posSplit);	
}

string getReqType(string header)
{
	return header.substr(0, header.find(' '));
}

void doGetOrHead(SocketState& socketState, char* sendBuff, string reqType)
{
	string lang, fileName;
	string htmlBody, header, fullRes;
	lang = getLang(socketState.header);
	fileName = getFileName(socketState.header);
	if (lang.compare("en") == 0)
		fileName.append("En.html");
	else if (lang.compare("he") == 0)
		fileName.append("He.html");
	else if (lang.compare("fr") == 0)
		fileName.append("Fr.html");
	
	if (reqType.compare("GET") == 0)
		htmlBody = extractFileContent(fileName);

	header = setHeader(htmlBody, STATUS_OK);
	fullRes.append(header);
	fullRes.append(htmlBody);
	fullRes.append("\0");
	sprintf(sendBuff, fullRes.c_str());

}

void doPut(SocketState& socketState, char* sendBuff, string& status)
{
	string htmlBody, header, fullRes;
	string fileName;
	fileName = getFileNameToCreate(socketState.header);
	struct stat buffer;
	string fullPath = PATH;
	fullPath.append(fileName);

	if (stat(fullPath.c_str(), &buffer) == 0)
	{
		editFile(fullPath, socketState.body);
		status = STATUS_OK;
	}
	
	else
	{
		createNewFile(fullPath, socketState.body);
		status = STATUS_CREATED;
	}

	htmlBody = "";
	header = setHeader(htmlBody, status,"");
	fullRes.append(header);
	fullRes.append(htmlBody);
	fullRes.append("\0");
	sprintf(sendBuff, fullRes.c_str());
}

void editFile(string fullPath, string body)
{
	FILE* file;
	file = fopen(fullPath.c_str(), "w");
	if (file == NULL) {
		printf("Error opening file!\n");
		return;
	}
	
	fwrite(body.c_str(), sizeof(char) ,body.size(), file);
	fclose(file);
}

void createNewFile(string fullPath, string body)
{
	FILE* file;
	file = fopen(fullPath.c_str(), "w");
	if (file == NULL) {
		printf("Error creating file!\n");
		return;
	}
	fprintf(file, body.c_str());
	fclose(file);
}

string getFileNameToCreate(string header)
{
	int startPos = header.find('/');
	int endPos = header.find(" ", startPos + 1);
	return header.substr(startPos + 1, endPos - startPos - 1);
}

string extractFileContent(string fileName)
{
	FILE* fp;
	char body[2048];
	long file_size;
	string fullPath = PATH;
	fullPath.append(fileName);

	
	fp = fopen(fullPath.c_str(), "r");
	if (fp == NULL) {
		cout << "Could not open file" << endl;
		return NULL;
	}

	/* Get the size of the file */
	fseek(fp, 0L, SEEK_END);
	file_size = ftell(fp);
	rewind(fp);

	/* Read the file into the buffer */
	size_t newLen = fread(body, sizeof(char), file_size, fp);
	if (ferror(fp) != 0) {
		fputs("Error reading file", stderr);
	}

	fclose(fp);
	body[newLen] = '\0';
	return body;

	
}

string getLang(string header)
{
	int pos = header.find("lang=");
	return header.substr(pos + 5, 2);
}

string getFileName(string header)
{
	int startPos = header.find('/');
	int endPos = header.find('.');
	return header.substr(startPos + 1, endPos - startPos - 1);

}

void doPost(SocketState& socketState, char* sendBuff) {
	cout << socketState.body << endl;
	sendBuff = NULL;
}

void doOptions(char* sendBuff) {
	string msg = "Allow: GET, HEAD, PUT, DELETE, TRACE, OPTIONS\r\n";
	strcpy(sendBuff, msg.c_str());
	strcat(sendBuff, "\n");
}

string setHeader(string body, string status,string add) 
{
	string header;
	header = status + "\r\n" + add +
		"Content-Type: text/html\r\n" +
		"Content-Length: " + to_string(body.length()) + "\r\n" +
		"Connection: keep-Alive" + "\r\n\r\n";
	return header;
}


void doDelete(SocketState& socketState, string& status)
{
	
	string fileName = getFileNameToCreate(socketState.header);
	if (remove((PATH + fileName).c_str())==0)
	{
		status = STATUS_OK;
	}
	else
	{
		status = STATUS_NO_CONTENT;
	}
}
