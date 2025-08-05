#ifndef RULES_H
#define RULES_H
#include "board.h"

typedef struct { int fr; int fc; int tr; int tc; int capture; int cr; int cc; } Move;

Color opponent(Color p);
int has_capture_from(const Board* b, int r, int c);
int generate_legal_moves(const Board* b, Color player, Move* moves, int max_moves);
int generate_legal_moves_for_piece(const Board* b, int r, int c, Color player, Move* moves, int max_moves);
int apply_move(Board* b, Move m);

#endif
