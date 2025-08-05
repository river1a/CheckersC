#include "board.h"

static void clear_board(Board* b) {
    for (int r=0;r<BOARD_SIZE;r++) for (int c=0;c<BOARD_SIZE;c++) {
        b->squares[r][c].color=COLOR_NONE;
        b->squares[r][c].king=0;
    }
}

void board_init(Board* b) {
    clear_board(b);
    for (int r=0;r<3;r++) for (int c=0;c<BOARD_SIZE;c++) if ((r+c)%2==1) b->squares[r][c].color=COLOR_RED;
    for (int r=BOARD_SIZE-3;r<BOARD_SIZE;r++) for (int c=0;c<BOARD_SIZE;c++) if ((r+c)%2==1) b->squares[r][c].color=COLOR_WHITE;
}

int in_bounds(int r, int c) { return r>=0 && r<BOARD_SIZE && c>=0 && c<BOARD_SIZE; }
