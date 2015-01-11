#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <curses.h>
#include <menu.h>
#include <fcntl.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define MAXDEPTH 5
#define MAXITEMS 50
#define SUDO 1
#define PBRUN 2

#ifdef DEBUG
 #define D if(1) 
#else
 #define D if(0) 
#endif

char *menuitems[MAXITEMS];
char *menufiles[MAXITEMS];
char *hosts[MAXITEMS];
char *users[MAXITEMS];
char *jumps[MAXITEMS];
int privtypes[MAXITEMS];

int hostlist=0;
char errormsg[200];
char mypass[20];	
char myuser[20];


int debug_first_call=1;
int depth=0;
int maxdepth=5;
char *mtitle[MAXDEPTH];
char *mfname[MAXDEPTH];
char cwd[1024];

FILE *fpdebug;

ITEM **my_items;
MENU *my_menu;
WINDOW *my_menu_win;

int get_items(const char *mfile);
void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color);
void print_menu(const char *subtitle);
void init_curses();
void get_menutype(char* line);
void finish();
int process_menu(const char *title, const char *fname);
char *strstrip(char *s);
int ssopass(char *user,char *host, char *pass, int privtype, char *jump);
char *get_privtype(int type);
void debug_(char *orig, char *msg);

char msg[200];


int main(){

	D debug_("main", "");

	printf("User: ");
	fgets (myuser, sizeof(myuser), stdin);
	myuser[strlen(myuser)-1]='\0';
	
	sprintf(mypass,"%s",getpass("Password: "));
	//mypass[strlen(mypass)-1]='\0';

	D sprintf(msg,"user=%s. password=%s.\n",myuser,mypass);
	D debug_("main", msg);

	int i=0;

        while(i<maxdepth){
                mtitle[i] = malloc( 30 * sizeof *mtitle[i] );
                mfname[i] = malloc( 30 * sizeof *mfname[i] );
                i++;
        }
        i=0;

	D debug_("main:", "Adding Top Level Menu and Filename");
	strcpy(mtitle[depth],"Menu");	
	strcpy(mfname[depth],"main");	

	D debug_("main:", "Starting Menu");
	while(1){
		process_menu(mtitle[depth],mfname[depth]);
	}

	D fclose(fpdebug);
}

