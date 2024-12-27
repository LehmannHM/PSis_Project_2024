#include <zmq.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "../common.h"

typedef struct {
    void *requester;
    interaction_message *m;
    connect_message *connect_reply;
    char *letters;
    volatile int *exit_flag;
} input_thread_args;

typedef struct {
    void *subscriber;
    game_state *current_state;
    WINDOW *number_window;
    WINDOW *game_window;
    WINDOW *score_window;
    volatile int *exit_flag;
    char current_player;
} display_thread_args;

volatile int exit_flag = 0;

void *handle_input(void *args) {
    input_thread_args *input_args = (input_thread_args *)args;
    
    handle_keyboard_input(input_args->requester, input_args->m, input_args->connect_reply, input_args->letters, NULL);

    // set exit flag
    *(input_args->exit_flag) = 1;

    // disconnect
    input_args->m->msg_type = 3;
    zmq_send(input_args->requester, input_args->m, sizeof(*input_args->m), 0);
    zmq_recv(input_args->requester, NULL, 0, 0);
    return NULL;
}

void *update_display(void *args) {
    display_thread_args *display_args = (display_thread_args *)args;
    while (!*(display_args->exit_flag)) {
        zmq_recv(display_args->subscriber, display_args->current_state, sizeof(game_state), 0);
        display_outer_space(display_args->current_state, display_args->number_window, display_args->game_window, display_args->score_window, &display_args->current_player);
    }
    return NULL;
}

int main() {
    void *context = zmq_ctx_new();
    void *requester = zmq_socket(context, ZMQ_REQ);
    void *subscriber = zmq_socket(context, ZMQ_SUB);

    if (zmq_connect(requester, INTERACTION_ADDRESS) != 0) {
        printf("Error connecting to server: %s\n", zmq_strerror(zmq_errno()));
    }

    if (zmq_connect(subscriber, DISPLAY_ADDRESS) != 0) {
        printf("Error connecting to display server: %s\n", zmq_strerror(zmq_errno()));
    }

    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    WINDOW *number_window = newwin(FIELD_SIZE + 3, FIELD_SIZE + 3, 0, 0);
    WINDOW *game_window = newwin(FIELD_SIZE + 2, FIELD_SIZE + 2, 1, 1);
    WINDOW *score_window = newwin(15, 30, 1, FIELD_SIZE + 5);
    box(score_window, 0, 0);
    wrefresh(score_window);

    interaction_message m;
    m.msg_type = 0;
    if (zmq_send(requester, &m, sizeof(m), 0) == -1) {
        printf("Error sending message: %s\n", zmq_strerror(zmq_errno()));
    }

    connect_message connect_reply;
    zmq_recv(requester, &connect_reply, sizeof(connect_message), 0);

    if (connect_reply.id < 'A' || connect_reply.id > 'H') {
        printf("Max Players reached");
        return 0;
    }

    m.id = connect_reply.id;
    memcpy(m.token, connect_reply.token, TOKEN_SIZE);

    char letters[8] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
    game_state current_state;

    pthread_t input_thread, display_thread;
    input_thread_args input_args = {requester, &m, &connect_reply, letters, &exit_flag};
    display_thread_args display_args = {subscriber, &current_state, number_window, game_window, score_window, &exit_flag, m.id};

    pthread_create(&input_thread, NULL, handle_input, &input_args);
    pthread_create(&display_thread, NULL, update_display, &display_args);

    pthread_join(input_thread, NULL);
    pthread_join(display_thread, NULL);

    endwin();
    zmq_close(requester);
    zmq_close(subscriber);
    zmq_ctx_destroy(context);
    return 0;
}