// Compile with: gcc main.c -o main $(sdl2-config --cflags --libs) -lSDL2_image

#include <stdio.h>
#include <ctype.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <SDL2/SDL_image.h>
#include <pthread.h>
#include <string.h>

// Board and cell size
#define WINDOW_SIZE 960
#define CELL_SIZE 120

// Pieces
enum PieceType
{
    WP,
    WR,
    WN,
    WB,
    WQ,
    WK, // White pieces
    BP,
    BR,
    BN,
    BB,
    BQ,
    BK, // Black pieces
    NONE
};

// Board coordinate
struct coordinate
{
    int x;
    int y;
};

// Piece structure
struct piece
{
    struct coordinate coord;
    enum PieceType recog;
    int moved;
    int castled;
    int doubleMove;
    int color; // 1 = white, 0 = black
};

// Backend board representation
struct piece board[8][8];
bool attack_map[8][8] = {0};

// Piece textures
SDL_Texture *pieceTextures[12];

int init();
int print_board(struct piece board[8][8]);
SDL_Texture *loadTexture(SDL_Renderer *renderer, const char *path);
void drawPiece(SDL_Renderer *renderer, SDL_Texture *texture, int row, int col);
int getTextureIndex(struct piece p);
void input(char turn, struct coordinate move[2]);
void convertToCoord(char coord[], struct coordinate move[2]);
void moveValidity(struct coordinate move[], bool AMCall);
void movePawn(struct coordinate move[], bool AMCall);
struct coordinate generateAM(struct coordinate move[], int attacker_color);
void moveKing(struct coordinate move[]);
void moveQueen(struct coordinate move[]);
void moveBishop(struct coordinate move[]);
void moveKnight(struct coordinate move[]);
void moveRook(struct coordinate move[]);
void increaseSizeMoveset();

// prototypes for castling helpers (needed because run() calls them)
bool canCastle(int color, char side);
void doCastle(int color, char side);


// moveset coordinate array
struct coordinate *moveset;
// position to store at moveset
int pos = 0;
// track how many elements allocated
int moveset_capacity = 1;

//put coordinate in moveset
void putMoveset(struct coordinate coord)
{
    if (pos >= moveset_capacity)
        increaseSizeMoveset();

    moveset[pos] = coord;
    pos++;
}

// increase size of moveset
void increaseSizeMoveset()
{
    moveset_capacity *= 2;
    moveset = realloc(moveset, moveset_capacity * sizeof(struct coordinate));
}

//clear all the values in moveset
void clearMoveset()
{
    // don't shrink the allocated buffer every clear — just reset position
    pos = 0;
}

// Init the board with
int init()
{
    // initialise moveset variable
    moveset = malloc(10 * sizeof(struct coordinate));
    moveset_capacity = 10; // keep capacity in sync with allocation
    pos = 0;

    // initilisation with empty first
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            board[row][col] = (struct piece){{row, col}, NONE, 0, 0, 0, 0};
        }
    }

    // Place pawns for both colors
    for (int i = 0; i < 8; i++)
    {
        struct piece wpawn = {{6, i}, WP, 0, 0, 0, 1};
        board[6][i] = wpawn;
        struct piece bpawn = {{1, i}, BP, 0, 0, 0, 0};
        board[1][i] = bpawn;
    }

    // White pieces (bottom row)
    struct piece wrook1 = {{7, 0}, WR, 0, 0, 0, 1};
    struct piece wknight1 = {{7, 1}, WN, 0, 0, 0, 1};
    struct piece wbishop1 = {{7, 2}, WB, 0, 0, 0, 1};
    struct piece wqueen = {{7, 3}, WQ, 0, 0, 0, 1};
    struct piece wking = {{7, 4}, WK, 0, 0, 0, 1};
    struct piece wbishop2 = {{7, 5}, WB, 0, 0, 0, 1};
    struct piece wknight2 = {{7, 6}, WN, 0, 0, 0, 1};
    struct piece wrook2 = {{7, 7}, WR, 0, 0, 0, 1};

    // Place white pieces
    board[7][0] = wrook1;
    board[7][1] = wknight1;
    board[7][2] = wbishop1;
    board[7][3] = wqueen;
    board[7][4] = wking;
    board[7][5] = wbishop2;
    board[7][6] = wknight2;
    board[7][7] = wrook2;

    // Black pieces (top row)
    struct piece brook1 = {{0, 0}, BR, 0, 0, 0, 0};
    struct piece bknight1 = {{0, 1}, BN, 0, 0, 0, 0};
    struct piece bbishop1 = {{0, 2}, BB, 0, 0, 0, 0};
    struct piece bqueen = {{0, 3}, BQ, 0, 0, 0, 0};
    struct piece bking = {{0, 4}, BK, 0, 0, 0, 0};
    struct piece bbishop2 = {{0, 5}, BB, 0, 0, 0, 0};
    struct piece bknight2 = {{0, 6}, BN, 0, 0, 0, 0};
    struct piece brook2 = {{0, 7}, BR, 0, 0, 0, 0};

    // Place black pieces
    board[0][0] = brook1;
    board[0][1] = bknight1;
    board[0][2] = bbishop1;
    board[0][3] = bqueen;
    board[0][4] = bking;
    board[0][5] = bbishop2;
    board[0][6] = bknight2;
    board[0][7] = brook2;

    return 0;
}

