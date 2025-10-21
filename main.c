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

// Piece textures
SDL_Texture *pieceTextures[12];

int init();
int print_board(struct piece board[8][8]);
SDL_Texture *loadTexture(SDL_Renderer *renderer, const char *path);
void drawPiece(SDL_Renderer *renderer, SDL_Texture *texture, int row, int col);
int getTextureIndex(struct piece p);
void input(char turn, struct coordinate move[2]);
void convertToCoord(char coord[], struct coordinate move[2]);
void moveValidity(struct coordinate move []);
void movePawn(struct coordinate move []);

//moveset coordinate array
struct coordinate* moveset; 
int pos = 0;

//increase size of moveset
void increaseSizeMoveset(){
    moveset =  realloc(moveset, sizeof( moveset) * 2);
}

void putMoveset(struct coordinate coord){

    if(sizeof(moveset) / sizeof(struct coordinate) == pos-1 ) increaseSizeMoveset();
    moveset[pos] =  coord; 
    pos++;

}

void clearMoveset(){
    moveset =  realloc(moveset, sizeof( struct coordinate));
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

void* run(void* arg){

    //keeping turn as a pointer for preview later, possible errors in this section
    //player move
    char pmove = 'w';
    char* turn = &pmove;
    struct coordinate move[2];
    
    while(true){
        
        // takes requested move coordinate by user
        input(*turn, move);

        // checks the validity of input provided
        if(board[move[0].x][move[0].y].recog == NONE){
            printf("Empty square cannot be selected\n");
           
            continue;
        }
        else if(board[move[0].x][move[0].y].color == 0 && *turn == 'w'){
            printf("You cannot move opponent's piece\n");
            continue;
        }
        else if(board[move[0].x][move[0].y].color == 1 && *turn == 'b'){
            printf("You cannot move opponent's piece\n");
            continue;
        }else if(board[move[1].x][move[1].y].recog != NONE && board[move[0].x][move[0].y].color == board[move[1].x][move[1].y].color){
            printf("You cannot capture your own piece\n");
            continue;
        }
        // if the move is proper the coordinates are stored in the move array in from(0) and to(1) manner
        //updating the board
        /*--------------------------VALIDITY OF MOVE CHECK-----------------*/

        moveValidity(move);
        for(int i =0 ; i < pos; i++){
            printf("X: %d,Y: %d \n",moveset[i].x, moveset[i].y );
        }

        bool canMove = false;
        for(int i =0 ; i < pos; i++){
            if(move[1].x == moveset[i].x && move[1].y == moveset[i].y){
                 canMove = true;
            }
        }




        
        /*-----------------------------------------------------------------*/
        if(canMove){

        board[move[1].x][move[1].y] = board[move[0].x][move[0].y];
        board[move[1].x][move[1].y].coord.x  = move[1].x;
        board[move[1].x][move[1].y].coord.y  = move[1].y;
        board[move[0].x][move[0].y].recog = NONE;

        // alternates between whose move it is now
        *turn = (*turn == 'b') ? 'w' : 'b';

        }

        // clearing moveset
        clearMoveset();


    }

}

/*--------------------------VALIDITY OF MOVE CHECK-----------------*/


void moveValidity(struct coordinate move []){

    if(board[move[0].x][move[0].y].recog == WP || board[move[0].x][move[0].y].recog == BP){
        movePawn(move);
    }else if(board[move[0].x][move[0].y].recog == WR || board[move[0].x][move[0].y].recog == BR ){

    }else if(board[move[0].x][move[0].y].recog == WN || board[move[0].x][move[0].y].recog == BN){

    }else if(board[move[0].x][move[0].y].recog == WB || board[move[0].x][move[0].y].recog == BB){

    }else if(board[move[0].x][move[0].y].recog == WQ || board[move[0].x][move[0].y].recog == BQ){

    }else if(board[move[0].x][move[0].y].recog == WK || board[move[0].x][move[0].y].recog == BK){

    }else{
        printf("None");
    }
        
    
    
}

void movePawn(struct coordinate move []){
    // to encompass en passant,
    // moved -> 1
    // doubleMove -> 0 previous move wasnt double
    // doubleMove -> 1 previous move was double

   struct piece p = board[move[0].x][move[0].y];
    
    if(p.color == 0) {
        //Adding double moves
        //+2
        if(p.moved == 0){
            if(move[0].x + 2 <= 7 && board[move[0].x+2][move[1].y].recog == NONE ){
            struct coordinate coord = {move[0].x +2, move[0].y};
            putMoveset(coord);
            }
            p.doubleMove = 1;
            p.moved = 1;
        }

        //+1
        if(move[0].x + 1 <= 7 && board[move[0].x +1][move[1].y].recog == NONE ){
            struct coordinate coord = {move[0].x +1, move[0].y};
            putMoveset(coord);
            p.doubleMove = 0;
            p.moved = 1;
        }

        // diagonal right
        if(move[0].y + 1 <= 7 && move[0].x + 1 <= 7){
            if(board[move[0].x +1][move[1].y+1].recog != NONE){
                if(p.color != board[move[0].x +1][move[1].y+1].color){
                    struct coordinate coord = {move[0].x +1, move[0].y+1};
                    putMoveset(coord);
                }
            }
            p.doubleMove = 0;
            p.moved = 1;
        }

        //diagonal left 
        if(move[0].y - 1 >= 0 && move[0].x + 1 <= 7){
            if(board[move[0].x +1][move[1].y-1].recog != NONE){
                if(p.color != board[move[0].x +1][move[1].y-1].color){
                    struct coordinate coord = {move[0].x +1, move[0].y-1};
                    putMoveset(coord);
                }
            }
            p.doubleMove = 0;
            p.moved = 1;
        }

        //en passant 

        if(move[0].y-1 >= 0){
            

        }

        if(move[0].y+1 <= 7){

        }


    }else{
        //Adding double moves
        //-2
        if(p.moved == 0){
            if(move[0].x - 2 >= 0 && board[move[0].x-2][move[1].y].recog == NONE ){
            struct coordinate coord = {move[0].x -2, move[0].y};
            putMoveset(coord);
            }
            p.doubleMove = 1;
            p.moved = 1;
        }

        //+1
        if(move[0].x - 1 >= 0 && board[move[0].x -1][move[1].y].recog == NONE ){
            struct coordinate coord = {move[0].x -1, move[0].y};
            putMoveset(coord);
            p.doubleMove = 0;
            p.moved = 1;
        }

        // diagonal left
        if(move[0].y - 1 >= 0 && move[0].x - 1 >= 0){
            if(board[move[0].x - 1][move[1].y- 1].recog != NONE){
                if(p.color != board[move[0].x - 1][move[1].y - 1].color){
                    struct coordinate coord = {move[0].x - 1, move[0].y- 1};
                    putMoveset(coord);
                }
            
            }
            p.doubleMove = 0;
            p.moved = 1;
        }

        // diagonal right
        if(move[0].y + 1 <=7  && move[0].x - 1 >= 0){
            if(board[move[0].x - 1][move[1].y+ 1].recog != NONE){
                if(p.color != board[move[0].x - 1][move[1].y + 1].color){
                    struct coordinate coord = {move[0].x - 1, move[0].y + 1};
                    putMoveset(coord);
                }
            
            }
            p.doubleMove = 0;
            p.moved = 1;
        }

        //en passant 

    }

        
}




        
/*-----------------------------------------------------------------*/



//Make the move after checking legality of the move


//take move input in format e3b4
void input(char turn, struct coordinate move[2]){
    char coords[5];
    printf("Enter move for %c: ",turn);
    scanf("%4s", coords);
    convertToCoord(coords, move);

}

void convertToCoord(char coord[], struct coordinate move[]){
    // expecting input like "a8a6"
    int col1 = coord[0] - 'a';       // file → col
    int row1 = 8 - (coord[1] - '0'); // rank → row (flip so 8=0, 1=7)

    int col2 = coord[2] - 'a';
    int row2 = 8 - (coord[3] - '0');

    if (0 <= col1 && col1 < 8 && 0 <= col2 && col2 < 8 &&
        0 <= row1 && row1 < 8 && 0 <= row2 && row2 < 8) {

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
    
    //Create seperate thread for the terminal input function
    pthread_t inputThread;
    pthread_create(&inputThread,NULL, run,NULL);

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
        
        //Chess Logic code 



        
        

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


