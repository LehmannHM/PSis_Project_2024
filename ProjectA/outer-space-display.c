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
    
    void *context = zmq_ctx_new ();
    void *subscriber = zmq_socket (context, ZMQ_SUB);
    zmq_connect (subscriber, DISPLAY_ADDRESS);

    //
    zmq_setsockopt (subscriber, ZMQ_SUBSCRIBE,empty, 0);  

	initscr();		    	
	cbreak();				
    keypad(stdscr, TRUE);   
	noecho();			    

    /* creates a window and draws a border */
    WINDOW *number_window = newwin(FIELD_SIZE + 3, FIELD_SIZE + 3, 0, 0);
    WINDOW *game_window = newwin(FIELD_SIZE + 2, FIELD_SIZE + 2, 1, 1);
    WINDOW *score_window = newwin(15, 30, 1, FIELD_SIZE+5);
    box(score_window, 0 , 0);	

	wrefresh(score_window);
    game_state current_state;

    int i,j;

    char letters[8] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};

    while (1)
    {
        // receive message

        //zmq_recv (subscriber, &pass_recv, strlen(password), 0);
        zmq_recv (subscriber, &current_state, sizeof(current_state), 0);  // msg type

        // update game screen
        for (int i = 2; i <= FIELD_SIZE + 1; i++) {
        mvwprintw(number_window, i, 0, "%d", (i-1) % 10);
    }

    // Print row numbers on left and game field
    for (int i = 2; i <= FIELD_SIZE + 1; i++) {
        mvwprintw(number_window, 0, i, "%d", (i-1) % 10);
        for (int j = 1; j < FIELD_SIZE + 1; j++) {
            wmove(game_window, j, i);
            waddch(game_window, current_state.game_field[i-1][j-1] | A_BOLD);
        }
    }

    // update score screen
    werase(score_window);
    mvwprintw(score_window, 1, 1, "SCORE");

    int count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++){

        if(current_state.astronaut_scores[i] > -1){
        mvwprintw(score_window, count +2, 1, "%c - %d",letters[i], current_state.astronaut_scores[i]);
        count++;
        }
    } 
  
    box(game_window, 0, 0);
    wrefresh(number_window);
    wrefresh(game_window);
    wrefresh(score_window);
    }

  	endwin();			/* End curses mode		  */

    zmq_close (subscriber);
    zmq_ctx_destroy (context);
    return 0;

}