// Print the board to the console (for debugging)
int print_board(struct piece board[8][8])
{
    // mapping for enums -> printable chars
    const char pieceChars[] = { 'P','R','N','B','Q','K', 'p','r','n','b','q','k', '_' };

    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (board[i][j].recog != NONE)
            {
                char c = pieceChars[board[i][j].recog];
                printf("%c ", c);
            }
            else
            {
                printf("_ ");
            }
        }
        printf("\n");
    }
    return 0;
}

// Load piece images png file
void loadImagesPNG(SDL_Renderer *renderer)
{
    // Load piece images (PNG files)
    pieceTextures[WP] = loadTexture(renderer, "images/Chess_plt45.png");
    pieceTextures[WR] = loadTexture(renderer, "images/Chess_rlt45.png");
    pieceTextures[WN] = loadTexture(renderer, "images/Chess_nlt45.png");
    pieceTextures[WB] = loadTexture(renderer, "images/Chess_blt45.png");
    pieceTextures[WQ] = loadTexture(renderer, "images/Chess_qlt45.png");
    pieceTextures[WK] = loadTexture(renderer, "images/Chess_klt45.png");
    pieceTextures[BP] = loadTexture(renderer, "images/Chess_pdt45.png");
    pieceTextures[BR] = loadTexture(renderer, "images/Chess_rdt45.png");
    pieceTextures[BN] = loadTexture(renderer, "images/Chess_ndt45.png");
    pieceTextures[BB] = loadTexture(renderer, "images/Chess_bdt45.png");
    pieceTextures[BQ] = loadTexture(renderer, "images/Chess_qdt45.png");
    pieceTextures[BK] = loadTexture(renderer, "images/Chess_kdt45.png");
}