int process_menu(const char *title, const char *fname){

	int c, i, idx, rc;
	char selected_item[50];
	char menufile[50];
	char user[20];
	char host[20];
	char jump[20];
	int privtype;
	char cmd[5];

	init_curses();

	D debug_("process_menu", "");
	D sprintf(msg,"func args: title=%s fname=%s",title,fname);
	D debug_("process_menu", msg);

	//return to previous menu if a selected item does not exist
	if(!get_items(fname)){
		depth--;
		D debug_("process_menu", "returning");
		return 0;
	}


	D debug_("process_menu", "call print_menu");
	print_menu(title);

	D debug_("process_menu", "check which key is pressed");
	while((c = wgetch(my_menu_win)) != 113 ) /* q to quit*/
	{       switch(c)
	        {	case KEY_DOWN:
				D debug_("process_menu","key down selected");
				menu_driver(my_menu, REQ_DOWN_ITEM);
				break;
			case KEY_UP:
				D debug_("process_menu","key up selected");
				menu_driver(my_menu, REQ_UP_ITEM);
				break;
			case KEY_NPAGE:
				D debug_("process_menu","page down selected");
				menu_driver(my_menu, REQ_SCR_DPAGE);
				break;
			case KEY_PPAGE:
				D debug_("process_menu","page up selected");
				menu_driver(my_menu, REQ_SCR_UPAGE);
				break;
			case 98: /* back */
				if(depth>0) depth--;
				D sprintf(msg,"back key pressed: depth=%d\n",depth);
				D debug_("process_menu",msg);
				return 0;
			case 10: /* Enter */
				strcpy(selected_item,item_name(current_item(my_menu)));
				D debug_("process_menu","enter key pressed");
				D sprintf(msg,"selected_item=%s hostlist=%d mfname=%s depth=%d",selected_item,hostlist,mfname[depth],depth);
				D debug_("process_menu",msg);

				idx=item_index(current_item(my_menu));

				D sprintf(msg,"idx=%d",idx);
				D debug_("process_menu",msg);

				if(!strcmp(selected_item,"Exit")){
					D debug_("process_menu","Exit selected");
					finish();
					exit(0);
				}

				if(!strcmp(selected_item,"<<Back")){
					D debug_("process_menu","Back selected");
					if(depth>0) depth--;
					return 0;
                                }

				//print child menu
				if(!hostlist){
					D debug_("process_menu","printing child menu");	
					strcpy(menufile,menufiles[idx]);

					D sprintf(msg,"selected_item=%s, menufile=%s\n",selected_item,menufile);
					D debug_("process_menu",msg);

					depth++;
                                        strcpy(mtitle[depth],selected_item);
                                        strcpy(mfname[depth],menufile);
					finish();

					D sprintf(msg,"mname=%s depth=%d\n",mfname[depth],depth);
                                        D debug_("process_menu",msg);

					return process_menu(mtitle[depth],mfname[depth]);
				}

				//run ssh user@host
				if(hostlist){

					D debug_("process_menu","running ssh");
					strcpy(host,hosts[idx]);
					strcpy(user,users[idx]);
					privtype=privtypes[idx];
					int status;
					finish();

					D sprintf(msg,"host=%s user=%s privtype=%d",host,user,privtype);
					D debug_("process_menu",msg);

					if (jumps[idx]!=NULL){
						strcpy(jump,jumps[idx]);
						status = ssopass(user,host,mypass,privtype,jump);

						D debug_("process_menu","using a jump host");
						D sprintf(msg,"status=%d jump=%s",status,jump);
						D debug_("process_menu",msg);

					}else{
						status = ssopass(user,host,mypass,privtype,NULL);

						D sprintf(msg,"status=%d",status);
                                                D debug_("process_menu",msg);

					}

					if(status!=0) {
						sprintf(errormsg,"Error connecting to %s",host);
						D debug_("process_menu",errormsg);
						
					}

					return status;
				}

                                break;
		}
                wrefresh(my_menu_win);
	}	

	finish();
	exit(0);
}


//Initialize curses
void init_curses(){
	D debug_("init_curses","");
        initscr();
        start_color();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_CYAN, COLOR_BLACK);
}

void print_menu(const char *subtitle){
	int i;
	TEXT j;
	char title[50]="SSOTERM";

	D debug_("print_menu","");

	strcat(title," - ");
	strcat(title,subtitle);

	sprintf(msg,"subtitle=%s title=%s",subtitle, title);	
	D debug_("print_menu", msg);
	
	// Create menu
        my_menu = new_menu((ITEM **)my_items);


        // Create the window to be associated with the menu
        my_menu_win = newwin(10, 50, 4, 4);
        keypad(my_menu_win, TRUE);

        // Set main window and sub window
        set_menu_win(my_menu, my_menu_win);
        set_menu_sub(my_menu, derwin(my_menu_win, 6, 38, 3, 1));
        set_menu_format(my_menu, 5, 1);

        // Set menu mark to the string " * "
        set_menu_mark(my_menu, " * ");

        // Print a border around the main window and print a title */
        box(my_menu_win, 0, 0);
        print_in_middle(my_menu_win, 1, 0, 50, title, COLOR_PAIR(1));
        mvwaddch(my_menu_win, 2, 0, ACS_LTEE);
        mvwhline(my_menu_win, 2, 1, ACS_HLINE, 48);
        mvwaddch(my_menu_win, 2, 49, ACS_RTEE);

        // Post the menu
        post_menu(my_menu);

        wrefresh(my_menu_win);

	//display an error message if set
	move(LINES-3, 0);
        clrtoeol();
	if(errormsg!=NULL && errormsg[0]!=0){
		D fprintf(fpdebug,"print_menu: errormsg: %s\n",errormsg);
        	attron(COLOR_PAIR(1));
        	mvprintw(LINES - 3, 0, errormsg);
        	attroff(COLOR_PAIR(1));
		memset(errormsg, 0, sizeof(errormsg));
	}

        attron(COLOR_PAIR(2));
        mvprintw(LINES - 2, 0, "Use PageUp and PageDown to scoll down or up a page of items");
        mvprintw(LINES - 1, 0, "Arrow Keys to navigate (q to Exit, b to to back)");
        attroff(COLOR_PAIR(2));
        refresh();

}

