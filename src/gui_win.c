#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include "board.h"
#include "rules.h"
#include "ai.h"

#define CELL 80
#define BOARD_PIX (CELL*8)
#define MARGIN 16
#define TIMER_AI 1

static Board g_board;
static Color g_turn = COLOR_RED;
static int sel_r = -1, sel_c = -1;
static Move sel_moves[128];
static int sel_count = 0;
static int g_vs_ai = 0;
static Color g_ai_color = COLOR_WHITE;
static int g_ai_thinking = 0;

static int global_capture_available() {
    for (int r=0;r<BOARD_SIZE;r++) for (int c=0;c<BOARD_SIZE;c++) {
        if (g_board.squares[r][c].color==g_turn && has_capture_from(&g_board,r,c)) return 1;
    }
    return 0;
}

static int any_moves_exist_for(Color player) {
    Move tmp[256];
    return generate_legal_moves(&g_board, player, tmp, 256) > 0;
}

static void show_win_and_quit(HWND h, Color winner) {
    const char* t = winner==COLOR_RED ? "Red wins" : "White wins";
    MessageBoxA(h, t, "Game Over", MB_OK | MB_ICONINFORMATION);
    DestroyWindow(h);
}

static void compute_moves_for_selection() {
    sel_count = 0;
    if (sel_r<0 || sel_c<0) return;
    Move tmp[128];
    int k = generate_legal_moves_for_piece(&g_board, sel_r, sel_c, g_turn, tmp, 128);
    int need_cap = global_capture_available();
    for (int i=0;i<k;i++) {
        if (need_cap && !tmp[i].capture) continue;
        sel_moves[sel_count++] = tmp[i];
    }
}

static void clear_selection() {
    sel_r = sel_c = -1;
    sel_count = 0;
}

static void pick_square(int px, int py, int* r, int* c) {
    px -= MARGIN; py -= MARGIN;
    if (px<0 || py<0 || px>=BOARD_PIX || py>=BOARD_PIX) { *r=-1; *c=-1; return; }
    *c = px / CELL;
    *r = py / CELL;
}

static void draw_board(HDC hdc) {
    HBRUSH dark = CreateSolidBrush(RGB(102,51,0));
    HBRUSH light = CreateSolidBrush(RGB(240,217,181));
    HBRUSH selb = CreateSolidBrush(RGB(255,255,0));
    HPEN grid = CreatePen(PS_SOLID, 1, RGB(0,0,0));
    for (int r=0;r<8;r++) for (int c=0;c<8;c++) {
        RECT cell = { MARGIN + c*CELL, MARGIN + r*CELL, MARGIN + (c+1)*CELL, MARGIN + (r+1)*CELL };
        HBRUSH b = ((r+c)%2)? dark : light;
        FillRect(hdc, &cell, b);
    }
    if (sel_r>=0) {
        RECT cell = { MARGIN + sel_c*CELL, MARGIN + sel_r*CELL, MARGIN + (sel_c+1)*CELL, MARGIN + (sel_r+1)*CELL };
        FrameRect(hdc, &cell, selb);
        InflateRect(&cell, -3, -3);
        FrameRect(hdc, &cell, selb);
    }
    for (int i=0;i<sel_count;i++) {
        int r = sel_moves[i].tr, c = sel_moves[i].tc;
        int cx = MARGIN + c*CELL + CELL/2;
        int cy = MARGIN + r*CELL + CELL/2;
        HBRUSH hint = CreateSolidBrush(RGB(255,255,0));
        HBRUSH oldb = SelectObject(hdc, hint);
        HPEN oldp = SelectObject(hdc, grid);
        Ellipse(hdc, cx-10, cy-10, cx+10, cy+10);
        SelectObject(hdc, oldb);
        SelectObject(hdc, oldp);
        DeleteObject(hint);
    }
    SelectObject(hdc, grid);
    for (int i=0;i<=8;i++) {
        MoveToEx(hdc, MARGIN, MARGIN + i*CELL, NULL);
        LineTo(hdc, MARGIN + 8*CELL, MARGIN + i*CELL);
        MoveToEx(hdc, MARGIN + i*CELL, MARGIN, NULL);
        LineTo(hdc, MARGIN + i*CELL, MARGIN + 8*CELL);
    }
    DeleteObject(dark);
    DeleteObject(light);
    DeleteObject(selb);
    DeleteObject(grid);
}

static void draw_pieces(HDC hdc) {
    for (int r=0;r<8;r++) for (int c=0;c<8;c++) {
        Piece p = g_board.squares[r][c];
        if (p.color==COLOR_NONE) continue;
        int x0 = MARGIN + c*CELL + 8;
        int y0 = MARGIN + r*CELL + 8;
        int x1 = MARGIN + (c+1)*CELL - 8;
        int y1 = MARGIN + (r+1)*CELL - 8;
        COLORREF col = (p.color==COLOR_RED)? RGB(200,0,0):RGB(245,245,245);
        HBRUSH b = CreateSolidBrush(col);
        HPEN pen = CreatePen(PS_SOLID, 2, RGB(0,0,0));
        HBRUSH oldb = SelectObject(hdc, b);
        HPEN oldp = SelectObject(hdc, pen);
        Ellipse(hdc, x0,y0,x1,y1);
        if (p.king) {
            HFONT f = CreateFontA(28,0,0,0,FW_BOLD,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_DONTCARE,"Segoe UI");
            HFONT of = SelectObject(hdc, f);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(0,0,0));
            RECT tr = { x0, y0, x1, y1 };
            DrawTextA(hdc, "K", -1, &tr, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            SelectObject(hdc, of);
            DeleteObject(f);
        }
        SelectObject(hdc, oldb);
        SelectObject(hdc, oldp);
        DeleteObject(b);
        DeleteObject(pen);
    }
}

