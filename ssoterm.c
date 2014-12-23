#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <curses.h>
#include <menu.h>
#include <fcntl.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define MAXDEPTH 5
#define MAXITEMS 50

#ifdef DEBUG
 #define D if(1) 
#else
 #define D if(0) 
#endif

char *menuitems1[MAXITEMS];
char *menuitems2[MAXITEMS];
char *menufiles[MAXITEMS];
char *hosts[MAXITEMS];
char *users[MAXITEMS];

char mtype[10];
char errormsg[200];
char mypass[20];	

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
void read_config(FILE* fp);
void finish();
int process_menu(const char *title, const char *fname);
char *strstrip(char *s);
int sshpass(char *user,char *host, char *pass);


int main(){

	char debug_log[1024];
	getcwd(cwd, sizeof(cwd));
	
	sprintf(debug_log,"%s/debug.log",cwd);
	
	D fpdebug=fopen(debug_log, "w+");
	fprintf(fpdebug,"cwd=%s\n",cwd);
	fflush(fpdebug);
	sprintf(mypass,"%s\n",getpass("Password: "));

	int i=0;

        while(i<maxdepth){
                mtitle[i] = malloc( 30 * sizeof *mtitle[i] );
                mfname[i] = malloc( 30 * sizeof *mfname[i] );
                i++;
        }
        i=0;

	strcpy(mtitle[depth],"Menu");	
	strcpy(mfname[depth],"main");	

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
	char cmd[5];

	init_curses();

	D fprintf(fpdebug,"process_menu\n");
	D fprintf(fpdebug,"process_menu: title=%s fname=%s\n",title,fname);

	//return to previous menu if a selected item does not exist
	if(!get_items(fname)){
		depth--;
		return 0;
	}

	print_menu(title);

	while((c = wgetch(my_menu_win)) != 113 ) /* q to quit*/
	{       switch(c)
	        {	case KEY_DOWN:
				menu_driver(my_menu, REQ_DOWN_ITEM);
				break;
			case KEY_UP:
				menu_driver(my_menu, REQ_UP_ITEM);
				break;
			case KEY_NPAGE:
				menu_driver(my_menu, REQ_SCR_DPAGE);
				break;
			case KEY_PPAGE:
				menu_driver(my_menu, REQ_SCR_UPAGE);
				break;
			case 98: /* back */
				if(depth>0) depth--;
				D fprintf(fpdebug,"process_menu: back: depth=%d\n",depth);
				return 0;
			case 10: /* Enter */
				strcpy(selected_item,item_name(current_item(my_menu)));
				D fprintf(fpdebug,"process_menu: selected_item=%s mtype=%s mfname=%s depth=%d\n",selected_item,mtype,mfname[depth],depth);
				idx=item_index(current_item(my_menu));

				if(!strcmp(selected_item,"Exit")){
					finish();
					exit(0);
				}

				if(!strcmp(selected_item,"<<Back")){
					if(depth>0) depth--;
					return 0;
                                }

				//print a submenu
				if(!strcmp(mtype,"menu")){
					strcpy(menufile,menufiles[idx]);
					D fprintf(fpdebug,"process_menu: selected_item=%s, menufile=%s\n",selected_item,menufile);
					depth++;
                                        strcpy(mtitle[depth],selected_item);
                                        strcpy(mfname[depth],menufile);
					finish();

					D fprintf(fpdebug,"process_menu: mname=%s depth=%d\n",mfname[depth],depth);
					return process_menu(mtitle[depth],mfname[depth]);
				}

				//run ssh user@host
				if(!strcmp(mtype,"sshpass")){
					strcpy(host,hosts[idx]);
					strcpy(user,users[idx]);
					D fprintf(fpdebug,"process_menu: host=%s user=%s\n",host,user);
					finish();
					int status = sshpass(user,host,mypass);
					if(status!=0) {
						sprintf(errormsg,"Error connecting to %s",host);
					}
					D fprintf(fpdebug,"process_menu: cmd status=%d\n",status);
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
	D fprintf(fpdebug,"init_curses\n");
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

	D fprintf(fpdebug,"print_menu\n");

	strcat(title," - ");
	strcat(title,subtitle);

	fflush(fpdebug);
	
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

	D fprintf(fpdebug,"print_menu: %s\n",title);

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

	D fprintf(fpdebug,"print_in_middle\n");

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

	D fprintf(fpdebug,"print_in_middle: length=%d,starty=%d,y=%d,startx=%d,x=%d,string=%s\n",length,starty,y,startx,x,string);
	mvwprintw(win, y, x, "%s", string);
	wattroff(win, color);
	refresh();
}

int get_items(const char *mfile){
	char item1[100];
        char item2[100];

        int i=0;
        int c;
	int item_count;
	char *token1, *token2;
	const char *delim=";";
	char ftemp[200];

	D fprintf(fpdebug,"get_items\n");

	fflush(fpdebug);

	sprintf(ftemp,"%s/%s",cwd,mfile);

        FILE *fp;
        fp=fopen(ftemp,"r");

        if(fp==NULL) {
		strcat(errormsg,"File not found: ");
		strcat(errormsg,ftemp);
                D fprintf(fpdebug,"get_items: %s\n",errormsg);
		return 0;
        }

	fflush(fpdebug);

	// Create up to maxitems (default 50) in a menulist
        while(i<MAXITEMS){
                menuitems1[i] = malloc( MAXITEMS * sizeof *menuitems1[i] );
                menuitems2[i] = malloc( MAXITEMS * sizeof *menuitems2[i] );
                menufiles[i] = malloc( MAXITEMS * sizeof *menufiles[i] );
                hosts[i] = malloc( MAXITEMS * sizeof *hosts[i] );
                users[i] = malloc( MAXITEMS * sizeof *users[i] );
                i++;
        }

	fflush(fpdebug);
	read_config(fp);

	i=0;
	while(fgets(item1,sizeof(item1),fp)!=NULL){

		//skip blank lines
		if (strlen(item1) < 2){
			continue;
		}

		//get the first token (menu item)
		token1=(strtok(item1, delim));
                D fprintf(fpdebug,"get_items: i=%d token1 %s\n",i,token1);
		strcpy(menuitems1[i], token1);

		//get the second token (menufile or user account)
		token2 = strstrip(strtok(NULL, delim));
                D fprintf(fpdebug,"get_items: i=%d token2 %s\n",i,token2);

		//if menu, store the menufile corresponding to a menu item
		if(!strcmp(mtype,"menu")){
			if (token2!=NULL){
				strcpy(menufiles[i], token2);
			}
		}

		//if ssh, store the host and user
		if(!strcmp(mtype,"sshpass")){
                        if (token2!=NULL){
				D fprintf(fpdebug,"get_items: token2=%s.\n",token2);
                                strcpy(hosts[i], token1);
                                strcpy(users[i], token2);
				strcat(menuitems1[i]," - ");
				strcat(menuitems1[i],token2);
                        }
                }

		i++;
	}

	if(!strcmp(mfile,"main")){
		strcpy(menuitems1[i],"Exit");
	}else{
		strcpy(menuitems1[i],"<<Back");
	}

	//store new_item for each item
	my_items = (ITEM **)calloc(MAXITEMS, sizeof(ITEM *));
	for(i=0; i < MAXITEMS; i++){
                D fprintf(fpdebug,"get_items: i=%d menuitem %s\n",i,menuitems1[i]);
		my_items[i] = new_item(menuitems1[i], "");
	}

        fclose(fp);
	return 1;
	
}

void read_config(FILE* fp){
	char line[100];
	const char *delim=":";
	char *token;

	D fprintf(fpdebug,"read_config\n");
	fgets(line, sizeof(line), fp);
	token=strstrip(strtok(line, delim));
	D fprintf(fpdebug,"read_config: token=%s.\n",token);

        if (!strcmp(token,"type")){
		strcpy(mtype,strstrip(strtok(NULL, delim)));
		D fprintf(fpdebug,"read_config: mtype=%s.\n",mtype);
        }
}

//Unpost and free all the memory taken up
void finish(){
	int i;
	D fprintf(fpdebug,"finish\n");
        unpost_menu(my_menu);
	free_menu(my_menu);
        for(i = 0; i < MAXITEMS; i++)
		free_item(my_items[i]);
        endwin();
}

//strip whitespace
char *strstrip(char *s)
{
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

//Write the password to a file descriptor (pseudoterminal) to be read from sshpass
int sshpass(char *user,char *host, char *pass){
	char cmd[70];
	char *sshpass="/usr/bin/sshpass";
        D fprintf(fpdebug,"sshpass\n");
	if( access( sshpass, F_OK ) == -1 ) {
		fprintf(stderr,"Sorry unable to run. %s does not exist.\n",sshpass);			
		exit(1);
	}
	int fd=posix_openpt(O_RDWR);
        write( fd, pass, strlen(pass) );
        sprintf(cmd,"/usr/bin/sshpass -d%d ssh %s@%s", fd, user, host);
        D fprintf(fpdebug,"sshpass: %s\n",cmd);
        int status = system(cmd);
        D fprintf(fpdebug,"sshpass: status=%d\n",status);
        close(fd);
	return status;
}

