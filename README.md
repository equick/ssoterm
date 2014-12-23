#SSOTERM

This is a menu wrapper for logging into remote hosts without having to repeatedly enter a password. It's aimed at administrators who manage large estates and designed to be easy to configure. The user is prompted for their password once at startup and this is stored in the process memory, but never revealed in ps ouput.

![screenshot](http://i.imgur.com/p1HhoFN.png?1)

###DEPENDENCIES

sshpass is required. This can be downloaded from http://sshpass.sourceforge.net/
ssoterm expects this to exists at /usr/bin/sshpass

###INSTALL

Run the following command to compile:

  `gcc ssoterm.c -o ssoterm -lmenu -lncurses`

###RUN

In a terminal type:

  `./ssoterm`

This will prompt for your password which is used to log in to all hosts.
Navigate in the menu to the host you want to connect to.

###CONFIGURATION

Example menus are provided as shown below and can be named as required. 
The top level file called main is the only file that must stay the same.

```
main
-hostgroup1
   -hostgroup1_dev
   +hostgroup1_uat
   +hostgroup1_prod
   +hostgroup1_dr
+hostgroup1
+hostgroup1
```

###SUPPORT

If there are problems running ssoterm, compile with -DDEBUG as follows:

  `gcc -DDEBUG  ssoterm.c -o ssoterm -lmenu -lncurses`

This will create debug.log which should help to see where it's going wrong.

###TODO

*Add functionality for sudo and powerbroker

*Add functionality to log in via a jumpbox