// Load image file into SDL_Texture
SDL_Texture *loadTexture(SDL_Renderer *renderer, const char *path)
{
    SDL_Surface *surface = IMG_Load(path);
    if (!surface)
    {
        printf("IMG_Load failed: %s\n", IMG_GetError());
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

// Draw a piece at a given board coordinate
void drawPiece(SDL_Renderer *renderer, SDL_Texture *texture, int row, int col)
{
    SDL_Rect dst = {
        col * CELL_SIZE,
        row * CELL_SIZE,
        CELL_SIZE,
        CELL_SIZE};
    SDL_RenderCopy(renderer, texture, NULL, &dst);
}

// Map struct piece to enum PieceType index for texture lookup
int getTextureIndex(struct piece p)
{
    if (p.recog >= WP && p.recog <= BK)
    {
        return p.recog;
    }
    return -1;
}

// Cleanup
void cleanup(SDL_Renderer *renderer, SDL_Window *window)
{
    for (int i = 0; i < 12; i++)
    {
        if (pieceTextures[i])
            SDL_DestroyTexture(pieceTextures[i]);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

// Draw chessboard squares and pieces
void drawChessBoard(SDL_Renderer *renderer)
{
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            // Draw board square
            SDL_Rect square = {
                col * CELL_SIZE,
                row * CELL_SIZE,
                CELL_SIZE,
                CELL_SIZE};
            if ((row + col) % 2 == 0)
                SDL_SetRenderDrawColor(renderer, 240, 217, 181, 255);
            else
                SDL_SetRenderDrawColor(renderer, 181, 136, 99, 255);
            SDL_RenderFillRect(renderer, &square);

            // Draw piece if present
            int idx = getTextureIndex(board[row][col]);
            if (idx != -1 && pieceTextures[idx])
            {
                drawPiece(renderer, pieceTextures[idx], row, col);
            }
        }
    }
}

//---Game Loop---
void *run(void *arg)
{
    // keeping turn as a pointer for preview later, possible errors in this section
    // player move
    char pmove = 'w';
    char *turn = &pmove;
    struct coordinate move[2];

    while (true)
    {
        /* ---------- checkmate / stalemate ---------- */
        struct coordinate wk[2] = {{-1,-1},{-1,-1}};
        struct coordinate bk[2] = {{-1,-1},{-1,-1}};

        // find kings
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                if (board[i][j].recog == WK) { wk[0].x = i; wk[0].y = j; wk[1].x = i; wk[1].y = j; }
                else if (board[i][j].recog == BK) { bk[0].x = i; bk[0].y = j; bk[1].x = i; bk[1].y = j; }
            }
        }

        int cur_color = (*turn == 'w') ? 1 : 0;
        struct coordinate king_pos = (cur_color == 1) ? wk[0] : bk[0];

        if (king_pos.x >= 0 && king_pos.x < 8 && king_pos.y >= 0 && king_pos.y < 8) {
            // generate opponent attack-map for current position (attacker = !cur_color)
            struct coordinate am_query[2] = {{0,0}, { king_pos.x, king_pos.y }};
            memset(attack_map, 0, sizeof(attack_map));
            clearMoveset();
            generateAM(am_query, !cur_color);
            bool in_check = attack_map[king_pos.x][king_pos.y];
            clearMoveset();

            // search for any legal move for current side
            int found_legal = 0;
            for (int src_row = 0; src_row < 8 && !found_legal; src_row++) {
                for (int src_col = 0; src_col < 8 && !found_legal; src_col++) {
                    if (board[src_row][src_col].recog == NONE) continue;
                    if (board[src_row][src_col].color != cur_color) continue;

                    // generate moves for this piece
                    struct coordinate test_move[2] = {{src_row, src_col}, {src_row, src_col}};
                    clearMoveset();
                    moveValidity(test_move, false);

                    for (int move_index = 0; move_index < pos && !found_legal; move_index++) {
                        struct coordinate dest = moveset[move_index];
                        if (dest.x < 0 || dest.x > 7 || dest.y < 0 || dest.y > 7) continue;

                        // backup source and dest
                        struct piece src_backup = board[src_row][src_col];
                        struct piece dest_backup = board[dest.x][dest.y];

                        // en-passant handling: if pawn diagonal move to empty square, possible ep capture
                        struct piece ep_victim = {0};
                        int ep_row = -1, ep_col = -1;
                        int ep_taken = 0;
                        if ((src_backup.recog == WP || src_backup.recog == BP) &&
                            (src_col != dest.y) && (src_row != dest.x) && dest_backup.recog == NONE) {
                            int victim_row = (src_backup.recog == WP) ? dest.x + 1 : dest.x - 1;
                            int victim_col = dest.y;
                            if (victim_row >= 0 && victim_row < 8 && victim_col >= 0 && victim_col < 8) {
                                struct piece maybe = board[victim_row][victim_col];
                                if ((maybe.recog == WP || maybe.recog == BP) &&
                                    maybe.color != src_backup.color &&
                                    maybe.doubleMove == 1) {
                                    ep_victim = maybe; ep_row = victim_row; ep_col = victim_col; ep_taken = 1;
                                    board[victim_row][victim_col] = (struct piece){{victim_row, victim_col}, NONE,0,0,0,0};
                                }
                            }
                        }

                        // apply move
                        board[dest.x][dest.y] = board[src_row][src_col];
                        board[dest.x][dest.y].coord.x = dest.x;
                        board[dest.x][dest.y].coord.y = dest.y;
                        board[src_row][src_col] = (struct piece){{src_row, src_col}, NONE,0,0,0,0};

                        // determine defender king position after move
                        struct coordinate new_king = king_pos;
                        if (src_backup.recog == WK || src_backup.recog == BK) new_king = dest;

                        // regenerate opponent attack-map after this hypothetical move
                        struct coordinate am_q2[2] = {{0,0}, { new_king.x, new_king.y }};
                        memset(attack_map, 0, sizeof(attack_map));
                        clearMoveset();
                        generateAM(am_q2, !cur_color);

                        bool king_safe = true;
                        if (new_king.x >= 0 && new_king.x < 8 && new_king.y >= 0 && new_king.y < 8) {
                            if (attack_map[new_king.x][new_king.y]) king_safe = false;
                        } else king_safe = false;

                        // rollback
                        board[src_row][src_col] = src_backup;
                        board[dest.x][dest.y] = dest_backup;
                        if (ep_taken && ep_row >= 0) board[ep_row][ep_col] = ep_victim;
                        clearMoveset();

                        if (king_safe) { found_legal = 1; break; }
                    } // end moves loop
                }
            } // end board scan

            if (!found_legal) {
                if (in_check) {
                    if (cur_color == 1) printf("Checkmate by black\n"); else printf("Checkmate by white\n");
                    break;
                } else {
                    // only declare stalemate if all adjacent king squares are either off-board, occupied by own piece, or attacked
                    int kx = king_pos.x, ky = king_pos.y;
                    int adj_has_safe = 0;
                    for (int dx = -1; dx <= 1 && !adj_has_safe; dx++) {
                        for (int dy = -1; dy <= 1 && !adj_has_safe; dy++) {
                            if (dx == 0 && dy == 0) continue;
                            int nx = kx + dx, ny = ky + dy;
                            if (nx < 0 || nx > 7 || ny < 0 || ny > 7) continue;
                            // square is candidate if empty or capturable (enemy piece)
                            if (board[nx][ny].recog == NONE || board[nx][ny].color != cur_color) {
                                // if not attacked it's a safe adjacent square
                                if (!attack_map[nx][ny]) { adj_has_safe = 1; break; }
                            }
                        }
                    }
                    if (!adj_has_safe) {
                        printf("Stalemate\n");
                        break;
                    }
                }
            }
            clearMoveset();
        }
        /* ---------- check mate/stalemate ---------- */

        // takes requested move coordinate by user
        input(*turn, move);

        // Handle castling sentinel before accessing board indices
        if (move[0].x < 0)
        {
            char side = (move[0].x == -2) ? 'r' : 'l';
            if (canCastle(cur_color, side))
            {
                doCastle(cur_color, side);
                // switch turn
                *turn = (*turn == 'b') ? 'w' : 'b';
            }
            else
            {
                printf("Illegal castle %c for %c\n", side, (*turn));
            }
            clearMoveset();
            continue;
        }

        // checks the validity of input provided
        if (board[move[0].x][move[0].y].recog == NONE)
        {
            printf("Empty square cannot be selected\n");
            continue;
        }
        else if (board[move[0].x][move[0].y].color == 0 && *turn == 'w')
        {
            printf("You cannot move opponent's piece\n");
            continue;
        }
        else if (board[move[0].x][move[0].y].color == 1 && *turn == 'b')
        {
            printf("You cannot move opponent's piece\n");
            continue;
        }
        else if (board[move[1].x][move[1].y].recog != NONE && board[move[0].x][move[0].y].color == board[move[1].x][move[1].y].color)
        {
            printf("You cannot capture your own piece\n");
            continue;
        }

        /*--------------------------VALIDITY OF MOVE CHECK-----------------*/
        moveValidity(move, false);

        for (int i = 0; i < pos; i++)
        {
            printf("X: %d,Y: %d \n", moveset[i].x, moveset[i].y);
        }

        struct piece p = board[move[0].x][move[0].y];
        bool canMove = false;
        for (int i = 0; i < pos; i++)
        {
            if (move[1].x == moveset[i].x && move[1].y == moveset[i].y)
            {
                canMove = true;

                // if pawn and if its double move set it to moved and set move to moved
                if (p.recog == WP || p.recog == BP)
                {
                    if (abs(move[1].x - move[0].x) == 2)
                    {
                        board[move[0].x][move[0].y].doubleMove = 1;
                    }
                    else
                    {
                        board[move[0].x][move[0].y].doubleMove = 0;
                    }
                }
                board[move[0].x][move[0].y].moved = 1;
            }
        }
        /*-----------------------------------------------------------------*/

        if (canMove)
        {
            struct piece src_backup = board[move[0].x][move[0].y];

            struct piece captured = board[move[1].x][move[1].y];

            struct piece ep_victim = {0};
            int ep_row = -1, ep_col = -1;
            int ep_taken = 0;

            if (move[0].y != move[1].y && move[0].x != move[1].x)
            {
                if (p.recog == WP || p.recog == BP)
                {
                    if (board[move[1].x][move[1].y].recog == NONE)
                    {
                        int victimRow = (p.recog == WP) ? move[1].x + 1 : move[1].x - 1;
                        int victimCol = move[1].y;
                        if (victimRow >= 0 && victimRow < 8 && victimCol >= 0 && victimCol < 8)
                        {
                            struct piece maybe = board[victimRow][victimCol];
                            if ((maybe.recog == WP || maybe.recog == BP) &&
                                maybe.color != p.color &&
                                maybe.doubleMove == 1)
                            {
                                ep_victim = maybe;
                                ep_row = victimRow;
                                ep_col = victimCol;
                                ep_taken = 1;
                                board[victimRow][victimCol] = (struct piece){{victimRow, victimCol}, NONE, 0, 0, 0, 0};
                            }
                        }
                    }
                }
            }

            board[move[1].x][move[1].y] = board[move[0].x][move[0].y];
            board[move[1].x][move[1].y].coord.x = move[1].x;
            board[move[1].x][move[1].y].coord.y = move[1].y;
            board[move[0].x][move[0].y] = (struct piece){{move[0].x, move[0].y}, NONE, 0, 0, 0, 0};

            struct coordinate king_pos_after_move = {-1,-1};
            // find mover's king (its color == src_backup.color)
            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 8; j++) {
                    if ((board[i][j].recog == WK && src_backup.color == 1) ||
                        (board[i][j].recog == BK && src_backup.color == 0)) {
                        king_pos_after_move.x = i;
                        king_pos_after_move.y = j;
                        break;
                    }
                }
                if (king_pos_after_move.x != -1) break;
            }

            // generate attack map for opponent now (use the destination piece as reference; generateAM expects move[] but only uses board)
            struct coordinate am_query2[2] = {{0,0}, { move[1].x, move[1].y }};
            memset(attack_map, 0, sizeof(attack_map));
            clearMoveset();
            generateAM(am_query2, !cur_color);

            // if mover's king is under attack, rollback and reject move
            if (king_pos_after_move.x >= 0 && attack_map[king_pos_after_move.x][king_pos_after_move.y]) {
                board[move[0].x][move[0].y] = src_backup;
                board[move[0].x][move[0].y].coord.x = move[0].x;
                board[move[0].x][move[0].y].coord.y = move[0].y;

                board[move[1].x][move[1].y] = captured;

                if (ep_taken && ep_row >= 0)
                    board[ep_row][ep_col] = ep_victim;

                clearMoveset();
                continue;
            }

            *turn = (*turn == 'b') ? 'w' : 'b';

            clearMoveset();
        }

        // clearing moveset
        clearMoveset();
    }
}

