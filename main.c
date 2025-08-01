// Compile with: gcc main.c -o main $(sdl2-config --cflags --libs) -lSDL2_image

#include <stdio.h>
#include <ctype.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <SDL2/SDL_image.h>

// --- Definitions and Constants ---

// Piece recognizers
#define Pawn   'p'
#define Rook   'r'
#define Knight 'n'
#define Bishop 'b'
#define King   'k'
#define Queen  'q'

// Board and cell size
#define WINDOW_SIZE 960
#define CELL_SIZE   120

// Enum for piece types (for texture indexing)
enum PieceType {
    WP, WR, WN, WB, WQ, WK, // White pieces
    BP, BR, BN, BB, BQ, BK, // Black pieces
    NONE
};

// --- Struct Definitions ---

// Board coordinate
struct coordinate {
    int x;
    int y;
};

// Piece structure
struct piece {
    struct coordinate coord;
    char recog;
    int moved;
    int castled;
    int doubleMove;
    int color; // 1 = white, 0 = black
};

// --- Global Variables ---

// Board representation
struct piece board[8][8];

// Piece textures
SDL_Texture* pieceTextures[12];

// --- Function Declarations ---

int init();
int print_board(struct piece board[8][8]);
SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* path);
void drawPiece(SDL_Renderer* renderer, SDL_Texture* texture, int row, int col);
int getTextureIndex(struct piece p);

// --- Function Implementations ---

// Initialize the chess board with pieces in starting positions
int init() {
    // Place pawns for both colors
    for (int i = 0; i < 8; i++) {
        struct piece wpawn = {{-1, -1}, Pawn, 0, 0, 0, 1};
        board[6][i] = wpawn;
        struct piece bpawn = {{-1, -1}, Pawn, 0, 0, 0, 0};
        board[1][i] = bpawn;
    }

    // White pieces (bottom row)
    struct piece wrook1   = {{7, 0}, Rook,   0, 0, 0, 1};
    struct piece wknight1 = {{7, 1}, Knight, 0, 0, 0, 1};
    struct piece wbishop1 = {{7, 2}, Bishop, 0, 0, 0, 1};
    struct piece wqueen   = {{7, 3}, Queen,  0, 0, 0, 1};
    struct piece wking    = {{7, 4}, King,   0, 0, 0, 1};
    struct piece wbishop2 = {{7, 5}, Bishop, 0, 0, 0, 1};
    struct piece wknight2 = {{7, 6}, Knight, 0, 0, 0, 1};
    struct piece wrook2   = {{7, 7}, Rook,   0, 0, 0, 1};

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
    struct piece brook1   = {{0, 0}, Rook,   0, 0, 0, 0};
    struct piece bknight1 = {{0, 1}, Knight, 0, 0, 0, 0};
    struct piece bbishop1 = {{0, 2}, Bishop, 0, 0, 0, 0};
    struct piece bking    = {{0, 3}, King,   0, 0, 0, 0};
    struct piece bqueen   = {{0, 4}, Queen,  0, 0, 0, 0};
    struct piece bbishop2 = {{0, 5}, Bishop, 0, 0, 0, 0};
    struct piece bknight2 = {{0, 6}, Knight, 0, 0, 0, 0};
    struct piece brook2   = {{0, 7}, Rook,   0, 0, 0, 0};

    // Place black pieces
    board[0][0] = brook1;
    board[0][1] = bknight1;
    board[0][2] = bbishop1;
    board[0][3] = bking;
    board[0][4] = bqueen;
    board[0][5] = bbishop2;
    board[0][6] = bknight2;
    board[0][7] = brook2;

    return 0;
}

// Print the board to the console (for debugging)
int print_board(struct piece board[8][8]) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (board[i][j].recog != 0) {
                if (board[i][j].color == 1)
                    printf("%c ", toupper(board[i][j].recog));
                else
                    printf("%c ", board[i][j].recog);
            } else {
                printf("_ ");
            }
        }
        printf("\n");
    }
    return 0;
}

// Load image file into SDL_Texture
SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* path) {
    SDL_Surface* surface = IMG_Load(path);
    if (!surface) {
        printf("IMG_Load failed: %s\n", IMG_GetError());
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

// Draw a piece at a given board coordinate
void drawPiece(SDL_Renderer* renderer, SDL_Texture* texture, int row, int col) {
    SDL_Rect dst = {
        col * CELL_SIZE,
        row * CELL_SIZE,
        CELL_SIZE,
        CELL_SIZE
    };
    SDL_RenderCopy(renderer, texture, NULL, &dst);
}

// Map struct piece to enum PieceType index for texture lookup
int getTextureIndex(struct piece p) {
    if (p.recog == 0) return -1;
    bool isWhite = p.color == 1;
    switch (tolower(p.recog)) {
        case 'p': return isWhite ? WP : BP;
        case 'r': return isWhite ? WR : BR;
        case 'n': return isWhite ? WN : BN;
        case 'b': return isWhite ? WB : BB;
        case 'q': return isWhite ? WQ : BQ;
        case 'k': return isWhite ? WK : BK;
    }
    return -1;
}

// --- Main Program ---

int main() {
    // Initialize SDL and SDL_image
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    // Create window and renderer
    SDL_Window* window = SDL_CreateWindow("Chessboard",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_SIZE, WINDOW_SIZE, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    // Initialize the chess board
    init();

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

    // --- Main Loop ---
    bool running = true;
    SDL_Event event;

    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
        }

        // Clear screen to white
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); 
        SDL_RenderClear(renderer);

        // Draw chessboard squares and pieces
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                // Draw board square
                SDL_Rect square = {
                    col * CELL_SIZE,
                    row * CELL_SIZE,
                    CELL_SIZE,
                    CELL_SIZE
                };
                if ((row + col) % 2 == 0)
                    SDL_SetRenderDrawColor(renderer, 240, 217, 181, 255); // Light square
                else
                    SDL_SetRenderDrawColor(renderer, 181, 136, 99, 255);  // Dark square
                SDL_RenderFillRect(renderer, &square);

                // Draw piece if present
                int idx = getTextureIndex(board[row][col]);
                if (idx != -1 && pieceTextures[idx]) {
                    drawPiece(renderer, pieceTextures[idx], row, col);
                }
            }
        }

        // Present the rendered frame
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // --- Cleanup ---
    for (int i = 0; i < 12; i++) {
        if (pieceTextures[i])
            SDL_DestroyTexture(pieceTextures[i]);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}


