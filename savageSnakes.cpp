#include "SDL.h"
#include "SDL_image.h"
#include <iostream>
#include <vector>
#include <iterator>
#include <string>
#include <random>
#include <chrono>
#include <list>

bool running = 1;
enum directions { NORTH, EAST, SOUTH, WEST, TOP_RIGHT_CORNER, TOP_LEFT_CORNER, BOTTEM_RIGHT_CORNER, BOTTEM_LEFT_CORNER };
enum gameStates { ASDF, GAME_TICK, DRAW };
enum snakeColors {GREEN, BLUE, RED};

//These strings are the file names for the different possible snakeParts textures
std::string greenHeadString   = "greenHead.png";
std::string greenBodyString   = "greenBody.png";
std::string greenTailString   = "greenTail.png";
std::string greenCornerString = "greenBodyCorner.png";

std::string blueHeadString   = "blueHead.png";
std::string blueBodyString   = "blueBody.png";
std::string blueTailString   = "blueTail.png";
std::string blueCornerString = "blueBodyCorner.png";

std::string redHeadString   = "redHead.png";
std::string redBodyString   = "redBody.png";
std::string redTailString   = "redTail.png";
std::string redCornerString = "redBodyCorner.png";

//global vars for the window, renderer, and event
SDL_Window   *window;
SDL_Renderer *renderer;
SDL_Event e;

int snakeID = 0;
int windowHeight = 800;
int windowWidth = 500;

//These are the number ranges that use the rng, used for the spawning x position of the snake, the length of the snake, and the color of the snake
std::uniform_real_distribution <double> startingXRange(0, windowWidth / 50);
std::uniform_real_distribution <double> lengthRange(3, 10);
std::uniform_real_distribution <double> colorRange(0, 3);
std::default_random_engine rng(std::chrono::system_clock::now().time_since_epoch().count());

// loadTexture() takes the filename and renderer and loads the image to a texture
SDL_Texture *loadTexture(const std::string &file, SDL_Renderer *ren) {
    SDL_Texture *texture = IMG_LoadTexture(renderer, file.c_str());
    if(texture == nullptr) std::cout << "ERROR" << std::endl;
    return texture;
}