/*--------------------------VALIDITY OF MOVE CHECK-----------------*/

// generate attack map of enemy
struct coordinate generateAM(struct coordinate move[], int attacker_color)
{
    struct coordinate king_pos = {-1, -1};
    struct coordinate current_coord;
    memset(attack_map, 0, sizeof(attack_map));
    clearMoveset();

    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            struct piece current = board[i][j];
            if (current.recog == NONE)
                continue;

            if (current.color != attacker_color)
                continue;

            if (current.recog == WK || current.recog == BK)
            {
                king_pos.x = i;
                king_pos.y = j;
            }

            struct coordinate nmove[2];
            nmove[0].x = i; nmove[0].y = j;
            nmove[1].x = i; nmove[1].y = j; 
            moveValidity(nmove, true);
        }
    }

    for (int k = 0; k < pos; k++)
    {
        int x = moveset[k].x;
        int y = moveset[k].y;
        if (x >= 0 && x < 8 && y >= 0 && y < 8)
            attack_map[x][y] = true;
    }

    clearMoveset();
    return king_pos;
}

void moveValidity(struct coordinate move[], bool AMCall)
{

    if (board[move[0].x][move[0].y].recog == WP || board[move[0].x][move[0].y].recog == BP)
    {
        movePawn(move, AMCall);
    }
    else if (board[move[0].x][move[0].y].recog == WR || board[move[0].x][move[0].y].recog == BR)
    {
        moveRook(move);
    }
    else if (board[move[0].x][move[0].y].recog == WN || board[move[0].x][move[0].y].recog == BN)
    {
        moveKnight(move);
    }
    else if (board[move[0].x][move[0].y].recog == WB || board[move[0].x][move[0].y].recog == BB)
    {
        moveBishop(move);
    }
    else if (board[move[0].x][move[0].y].recog == WQ || board[move[0].x][move[0].y].recog == BQ)
    {
        moveQueen(move);
    }
    else if (board[move[0].x][move[0].y].recog == WK || board[move[0].x][move[0].y].recog == BK)
    {
        moveKing(move);
    }
    else
    {
        printf("None");
    }
}

