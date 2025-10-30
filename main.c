// Compile with: gcc main.c -o main $(sdl2-config --cflags --libs) -lSDL2_image

#include <stdio.h>
#include <ctype.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <SDL2/SDL_image.h>
#include <pthread.h>

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
struct coordinate generateAM(struct coordinate move[]);
void moveKing(struct coordinate move[]);
void moveQueen(struct coordinate move[]);
void moveBishop(struct coordinate move[]);
void moveKnight(struct coordinate move[]);
void moveRook(struct coordinate move[]);

// moveset coordinate array
struct coordinate *moveset;
int pos = 0;

// increase size of moveset
int moveset_capacity = 1; // track how many elements allocated

void increaseSizeMoveset()
{
    moveset_capacity *= 2;
    moveset = realloc(moveset, moveset_capacity * sizeof(struct coordinate));
}

void putMoveset(struct coordinate coord)
{
    if (pos >= moveset_capacity)
        increaseSizeMoveset();

    moveset[pos] = coord;
    pos++;
}

void clearMoveset()
{
    moveset_capacity = 1;
    moveset = realloc(moveset, moveset_capacity * sizeof(struct coordinate));
    pos = 0;
}

// Init the board with
int init()
{
    // initialise moveset variable
    moveset = malloc(10 * sizeof(struct coordinate));

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
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (board[i][j].recog != 0)
            {
                if (board[i][j].color == 1)
                    printf("%c ", toupper(board[i][j].recog));
                else
                    printf("%c ", board[i][j].recog);
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

        // takes requested move coordinate by user
        input(*turn, move);

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
            /* save current source piece (after any flags you set above) for rollback */
            struct piece src_backup = board[move[0].x][move[0].y];

            /* save the piece currently on destination for rollback */
            struct piece captured = board[move[1].x][move[1].y];

            /* en-passant bookkeeping */
            struct piece ep_victim = {0};
            int ep_vx = -1, ep_vy = -1;
            int ep_taken = 0;

            /* en-passant detection & remove victim (if any) */
            if (move[0].y != move[1].y && move[0].x != move[1].x)
            {
                if (p.recog == WP || p.recog == BP)
                {
                    if (board[move[1].x][move[1].y].recog == NONE)
                    {
                        int vx = (p.recog == WP) ? move[1].x + 1 : move[1].x - 1;
                        int vy = move[1].y;
                        if (vx >= 0 && vx < 8 && vy >= 0 && vy < 8)
                        {
                            struct piece maybe = board[vx][vy];
                            if ((maybe.recog == WP || maybe.recog == BP) &&
                                maybe.color != p.color &&
                                maybe.doubleMove == 1)
                            {
                                ep_victim = maybe;
                                ep_vx = vx; ep_vy = vy;
                                ep_taken = 1;
                                board[vx][vy] = (struct piece){{vx, vy}, NONE, 0, 0, 0, 0};
                            }
                        }
                    }
                }
            }

            /* apply the move (move piece struct to destination) */
            board[move[1].x][move[1].y] = board[move[0].x][move[0].y];
            board[move[1].x][move[1].y].coord.x = move[1].x;
            board[move[1].x][move[1].y].coord.y = move[1].y;
            board[move[0].x][move[0].y] = (struct piece){{move[0].x, move[0].y}, NONE, 0, 0, 0, 0};

            /* generate opponent attack map (generateAM should build attacks for the opponent) */
            struct coordinate king_pos = generateAM(move);
            for (int i = 0; i < 8; i++)
            {
                for (int j = 0; j < 8; j++)
                    printf("%d ", attack_map[i][j]); // prints 0 or 1
                printf("\n");
            }

            /* find defender king (the side that attempted this move) */
            int defender_color = src_backup.color;
            struct coordinate defender_king = {-1, -1};
            for (int i = 0; i < 8; i++)
            {
                for (int j = 0; j < 8; j++)
                {
                    if ((board[i][j].recog == WK && defender_color == 1) ||
                        (board[i][j].recog == BK && defender_color == 0))
                    {
                        defender_king.x = i;
                        defender_king.y = j;
                        break;
                    }
                }
                if (defender_king.x != -1)
                    break;
            }

            /* if defender king is attacked -> rollback everything and continue */
            if (defender_king.x >= 0 && attack_map[defender_king.x][defender_king.y])
            {
                /* restore source (use src_backup to preserve flags exactly) */
                board[move[0].x][move[0].y] = src_backup;
                board[move[0].x][move[0].y].coord.x = move[0].x;
                board[move[0].x][move[0].y].coord.y = move[0].y;

                /* restore destination */
                board[move[1].x][move[1].y] = captured;

                /* restore en-passant victim if removed */
                if (ep_taken && ep_vx >= 0)
                    board[ep_vx][ep_vy] = ep_victim;

                /* illegal move — do not switch turn */
                continue;
            }

            /* move is legal: proceed to switch turn */
            *turn = (*turn == 'b') ? 'w' : 'b';
        }

        // clearing moveset
        clearMoveset();
    }
}


