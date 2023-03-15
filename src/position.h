#ifndef GAME_H
#define GAME_H

#include <string>
#include <list>
#include <cctype>

#include "constants.h"

class Move;
#include "move.h"

class Position {
    public:

        char squares[64];

        uint64_t ownPieces;
        uint64_t occupied;
        uint64_t pieces[6];
        
        short fullMoveClock; //current move number. Starts at one and is incremented after blacks turn
        bool whitesTurn;
        
        
        Position(std::string fen = START_POSITION_FEN);

        /**
         * performs the specified move
         * @param move a legal move (as can be checked with MoveLegal()) that is to be executed on the current position
         */
        void makeMove(Move move);

        /**
         * undoes the last move made with playMove() or nullmove()
         */
        void undo();

        /**
         * checks if the current position is drawn by
         * the fifty move rule, 
         * a repetition inside the current search line,
         * a threefold repetition considering the whole game
         * or by insufficient material
         */
        bool isPositionDraw(int distanceToRoot);


        char getPieceOnSquare(char square);

        uint64_t getPositionHash();


        bool ownKingInCheck();

        bool isCapture(Move);
        bool moveLegal(Move move);

        template<char pieceType>
        uint64_t getAttacks(char square);


        /**
         * @param moveBuffer is used to return legal moves in the current position.
         * @returns the number of legal moves in the current position.
         */
        int getLegalMoves(Move *moveBuffer);

        /**
         * @param kingInCheck will be set to true if the king of the side to move is in check
         * @param moveBuffer is used to return the legal moves in the current position.
         * @returns the number of legal moves int the current position.
         */
        int getLegalMoves(bool& kingInCheck, Move *moveBuffer);


        /**
         * purely for debugging
         */
        void printInternalRepresentation();


    private:
        struct HistoryObject {
            uint64_t oldOwnPieces;
            uint64_t oldOccupied;
            uint64_t hash;
            Move lastMove;
            char capturedPiece;

            char castlingRights; //bits addressed by castling constants (WHITE_CASTLE_KINGSIDE, etc.)
            char enpassantFile; //-1 for none, if there is a pawn were the en-passant rule is applicable 0-7 are used
            char halfMoveClock; //number of half moves since capture or pawn advance
        };

        std::list<HistoryObject> history;


        bool wouldKingBeInCheck(char square);

        uint64_t calculateHash();

        uint64_t getCheckBlockingSquares();

        template<char direction>
        void lookForCheck(uint64_t& checkBlockingSquares, uint64_t occupiedSquares, char kingSquare);

        template<char direction>
        inline void checkForPins(uint64_t& pinnedPieces, uint64_t targetSquares, Move *moveBuffer, int& numOfMoves);

};



#endif