void moveKing(struct coordinate move[])
{
    struct piece p = board[move[0].x][move[0].y];

    if (move[0].x - 1 >= 0)
    {
        if (board[move[0].x - 1][move[0].y].color != p.color ||
            board[move[0].x - 1][move[0].y].recog == NONE)
        {
            struct coordinate coord = {move[0].x - 1, move[0].y};
            putMoveset(coord);
        }
    }

    if (move[0].x + 1 <= 7)
    {
        if (board[move[0].x + 1][move[0].y].color != p.color ||
            board[move[0].x + 1][move[0].y].recog == NONE)
        {
            struct coordinate coord = {move[0].x + 1, move[0].y};
            putMoveset(coord);
        }
    }

    if (move[0].y - 1 >= 0)
    {
        if (board[move[0].x][move[0].y - 1].color != p.color ||
            board[move[0].x][move[0].y - 1].recog == NONE)
        {
            struct coordinate coord = {move[0].x, move[0].y - 1};
            putMoveset(coord);
        }
    }

    if (move[0].y + 1 <= 7)
    {
        if (board[move[0].x][move[0].y + 1].color != p.color ||
            board[move[0].x][move[0].y + 1].recog == NONE)
        {
            struct coordinate coord = {move[0].x, move[0].y + 1};
            putMoveset(coord);
        }
    }

    if (move[0].x - 1 >= 0 && move[0].y - 1 >= 0)
    {
        if (board[move[0].x - 1][move[0].y - 1].color != p.color ||
            board[move[0].x - 1][move[0].y - 1].recog == NONE)
        {
            struct coordinate coord = {move[0].x - 1, move[0].y - 1};
            putMoveset(coord);
        }
    }

    if (move[0].x + 1 <= 7 && move[0].y + 1 <= 7)
    {
        if (board[move[0].x + 1][move[0].y + 1].color != p.color ||
            board[move[0].x + 1][move[0].y + 1].recog == NONE)
        {
            struct coordinate coord = {move[0].x + 1, move[0].y + 1};
            putMoveset(coord);
        }
    }

    if (move[0].x + 1 <= 7 && move[0].y - 1 >= 0)
    {
        if (board[move[0].x + 1][move[0].y - 1].color != p.color ||
            board[move[0].x + 1][move[0].y - 1].recog == NONE)
        {
            struct coordinate coord = {move[0].x + 1, move[0].y - 1};
            putMoveset(coord);
        }
    }

    if (move[0].x - 1 >= 0 && move[0].y + 1 <= 7)
    {
        if (board[move[0].x - 1][move[0].y + 1].color != p.color ||
            board[move[0].x - 1][move[0].y + 1].recog == NONE)
        {
            struct coordinate coord = {move[0].x - 1, move[0].y + 1};
            putMoveset(coord);
        }
    }
}

void moveQueen(struct coordinate move[])
{

    struct piece p = board[move[0].x][move[0].y];

    if (move[0].x - 1 >= 0)
    {
        if (board[move[0].x - 1][move[0].y].color != p.color ||
            board[move[0].x - 1][move[0].y].recog == NONE)
        {
            struct coordinate coord = {move[0].x - 1, move[0].y};
            putMoveset(coord);
        }
    }

    if (move[0].x + 1 <= 7)
    {
        if (board[move[0].x + 1][move[0].y].color != p.color ||
            board[move[0].x + 1][move[0].y].recog == NONE)
        {
            struct coordinate coord = {move[0].x + 1, move[0].y};
            putMoveset(coord);
        }
    }

    if (move[0].y - 1 >= 0)
    {
        if (board[move[0].x][move[0].y - 1].color != p.color ||
            board[move[0].x][move[0].y - 1].recog == NONE)
        {
            struct coordinate coord = {move[0].x, move[0].y - 1};
            putMoveset(coord);
        }
    }

    if (move[0].y + 1 <= 7)
    {
        if (board[move[0].x][move[0].y + 1].color != p.color ||
            board[move[0].x][move[0].y + 1].recog == NONE)
        {
            struct coordinate coord = {move[0].x, move[0].y + 1};
            putMoveset(coord);
        }
    }

    if (move[0].x - 1 >= 0 && move[0].y - 1 >= 0)
    {
        if (board[move[0].x - 1][move[0].y - 1].color != p.color ||
            board[move[0].x - 1][move[0].y - 1].recog == NONE)
        {
            struct coordinate coord = {move[0].x - 1, move[0].y - 1};
            putMoveset(coord);
        }
    }

    if (move[0].x + 1 <= 7 && move[0].y + 1 <= 7)
    {
        if (board[move[0].x + 1][move[0].y + 1].color != p.color ||
            board[move[0].x + 1][move[0].y + 1].recog == NONE)
        {
            struct coordinate coord = {move[0].x + 1, move[0].y + 1};
            putMoveset(coord);
        }
    }

    if (move[0].x + 1 <= 7 && move[0].y - 1 >= 0)
    {
        if (board[move[0].x + 1][move[0].y - 1].color != p.color ||
            board[move[0].x + 1][move[0].y - 1].recog == NONE)
        {
            struct coordinate coord = {move[0].x + 1, move[0].y - 1};
            putMoveset(coord);
        }
    }

    if (move[0].x - 1 >= 0 && move[0].y + 1 <= 7)
    {
        if (board[move[0].x - 1][move[0].y + 1].color != p.color ||
            board[move[0].x - 1][move[0].y + 1].recog == NONE)
        {
            struct coordinate coord = {move[0].x - 1, move[0].y + 1};
            putMoveset(coord);
        }
    }

    moveRook(move);
    moveBishop(move);
}

