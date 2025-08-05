#include "rules.h"

Color opponent(Color p) { return p==COLOR_RED?COLOR_WHITE:COLOR_RED; }

int has_capture_from(const Board* b, int r, int c) {
    Piece p = b->squares[r][c];
    if (p.color==COLOR_NONE) return 0;
    int dirs[4][2]={{1,1},{1,-1},{-1,1},{-1,-1}};
    for (int i=0;i<4;i++) {
        int dr=dirs[i][0], dc=dirs[i][1];
        if (!p.king && ((p.color==COLOR_RED && dr<0) || (p.color==COLOR_WHITE && dr>0))) continue;
        int mr=r+dr, mc=c+dc, jr=r+2*dr, jc=c+2*dc;
        if (in_bounds(jr,jc) && in_bounds(mr,mc)) {
            Piece mid=b->squares[mr][mc], dst=b->squares[jr][jc];
            if (mid.color==opponent(p.color) && dst.color==COLOR_NONE) return 1;
        }
    }
    return 0;
}

static void add_move(Move* moves, int* n, int fr,int fc,int tr,int tc,int cap,int cr,int cc) {
    moves[*n].fr=fr; moves[*n].fc=fc; moves[*n].tr=tr; moves[*n].tc=tc; moves[*n].capture=cap; moves[*n].cr=cr; moves[*n].cc=cc; (*n)++;
}

int generate_legal_moves_for_piece(const Board* b, int r, int c, Color player, Move* moves, int max_moves) {
    int n=0;
    Piece p=b->squares[r][c];
    if (p.color!=player) return 0;
    int must_capture = has_capture_from(b,r,c);
    int dirs[4][2]={{1,1},{1,-1},{-1,1},{-1,-1}};
    for (int i=0;i<4;i++) {
        int dr=dirs[i][0], dc=dirs[i][1];
        if (!p.king && ((player==COLOR_RED && dr<0) || (player==COLOR_WHITE && dr>0))) continue;
        int mr=r+dr, mc=c+dc, jr=r+2*dr, jc=c+2*dc;
        if (must_capture) {
            if (in_bounds(jr,jc)) {
                Piece mid=b->squares[mr][mc], dst=b->squares[jr][jc];
                if (mid.color==opponent(player) && dst.color==COLOR_NONE) {
                    if (n<max_moves) add_move(moves,&n,r,c,jr,jc,1,mr,mc);
                }
            }
        } else {
            if (in_bounds(mr,mc) && b->squares[mr][mc].color==COLOR_NONE) {
                if (n<max_moves) add_move(moves,&n,r,c,mr,mc,0,-1,-1);
            }
        }
    }
    return n;
}

int generate_legal_moves(const Board* b, Color player, Move* moves, int max_moves) {
    int n=0;
    int global_capture=0;
    for (int r=0;r<BOARD_SIZE;r++) for (int c=0;c<BOARD_SIZE;c++) {
        if (b->squares[r][c].color==player && has_capture_from(b,r,c)) { global_capture=1; break; }
    }
    for (int r=0;r<BOARD_SIZE;r++) for (int c=0;c<BOARD_SIZE;c++) {
        Move temp[64];
        int k=generate_legal_moves_for_piece(b,r,c,player,temp,64);
        if (global_capture) {
            for (int i=0;i<k;i++) if (temp[i].capture) { if (n<max_moves) moves[n++]=temp[i]; }
        } else {
            for (int i=0;i<k;i++) { if (n<max_moves) moves[n++]=temp[i]; }
        }
    }
    return n;
}

static void maybe_promote(Board* b, int r, int c) {
    Piece* p=&b->squares[r][c];
    if (p->color==COLOR_RED && r==BOARD_SIZE-1) p->king=1;
    if (p->color==COLOR_WHITE && r==0) p->king=1;
}

int apply_move(Board* b, Move m) {
    Piece p=b->squares[m.fr][m.fc];
    b->squares[m.fr][m.fc].color=COLOR_NONE;
    b->squares[m.fr][m.fc].king=0;
    b->squares[m.tr][m.tc]=p;
    if (m.capture) {
        b->squares[m.cr][m.cc].color=COLOR_NONE;
        b->squares[m.cr][m.cc].king=0;
    }
    int was_king=p.king;
    maybe_promote(b,m.tr,m.tc);
    if (!was_king && b->squares[m.tr][m.tc].king) return 2;
    return m.capture?1:0;
}
