#ifndef MOVE_H
#define MOVE_H

#include <string>

class Position;
#include "position.h"

class Move {
    public:
        unsigned char from;
        unsigned char to;
        unsigned char specialMove; //1: en passant, 2: castling 3: promotion
        unsigned char flag; // promotion move : KNIGHT/BISHOP/ROOK/QUEEN
                            // castling: WHITE_CASTLE_KINGSIDE/WHITE_CASTLE_QUEENSIDE/BLACK_CASTLE_KINGSIDE/BLACK_QUEENSIDE

        Move();

        /**
         * initializes the move with the given from and to squares. The move is neither promotion, castling move nor en-passant-capture
         */
        Move(char from, char to);


        Move(char from, char to, char specialMove, char flag);

        /**
         * rebuilds the compressed move
         * @param compressedMove 
         */
        Move(unsigned short compressedMove);
        
        /**
         * returns the the algebraic notation of the move, used by the uci protocoll 
         */
        std::string toString();

        /**
         * compresses the move to 16 bits
         */
        short compress();

        //overloaded operators, for checking equality and inequality of two moves
        friend bool operator==(const Move& m1, const Move& m2);
        friend bool operator!=(const Move& m1, const Move& m2);

    private:
        std::string intToField(char field);
};

#endif