void moveBishop(struct coordinate move[])
{

    struct piece p = board[move[0].x][move[0].y];

    for (int i = move[0].x + 1, j = move[0].y + 1; i <= 7 && j <= 7; i++, j++)
    {

        if (board[i][j].recog == NONE)
        {
            struct coordinate coord = {i, j};

            putMoveset(coord);
        }
        else if (board[i][j].color != p.color)
        {
            struct coordinate coord = {i, j};
            putMoveset(coord);
            break;
        }
        else
        {
            break;
        }
    }

    for (int i = move[0].x - 1, j = move[0].y - 1; i >= 0 && j >= 0; i--, j--)
    {

        if (board[i][j].recog == NONE)
        {
            struct coordinate coord = {i, j};
            putMoveset(coord);
        }
        else if (board[i][j].color != p.color)
        {
            struct coordinate coord = {i, j};
            putMoveset(coord);
            break;
        }
        else
        {
            break;
        }
    }

    for (int i = move[0].x - 1, j = move[0].y + 1; i >= 0 && j <= 7; i--, j++)
    {

        if (board[i][j].recog == NONE)
        {
            struct coordinate coord = {i, j};
            putMoveset(coord);
        }
        else if (board[i][j].color != p.color)
        {
            struct coordinate coord = {i, j};
            putMoveset(coord);
            break;
        }
        else
        {
            break;
        }
    }

    for (int i = move[0].x + 1, j = move[0].y - 1; i <= 7 && j >= 0; i++, j--)
    {

        if (board[i][j].recog == NONE)
        {
            struct coordinate coord = {i, j};
            putMoveset(coord);
        }
        else if (board[i][j].color != p.color)
        {
            struct coordinate coord = {i, j};
            putMoveset(coord);
            break;
        }
        else
        {
            break;
        }
    }
}

void moveKnight(struct coordinate move[])
{
    struct piece p = board[move[0].x][move[0].y];

    if (move[0].x - 2 >= 0 && move[0].y - 1 >= 0)
    {
        if (board[move[0].x - 2][move[0].y - 1].color != p.color ||
            board[move[0].x - 2][move[0].y - 1].recog == NONE)
        {
            struct coordinate coord = {move[0].x - 2, move[0].y - 1};
            putMoveset(coord);
        }
    }

    if (move[0].x - 2 >= 0 && move[0].y + 1 <= 7)
    {
        if (board[move[0].x - 2][move[0].y + 1].color != p.color ||
            board[move[0].x - 2][move[0].y + 1].recog == NONE)
        {
            struct coordinate coord = {move[0].x - 2, move[0].y + 1};
            putMoveset(coord);
        }
    }

    if (move[0].x - 1 >= 0 && move[0].y + 2 <= 7)
    {
        if (board[move[0].x - 1][move[0].y + 2].color != p.color ||
            board[move[0].x - 1][move[0].y + 2].recog == NONE)
        {
            struct coordinate coord = {move[0].x - 1, move[0].y + 2};
            putMoveset(coord);
        }
    }

    if (move[0].x - 1 >= 0 && move[0].y - 2 >= 0)
    {
        if (board[move[0].x - 1][move[0].y - 2].color != p.color ||
            board[move[0].x - 1][move[0].y - 2].recog == NONE)
        {
            struct coordinate coord = {move[0].x - 1, move[0].y - 2};
            putMoveset(coord);
        }
    }

    if (move[0].x + 1 <= 7 && move[0].y + 2 <= 7)
    {
        if (board[move[0].x + 1][move[0].y + 2].color != p.color ||
            board[move[0].x + 1][move[0].y + 2].recog == NONE)
        {
            struct coordinate coord = {move[0].x + 1, move[0].y + 2};
            putMoveset(coord);
        }
    }

    if (move[0].x + 1 <= 7 && move[0].y - 2 >= 0)
    {
        if (board[move[0].x + 1][move[0].y - 2].color != p.color ||
            board[move[0].x + 1][move[0].y - 2].recog == NONE)
        {
            struct coordinate coord = {move[0].x + 1, move[0].y - 2};
            putMoveset(coord);
        }
    }

    if (move[0].x + 2 <= 7 && move[0].y - 1 >= 0)
    {
        if (board[move[0].x + 2][move[0].y - 1].color != p.color ||
            board[move[0].x + 2][move[0].y - 1].recog == NONE)
        {
            struct coordinate coord = {move[0].x + 2, move[0].y - 1};
            putMoveset(coord);
        }
    }

    if (move[0].x + 2 <= 7 && move[0].y + 1 <= 7)
    {
        if (board[move[0].x + 2][move[0].y + 1].color != p.color ||
            board[move[0].x + 2][move[0].y + 1].recog == NONE)
        {
            struct coordinate coord = {move[0].x + 2, move[0].y + 1};
            putMoveset(coord);
        }
    }
}