void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color){
	int length, x, y;
	float temp;

	D debug_("print_in_middle","");

	if(win == NULL)
		win = stdscr;
	getyx(win, y, x);
	if(startx != 0)
		x = startx;
	if(starty != 0)
		y = starty;
	if(width == 0)
		width = 80;

	length = strlen(string);
	temp = (width - length)/ 2;
	x = startx + (int)temp;
	wattron(win, color);

	sprintf(msg,"coordinates: length=%d,starty=%d,y=%d,startx=%d,x=%d,string=%s\n",length,starty,y,startx,x,string);
	D debug_("print_in_middle",msg);
	mvwprintw(win, y, x, "%s", string);
	wattroff(win, color);
	refresh();
}

int get_items(const char *mfile){
	char line[100];
        int i=0;
        int c;
	int item_count;
	char *token=NULL;
	const char *delim=";";
	char ftemp[200];

	D debug_("get_items","");

	sprintf(ftemp,"%s/%s",cwd,mfile);

        FILE *fp;
        fp=fopen(ftemp,"r");

        if(fp==NULL) {
		strcat(errormsg,"File not found: ");
		strcat(errormsg,ftemp);
		D debug_("get_items",errormsg);
		return 0;
        }

	// Create up to maxitems (default 50) in a menulist
        while(i<MAXITEMS){
                menuitems[i] = malloc( MAXITEMS * sizeof *menuitems[i] );
                menufiles[i] = malloc( MAXITEMS * sizeof *menufiles[i] );
                hosts[i] = malloc( MAXITEMS * sizeof *hosts[i] );
                users[i] = malloc( MAXITEMS * sizeof *users[i] );
                jumps[i] = malloc( MAXITEMS * sizeof *jumps[i] );
                i++;
        }

	//read in the menu file
	D debug_("get_items","reading in the menu file");
	i=0;
	int first=1;
	while(fgets(line,sizeof(line),fp)!=NULL){

		D sprintf(msg,"line=%s",line);
		D debug_("get_items",msg);

		//skip blank lines
		if (strlen(line) < 2){
			D debug_("get_items","skipping blank line");
			continue;
		}

		//first line should specify menu or hosts
		if(first){
			get_menutype(line);
			D hostlist?debug_("get_items","menu type=hostlist"):debug_("get_items","menu type=menu");
			first=0;
			continue;
		}

		// read delimited fields on a line
		int j=0;
		char *user_ptr=NULL;
		char parsed_user[20];
		memset(parsed_user,0,sizeof(parsed_user));
		int c=0;
		token=strtok(line, delim);
		char app_user[20];
		memset(app_user,0,sizeof(app_user));
		while(token!=NULL){

			D debug_("get_items","read token from line");

			token=strstrip(token);

			D sprintf(msg,"token number j=%d, line number i=%d token string=%s",j,i,token);
			D debug_("get_items",msg);

			switch(j){

				//get the first token (menu item)
				case 0:	
					D debug_("get_items","first token");
					strcpy(menuitems[i], token);
					if(hostlist)
						strcpy(hosts[i], token);	
					break;

				//get the second token (menufile or user account)
				case 1:
					D debug_("get_items","second token");
					if(hostlist){

						//get the user
						int n=0;
						while(n<=strlen(token) && !isspace(token[n])){
							app_user[n]=token[n];
							n++;	
						}
						strcpy(users[i],app_user);

						sprintf(menuitems[i],"%s - %s",hosts[i],token);

						D sprintf(msg,"app_user=%s menuitem=%s",app_user,menuitems[i]);
						D debug_("get_items",msg);

					}else{
						//if menu, store the menufile corresponding to a menu item
						strcpy(menufiles[i], token);
						D sprintf(msg,"storing menufile=%s",menufiles[i]);
						D debug_("get_items",msg);
                        		}
					break;
				
				//record whether this requires sudo or pbrun
				case 2:
					D debug_("get_items","third token");
					if(hostlist){
						if(!strcmp(token,"sudo")){
							privtypes[i]=SUDO;	
						}else{
							privtypes[i]=PBRUN;
						}
						D sprintf(msg,"privtype=%d 1 means sudo, 2 means pbrun",privtypes[i]);
						D debug_("get_items",msg);
					}
					break;

				//record if there's a jump host
                                case 3:
					D debug_("get_items","fourth token");
                                        if(hostlist){
						strcpy(jumps[i], token);
						D sprintf(msg,"jumphost=%s",jumps[i]);
						D debug_("get_items",msg);
					}
					break;

				default:
					break;

			}		
			
			j++;
			token=strtok(NULL, delim);
                }

		//if there's no hostlist specified for this host, ensure this is null
		if(hostlist && j<=3){
			jumps[i]=NULL;
			D sprintf(msg,"No jumphost found, setting to NULL");
			D debug_("get_items",msg);
		}

		i++;
	}

	if(!strcmp(mfile,"main")){
		strcpy(menuitems[i],"Exit");
	}else{
		strcpy(menuitems[i],"<<Back");
	}

	//store new_item for each item
	D debug_("get_items","storing menuitems");
	my_items = (ITEM **)calloc(MAXITEMS, sizeof(ITEM *));
	for(i=0; i < MAXITEMS; i++){
		if(strlen(menuitems[i])>1){
			my_items[i] = new_item(menuitems[i], "");

			D sprintf(msg,"storing menuitem i=%d menuitem %s",i,menuitems[i]);
                	D debug_("get_items",msg);
		}else{
			break;
		}
	}

        fclose(fp);
	return 1;
	
}

