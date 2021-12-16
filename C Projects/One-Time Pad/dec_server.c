#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


/* Encrypts the ciphertext with the keytext and puts it in enc_text. */
void decrypt(char *dec_text, char *ciphertext, char *keytext) {
  // Int variables used to do the appropriate conversions.
  int temp1, temp2, temp3;
  for(int i = 0; i < strlen(ciphertext); i++) {
    // Decrements the chars' dec values to be 0 = 'A'.... 25 = 'Z' and 26 = SPACE.
    temp1 = ciphertext[i] - 65;
    temp2 = keytext[i] - 65;
    // Gives each SPACE char the temporary dec value of 26.
    if (ciphertext[i] == 32) {
        temp1 = 26;
    }

    if (keytext[i] == 32) {
        temp2 = 26;
    }

    // Temp intermediate of finding the difference between the chars and getting the MOD 27 value.
    temp3 = ((temp1 - temp2 + 27) % 27);
    
    // If temp3 is 26, change value to 32 = SPACE.
    if (temp3 == 26) {
        dec_text[i] = 32;
    }
    // Otherwise the value is temp3 + 65. 
    else {
        dec_text[i] = (temp3 + 65);
    }
  }
}


/* Set up the address struct for the server socket. */
void setupAddressStruct(struct sockaddr_in* address, int portNumber){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 
  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);
  // Allow a client at any address to connect to this server
  address->sin_addr.s_addr = INADDR_ANY;
}

/* Main, start of the enc_server. */
int main(int argc, char *argv[]){
  int connectFD, chars_read, chars_written, len;
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);

  // Check for the correct number of arguments.
  if (argc < 2) { 
    fprintf(stderr, "USAGE: %s port\n", argv[0]); 
    exit(1);
  } 
  
  // Create the socket that will listen for connections.
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    fprintf(stderr, "SERVER: ERROR opening socket\n");
    exit(1);
  }

  // Set up the address struct for the server socket.
  setupAddressStruct(&serverAddress, atoi(argv[1]));

  // Bind the listen socket with the ip address so that we can starting listening to requests.
  if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
    fprintf(stderr, "SERVER: ERROR on binding\n");
    exit(1);
  }

  // Start listening for connetions. Allow up to 5 connections to queue up.
  listen(listenSocket, 5); 
  
  // Accept a connection, blocking if one is not available until one connects.
  while(1) {

    // Accept the connection request which creates a connection socket
    connectFD = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 
    if (connectFD < 0){
      fprintf(stderr, "SERVER: ERROR on accept\n");
    }
    
    // Create a new child fork process so we can run multiple processes concurrently.
    int childStatus;
    pid_t childPid = fork();
    switch (childPid) {
        // Failed fork, something went horribly wrong.
        case -1:
            fprintf(stderr, "SERVER: ERROR fork child process.\n");
            exit(1);
            break;
        // Proceed to launch the child program as normal.
        case 0: {
            char cs_check[4], buffsize[10];
            char authenticate[5] = "fals";
            int client_check, input_size;
    
            // Receives the the identifer from the client (either enc or dec).
            int server_check = recv(connectFD, cs_check, 5, 0);
            if (server_check < 0) {
                fprintf(stderr, "SERVER: ERROR with client check\n");
            }

            // Checks to see if the correct client is trying to connect, which should only be dec.
            if (strncmp(cs_check, "dec", 3) == 0) {
                memset(authenticate, '\0', 5);
                strcpy(authenticate, "true");
            }

            /* Handles the case where the wrong client is trying to connect to the server and closes the socket. */
            if(strncmp(authenticate, "fals", 4) == 0) {
                client_check = send(connectFD, authenticate, 5, 0);
                if (client_check < 0) {
                    fprintf(stderr, "SERVER: ERROR writing to socket\n");
                }
                close(connectFD);
                break;
            }

            // Sends back the authentication from the server to the client.
            client_check = send(connectFD, authenticate, 5, 0);
            if (client_check < 0) {
            fprintf(stderr, "SERVER: ERROR writing to socket\n");
            }

            // Clear out the buffer for use.
            memset(buffsize,'\0', sizeof(buffsize));
            // Get the length of the incoming input from the client.
            chars_read = recv(connectFD, buffsize, sizeof(buffsize), 0);
            if (chars_read < 0) {
                fprintf(stderr, "SERVER: ERROR receiving size of input from client\n");
            }

            // Covert the received input size into a number from a string.
            input_size = atoi(buffsize);

            // Prepare ciphertext and keytext buffers to hold the incoming data.
            char ciphertext[input_size+1];
            memset(ciphertext, '\0', sizeof(ciphertext));
            char keytext[input_size+1];
            memset(keytext, '\0', sizeof(keytext));
            // Prepare encrypt text to hold plaint text after encryption.
            char plaintext[input_size+1];
            memset(plaintext, '\0', sizeof(plaintext));

            // Receive the incoming ciphertext data from client.
            chars_read = recv(connectFD, ciphertext, sizeof(ciphertext)-1, MSG_WAITALL);
            if (chars_read < 0) {
              fprintf(stderr, "SERVER: ERROR receiving ciphertext from client\n");
            }

            // Receive the incoming keytext data from client.
            chars_read = recv(connectFD, keytext, sizeof(keytext)-1, MSG_WAITALL);
            if (chars_read < 0) {
              fprintf(stderr, "SERVER: ERROR receiving keytext from client\n");
            }
            
            // Encrypts the ciphertext and puts the results in encrypt_text.
            decrypt(plaintext, ciphertext, keytext);

            // Sends back the now deciphered plaintext to the client. 
            chars_written = 0;
            len = 0;
            while (chars_written < input_size) {
              len = send(connectFD, plaintext, strlen(plaintext), 0);
              if (len < 0) {
                fprintf(stderr, "SERVER: ERROR sending plaintext to client.\n");
                exit(1);
              }
              chars_written += len;
              len = 0;
            }
            exit(0);
            break;
           }

        default:
          waitpid(childPid, &childStatus, WNOHANG);
    }
    // Close current connected socket.
    close(connectFD);

  }
  // Close the listening socket.
  close(listenSocket); 
  return 0;
}