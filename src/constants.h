#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string.h>

const char NO_PIECE = -1;
const char PAWN = 0;
const char KNIGHT = 1;
const char BISHOP = 2;
const char ROOK = 3;
const char QUEEN = 4;
const char KING = 5;


const char NORTH = 0;
const char NORTH_EAST = 1;
const char EAST = 2;
const char SOUTH_EAST = 3;
const char SOUTH = 4;
const char SOUTH_WEST = 5;
const char WEST = 6;
const char NORTH_WEST = 7;


const char WHITE_CASTLE_KINGSIDE = 0;
const char WHITE_CASTLE_QUEENSIDE = 1;
const char BLACK_CASTLE_KINGSIDE = 2;
const char BLACK_CASTLE_QUEENSIDE = 3;

const char CASTLE_ROOK_DEST[4] = {
    61, 59, 5, 3
};

const char CASTLE_ROOK_ORIGIN[4] = {
    63, 56, 7, 0
};


const std::string START_POSITION_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";


const char WHITE = 0;
const char BLACK = 1;


const uint64_t lightSquares = 0xaa55aa55aa55aa55;
const uint64_t darkSquares = 0x55aa55aa55aa55aa;


#endif