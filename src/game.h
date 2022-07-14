#ifndef GAME_H
#define GAME_H

#include <string>
#include <vector>
#include <list>
#include <cctype>

class Game { 
    public:

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

        inline static const std::string START_POSITION_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        static const char NO_PIECE = 0;
        static const char PAWN = 1;
        static const char KNIGHT = 2;
        static const char BISHOP = 3;
        static const char ROOK = 4;
        static const char QUEEN = 5;
        static const char KING = 6;
        static const short PIECE_VALUES[6];
        
        
        /**
         * initializes the board with the position given in fen notation
         */
        Game(std::string fen = Game::START_POSITION_FEN);

        /**
         * @returns true, if the given move is legal in the current position
         */ 
        bool moveLegal(Move move);
        

        /**
         * @param moveBuffer is used to return legal moves in the current position.
         * @returns the number of legal moves in the current position.
         */
        int getLegalMoves(Game::Move *moveBuffer);

        /**
         * @param kingInCheck will be set to true if the king of the side to move is in check
         * @param moveBuffer is used to return the legal moves in the current position.
         * @returns the number of legal moves int the current position.
         */
        int getLegalMoves(bool& kingInCheck, Game::Move *moveBuffer);

        /**
         * @param kingInCheck will be set to true if the king of the side to move is in check
         * @returns the number of legal moves int the current position.
         */
        int getNumOfMoves(bool &kingInCheck);

        /**
         * performs the specified move
         * @param move a legal move (as can be checked with Game::moveLegal()) that is to be executed on the current position
         */
        void playMove(Move move);

        /**
         * @returns true if the move given in Game::Move is a capture move
         */
        bool isCapture(Game::Move);


        //returns true, if the player king of the player now to play is in check
        bool ownKingInCheck();
        
        /**
         * undoes the last move made with playMove() or nullmove()
         */
        void undo();

        /**
         * this is the static evaluation function
         * @returns the static evaluation of the current position
         */
        int getLeafEvaluation();

        /**
         * the static evaluation function. kingInCheck and numOfMoves are arguments, that can be passed to the function to
         * avoid recalculation
         * @returns the static evaluation of the current position
         */
        int getLeafEvaluation(bool kingInCheck, int numOfMoves);

        /**
         * returns true, if the king would be in check if it was standing on the square specified by the parameter square and not on its current square
         */
        bool wouldKingBeInCheck(char square);


        uint64_t getTargetSquares(char kingSquare);

        /**
         * purely for debugging
         */
        void printInternalRepresentation();

        /**
         * checks if the current position is drawn by
         * the fifty move rule, 
         * a repetition inside the current search line
         * a threefold repetition considering the whole game
         * or by insufficient material
         */
        bool isPositionDraw(int distanceToRoot);


        /**
         * @returns how many halfmoves have been played since the last irreversible move (relevant for the 50-move rule)
         */
        int getHalfMoveClock();

        /**
         * @returns true if white has to play now
         */
        bool whiteToMove();

        /**
         * returns a 64 bit hash of the current position
         * @return uint64_t 
         */
        uint64_t getPositionHash();

    
        char getPieceOnSquare(char square);



    private:
        struct Position {
            uint64_t ownPieces;
            uint64_t pawns;
            uint64_t filesAndRanks;
            uint64_t diagonals;
            uint64_t knights;
            uint64_t kings;
            
            char castlingRights; //bit 0: white long, 1: white short, 2: black long, 3: block short
            char enpassantFile; //-1 for none, if there is a pawn were the en-passant rule is applicable 0-7 are used
            char halfMoveClock; //number of half moves since capture or pawn advance (relevant for 50 move rule)
        };

        std::list<Position> history;

        int fullMoveClock; //current full move number. Starts at one and is incremented after blacks turn. Not all of the game may reside in Game::history

        bool whitesTurn;

        //helper function
        template<char direction, bool returnMoves>
        inline void checkForPins(char& kingSquare, uint64_t& occupiedSquares, uint64_t& targetSquares, uint64_t& straightPiecesToMove, uint64_t& diagonalPiecesToMove, uint64_t& pawnsToMove, uint64_t& knightsToMove, Game::Move *moves,  int& numOfMoves);

        uint64_t getCheckBlockingSquares();
        template<char direction>
        void lookForCheck(uint64_t& checkBlockingSquares, uint64_t occupiedSquares, char kingSquare);

        template<bool returnMoves>
        void generatePawnMoves(uint64_t to, char fromSquareOffset, int& numOfMoves, Game::Move *moveBuffer);

        //bit board helper
        static const char NORTH = 0;
        static const char NORTH_EAST = 1;
        static const char EAST = 2;
        static const char SOUTH_EAST = 3;
        static const char SOUTH = 4;
        static const char SOUTH_WEST = 5;
        static const char WEST = 6;
        static const char NORTH_WEST = 7;

        template<char direction>
        uint64_t getRayInDirection(char square);

        template<char direction>
        uint64_t shift(uint64_t square);
        
        uint64_t getKnightMoveSquares(char square);
        uint64_t getKingMoveSquares(char square);
        uint64_t getPawnAttacks(char square, bool whitePawn);

        template<char direction>
        uint64_t getFirstBlockerInDirection(char square, uint64_t occ);

        template<char direction>
        uint64_t getSquaresUntilBlocker(char square, uint64_t blockingSquare);

        template<bool returnMoves>
        int getLegalMoves(bool& kingInCheck, Game::Move *moveBuffer);
        
        //debugging:
        void printBitBoard(uint64_t board);


};

bool operator==(const Game::Move& m1, const Game::Move& m2);
bool operator!=(const Game::Move& m1, const Game::Move& m2);

#endif