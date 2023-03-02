#ifndef MOVE_H
#define MOVE_H

#include <string>

class Move {
    public:
        char from;
        char to;
        char flags; // 2/3/4/5: promote to knight/bishop/rook/queen, 0 if no promotion possible


        Move();

        /**
         * parses the move given in the algebraic notation used by the uci protocoll in the argument
         * @throws invalid_argument if the move is not in the valid format
         */
        Move(std::string move);

        /**
         * initializes the move with the given from and to squares. The move is a move without a promotion
         */
        Move(char from, char to);

        /**
         * initializes the move with the given from and to squares.
         * @param promotion 0 for no promotion, 2 for knight, 2 for bishop, 4 for rook and 5 for queen
         */
        Move(char from, char to, char promotion);

        /**
         * rebuilds the compressed move
         * @param compressedMove 
         */
        Move(short compressedMove);
        
        /**
         * returns the the algebraic notation of the move, used by the uci protocoll 
         */
        std::string toString();

        /**
         * compresses the move to 16 bits
         * @return short 
         */
        short compress();

        //overloaded operators, for checking equality and inequality of two moves
        friend bool operator==(const Move& m1, const Move& m2);
        friend bool operator!=(const Move& m1, const Move& m2);

    private:
        std::string intToField(char field);

        char fieldStringToInt(std::string fieldCoords);
};

#endif