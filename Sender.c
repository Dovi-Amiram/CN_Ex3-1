#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#define SERVER_PORT 5060
#define SERVER_IP_ADDRESS "127.0.0.1"
#define FILE_TO_SEND "1mb.txt"
#define BUFFER_SIZE 16


int main() {
    
    //open a file
    FILE *f = fopen(FILE_TO_SEND, "r");
    //checking
    if (f == NULL){
        fprintf(stderr, "Error opening file");
        return 1;
    }
    //calculate the file size
    fseek(f, 0, SEEK_END);
    int fileSize = (int) ftell(f);
    fseek(f, 0, SEEK_SET);

    //put the file in chat array
    char filedata[fileSize];
    fread(filedata, sizeof(char), fileSize, f);
    
    //close the file
    fclose(f);

    //open a socket
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //checking
    if (sock == -1) {
        printf("Could not create socket : %d", errno);
        return -1;
    } else printf("New socket opened\n");

    /*
    "sockaddr_in" is the "derived" from sockaddr structure
    used for IPv4 communication. For IPv6, use sockaddr_in6
    */
    struct sockaddr_in serverAddress;

    /*
    fills the first 'sizeof(serverAddress)' bytes of the memory 
    area pointed to by &serverAddress with the constant 0.
    */
    memset(&serverAddress, 0, sizeof(serverAddress));

    //address family, AF_INET(while using TCP) undighned 
    serverAddress.sin_family = AF_INET;
    //change the port number from little endian => network endian(big endian): 
    serverAddress.sin_port = htons(SERVER_PORT);
    //convert IPv4 and IPv6 addresses from text to binary form
    int rval = inet_pton(AF_INET, (const char *) SERVER_IP_ADDRESS, &serverAddress.sin_addr);
    //checking
    if (rval <= 0){
        printf("inet_pton() failed");
        return -1;
    }

    // Make a connection to the server with socket SendingSocket.
    int connectResult = connect(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    //checking
    if (connectResult == -1){
        printf("connect() failed with error code : %d", errno);
        close(sock);
        return -1;
    }else printf("connected to server\n");

    int dummy = 0;//agreed sign
    
    send(sock, &fileSize, sizeof(int), 0); //send the filesize to the receiver
    recv(sock, &dummy, sizeof(int), 0);//waiting for receiver to get the agreed sign
    
    while (1)
    {
        char cc_algo[BUFFER_SIZE];//char array for changing the algorithem
        strcpy(cc_algo, "cubic");//copy the string "cubic" into cc_algo
        socklen_t len = strlen(cc_algo);//saving the size of the str in the cc_algo in socklen_t variable
        //checking & defult the cubic algorithem
        if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, cc_algo, len) == -1){
            perror("setsockopt");
            return -1;
        }

        printf("Sending part A using cubic CC algorithm\n");

        long bytesSent = -1;//present the number of bytes that send

        //send the first half of the file
        bytesSent = send(sock, filedata, (fileSize/2), 0);
        //checking
        if (bytesSent == -1) printf("send() failed with error code : %d", errno);
        else if (bytesSent == 0) printf("peer has closed the TCP connection prior to send().\n");
        else if (bytesSent < (fileSize/2)) printf("sent only %ld bytes from the required %d.\n", bytesSent, (fileSize/2));
        else printf("Sent %ld bytes\n", bytesSent);
        
        int sender_xor = 2421 ^ 7494; //the sender xor for the authentication
        int receiver_xor;//the xor that the sender get from the receiver

        //receiv the xor from the receiver
        recv(sock, &receiver_xor, sizeof(int), 0);
        //checking
        if (receiver_xor == sender_xor) printf("Part A was successfully sent\n");
        else printf("client: something went wrong\n");

        strcpy(cc_algo, "reno");//copy the string "reno" into cc_algo
        len = strlen(cc_algo);//saving the size of the str in the cc_algo in socklen_t variable
        //checking & defult the cubic algorithem
        if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, cc_algo, len) != 0){
            perror("setsockopt");
            return -1;
        }

        printf("Sending part B using reno CC algorithm\n");

        //send the second half of the file
        bytesSent = send(sock, filedata+(fileSize/2)-1, (fileSize/2), 0);
        //checking
        if (bytesSent == -1) printf("send() failed with error code : %d", errno);
        else if (bytesSent == 0) printf("peer has closed the TCP connection prior to send().\n");
        else if (bytesSent < (fileSize/2)) printf("sent only %ld bytes from the required %d.\n", bytesSent, (fileSize/2));
        else printf("Sent %ld bytes\n", bytesSent);
        
        printf("Part B was successfully sent\n");
        
        char choose; //the user decision
        printf("Send again? Y = yes, N = no\n");
        scanf(" %c", &choose);

        send(sock, &dummy, sizeof(int), 0);//sending the receiver the agreed sign

        //if the user want to close the program --> close the socket and break
        if (choose == 'N'){
            close(sock);
            break;
        }
        

    }
    
    return 0;
}

