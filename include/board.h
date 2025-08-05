#ifndef BOARD_H
#define BOARD_H

#define BOARD_SIZE 8

typedef enum { COLOR_NONE=0, COLOR_RED=1, COLOR_WHITE=2 } Color;

typedef struct { Color color; int king; } Piece;

typedef struct { Piece squares[BOARD_SIZE][BOARD_SIZE]; } Board;

void board_init(Board* b);
int in_bounds(int r, int c);

#endif