/*--------------------------VALIDITY OF MOVE CHECK-----------------*/

struct coordinate generateAM(struct coordinate move[])
{
    struct coordinate king_pos = { -1, -1 };
    struct coordinate current_coord;
    memset(attack_map, 0, sizeof(attack_map));
    clearMoveset();

    /* attacker is the opponent of the piece now on move[1] (the mover) */
    int attacker_color = board[move[1].x][move[1].y].color ^ 1;

    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            struct piece current = board[i][j];
            if (current.recog == NONE) continue;

            /* only generate attacks for attacker_color */
            if (current.color != attacker_color) {
                /* but still record king position if it's defender king (optional) */
                if (current.recog == WK || current.recog == BK) {
                    /* nothing — attacker_color != current.color so this is opponent king */
                }
                continue;
            }

            /* record king pos for defender side if encountered (not strictly needed here) */
            if (current.recog == WK || current.recog == BK)
            {
                king_pos.x = i;
                king_pos.y = j;
            }

            current_coord.x = i;
            current_coord.y = j;

            /* prepare nmove safely: source is (i,j); destination set to a safe value */
            struct coordinate nmove[2];
            nmove[0].x = i; nmove[0].y = j;
            nmove[1].x = i; nmove[1].y = j; /* moveValidity must not read uninitialized nmove[1] */

