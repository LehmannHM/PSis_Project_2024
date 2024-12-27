#include <zmq.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include "common.h"

void handle_keyboard_input(void *requester, interaction_message *m, connect_message *connect_reply, char *letters, score_callback_t callback) {
    int ch;
    while ((ch = getch()) != 'q' && ch != 'Q') {
        m->msg_type = 1;
        
        switch (ch) {
            case KEY_UP:
                m->direction = UP;
                break;
            case KEY_DOWN:
                m->direction = DOWN;
                break;
            case KEY_LEFT:
                m->direction = LEFT;
                break;
            case KEY_RIGHT:
                m->direction = RIGHT;
                break;
            case ' ':
                m->msg_type = 2;
                break;
            default:
                continue;
        }

        // send action to server
        zmq_send(requester, m, sizeof(*m), 0);

        // receive scores
        zmq_recv(requester, connect_reply, sizeof(*connect_reply), 0);

        // call the callback if provided
        if (callback) {
            callback(connect_reply);
        }
    }
}

void display_outer_space(game_state *state, WINDOW *number_window, WINDOW *game_window, WINDOW *score_window, char *current_player) {
    char letters[8] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};

    for (int i = 2; i <= FIELD_SIZE + 1; i++) {
        mvwprintw(number_window, i, 0, "%d", (i-1) % 10);
    }

    for (int i = 2; i <= FIELD_SIZE + 1; i++) {
        mvwprintw(number_window, 0, i, "%d", (i-1) % 10);
        for (int j = 1; j < FIELD_SIZE + 1; j++) {
            wmove(game_window, j, i);
            waddch(game_window, state->game_field[i-1][j-1] | A_BOLD);
        }
    }

    werase(score_window);
    mvwprintw(score_window, 1, 1, "SCORE");

    int count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (state->astronaut_scores[i] > -1) {
            mvwprintw(score_window, count+2, 1, "%c - %d", letters[i], state->astronaut_scores[i]);
            if (current_player && letters[i] == *current_player) {
                mvwprintw(score_window, count+2, 10, "[YOUR PLAYER]");
            }
            count++;
        }
    }

    box(game_window, 0, 0);
    wrefresh(number_window);
    wrefresh(game_window);
    wrefresh(score_window);
}