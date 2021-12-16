#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()


/* Creates a address struct */
void address_setup(struct sockaddr_in* address, int portNumber) {
 
  // Clear out the address struct.
  memset((char*) address, '\0', sizeof(*address)); 
  // The address should be network capable.
  address->sin_family = AF_INET;
  // Store the port number.
  address->sin_port = htons(portNumber);
  // Get the DNS entry for this host name.
  struct hostent* hostInfo = gethostbyname("localhost"); 
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
}


/* Start of the main program enc_client */
int main(int argc, char *argv[]) {

  if (argc < 4) { 
    fprintf(stderr, "Missing Arguments: plaintext, key, port.\n"); 
    exit(0); 
  } 

  /* Initialization of a bunch of variables. */
  char ch; 
  int len = 0, pt_count = 0, key_count = 0, socketFD, chars_written, chars_read;
  // plaintext and keytext buffers hold the chars from the input files.
  // buff_size is to send the num of chars the plaintext to recv() to the server.
  // authenticate holds a small message to verify we're connecting to the correct server.
  char plaintext[100000], keytext[100000], buff_size[10], authenticate[5], cs_check[4] = "enc";

  // Opens the plaintext file.
  FILE *plaintext_fp = fopen(argv[1], "r");
  if (plaintext_fp == NULL) {
  fprintf(stderr, "Something is wrong with the plaintext file, argv[1]\n");
  exit(1);
  }

  // Reads each char until we get to the end of the file marker.
  while ((ch = getc(plaintext_fp)) != EOF) {
    if ((ch >= 65 && ch <= 90) || (ch == 32)) {

      plaintext[len++] = ch;
    // We reached end of the file, break out. 10 = '\'
    } else if (ch == 10) {
      break;

    // Bad characters detected in the plaintext file.
    } else {
      fprintf(stderr, "Bad character(s) detected in plaintext.\n");
      exit(1);
    }
  }
  
  // Update the plaintext char count.
  pt_count = strlen(plaintext);
  // Close the plain text file.
  fclose(plaintext_fp);

  //Reinitialize len for reuse.
  len = 0;
  // Opens the key file.
  FILE *key_fp = fopen(argv[2], "r");
  if (key_fp == NULL) {
    fprintf(stderr, "Something is wrong with the keytext file, argv[2]\n");
    exit(1);
  }

  // Reads each char until we get to the end of the file marker.
  while ((ch = getc(key_fp)) != EOF) {
    if ((ch >= 65 && ch <= 90) || (ch == 32)) {
      keytext[len++] = ch;
      key_count++;

    // We reached end of the file, break out. 10 = '\'
    } else if (ch == 10) {
      break;

    // Bad characters detected in the plaintext file.
    } else {
      fprintf(stderr, "Bad character(s) detected in key file.\n");
      exit(1);
    }
  }

  // Close the key file.
  fclose(key_fp);

  // Checking to see if the key file is large enough to encrypt.
  if (pt_count > key_count) {
    fprintf(stderr, "The key file isn't large enough, submit another key file.\n");
    exit(1);
  }

  // Shortens the keytext to be the same length as the plaintext. This is so we
  // have 1:1 encryptions and to shorten the amount of data sent to server.
  if (key_count > pt_count) {
    // Temp buffer to hold part of the original keytext data.
    char temp[pt_count];
    strncpy(temp, keytext, pt_count);
    // Clear out key text and copy the keytext data back to the original keytext buffer where
    // length of keytext = length of plaintext.
    memset(keytext, '\0', 100000);
    strcpy(keytext, temp);
  }

  // Create a socket to connect to the server.
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0){
    fprintf(stderr, "CLIENT: ERROR opening socket..\n");
    exit(1);
  }

  /* Create and initialize an address struct */
  struct sockaddr_in server_address;
  address_setup(&server_address, atoi(argv[3]));

  // Attempt a connnection to the server.
  if (connect(socketFD, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
    fprintf(stderr, "CLIENT: ERROR connecting\n");
    exit(1);
  }

  /* First the client sents an authentication message to the server. If the correct server
     is being connected to, then a valid authentication response will be sent back. */
  int client_check = send(socketFD, cs_check, 4, 0);
  // Something went wrong with writing to socket..
  if (client_check < 0) {
    fprintf(stderr, "CLIENT: ERROR writing to socket indentifier\n");
    exit(1);
  }

  // Receives the authentication response from the server. It will either be "true" or "fals".
  int server_check = recv(socketFD, authenticate, 5, 0);
  if (server_check < 0) {
    fprintf(stderr, "CLIENT: ERROR with server check\n");
    exit(1);
  }

  // Kill the connection to the server as it's not authenticated.
  if (strncmp(authenticate, "fals", 4) == 0) {
    fprintf(stderr, "CLIENT: ERROR on port: %s exit(2)\n", argv[3]);
    exit(2);
  }

  // Convert the length of plaintext to a string so we can tell the server the length.
  memset(buff_size, '\0', sizeof(buff_size));
  sprintf(buff_size, "%d", pt_count);

  // Sending the buffer lengths of plaintext and keytext to the server.
  chars_written = send(socketFD, buff_size, strlen(buff_size), 0);
  if (chars_written < 0) {
      fprintf(stderr, "CLIENT: ERROR sendingg size of buffer to server.\n");
      exit(1);
  }

  // Reinitialize len and chars_written to send the plaintext data.
  chars_written = 0;
  len = 0;
  while (chars_written < pt_count) {
    len = send(socketFD, plaintext, strlen(plaintext) , 0);
    if (len < 0) {
      perror("Error: ");
      fprintf(stderr, "CLIENT: ERROR sending plaintext to server.\n");
      exit(1);
    }
    chars_written += len;
    len = 0;
  }

  // Reinitialize len and chars_written to send the keytext data.
  chars_written = 0;
  len = 0;
  while (chars_written < pt_count) {
    len = send(socketFD, keytext, strlen(keytext), 0);
    if (len < 0) {
      fprintf(stderr, "CLIENT: ERROR sending keytext to server.\n");
      exit(1);
    }
    chars_written += len;
    len = 0;
  }

  // Prepare the input buffer which will hold the encrypted text.
  char input[pt_count+1];
  memset(input, '\0', sizeof(input));

  // Receive the now enrypted plaintext back from the server.
  chars_read = recv(socketFD, input, sizeof(input)-1, MSG_WAITALL);
  if (chars_read < 0) {
    fprintf(stderr, "CLIENT: ERROR receiving ciphertext from server\n");
    exit(1);
  }

  // Add a newline char back on.
  input[pt_count] = '\n';
  printf("%s", input);

  // Close the socket.
  close(socketFD);

  return 0;
}