void get_menutype(char* line){

	D debug_("get_menutype","");

	line=strstrip(line);

        if (!strcmp(line,"MENU")){
		hostlist=0;
	}else{
		hostlist=1;
	}

}

//Unpost and free all the memory taken up
void finish(){
	int i;
	D debug_("finish","");
        unpost_menu(my_menu);
	free_menu(my_menu);
        for(i = 0; i < MAXITEMS; i++)
		free_item(my_items[i]);
        endwin();
}

//strip whitespace
char *strstrip(char *s)
{
	D debug_("strstrip","");

	size_t size;
	char *end;

	size = strlen(s);
	if (!size)
		return s;
	end = s + size - 1;
	while (end >= s && isspace(*end))
		end--;
	*(end + 1) = '\0';
	while (*s && isspace(*s))
		s++;

	return s;
}

//Write the password to a file descriptor (pseudoterminal) to be read from ssopass
int ssopass(char *user,char *host, char *pass, int privtype, char *jump){
	char cmd[100];
	char *ssopass="/usr/bin/ssopass";
	D debug_("ssopass","");

	if( access( ssopass, F_OK ) == -1 ) {
		fprintf(stderr,"Sorry unable to run. %s does not exist.\n",ssopass);			
		exit(1);
	}
	
	int fd=posix_openpt(O_RDWR);

	//write the password and newline character
        //write( fd, pass, strlen(pass)+1 );
        write( fd, pass, strlen(pass) );

	if(!strcmp(myuser,user)){
        	sprintf(cmd,"/usr/bin/ssopass -h %s -u %s -d %d",host,myuser,fd);
	}else{
		if(jump==NULL){
			sprintf(cmd,"/usr/bin/ssopass -h %s -u %s -d %d -s %s -t %s",host,myuser,fd,user,get_privtype(privtype));
		}else{
			sprintf(cmd,"/usr/bin/ssopass -h %s -u %s -d %d -s %s -t %s -j %s",host,myuser,fd,user,get_privtype(privtype),jump);
		}
	}			

	D sprintf(msg,"command=%s",cmd);
	D debug_("ssopass",msg);

        int status = system(cmd);
        close(fd);
	return status;
}

char *get_privtype(int type){
	D debug_("get_privtype","");
	return (type==SUDO) ? "sudo":"pbrun";
}

void debug_(char *orig, char *msg){
	if(debug_first_call){
		char debug_log[1024];
		getcwd(cwd, sizeof(cwd));
        	sprintf(debug_log,"%s/debug.log",cwd);
		fpdebug=fopen(debug_log, "w+");
		debug_first_call=0;
	}
	fprintf(fpdebug,"%s: %s\n",orig,msg);
	fflush(fpdebug);
	//memset(msg,0,sizeof(msg));
}
