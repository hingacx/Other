Type in "make" in terminal to have everything compiled as needed.

Program Steps:
1. Generate an encryption key with keygen.c, this will be used to encrypt and decrypt text.
2. Run ./enc_server [RANDOM PORT 50000+] to get the server up and running.
3. Run ./enc_client [TEXT TO ENCRYPT] [KEY TEXT GENERATED FROM STEP 1] [PORT THAT ENC_SERVER IS ON]
4. Run ./dec_server [RANDOM PORT 50000+] to get the server up and running.
5. Run ./dec_client [TEXT TO DECRYPT] [KEY TEXT GENERATED FROM STEP 1] [PORT THAT DEC_SERVER IS ON]
