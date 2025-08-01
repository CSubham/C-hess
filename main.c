//gcc main.c -o main $(sdl2-config --cflags --libs)

//Linkers
#include <stdio.h>
#include <ctype.h>
#include <SDL2/SDL.h>
#include <stdbool.h>


// definition of recognizers
#define Pawn 'p'
#define Rook 'r'
#define Knight 'n'
#define Bishop 'b'
#define King 'k'
#define Queen 'q'

// position on the board
struct coordinate {
    int x;
    int y;
};

// pieces 
struct piece {
    struct coordinate coord;
    char recog;
    int moved;
    int castled;
    int doubleMove;
    int color;
};

// board representation 
struct piece board[8][8];
 
int init(){
    // placing pawns for both colors
    for(int i = 0; i <8; i++){
        struct piece wpawn = {{-1,-1},Pawn,0,0,0,1};
        board[6][i] = wpawn;
        struct piece bpawn = {{-1,-1},Pawn,0,0,0,0};
        board[1][i] = bpawn;
    }
    //White pieces
    struct piece wrook1 = {{7, 0}, Rook,   0, 0, 0, 1};
    struct piece wknight1 = {{7, 1}, Knight, 0, 0, 0, 1};
    struct piece wbishop1 = {{7, 2}, Bishop, 0, 0, 0, 1};
    struct piece wqueen = {{7, 3}, Queen,  0, 0, 0, 1};
    struct piece wking = {{7, 4}, King,   0, 0, 0, 1};
    struct piece wbishop2 = {{7, 5}, Bishop, 0, 0, 0, 1};
    struct piece wknight2 = {{7, 6}, Knight, 0, 0, 0, 1};
    struct piece wrook2 = {{7, 7}, Rook,   0, 0, 0, 1};
  
    //placing pieces
    board[7][0] = wrook1;
    board[7][1] = wknight1;
    board[7][2] = wbishop1;
    board[7][3] = wqueen;
    board[7][4] = wking;
    board[7][5] = wbishop2;
    board[7][6] = wknight2;
    board[7][7] = wrook2;

    //Black pieces
    struct piece brook1 = {{0, 0}, Rook,   0, 0, 0, 0};
    struct piece bknight1 = {{0, 1}, Knight, 0, 0, 0, 0};
    struct piece bbishop1 = {{0, 2}, Bishop, 0, 0, 0, 0};
    struct piece bking = {{0, 3}, King,   0, 0, 0, 0};
    struct piece bqueen = {{0, 4}, Queen,  0, 0, 0, 0};
    struct piece bbishop2 = {{0, 5}, Bishop, 0, 0, 0, 0};
    struct piece bknight2 = {{0, 6}, Knight, 0, 0, 0, 0};
    struct piece brook2 = {{0, 7}, Rook,   0, 0, 0, 0};
  
    //placing pieces
    board[0][0] = brook1;
    board[0][1] = bknight1;
    board[0][2] = bbishop1;
    board[0][3] = bking;
    board[0][4] = bqueen;
    board[0][5] = bbishop2;
    board[0][6] = bknight2;
    board[0][7] = brook2;

}

int print_board(struct piece board[8][8]){
    for(int i =0; i < 8; i ++){
        for(int j =0; j < 8; j++){
            if(board[i][j].recog != 0){
                if(board[i][j].color == 1 )
                printf("%c ", toupper( board[i][j].recog));
                else
                    printf("%c ",  board[i][j].recog);
                

            }else printf("_ ");
        }
        printf("\n");
    }

}


#define WINDOW_SIZE 960
#define CELL_SIZE 120


int main(){
   SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow(
        "Chessboard",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_SIZE, WINDOW_SIZE,
        0
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            SDL_Rect square = {
                col * CELL_SIZE,
                row * CELL_SIZE,
                CELL_SIZE,
                CELL_SIZE
            };

            
            if ((row + col) % 2 == 0)
                SDL_SetRenderDrawColor(renderer, 240, 217, 181, 255); // Light
            else
                SDL_SetRenderDrawColor(renderer, 181, 136, 99, 255);  // Dark

            SDL_RenderFillRect(renderer, &square);
        }
    }

    SDL_RenderPresent(renderer);
    SDL_Event event;
bool running = true;

while (running) {
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
        }
    }

    // Clear screen
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); 
    SDL_RenderClear(renderer);

    // Draw chessboard squares
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            SDL_Rect square = {
                col * CELL_SIZE,
                row * CELL_SIZE,
                CELL_SIZE,
                CELL_SIZE
            };
            if ((row + col) % 2 == 0)
                SDL_SetRenderDrawColor(renderer, 240, 217, 181, 255); // Light
            else
                SDL_SetRenderDrawColor(renderer, 181, 136, 99, 255);  // Dark

            SDL_RenderFillRect(renderer, &square);
        }
    }

    // TODO: Draw pieces here if needed

    SDL_RenderPresent(renderer);
    SDL_Delay(16);
}
    return 0;
}

