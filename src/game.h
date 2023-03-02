#ifndef GAME_H
#define GAME_H

#include <string>
#include <list>
#include <cctype>

#include "constants.h"
#include "move.h"

class Game {
    public:
        
        Game(std::string fen = START_POSITION_FEN);

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
         * a repetition inside the current search line
         * a threefold repetition considering the whole game
         * or by insufficient material
         */
        bool isPositionDraw(int distanceToRoot);


        class Position {
            public:
                uint64_t ownPieces;
                uint64_t pawns;
                uint64_t filesAndRanks;
                uint64_t diagonals;
                uint64_t knights;
                uint64_t kings;
                
                short fullMoveClock; //current move number. Starts at one and is incremented after blacks turn
                char castlingRights; //bit 0: white long, 1: white short, 2: black long, 3: block short
                char enpassantFile; //-1 for none, if there is a pawn were the en-passant rule is applicable 0-7 are used
                char halfMoveClock; //number of half moves since capture or pawn advance (relevant for 50 move rule)
                bool whitesTurn;
                


                Position(std::string fen = START_POSITION_FEN);
                
                Position(Position *pos, Move m);


                char getPieceOnSquare(char square);

                uint64_t getPositionHash();

                bool wouldKingBeInCheck(char square);

                bool ownKingInCheck();

                bool isCapture(Move);
                bool moveLegal(Move move);


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


                uint64_t getPseudoLegalBishopMoves(char square, uint64_t occupiedSquares);
                uint64_t getPseudoLegalBishopMoves(char square, uint64_t occupiedSquares, uint64_t ownPieces);

                uint64_t getPseudoLegalRookMoves(char square, uint64_t occupiedSquares);
                uint64_t getPseudoLegalRookMoves(char square, uint64_t occupiedSquares, uint64_t ownPieces);

                uint64_t getBishopAttacks(char square, uint64_t occupiedSquares);
                uint64_t getRookAttacks(char square, uint64_t occupiedSquares);


                template<char direction, bool returnMoves>
                inline void checkForPins(char& kingSquare, uint64_t& occupiedSquares, uint64_t& targetSquares, uint64_t& straightPiecesToMove, uint64_t& diagonalPiecesToMove, uint64_t& pawnsToMove, uint64_t& knightsToMove, Move *moves,  int& numOfMoves);

                uint64_t getCheckBlockingSquares();
                
                template<char direction>
                void lookForCheck(uint64_t& checkBlockingSquares, uint64_t occupiedSquares, char kingSquare);

                template<bool returnMoves>
                int getLegalMoves(bool& kingInCheck, Move *moveBuffer);


                /**
                 * purely for debugging
                 */
                void printInternalRepresentation();
        };

        Position *pos; //pointer to the current position

    private:
        std::list<Position> history;

};



#endif