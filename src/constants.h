#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string.h>

const char NO_PIECE = 0;
const char PAWN = 1;
const char KNIGHT = 2;
const char BISHOP = 3;
const char ROOK = 4;
const char QUEEN = 5;
const char KING = 6;


const char NORTH = 0;
const char NORTH_EAST = 1;
const char EAST = 2;
const char SOUTH_EAST = 3;
const char SOUTH = 4;
const char SOUTH_WEST = 5;
const char WEST = 6;
const char NORTH_WEST = 7;


const std::string START_POSITION_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";


const char WHITE = 0;
const char BLACK = 1;

#endif