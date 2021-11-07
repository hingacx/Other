/* DESCRIPTION: A project that implements a working version of a UNIX Shell. Not quite the same as UNIX or LINUX in terms of usability.
*
* NOTABLE IMPLEMENTATIONS: 
* - Built in Commands: exit, status, cd.
* - Other commands: Forked child processes.
* - "$$" is expanded to be the working pid of the shell whenever it's presented.
* - & at the end of a command line tells the program to run it as a background process.
* - Customized CTRL^C and CTRL^Z signals. Because of this, type in "exit", if you need to exit the program.
* - Supports commands lengths of 2048 characters and up to 512 arguments.
* - Can run up to 1000 child processes before needing to restart the program.
 */


I included a Makefile with the project. You can compile the code by typing "Make" in terminal.
If you want to do it manually in terminal you can type in "gcc --std=gnu99 -g -Wall -o smallsh smallsh.c"

Once the code is compiled, then you can run it by typing "./smallsh" in terminal.