// init() initializes SDL and creates an SDL_Window and SDL_Renderer
void init() {
    SDL_Init(SDL_INIT_EVERYTHING);

    window   = SDL_CreateWindow("Snakes", 10, 30, windowWidth, windowHeight, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
}


// tickCallBack() creates a userevent and pushes it to the queue at the given interval. I use that userevent as the gametick to move the snakes
Uint32 tickCallBack(Uint32 interval, void* param) {
    SDL_Event e;
    SDL_UserEvent userevent;
    userevent.type = SDL_USEREVENT;
    userevent.code = GAME_TICK;
    userevent.data1 = NULL;
    userevent.data2 = NULL;

    e.type = SDL_USEREVENT;
    e.user = userevent;

    SDL_PushEvent(&e);
    return interval;
}

/******************************************
*           SnakePart class               *
*                                         *
*******************************************/
struct SnakePart {
    SnakePart();
    void render();

    SDL_Texture * headTexture;
    SDL_Texture * bodyTexture;
    SDL_Texture * tailTexture;
    SDL_Texture * cornerTexture;
    
    int         slot;
    int         direction;
    int         corner;
    int         color;
    bool        isHead;
    bool        isBody;
    bool        isTail;
    SDL_Rect    sRect;
    std::string file;
};

SnakePart::SnakePart() {
    std::cout << "Constructor call" << std::endl;
    sRect.h = 50;
    sRect.w = 50;
    sRect.x = 0;
    sRect.y = 0;

    headTexture = nullptr;
    bodyTexture = nullptr;
    tailTexture = nullptr;
    cornerTexture = nullptr;

    direction = SOUTH;
    isHead = false;
    isBody = false;
    isTail = false;
}


// figures out which SnakePart::texture to render based on SnakePart::corner and SnakePart::direction
void SnakePart::render() {
    SDL_Texture * tex = nullptr;

    if (isHead == true) {
        tex = headTexture;
    } else if (isBody) {
        tex = bodyTexture;
    } else {
        tex = tailTexture;
    }
    if (corner == BOTTEM_LEFT_CORNER) {
         SDL_RenderCopy(renderer, cornerTexture, NULL, &sRect);
         std::cout << "bottem left\n";
    } else if (corner == BOTTEM_RIGHT_CORNER) {
        SDL_RenderCopyEx(renderer, cornerTexture, NULL, &sRect, 270, NULL, SDL_FLIP_NONE);
        std::cout << "bottem right\n";
    } else if (corner == TOP_LEFT_CORNER) {
        SDL_RenderCopyEx(renderer, cornerTexture, NULL, &sRect, 90, NULL, SDL_FLIP_NONE);
        std::cout << "top left\n";
    } else if (corner == TOP_RIGHT_CORNER) {
        SDL_RenderCopyEx(renderer, cornerTexture, NULL, &sRect, 180, NULL, SDL_FLIP_NONE);
        std::cout << "top right\n";
    } else {
        if (direction == NORTH) {
            SDL_RenderCopyEx(renderer, tex, NULL, &sRect, 90, NULL, SDL_FLIP_NONE);
        } else if (direction == EAST) {
            SDL_RenderCopyEx(renderer, tex, NULL, &sRect, 0, NULL, SDL_FLIP_HORIZONTAL);
        } else if (direction == SOUTH) {
            SDL_RenderCopyEx(renderer, tex, NULL, &sRect, 270, NULL, SDL_FLIP_NONE);
        } else if (direction == WEST) {
            SDL_RenderCopy(renderer, tex, NULL, &sRect);
        }
    }
} 

/******************************************
*               Snake class               *
*                                         *
*  an object with a vector of SnakeParts  *
*******************************************/

struct Snake {
    int                    id;
    int                    length;
    int                    direction;
    int                    color;
    bool                   wantsSouth;
    bool                   dieNextTick;
    SDL_Point              vel;
    std::vector<SnakePart> snakeVecMember;

    Snake() {
        id = snakeID;
        direction = SOUTH;
        length = lengthRange(rng);
        dieNextTick = false;
        vel.x = 0;
        vel.y = 50;
        color = colorRange(rng);
        createSnakeParts(length, color);
        texSnakeParts();
        arrangeSnakeParts();
        ++snakeID;
    }

    ~Snake(){
        if(dieNextTick){
            for (int i = 0; i < snakeVecMember.size(); ++i) {
                SDL_DestroyTexture(snakeVecMember.at(i).headTexture);
                SDL_DestroyTexture(snakeVecMember.at(i).bodyTexture);
                SDL_DestroyTexture(snakeVecMember.at(i).tailTexture);
                SDL_DestroyTexture(snakeVecMember.at(i).cornerTexture);
            }
        }
    }

    void createSnakeParts(int length, int color);
    void render();
    void arrangeSnakeParts();
    void texSnakeParts();
    void orient();
    void move();
    bool collisionSnakeCheck();
};


// createSnakeParts constructs a SnakePart and sets the color, colorStrings, and sets the isHead, isBody, isTail booleans. Pushes the created SnakePart to the snakeVecMember vector;
void Snake::createSnakeParts(int length, int color) {
    SnakePart sp;
    sp.color = color;
    std::string colorHeadString;
    std::string colorBodyString;
    std::string colorTailString;

    if (color == GREEN) {
        colorHeadString = "greenHead.png";
        colorBodyString = "greenBody.png";
        colorTailString = "greenTail.png";
    } else if (color == BLUE) {
        colorHeadString = "blueHead.png";
        colorBodyString = "blueBody.png";
        colorTailString = "blueTail.png";
    } else if (color == RED) {
        colorHeadString = "redHead.png";
        colorBodyString = "redBody.png";
        colorTailString = "redTail.png";
    }

    for (int i = 0; i < length; ++i) {
        sp.slot = i;
        if (i == 0) {
            sp.file = colorHeadString;
            sp.isHead = true;
            sp.isBody = false;
            sp.isTail = false;
        } else if (i == length - 1) {
            sp.file = colorTailString;
            sp.isHead = false;
            sp.isBody = false;
            sp.isTail = true;
        } else {
            sp.file = colorBodyString;
            sp.isHead = false;
            sp.isBody = true;
            sp.isTail = false;
        }
        snakeVecMember.push_back(sp);
        
    } 
}

// render() iterates through the snakeVecMember vector and calls the render function of each SnakePart
void Snake::render() {
    for (auto it = snakeVecMember.begin(); it != snakeVecMember.end(); ++it){
        it->render();
    }
}

// arrangeSnakeParts() iterates through the snakeVecMember vector and sets the initial sRect.x and sRect.y 
void Snake::arrangeSnakeParts() {
    int y = 0;
    for (auto it = snakeVecMember.begin(); it != snakeVecMember.end(); ++it) {
        it->sRect.x = 50 * (int)startingXRange(rng);
        it->sRect.y = y;
        y -= 50;
    }
}

// textSnakeParts() sets the correct texture for the correct snake color
void Snake::texSnakeParts() {
    if (color == GREEN) {
        for (auto it = snakeVecMember.begin(); it != snakeVecMember.end(); ++it) {
            it->headTexture = loadTexture(greenHeadString, renderer);
            it->bodyTexture = loadTexture(greenBodyString, renderer);
            it->tailTexture = loadTexture(greenTailString, renderer);
            it->cornerTexture = loadTexture(greenCornerString, renderer);
        }
    } else if (color == BLUE) {
        for (auto it = snakeVecMember.begin(); it != snakeVecMember.end(); ++it) {
            it->headTexture = loadTexture(blueHeadString, renderer);
            it->bodyTexture = loadTexture(blueBodyString, renderer);
            it->tailTexture = loadTexture(blueTailString, renderer);
            it->cornerTexture = loadTexture(blueCornerString, renderer);
        }
    } else if (color == RED) {
        for (auto it = snakeVecMember.begin(); it != snakeVecMember.end(); ++it) {
            it->headTexture = loadTexture(redHeadString, renderer);
            it->bodyTexture = loadTexture(redBodyString, renderer);
            it->tailTexture = loadTexture(redTailString, renderer);
            it->cornerTexture = loadTexture(redCornerString, renderer);
        }
    } else std::cout << "failed to texture snake" << std::endl;
}

// orient() sets the updated SnakePart::direction, gets called at the end of the Snake::move() function;
// the new direction is based on the current SnakeParts direction and the direction of the snakePart in front of it.
// so if the snakepart2 is facing south and snakepart1 is facing west, the Snake::corner will be set to BOTTEM_LEFT_CORNER
void Snake::orient() {
    for (auto it = snakeVecMember.rbegin(); it != snakeVecMember.rend(); ++it) {
        if (it == std::prev(snakeVecMember.rend())) {
            it->direction = direction;
        } else if (it == snakeVecMember.rbegin()) {
            it->direction = std::next(it+1)->direction;
        } else {
            it->direction = std::next(it)->direction; 
        }
    }
    
    for (auto snakeIt = snakeVecMember.begin(); snakeIt != (snakeVecMember.end()-1); ++snakeIt) {
        if (snakeIt->direction == SOUTH && std::next(snakeIt)->direction == EAST) {
            std::next(snakeIt)->corner = BOTTEM_LEFT_CORNER;
        } else if (snakeIt->direction == SOUTH && std::next(snakeIt)->direction == WEST) {
            std::next(snakeIt)->corner = BOTTEM_RIGHT_CORNER;
        } else if (snakeIt->direction == NORTH && std::next(snakeIt)->direction == WEST) {
            std::next(snakeIt)->corner = TOP_RIGHT_CORNER;
        } else if (snakeIt->direction == NORTH && std::next(snakeIt)->direction == EAST) {
            std::next(snakeIt)->corner = TOP_LEFT_CORNER;
        } else if (snakeIt->direction == WEST && std::next(snakeIt)->direction == SOUTH) {
            std::next(snakeIt)->corner = TOP_LEFT_CORNER;
        } else if (snakeIt->direction == EAST && std::next(snakeIt)->direction == NORTH) {
            std::next(snakeIt)->corner = BOTTEM_RIGHT_CORNER;
        } else if (snakeIt->direction == EAST && std::next(snakeIt)->direction == SOUTH) {
            std::next(snakeIt)->corner = TOP_RIGHT_CORNER;
        } else if (snakeIt->direction == WEST && std::next(snakeIt)->direction == NORTH) {
            std::next(snakeIt)->corner = BOTTEM_LEFT_CORNER;
        } else {
            std::next(snakeIt)->corner = NULL;
        }
        //If the iterator is pointing to the second to last SnakePart, then make the last SnakePart have no corner 
        if (snakeIt == std::prev(snakeVecMember.end()-1)) {
            std::next(snakeIt)->corner = NULL;
        }
    }
}

// iterates backward through the snakeVecMember and adds the snakePart::vel to the snakePart::sRect
void Snake::move() {
    for (auto it = snakeVecMember.rbegin(); it != snakeVecMember.rend(); ++it) {
        if (it == std::prev(snakeVecMember.rend())) {
            it->sRect.x += vel.x;
            it->sRect.y += vel.y;
        } else {
            it->sRect.x = std::next(it)->sRect.x;
            it->sRect.y = std::next(it)->sRect.y;
        }
    }

    orient();
}

std::vector<Snake> snakeMasterVec;
Snake * snakeMasterArr = new Snake[]();

// iterates through the snakeMasterVec and erase() any Snake with .dieNextTick set to true
void killSnakes() {
    for (int i = 0; i < snakeMasterVec.size(); ++i) {
        if (snakeMasterVec.at(i).dieNextTick) {
            //snakeMasterVec.at(i).snakeVecMember.erase(snakeMasterVec.at(i).snakeVecMember.begin(), snakeMasterVec.at(i).snakeVecMember.end());
            snakeMasterVec.erase(snakeMasterVec.begin() + i);
        }
    }
}

// collisionCheck() takes a pointer to a SDL_Rect and iterates through snakeMasterVec and iterates through snakeVecMember of each Snake
// and uses SDL_HasIntersection to check if the passed in SDL_Rect and the sRect of each SnakePart intersects
bool collisionCheck(SDL_Rect* rectCheck) {
    for (auto snakeIt = snakeMasterVec.begin(); snakeIt != snakeMasterVec.end(); ++snakeIt) {
        for (auto snakePartIt = snakeIt->snakeVecMember.begin(); snakePartIt != snakeIt->snakeVecMember.end(); ++snakePartIt) {
            if (SDL_HasIntersection(rectCheck, &snakePartIt->sRect)) {
                return true;
            }
        }
    }
    return false;
}

// This collisionCheck() checks the color of the passed in snake against the snake that intersects with the rect that was passed in. if so then that snake's dieNextTick bool = true
bool collisionCheck(SDL_Rect* rectCheck, Snake snake, bool * didSnakeDie) {
    for (auto snakeIt = snakeMasterVec.begin(); snakeIt != snakeMasterVec.end(); ++snakeIt) {
        for (auto snakePartIt = snakeIt->snakeVecMember.begin(); snakePartIt != snakeIt->snakeVecMember.end(); ++snakePartIt) {
            if (snakeIt->id != snake.id) {
                if (SDL_HasIntersection(rectCheck, &snakePartIt->sRect)) {
                    if (snakePartIt->color == snake.color) {
                        snakeIt->dieNextTick = true;
                        *didSnakeDie = true;
                    }
                    return true;
                }
            }
        }
    } 
    return false;
}


// Calls either of the collisionCheck() functions and if there is a collision it will check around the head of the snake
//to see if there are any open spaces and if there are the snake will change directions to keep moving until
//no open spaces are available.

bool Snake::collisionSnakeCheck() {
    bool snakeKilled = false;
    SDL_Rect checkRect;
    checkRect.h = 50;
    checkRect.w = 50;
    SDL_Rect checkEastRect;
    checkEastRect.h = 50;
    checkEastRect.w = 50;
    SDL_Rect checkWestRect;
    checkWestRect.h = 50;
    checkWestRect.w = 50;
    SDL_Rect checkSouthRect;
    checkSouthRect.h = 50;
    checkSouthRect.w = 50;
   
    checkRect.x = snakeVecMember.at(0).sRect.x + vel.x;
    checkRect.y = snakeVecMember.at(0).sRect.y + vel.y;

    checkEastRect.x = snakeVecMember.at(0).sRect.x + 50;
    checkEastRect.y = snakeVecMember.at(0).sRect.y + 0;

    checkWestRect.x = snakeVecMember.at(0).sRect.x - 50;
    checkWestRect.y = snakeVecMember.at(0).sRect.y + 0;

    checkSouthRect.x = snakeVecMember.at(0).sRect.x + 0;
    checkSouthRect.y = snakeVecMember.at(0).sRect.y + 50;

    //If there was a collision last tick, and it moved and there
    if (wantsSouth == true && !collisionCheck(&checkSouthRect) && checkRect.y < windowHeight - 50 || wantsSouth == true && collisionCheck(&checkSouthRect, *this, &snakeKilled) && snakeKilled == true && checkRect.y < windowHeight - 50) {
        vel.x = 0;
        vel.y = 50;
        direction = SOUTH;
        wantsSouth = false;
    }

    // Is there a collision?
    if (collisionCheck(&checkRect, *this, &snakeKilled) || checkRect.x < 0 || checkRect.x > windowWidth - 50 || checkRect.y > windowHeight - 50) {
        if (snakeKilled) {
            return false;
        } 
            //Is there space to the South?
            if (!collisionCheck(&checkSouthRect) && checkRect.y < windowHeight - 50) {
                vel.x = 0;
                vel.y = 50;
                direction = SOUTH;
                //Is there space to the West?
            } else if (!collisionCheck(&checkWestRect) && checkRect.x > 0 || collisionCheck(&checkWestRect, *this, &snakeKilled) && snakeKilled == true && checkRect.x > 0) {
                vel.x = -50;
                vel.y = 0;
                direction = WEST;
                wantsSouth = true;
                //Is there space to the East?
            } else if (!collisionCheck(&checkEastRect) && checkRect.x < windowWidth - 50 || collisionCheck(&checkEastRect, *this, &snakeKilled) && snakeKilled == true && checkRect.x < windowWidth - 50) {
                vel.x = 50;
                vel.y = 0;
                direction = EAST;
                wantsSouth = true;
                //If there are no open spaces then return true; there is a full collision.
            }else return true;
    }
    return false;
}

//returns a Snake Object
Snake snakeMakerFunc() {
    Snake snake;
    return snake;
}

int main(int argc, char* argv[]) {
    init();

    snakeMasterVec.push_back(Snake());

    int const TICKDELAY = 100;
    bool addSnake = false;
    SDL_Rect startingRect;
    startingRect.h = 50;
    startingRect.w = 50;
    startingRect.x = 200;
    startingRect.y = 0;

    SDL_TimerID my_timer_id = 0;
  //  my_timer_id = SDL_AddTimer(TICKDELAY, tickCallBack, NULL); 

    while (running) {
        while(SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
                break;
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_UP) {
                    if (!snakeMasterVec.empty()) {
                        snakeMasterVec.erase(snakeMasterVec.begin());
                    } else {
                        snakeMasterVec.push_back(Snake());
                    }
                } else if (e.key.keysym.sym == SDLK_LEFT) {
                    snakeMasterVec.back().vel.x = -50;
                    snakeMasterVec.back().vel.y =  0;
                    snakeMasterVec.back().direction = WEST;
                } else if (e.key.keysym.sym == SDLK_DOWN) {
                    snakeMasterVec.back().vel.x = 0;
                    snakeMasterVec.back().vel.y = 50;
                    snakeMasterVec.back().direction = SOUTH;
                } else if (e.key.keysym.sym == SDLK_RIGHT) {
                    snakeMasterVec.back().vel.x = 50;
                    snakeMasterVec.back().vel.y = 0;
                    snakeMasterVec.back().direction = EAST;
                } else if (e.key.keysym.sym == SDLK_SPACE) {
                    tickCallBack(1000, NULL);
                }
            } else if (e.type = SDL_USEREVENT) {
                //snakeMasterVec.shrink_to_fit();
                if (e.user.code == GAME_TICK) {
                    for (auto it = snakeMasterVec.begin(); it != snakeMasterVec.end(); ++it) {
                        if (!it->collisionSnakeCheck()) {
                            addSnake = false;
                            it->move();
                        } else addSnake = true;
                    }
                    killSnakes();
                    if (addSnake) {
                        if (!collisionCheck(&startingRect)) {
                            snakeMasterVec.emplace_back(Snake());
                        }
                    }
                }
            }
            SDL_SetRenderDrawColor(renderer, 0,0,0,0);
            SDL_RenderClear(renderer);
            for (auto it = snakeMasterVec.begin(); it != snakeMasterVec.end(); ++it){
                it->render();
            }
            SDL_SetRenderDrawColor(renderer, 255,0,0,0);
            for (int i = 0; i < windowHeight / 50; ++i){
                SDL_RenderDrawLine(renderer, 0,i*50,windowWidth,i*50);
            }
            

            SDL_RenderPresent(renderer);
        }
    }
    return 0;
}