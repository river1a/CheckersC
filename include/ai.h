#ifndef AI_H
#define AI_H
#include "board.h"
#include "rules.h"
int ai_best_sequence(const Board* b, Color player, Move* out_seq, int* out_count);
#endif