void moveRook(struct coordinate move[])
{
    struct piece p = board[move[0].x][move[0].y];

    // up
    for (int i = move[0].x - 1; i >= 0; i--)
    {
        if (board[i][move[0].y].recog == NONE)
        {
            struct coordinate coord = {i, move[0].y};
            putMoveset(coord);
        }
        else if (board[i][move[0].y].color != p.color)
        {
            struct coordinate coord = {i, move[0].y};
            putMoveset(coord);
            break;
        }
        else
        {
            break;
        }
    }

    // down
    for (int i = move[0].x + 1; i <= 7; i++)
    {
        if (board[i][move[0].y].recog == NONE)
        {
            struct coordinate coord = {i, move[0].y};
            putMoveset(coord);
        }
        else if (board[i][move[0].y].color != p.color)
        {
            struct coordinate coord = {i, move[0].y};
            putMoveset(coord);
            break;
        }
        else
        {
            break;
        }
    }

    // right (scan along columns; i is column index)
    for (int i = move[0].y + 1; i <= 7; i++)
    {
        if (board[move[0].x][i].recog == NONE)
        {
            struct coordinate coord = {move[0].x, i};
            putMoveset(coord);
        }
        else if (board[move[0].x][i].color != p.color)
        {
            struct coordinate coord = {move[0].x, i};
            putMoveset(coord);
            break;
        }
        else
        {
            break;
        }
    }

    // left (scan along columns leftwards)
    for (int i = move[0].y - 1; i >= 0; i--)
    {
        if (board[move[0].x][i].recog == NONE)
        {
            struct coordinate coord = {move[0].x, i};
            putMoveset(coord);
        }
        else if (board[move[0].x][i].color != p.color)
        {
            struct coordinate coord = {move[0].x, i};
            putMoveset(coord);
            break;
        }
        else
        {
            break;
        }
    }
}

void movePawn(struct coordinate move[], bool AMCall)
{
    struct piece p = board[move[0].x][move[0].y];
    int row = move[0].x;
    int col = move[0].y;

    // When generating attack map, pawns attack diagonals regardless of occupancy.
    if (AMCall)
    {
        if (p.color == 0) // black pawn attacks down (higher row index)
        {
            if (row + 1 <= 7 && col - 1 >= 0) putMoveset((struct coordinate){row + 1, col - 1});
            if (row + 1 <= 7 && col + 1 <= 7) putMoveset((struct coordinate){row + 1, col + 1});
        }
        else // white pawn attacks up (lower row index)
        {
            if (row - 1 >= 0 && col - 1 >= 0) putMoveset((struct coordinate){row - 1, col - 1});
            if (row - 1 >= 0 && col + 1 <= 7) putMoveset((struct coordinate){row - 1, col + 1});
        }
        return;
    }

    // Normal move generation (non-AMCall)
    if (p.color == 0)
    {
        // two-square move: check both intermediate and target squares
        if (p.moved == 0)
        {
            if (row + 2 <= 7 && board[row + 1][col].recog == NONE && board[row + 2][col].recog == NONE)
                putMoveset((struct coordinate){row + 2, col});
        }
        // one-square forward
        if (row + 1 <= 7 && board[row + 1][col].recog == NONE)
            putMoveset((struct coordinate){row + 1, col});

        // captures (diagonals)
        if (row + 1 <= 7 && col + 1 <= 7 && board[row + 1][col + 1].recog != NONE && board[row + 1][col + 1].color != p.color)
            putMoveset((struct coordinate){row + 1, col + 1});
        if (row + 1 <= 7 && col - 1 >= 0 && board[row + 1][col - 1].recog != NONE && board[row + 1][col - 1].color != p.color)
            putMoveset((struct coordinate){row + 1, col - 1});

        // en-passant captures
        if (row + 1 <= 7 && col - 1 >= 0)
        {
            struct piece maybe = board[row][col - 1];
            if ((maybe.recog == WP || maybe.recog == BP) && maybe.color != p.color && maybe.doubleMove == 1)
                putMoveset((struct coordinate){row + 1, col - 1});
        }
        if (row + 1 <= 7 && col + 1 <= 7)
        {
            struct piece maybe = board[row][col + 1];
            if ((maybe.recog == WP || maybe.recog == BP) && maybe.color != p.color && maybe.doubleMove == 1)
                putMoveset((struct coordinate){row + 1, col + 1});
        }
    }
    else
    {
        // white pawns
        if (p.moved == 0)
        {
            if (row - 2 >= 0 && board[row - 1][col].recog == NONE && board[row - 2][col].recog == NONE)
                putMoveset((struct coordinate){row - 2, col});
        }
        if (row - 1 >= 0 && board[row - 1][col].recog == NONE)
            putMoveset((struct coordinate){row - 1, col});

        if (row - 1 >= 0 && col - 1 >= 0 && board[row - 1][col - 1].recog != NONE && board[row - 1][col - 1].color != p.color)
            putMoveset((struct coordinate){row - 1, col - 1});
        if (row - 1 >= 0 && col + 1 <= 7 && board[row - 1][col + 1].recog != NONE && board[row - 1][col + 1].color != p.color)
            putMoveset((struct coordinate){row - 1, col + 1});

        if (row - 1 >= 0 && col - 1 >= 0)
        {
            struct piece maybe = board[row][col - 1];
            if ((maybe.recog == WP || maybe.recog == BP) && maybe.color != p.color && maybe.doubleMove == 1)
                putMoveset((struct coordinate){row - 1, col - 1});
        }
        if (row - 1 >= 0 && col + 1 <= 7)
        {
            struct piece maybe = board[row][col + 1];
            if ((maybe.recog == WP || maybe.recog == BP) && maybe.color != p.color && maybe.doubleMove == 1)
                putMoveset((struct coordinate){row - 1, col + 1});
        }
    }
}
/*-----------------------------------------------------------------*/

