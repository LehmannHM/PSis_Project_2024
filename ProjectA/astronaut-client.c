#include <ncurses.h>
#include "remote-char.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
 #include <ctype.h> 
 #include <stdlib.h>

#include <assert.h>
#include <zmq.h>

#include <string.h>

 

int main()
{

    // new
    void *context = zmq_ctx_new ();
    void *requester = zmq_socket (context, ZMQ_REQ);
    zmq_connect (requester, "ipc:///tmp/s1");

    //TODO_5
    // read the character from the user
    char ch;
    do{
        printf("what is your character(a..z)?: ");
        ch = getchar();
        ch = tolower(ch);  
    }while(!isalpha(ch));


    // TODO_6
    // send connection message
    remote_char_t m;
    m.msg_type = 0;
    m.ch = ch;



    //s_send(requester,m);
    zmq_send (requester, &m, sizeof(m), 0);

	initscr();			/* Start curses mode 		*/
	cbreak();				/* Line buffering disabled	*/
	keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
	noecho();			/* Don't echo() while we do getch */

    int n = 0;

    //TODO_9
    // prepare the movement message
    m.msg_type = 1;
    m.ch = ch;
    
    int key;
    char evita_prob[3];

    do
    {
        //poder continuar a mandar msg
        //remote_char_t *evita_prob = s_recv(requester);  // mais a frente?
        zmq_recv (requester, &evita_prob, strlen("ok"), 0);


    	key = getch();		
        n++;
        switch (key)
        {
        case KEY_LEFT:
            mvprintw(0,0,"%d Left arrow is pressed", n);
            //TODO_9
            // prepare the movement message
           m.direction = LEFT;
            break;
        case KEY_RIGHT:
            mvprintw(0,0,"%d Right arrow is pressed", n);
            //TODO_9
            // prepare the movement message
            m.direction = RIGHT;
            break;
        case KEY_DOWN:
            mvprintw(0,0,"%d Down arrow is pressed", n);
            //TODO_9
            // prepare the movement message
           m.direction = DOWN;
            break;
        case KEY_UP:
            mvprintw(0,0,"%d :Up arrow is pressed", n);
            //TODO_9
            // prepare the movement message
            m.direction = UP;
            break;

        case ' ':   // zapping the number of space is 32 btw (if we have to change)

        // send zapping message

        break;                       


        default:
            key = 'x'; 
            break;
        }

        //TODO_10
        //send the movement message
         if (key != 'x'){

            //s_send(requester,m);
            zmq_send (requester, &m, sizeof(m), 0);
            
        }
        refresh();			/* Print it on to the real screen */
    }while(key != 27 || key != 'q');     // 27 correspond to esc button add when key different from 'q' too? instead of above?

    // send disconnect message

    mvprintw(0,0,"%d :disconnect button is pressed", n);
    
    
  	endwin();			/* End curses mode		  */

	return 0;
}