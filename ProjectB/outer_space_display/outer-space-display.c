#include <ncurses.h>
#include "../common.h"
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

    zmq_setsockopt (subscriber, ZMQ_SUBSCRIBE,empty, 0);  

    initscr();		    	
    cbreak();				
    keypad(stdscr, TRUE);   
    noecho();			    

    WINDOW *number_window = newwin(FIELD_SIZE + 3, FIELD_SIZE + 3, 0, 0);
    WINDOW *game_window = newwin(FIELD_SIZE + 2, FIELD_SIZE + 2, 1, 1);
    WINDOW *score_window = newwin(15, 30, 1, FIELD_SIZE+5);
    box(score_window, 0 , 0);	

    wrefresh(score_window);
    game_state current_state;

    while (1)
    {
        zmq_recv (subscriber, &current_state, sizeof(current_state), 0);
        display_outer_space(&current_state, number_window, game_window, score_window);
    }

    endwin();
    zmq_close (subscriber);
    zmq_ctx_destroy (context);
    return 0;
}