/* Make the move after checking legality of the move
   take move input in format e3b4 */
void input(char turn, struct coordinate move[2])
{
    char coords[5];
    printf("Enter move for %c: ", turn);
    scanf("%4s", coords);
    
    convertToCoord(coords, move);
}
 
void convertToCoord(char coord[], struct coordinate move[])
{
    // accept "cr" or "cl" as castle commands
    if (coord[0] == 'c' && coord[1] == 'r' && coord[2] == '\0')
    {
        move[0] = (struct coordinate){-2, 0};
        move[1] = (struct coordinate){-2, 0};
        return;
    }
    if (coord[0] == 'c' && coord[1] == 'l' && coord[2] == '\0')
    {
        move[0] = (struct coordinate){-3, 0};
        move[1] = (struct coordinate){-3, 0};
        return;
    }
 
     // expecting input like "a8a6"
     int col1 = coord[0] - 'a';       
     int row1 = 8 - (coord[1] - '0'); 
 
     int col2 = coord[2] - 'a';
     int row2 = 8 - (coord[3] - '0');
 
     if (0 <= col1 && col1 < 8 && 0 <= col2 && col2 < 8 &&
         0 <= row1 && row1 < 8 && 0 <= row2 && row2 < 8)
     {

         printf("%d %d %d %d\n", row1, col1, row2, col2);

         move[0] = (struct coordinate){row1, col1}; // (row, col)
         move[1] = (struct coordinate){row2, col2};
         return;
     }

     printf("Coordinate conversion error\n");
 }

/*-------------------------- CASTLING HELPERS --------------------------*/

bool canCastle(int color, char side)
{
    // find king
    int king_row = -1, king_col = -1;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            if ((color == 1 && board[r][c].recog == WK) || (color == 0 && board[r][c].recog == BK))
            {
                king_row = r;
                king_col = c;
                break;
            }
    if (king_row == -1) return false;

    struct piece king = board[king_row][king_col];
    if (king.moved) return false; // king already moved

    int rook_col = (side == 'r') ? 7 : 0;
    struct piece rook = board[king_row][rook_col];
    // rook must exist, be same color, and unmoved
    if ( (color == 1 && rook.recog != WR) || (color == 0 && rook.recog != BR) ) return false;
    if (rook.moved) return false;

    // squares between king and rook must be empty
    int from = (king_col < rook_col) ? king_col : rook_col;
    int to = (king_col < rook_col) ? rook_col : king_col;
    for (int c = from + 1; c < to; c++)
        if (board[king_row][c].recog != NONE) return false;

    // squares king traverses must not be under attack and king must not be in check
    int dir = (side == 'r') ? 1 : -1;
    memset(attack_map, 0, sizeof(attack_map));
    clearMoveset();
    struct coordinate dummy[2] = {{0,0},{0,0}};
    generateAM(dummy, !color);

    if (attack_map[king_row][king_col]) return false;
    int pass_col = king_col + dir;
    if (pass_col < 0 || pass_col > 7) return false;
    if (attack_map[king_row][pass_col]) return false;
    int dest_col = king_col + 2 * dir;
    if (dest_col < 0 || dest_col > 7) return false;
    if (attack_map[king_row][dest_col]) return false;

    clearMoveset();
    return true;
}

// Perform castle on board (assumes legality checked). Updates moved flags.
void doCastle(int color, char side)
{
    // find king
    int king_row = -1, king_col = -1;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            if ((color == 1 && board[r][c].recog == WK) || (color == 0 && board[r][c].recog == BK))
            {
                king_row = r;
                king_col = c;
                break;
            }
    if (king_row == -1) return;

    int dir = (side == 'r') ? 1 : -1;
    int rook_col = (side == 'r') ? 7 : 0;

    // new king position and rook position
    int new_king_col = king_col + 2 * dir;
    int new_rook_col = king_col + dir;

    board[king_row][new_king_col] = board[king_row][king_col];
    board[king_row][new_king_col].coord.x = king_row;
    board[king_row][new_king_col].coord.y = new_king_col;
    board[king_row][new_king_col].moved = 1;

    board[king_row][king_col] = (struct piece){{king_row, king_col}, NONE, 0, 0, 0, 0};

    board[king_row][new_rook_col] = board[king_row][rook_col];
    board[king_row][new_rook_col].coord.x = king_row;
    board[king_row][new_rook_col].coord.y = new_rook_col;
    board[king_row][new_rook_col].moved = 1;

    board[king_row][rook_col] = (struct piece){{king_row, rook_col}, NONE, 0, 0, 0, 0};
}
/*---------------------- end castling helpers -----------------------*/

// Main code
int main()
{

    // Create seperate thread for the terminal input function
    pthread_t inputThread;
    pthread_create(&inputThread, NULL, run, NULL);

    // Initialize SDL and SDL_image
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    // Create window and renderer
    SDL_Window *window = SDL_CreateWindow("Chessboard",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WINDOW_SIZE, WINDOW_SIZE, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

    // Initialize the chess board
    init();

    // load png images
    loadImagesPNG(renderer);

    // Main Loop
    bool running = true;
    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = false;
        }

        /*-------------------------------------------------------------*/

        // Chess Logic code

        /*-------------------------------------------------------------*/

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Draw chessboard squares and pieces
        drawChessBoard(renderer);

        // Display window
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Exit
    cleanup(renderer, window);

    return 0;
}
