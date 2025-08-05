#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include "board.h"
#include "rules.h"
#include "ai.h"

typedef struct { Move m[32]; int n; } Seq;

static void board_copy(const Board* s, Board* d) { memcpy(d, s, sizeof(Board)); }

static int any_capture_global(const Board* b, Color player) {
    for (int r=0;r<BOARD_SIZE;r++) for (int c=0;c<BOARD_SIZE;c++) {
        if (b->squares[r][c].color==player && has_capture_from(b,r,c)) return 1;
    }
    return 0;
}

static void seq_push(Seq* s, Move m) { if (s->n<32) s->m[s->n++]=m; }

static void gen_caps_piece(const Board* b, int r, int c, Color player, Seq* cur, Seq* out, int* outn, int outcap) {
    if (*outn>=outcap) return;
    Move moves[64];
    int k = generate_legal_moves_for_piece(b,r,c,player,moves,64);
    int found=0;
    for (int i=0;i<k;i++) if (moves[i].capture) { found=1; break; }
    if (!found) {
        if (*outn<outcap) out[(*outn)++] = *cur;
        return;
    }
    for (int i=0;i<k;i++) {
        if (!moves[i].capture) continue;
        Board nb; board_copy(b,&nb);
        int res = apply_move(&nb, moves[i]);
        Seq next=*cur; seq_push(&next, moves[i]);
        if (res==2) {
            if (*outn<outcap) out[(*outn)++] = next;
        } else {
            if (has_capture_from(&nb, moves[i].tr, moves[i].tc)) {
                gen_caps_piece(&nb, moves[i].tr, moves[i].tc, player, &next, out, outn, outcap);
                if (*outn>=outcap) return;
            } else {
                if (*outn<outcap) out[(*outn)++] = next;
            }
        }
        if (*outn>=outcap) return;
    }
}

static int gen_all_sequences(const Board* b, Color player, Seq* out, int outcap) {
    int n=0;
    int mustcap = any_capture_global(b,player);
    if (mustcap) {
        for (int r=0;r<BOARD_SIZE;r++) for (int c=0;c<BOARD_SIZE;c++) {
            if (b->squares[r][c].color!=player) continue;
            if (!has_capture_from(b,r,c)) continue;
            Seq cur; cur.n=0;
            gen_caps_piece(b,r,c,player,&cur,out,&n,outcap);
            if (n>=outcap) return n;
        }
    } else {
        for (int r=0;r<BOARD_SIZE;r++) for (int c=0;c<BOARD_SIZE;c++) {
            if (b->squares[r][c].color!=player) continue;
            Move moves[64];
            int k=generate_legal_moves_for_piece(b,r,c,player,moves,64);
            for (int i=0;i<k;i++) {
                if (moves[i].capture) continue;
                if (n<outcap) { Seq s; s.n=0; s.m[s.n++]=moves[i]; out[n++]=s; }
                if (n>=outcap) return n;
            }
        }
    }
    return n;
}

static int evaluate(const Board* b, Color pov) {
    int score=0;
    for (int r=0;r<8;r++) for (int c=0;c<8;c++) {
        Piece p=b->squares[r][c];
        if (p.color==COLOR_NONE) continue;
        int v = p.king? 180:100;
        int adv = p.color==COLOR_RED ? r : (7-r);
        int ctr = (r>=2&&r<=5&&c>=2&&c<=5)? 3:0;
        int s = v + adv*2 + ctr;
        score += (p.color==pov)? s : -s;
    }
    Move tmp[128];
    int mymob = generate_legal_moves(b,pov,tmp,128);
    int oppmob = generate_legal_moves(b, pov==COLOR_RED?COLOR_WHITE:COLOR_RED, tmp,128);
    score += (mymob - oppmob)*5;
    return score;
}

static Color root;

static int search(Board* b, Color to_move, int depth, int alpha, int beta) {
    int cap=128;
    Seq* seqs = (Seq*)malloc(sizeof(Seq)*cap);
    int n = gen_all_sequences(b,to_move,seqs,cap);
    int result;
    if (depth==0 || n==0) {
        result = (n==0) ? (to_move==root ? INT_MIN/2 : INT_MAX/2) : evaluate(b, root);
        free(seqs);
        return result;
    }
    if (to_move==root) {
        int best=INT_MIN/2;
        for (int i=0;i<n;i++) {
            Board nb; board_copy(b,&nb);
            for (int k=0;k<seqs[i].n;k++) {
                int res=apply_move(&nb, seqs[i].m[k]);
                if (res==2) break;
            }
            int val = search(&nb, to_move==COLOR_RED?COLOR_WHITE:COLOR_RED, depth-1, alpha, beta);
            if (val>best) best=val;
            if (val>alpha) alpha=val;
            if (beta<=alpha) break;
        }
        result=best;
    } else {
        int best=INT_MAX/2;
        for (int i=0;i<n;i++) {
            Board nb; board_copy(b,&nb);
            for (int k=0;k<seqs[i].n;k++) {
                int res=apply_move(&nb, seqs[i].m[k]);
                if (res==2) break;
            }
            int val = search(&nb, to_move==COLOR_RED?COLOR_WHITE:COLOR_RED, depth-1, alpha, beta);
            if (val<best) best=val;
            if (val<beta) beta=val;
            if (beta<=alpha) break;
        }
        result=best;
    }
    free(seqs);
    return result;
}

int ai_best_sequence(const Board* b, Color player, Move* out_seq, int* out_count) {
    root = player;
    int cap=128;
    Seq* seqs = (Seq*)malloc(sizeof(Seq)*cap);
    int n = gen_all_sequences(b,player,seqs,cap);
    if (n<=0) { free(seqs); *out_count=0; return 0; }
    int bestVal = INT_MIN/2;
    int bestIdx = 0;
    for (int i=0;i<n;i++) {
        Board nb; board_copy(b,&nb);
        for (int k=0;k<seqs[i].n;k++) {
            int res=apply_move(&nb, seqs[i].m[k]);
            if (res==2) break;
        }
        int val = search(&nb, player==COLOR_RED?COLOR_WHITE:COLOR_RED, 6, INT_MIN/2, INT_MAX/2);
        if (val>bestVal) { bestVal=val; bestIdx=i; }
    }
    for (int i=0;i<seqs[bestIdx].n;i++) out_seq[i]=seqs[bestIdx].m[i];
    *out_count = seqs[bestIdx].n;
    free(seqs);
    return 1;
}
