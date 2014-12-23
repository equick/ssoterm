==DEPENDENCIES==

sshpass is required. This can be downloaded from http://sshpass.sourceforge.net/
ssoterm expects this to exists at /usr/bin/sshpass

==INSTALL==

Run the following command to compile:

   gcc ssoterm.c -o ssoterm -lmenu -lncurses

==RUN==

In a terminal type:

   ./ssoterm

This will prompt for your password which is used to log in to all hosts.
Navigate in the menu to the host you want to connect to.


==SUPPORT==

If there are problems running ssoterm, compile with -DDEBUG as follows:

   gcc -DDEBUG  ssoterm.c -o ssoterm -lmenu -lncurses

This will create debug.log which should help to see where it's going wrong.
