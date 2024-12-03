#include <ncurses.h>
#include "remote-char.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <stdlib.h>

#include <assert.h>
#include <zmq.h>

#include <string.h>

#define ACCESS_PASSWORD "yolo"   // remove password
#define WINDOW_SIZE 15 

// STEP 1
typedef struct astro_info   // para mim seria astro info
{
    int ch;
    int pos_x, pos_y;
    int connected;
    int score;
    int stun_time;    //  when i get zapped                            /// change to clock type
    //int zapp_line;    // don't need this as it will always be my position
    //int next_zapp_time;    // time until next zaaping
    int finished_zap_time;           // zap duration // change clock_type    !!!!!!!!!!!!!!!!!
    int ID;        // ???  // even - only moves horizontally 
                          // odd - only moves vertically

                          //code


} astro_info;

direction_t random_direction(){
    return  random()%4;

}

void new_position(int* x, int *y, direction_t direction){
        switch (direction)
        {
        case UP:
            (*x) --;
            if(*x ==0)
                *x = 2;
            break;
        case DOWN:
            (*x) ++;
            if(*x ==WINDOW_SIZE-1)
                *x = WINDOW_SIZE-3;
            break;
        case LEFT:
            (*y) --;
            if(*y ==0)
                *y = 2;
            break;
        case RIGHT:
            (*y) ++;
            if(*y ==WINDOW_SIZE-1)
                *y = WINDOW_SIZE-3;
            break;
        default:
            break;
        }
}

int find_ch_info(astro_info char_data[], int n_char, int ch){

    for (int i = 0 ; i < n_char; i++){
        if(ch == char_data[i].ch){
            return i;
        }
    }
    return -1;
}


void initialize_astro (astro_info astro[]){  // will have to change content (with & or *)

    // go one by one assigning the ID in this order 1,3,2,4,5,7,6,8 (do vector and run a loop)  

    // first 4: inner region
    // last 4: outter region
}

int main()
{	
    
       
    //STEP 2
    astro_info astro[8];
    int n_chars = 0;

    // RECEIVE messages from clients
    void *context = zmq_ctx_new ();
    void *responder = zmq_socket (context, ZMQ_REP);
    int rc = zmq_bind (responder, ZMQ_LOC);
    assert (rc == 0);

    // SEND messages from client to display?
    void *context_2 = zmq_ctx_new ();
    void *publisher = zmq_socket (context_2, ZMQ_PUB);
    int rc2 = zmq_bind (publisher, "tcp://*:5556");

    if (rc2 != 0) {
    printf("zmq_bind failed: %s\n", zmq_strerror(zmq_errno()));
    return -1;
}

    assert(rc2 == 0);

    

	initscr();		    	
	cbreak();				
    keypad(stdscr, TRUE);   
	noecho();			    

    /* creates a window and draws a border */
    WINDOW * my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);	
	wrefresh(my_win);

    int ch;
    int pos_x;
    int pos_y;


    direction_t  direction;

    remote_char_t m;

    display_info disp_message;

    while (1)
    {
        
        zmq_recv (responder, &m, sizeof(remote_char_t), 0);

        // send password so receiver can read message
        zmq_send (publisher,&ACCESS_PASSWORD, strlen(ACCESS_PASSWORD), ZMQ_SNDMORE);  //password

        disp_message.msg_type2 = m.msg_type;



        if(m.msg_type == 0){
            ch = m.ch;
            pos_x = WINDOW_SIZE/2;
            pos_y = WINDOW_SIZE/2;   

            // update ch
            disp_message.ch = ch;         

            //STEP 3
            astro[n_chars].ch = ch;
            astro[n_chars].pos_x = pos_x;
            astro[n_chars].pos_y = pos_y;
            n_chars++;

            // get initial pos
            disp_message.pos_x_new = pos_x;
            disp_message.pos_y_new = pos_y;

        }
        if(m.msg_type == 1){
            //STEP 4
            int ch_pos = find_ch_info(astro, n_chars, m.ch);


            if(ch_pos != -1){
                pos_x = astro[ch_pos].pos_x;
                pos_y = astro[ch_pos].pos_y;
                ch = astro[ch_pos].ch;

                // update ch
                disp_message.ch = ch; 

                // get old pos
                disp_message.pos_x = pos_x;
                disp_message.pos_y = pos_y;

                /*deletes old place */
                wmove(my_win, pos_x, pos_y);
                waddch(my_win,' ');

                /* claculates new direction */
                direction = m.direction;

                /* claculates new mark position */
                new_position(&pos_x, &pos_y, direction);
                astro[ch_pos].pos_x = pos_x;
                astro[ch_pos].pos_y = pos_y;

                // update new pos
                disp_message.pos_x_new = pos_x;
                disp_message.pos_y_new = pos_y;

            }        
        }
        /* draw mark on new position */

        zmq_send (publisher,&disp_message, sizeof(disp_message), 0);
        
        wmove(my_win, pos_x, pos_y);
        waddch(my_win,ch| A_BOLD);
        wrefresh(my_win);		

    // new imlpentation to work
    zmq_send (responder, "ok", strlen("ok"), 0);             // aqui tem que devolver outras informações tipo score
    }
  	endwin();			/* End curses mode		  */

    zmq_close (publisher);
    zmq_ctx_shutdown (context_2);

	return 0;
}