            moveValidity(nmove, true);
        }
    }

    /* set attack_map from moveset with bounds check */
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

    // to encompass en passant,
    // moved -> 1
    // doubleMove -> 0 previous move wasnt double
    // doubleMove -> 1 previous move was double

    struct piece p = board[move[0].x][move[0].y];

    if (p.color == 0)
    {
        if (!AMCall)
        {
            // Adding double moves
            //+2
            if (p.moved == 0)
            {
                if (move[0].x + 2 <= 7 && board[move[0].x + 2][move[1].y].recog == NONE)
                {
                    struct coordinate coord = {move[0].x + 2, move[0].y};
                    putMoveset(coord);
                }
            }

            //+1
            if (move[0].x + 1 <= 7 && board[move[0].x + 1][move[1].y].recog == NONE)
            {
                struct coordinate coord = {move[0].x + 1, move[0].y};
                putMoveset(coord);
            }
        }

        // diagonal right
        if (move[0].y + 1 <= 7 && move[0].x + 1 <= 7)
        {
            if (board[move[0].x + 1][move[1].y + 1].recog != NONE)
            {
                if (p.color != board[move[0].x + 1][move[1].y + 1].color)
                {
                    struct coordinate coord = {move[0].x + 1, move[0].y + 1};
                    putMoveset(coord);
                }
            }
        }

        // diagonal left
        if (move[0].y - 1 >= 0 && move[0].x + 1 <= 7)
        {
            if (board[move[0].x + 1][move[1].y - 1].recog != NONE)
            {
                if (p.color != board[move[0].x + 1][move[1].y - 1].color)
                {
                    struct coordinate coord = {move[0].x + 1, move[0].y - 1};
                    putMoveset(coord);
                }
            }
        }

        if (!AMCall)
        {
            // en passant
            if (move[0].x + 1 <= 7 &&
                move[0].y - 1 >= 0 &&
                (board[move[0].x][move[0].y - 1].recog == WP ||
                 board[move[0].x][move[0].y - 1].recog == BP) &&
                board[move[0].x][move[0].y - 1].color != p.color &&
                board[move[0].x][move[0].y - 1].doubleMove == 1)
            {

                struct coordinate coord = {move[0].x + 1, move[0].y - 1};
                putMoveset(coord);
            }

            if ((move[0].x + 1 <= 7 &&
                     move[0].y + 1 <= 7 &&
                     board[move[0].x][move[0].y + 1].recog == WP ||
                 board[move[0].x][move[0].y + 1].recog == BP) &&
                board[move[0].x][move[0].y + 1].color != p.color &&
                board[move[0].x][move[0].y + 1].doubleMove == 1

            )
            {

                struct coordinate coord = {move[0].x + 1, move[0].y + 1};
                putMoveset(coord);
            }
        }
    }
    else
    {

        if (!AMCall)
        {

            // Adding double moves
            //-2
            if (p.moved == 0)
            {
                if (move[0].x - 2 >= 0 && board[move[0].x - 2][move[1].y].recog == NONE)
                {
                    struct coordinate coord = {move[0].x - 2, move[0].y};
                    putMoveset(coord);
                }
            }

            //+1
            if (move[0].x - 1 >= 0 && board[move[0].x - 1][move[1].y].recog == NONE)
            {
                struct coordinate coord = {move[0].x - 1, move[0].y};
                putMoveset(coord);
            }
        }
    }
    // diagonal left
    if (move[0].y - 1 >= 0 && move[0].x - 1 >= 0)
    {
        if (board[move[0].x - 1][move[1].y - 1].recog != NONE)
        {
            if (p.color != board[move[0].x - 1][move[1].y - 1].color)
            {
                struct coordinate coord = {move[0].x - 1, move[0].y - 1};
                putMoveset(coord);
            }
        }
    }

    // diagonal right
    if (move[0].y + 1 <= 7 && move[0].x - 1 >= 0)
    {
        if (board[move[0].x - 1][move[1].y + 1].recog != NONE)
        {
            if (p.color != board[move[0].x - 1][move[1].y + 1].color)
            {
                struct coordinate coord = {move[0].x - 1, move[0].y + 1};
                putMoveset(coord);
            }
        }
    }
    if (!AMCall)
    {
        // en passant

        if (move[0].x - 1 >= 0 &&
            move[0].y - 1 >= 0 &&
            (board[move[0].x][move[0].y - 1].recog == WP ||
             board[move[0].x][move[0].y - 1].recog == BP) &&
            board[move[0].x][move[0].y - 1].color != p.color &&
            board[move[0].x][move[0].y - 1].doubleMove == 1

        )
        {

            struct coordinate coord = {move[0].x - 1, move[0].y - 1};
            putMoveset(coord);
        }

        if (move[0].x - 1 >= 0 &&
            move[0].y + 1 <= 7 &&
            (board[move[0].x][move[0].y + 1].recog == WP ||
             board[move[0].x][move[0].y + 1].recog == BP) &&
            board[move[0].x][move[0].y + 1].color != p.color &&
            board[move[0].x][move[0].y + 1].doubleMove == 1

        )
        {

            struct coordinate coord = {move[0].x - 1, move[0].y + 1};
            putMoveset(coord);
            // removing pawn capture due to en passant
        }
    }
}

/*-----------------------------------------------------------------*/

// Make the move after checking legality of the move

// take move input in format e3b4
void input(char turn, struct coordinate move[2])
{
    char coords[5];
    printf("Enter move for %c: ", turn);
    scanf("%4s", coords);
    convertToCoord(coords, move);
}

void convertToCoord(char coord[], struct coordinate move[])
{
    // expecting input like "a8a6"
    int col1 = coord[0] - 'a';       // file → col
    int row1 = 8 - (coord[1] - '0'); // rank → row (flip so 8=0, 1=7)

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
