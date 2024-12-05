#include <ncurses.h>
#include "common.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <ctype.h> 
#include <stdlib.h>


#include <assert.h>
#include <zmq.h>

#include <string.h>

int main ()
{
    const char *empty = "";

    // alterar cenas aqui
    void *context = zmq_ctx_new ();
    void *subscriber = zmq_socket (context, ZMQ_SUB);
    zmq_connect (subscriber, DISPLAY_ADDRESS);

    //
    zmq_setsockopt (subscriber, ZMQ_SUBSCRIBE,empty, 0);  
    //char password[] = "1";
    //zmq_setsockopt (subscriber, ZMQ_SUBSCRIBE,password, strlen(password));

	initscr();		    	
	cbreak();				
    keypad(stdscr, TRUE);   
	noecho();			    

    /* creates a window and draws a border */
    WINDOW * my_win = newwin(FIELD_SIZE, FIELD_SIZE, 0, 0);

    WINDOW * my_win_2 = newwin(15, 30, 0, FIELD_SIZE+2);
    box(my_win, 0 , 0);	
    box(my_win_2, 0 , 0);
	wrefresh(my_win);
    wrefresh(my_win_2);

    game_state current_state;

    int i,j;

    char letters[8] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};

    
    //char pass_recv [1];

    while (1)
    {
        
        // receive message

        //zmq_recv (subscriber, &pass_recv, strlen(password), 0);
        zmq_recv (subscriber, &current_state, sizeof(current_state), 0);  // msg type

        // update game screen
        for (i = 0; i < FIELD_SIZE; i++) {
        
            for (j = 0; j < FIELD_SIZE; j++) {
                wmove(my_win, j, i);
                waddch(my_win,current_state.game_field[i][j]| A_BOLD);
                
            }
        }

        // update score screen 
        mvwprintw(my_win_2, 1, 1, "SCORE");
    
        for (i = 0; i < MAX_PLAYERS; i++){

            mvwprintw(my_win_2, i+2, 1, "%c - %d",letters[i], current_state.astronaut_scores[i]);              
            
        }  

        // refresh screen
        box(my_win, 0 , 0);
        wrefresh(my_win);
        wrefresh(my_win_2); 
    }

  	endwin();			/* End curses mode		  */

    zmq_close (subscriber);
    zmq_ctx_destroy (context);
    return 0;

}