static void draw_status(HDC hdc) {
    RECT r = { MARGIN, MARGIN + BOARD_PIX + 8, MARGIN + BOARD_PIX, MARGIN + BOARD_PIX + 40 };
    HBRUSH b = CreateSolidBrush(RGB(240,240,240));
    FillRect(hdc, &r, b);
    DeleteObject(b);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0,0,0));
    HFONT f = CreateFontA(18,0,0,0,FW_SEMIBOLD,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_DONTCARE,"Segoe UI");
    HFONT of = SelectObject(hdc, f);
    const char* t = g_turn==COLOR_RED ? "Turn: Red" : "Turn: White";
    TextOutA(hdc, MARGIN, MARGIN + BOARD_PIX + 14, t, (int)strlen(t));
    SelectObject(hdc, of);
    DeleteObject(f);
}

static void refresh(HWND h) {
    InvalidateRect(h, NULL, FALSE);
}

static void ai_turn(HWND h) {
    g_ai_thinking = 1;
    Move seq[32];
    int n=0;
    if (!ai_best_sequence(&g_board, g_turn, seq, &n) || n<=0) { show_win_and_quit(h, opponent(g_turn)); g_ai_thinking=0; return; }
    for (int i=0;i<n;i++) {
        apply_move(&g_board, seq[i]);
        refresh(h);
        UpdateWindow(h);
        Sleep(120);
    }
    clear_selection();
    g_turn = opponent(g_turn);
    if (!any_moves_exist_for(g_turn)) { show_win_and_quit(h, opponent(g_turn)); g_ai_thinking=0; return; }
    refresh(h);
    UpdateWindow(h);
    g_ai_thinking = 0;
    if (g_vs_ai && g_turn==g_ai_color) SetTimer(h, TIMER_AI, 150, NULL);
}

static void try_select_or_move(HWND h, int r, int c) {
    if (g_ai_thinking) return;
    if (g_vs_ai && g_turn==g_ai_color) return;
    if (r<0 || c<0) { clear_selection(); refresh(h); return; }
    if (sel_r<0) {
        Piece p = g_board.squares[r][c];
        if (p.color!=g_turn) return;
        sel_r=r; sel_c=c;
        compute_moves_for_selection();
        if (sel_count==0) { clear_selection(); return; }
        refresh(h);
    } else {
        int idx=-1;
        for (int i=0;i<sel_count;i++) if (sel_moves[i].tr==r && sel_moves[i].tc==c) { idx=i; break; }
        if (idx<0) {
            Piece p = g_board.squares[r][c];
            if (p.color==g_turn) { sel_r=r; sel_c=c; compute_moves_for_selection(); refresh(h); }
            return;
        }
        int res = apply_move(&g_board, sel_moves[idx]);
        int nr = sel_moves[idx].tr, nc = sel_moves[idx].tc;
        if (res==1 && has_capture_from(&g_board,nr,nc)) {
            sel_r=nr; sel_c=nc; compute_moves_for_selection(); refresh(h); return;
        }
        clear_selection();
        g_turn = opponent(g_turn);
        if (!any_moves_exist_for(g_turn)) { show_win_and_quit(h, opponent(g_turn)); return; }
        refresh(h);
        UpdateWindow(h);
        if (g_vs_ai && g_turn==g_ai_color) SetTimer(h, TIMER_AI, 200, NULL);
    }
}

static LRESULT CALLBACK WndProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    switch(msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(h, &ps);
        draw_board(hdc);
        draw_pieces(hdc);
        draw_status(hdc);
        EndPaint(h, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(l);
        int y = GET_Y_LPARAM(l);
        int r,c; pick_square(x,y,&r,&c);
        try_select_or_move(h,r,c);
        return 0;
    }
    case WM_TIMER:
        if (w==TIMER_AI) { KillTimer(h, TIMER_AI); ai_turn(h); }
        return 0;
    case WM_SIZE:
        refresh(h);
        return 0;
    case WM_KEYDOWN:
        if (w==VK_ESCAPE) { clear_selection(); refresh(h); }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(h,msg,w,l);
}

int APIENTRY WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR lp, int n) {
    int mode = MessageBoxA(NULL, "Choose mode:\nYes = Player vs Player\nNo = Player vs Computer", "Mode", MB_YESNO | MB_ICONQUESTION);
    if (mode==IDYES) { g_vs_ai=0; } else { g_vs_ai=1; int side=MessageBoxA(NULL,"Play as Red? Yes=Red, No=White","Choose Side",MB_YESNO|MB_ICONQUESTION); g_ai_color=(side==IDYES)?COLOR_WHITE:COLOR_RED; }
    board_init(&g_board);
    g_turn=COLOR_RED;
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hi;
    wc.lpszClassName = "CheckersCWnd";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    RegisterClassA(&wc);
    int w = BOARD_PIX + MARGIN*2;
    int h = BOARD_PIX + MARGIN*2 + 48;
    HWND win = CreateWindowExA(0, "CheckersCWnd", "CheckersC", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, w, h, NULL, NULL, hi, NULL);
    ShowWindow(win, n);
    UpdateWindow(win);
    if (g_vs_ai && g_turn==g_ai_color) SetTimer(win, TIMER_AI, 200, NULL);
    MSG msg;
    while (GetMessage(&msg,NULL,0,0)>0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
