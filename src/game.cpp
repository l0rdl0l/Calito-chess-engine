#include <vector>
#include <array>
#include <list>
#include <cctype>

#include <iostream>

#include "game.h"


const short Game::PIECE_VALUES[] = {0,100,300,300,500,900}; 


//returns a bitboard with the only set bit at the given square
uint64_t getBitboard(char square) {
    return (((uint64_t ) 1) << square);
}

//boiler plate code for the compile time generated look up tables
template<std::size_t Length, typename Generator>
constexpr auto lut(Generator&& f){
    using content_type = decltype(f(std::size_t{0}));
    std::array<content_type, Length> arr {};

    for(std::size_t i = 0; i < Length; i++){
        arr[i] = f(i);
    }

    return arr;
}

uint64_t constexpr getPseudoRandomNumber(int n){
    uint64_t state = 0;
    for(int i = 0; i < n+2; i++) {
        state = (2862933555777941757 * state + 3037000493);
    }
    return state;
}

inline constexpr auto zobristPieces = lut<2*6*64>([] (std::size_t n) {
    return getPseudoRandomNumber(n);
});

inline constexpr auto zobristCastlingRights = lut<16>([] (std::size_t n) {
   return getPseudoRandomNumber(2*6*64+n);
});

inline constexpr auto zobristEnPassat = lut<8>([] (std::size_t n) {
    return getPseudoRandomNumber(2*6*64+16+n);
});

inline constexpr auto zobristPlayerToMove = lut<2>([] (std::size_t n) {
    return getPseudoRandomNumber(2*6*64+16+8+n);
});

//generates a look up table containing bitboards for knight moves for each square
inline constexpr auto knightAttacks = lut<64>([](std::size_t n){
    uint64_t result = 0;
    uint64_t square = ((uint64_t) 1) << n;
    if(n % 8 > 0)
        result |= square << 15;
    if(n % 8 > 1)
        result |= square << 6;
    if(n % 8 > 1)
        result |= square >> 10;
    if(n % 8 > 0)
        result |= square >> 17;
    if(n % 8 < 7)
        result |= square >> 15;
    if(n % 8 < 6)
        result |= square >> 6;
    if(n % 8 < 6)
        result |= square << 10;
    if(n % 8 < 7)
        result |= square << 17;
    return result;
});

//generates a look up table containing bitboards for possible king moves from each square
inline constexpr auto kingMovesLUT = lut<64>([](std::size_t n) {
    uint64_t square = ((uint64_t) 1) << n;
    uint64_t result = 0;

    result |= square << 8;
    result |= square >> 8;
    if(n % 8 != 0) {
        result |= square >> 1;
        result |= square << 7;
        result |= square >> 9;
    }
    if(n % 8 != 7) {
        result |= square << 1;
        result |= square >> 7;
        result |= square << 9;
    }
    return result;
});

//generates a look up table for each square, containing a bitboard in which every square north-east of the given square is set
inline constexpr auto squaresNorthEastLUT = lut<64>([](std::size_t n) {
    uint64_t result = 0;
    uint64_t square = ((uint64_t) 1) << n;
    for(int i = n % 8; i < 7; i++) {
        square >>= 7;
        result |= square;
    }
    return result;
});

inline constexpr auto squaresSouthEastLUT = lut<64>([](std::size_t n) {
    uint64_t result = 0;
    uint64_t square = ((uint64_t) 1) << n;
     for(int i = n % 8; i < 7; i++) {
        square <<= 9;
        result |= square;
    }
    return result;
});

inline constexpr auto squaresSouthWestLUT = lut<64>([](std::size_t n) {
    uint64_t result = 0;
    uint64_t square = ((uint64_t) 1) << n;
     for(int i = 7 - n % 8; i < 7; i++) {
        square <<= 7;
        result |= square;
    }
    return result;
});

inline constexpr auto squaresNorthWestLUT = lut<64>([](std::size_t n) {
    uint64_t result = 0;
    uint64_t square = ((uint64_t) 1) << n;
     for(int i = 7 - n % 8; i < 7; i++) {
        square >>= 9;
        result |= square;
    }
    return result;
});

inline constexpr auto pawnAttacksLUT = lut<64>([](std::size_t n) { //first board in entry: fields attacked by a white pawn, second board: fields attacked by a black pawn.
    std::array<uint64_t, 2> result = {0,0};
    uint64_t square = ((uint64_t) 1) << n;

    if(n%8 != 0) {
        result[0] |= square >> 9;
        result[1] |= square << 7;
    }

    if(n%8 != 7) {
        result[0] |= square >> 7;
        result[1] |= square << 9;
    }
    return result;
});

template<char direction>
uint64_t Game::getRayInDirection(char square) {
    if (direction == NORTH) {
        return ((uint64_t) 0x0080808080808080) >> (63-square);
    } else if (direction == NORTH_EAST) {
        return squaresNorthEastLUT[square];
    } else if (direction == EAST) {
        return (((uint64_t) 0xfe) << square) & (((uint64_t) 0xff) << (square & 0xf8));
    } else if (direction == SOUTH_EAST) {
        return squaresSouthEastLUT[square];
    } else if (direction == SOUTH) {
        return ((uint64_t) 0x0101010101010100) << square;
    } else if (direction == SOUTH_WEST) {
        return squaresSouthWestLUT[square];
    } else if (direction == WEST) {
        return (((uint64_t) 0x7fffffffffffffff) >> (63-square)) & (((uint64_t) 0xff) << (square & 0xf8));
    } else if (direction == NORTH_WEST) {
        return squaresNorthWestLUT[square];
    }
}

//returns a bitboard with all squares reachable by a knights move from the given square
inline uint64_t Game::getKnightMoveSquares(char square) {
    return knightAttacks[square];
}

//returns a bitboard with all squares reachable by a king move.
inline uint64_t Game::getKingMoveSquares(char square) {
    return kingMovesLUT[square];
}

//returns a bit board in which the squares attacked by a pawn at the given square are set. The parameter blackPawn specifies wether the pawn is black or white
inline uint64_t Game::getPawnAttacks(char square, bool blackPawn) {
    return pawnAttacksLUT[square][blackPawn];
}

//shifts the whole bitboard in the given direction. Squares on the new edges are filled with zero
/*
    0 0 1 1 1 0 0 1
    0 0 0 0 0 0 0 1
    1 0 0 0 0 0 0 0
    1 0 1 0 1 0 1 0
    0 1 0 1 0 1 0 1
    0 0 0 0 0 0 0 0
    1 1 1 1 1 0 0 1
    0 1 0 1 0 0 0 1

    shifted to the north west gives:

    0 0 0 0 0 0 1 0
    0 0 0 0 0 0 0 0
    0 1 0 1 0 1 0 0
    1 0 1 0 1 0 1 0
    0 0 0 0 0 0 0 0
    1 1 1 1 0 0 1 0
    1 0 1 0 0 0 1 0
    0 0 0 0 0 0 0 0

*/
template<char direction>
inline uint64_t Game::shift(uint64_t square) {
    if(direction == NORTH) {
        return square >> 8;
    } else if(direction == NORTH_EAST) {
        return (square >> 7) & ~((uint64_t) 0x0101010101010101);
    } else if(direction == EAST) {
        return (square << 1) & ~((uint64_t) 0x0101010101010101);
    } else if(direction == SOUTH_EAST) {
        return (square << 9) & ~((uint64_t) 0x0101010101010101);
    } else if(direction == SOUTH) {
        return square << 8;
    } else if(direction == SOUTH_WEST) {
        return (square << 7) & ~((uint64_t) 0x8080808080808080);
    } else if(direction == WEST) {
        return (square >> 1) & ~((uint64_t) 0x8080808080808080);
    } else if(direction == NORTH_WEST) {
        return (square >> 9) & ~((uint64_t) 0x8080808080808080);
    }
}


//initializes the string with the Board given in FEN notation in the parameter
Game::Game(std::string fen) {

    int index = 0;
    int rank = 0;
    int file = 0;

    Game::Position initPos;
    initPos.diagonals = 0;
    initPos.filesAndRanks = 0;
    initPos.kings = 0;
    initPos.knights = 0;
    initPos.pawns = 0;
    initPos.ownPieces = 0;
    initPos.castlingRights = 0;
    initPos.enpassantFile = -1;

    this->whitesTurn = true;

    //reading pieces
    char c;
    while((c = fen.at(index)) != ' ') {
        
        if(c == '/') {
            rank++;
            file = 0;
        } else if(c >= 0x30 && c <= 0x38) {
            file += c - 0x30;
        } else {
            if(toupper(c) == 'P') {
                initPos.pawns |= getBitboard(8*rank+file);
            } else if(toupper(c) == 'N') {
                initPos.knights |= getBitboard(8*rank+file);
            } else if(toupper(c) == 'K') {
                initPos.kings |= getBitboard(8*rank+file);
            } else if(toupper(c) == 'B') {
                initPos.diagonals |= getBitboard(8*rank+file);
            } else if(toupper(c) == 'R') {
                initPos.filesAndRanks |= getBitboard(8*rank+file);
            } else if(toupper(c) == 'Q') {
                initPos.filesAndRanks |= getBitboard(8*rank+file);
                initPos.diagonals |= getBitboard(8*rank+file);
            }
            bool whitePiece = (c == toupper(c));
            initPos.ownPieces |= ((uint64_t) whitePiece) << (8*rank+file);
            file++;
            
        }
        index++;
    }

    index ++;
    if(fen.at(index) == 'b') {
        initPos.ownPieces = (initPos.pawns | initPos.diagonals | initPos.filesAndRanks | initPos.knights | initPos.kings) & ~initPos.ownPieces;
        this->whitesTurn = false;
    }

    index += 2;
    while((c = fen.at(index)) != ' ') {
        if(c == 'k')
            initPos.castlingRights |= 1 << 3;
        if(c == 'q')
            initPos.castlingRights |= 1 << 2;
        if(c == 'K')
            initPos.castlingRights |= 1 << 1;
        if(c == 'Q') {
            initPos.castlingRights |= 1 << 0;
        }
        index++;
    }

    index ++;
    if((c = fen.at(index)) != '-') {
        initPos.enpassantFile = c - 97;
        index++;
    }
    index += 2;

    initPos.halfMoveClock = 0;
    while((c = fen.at(index)) != ' ') {
        initPos.halfMoveClock *= 10;
        initPos.halfMoveClock += c - 0x30;
        index++;
    }
    index ++;

    this->fullMoveClock = 0;
    while(index < fen.length()) {
        c = fen.at(index);
        this->fullMoveClock *= 10;
        this->fullMoveClock += c - 0x30;
        index++;
    }
    this->history.push_front(initPos);
}

bool Game::isPositionDraw(int distanceToRoot) {

    //draw by fifty move rule
    if(this->getHalfMoveClock() >= 100) 
        return true;


    int repetitions = 0;
    
    std::list<Game::Position>::iterator it = history.begin();
    for(int i = 2; i < history.size() && i <= history.front().halfMoveClock; i += 2) {
        it++;
        it++;

        if(         it->castlingRights == history.front().castlingRights
                &&  it->enpassantFile == history.front().enpassantFile
                &&  it->diagonals == history.front().diagonals
                &&  it->filesAndRanks == history.front().filesAndRanks
                &&  it->kings == history.front().kings
                &&  it->knights == history.front().knights
                &&  it->ownPieces == history.front().ownPieces
                &&  it->pawns == history.front().pawns) {
                
            repetitions++;
        }

        //the position occured inside the currently searched line and is therefore evaluated as draw
        if(repetitions >= 1 && i <= distanceToRoot) {
            return true;
        }
        //draw by threefold repetition
        if(repetitions >= 2) {
            return true;
        }
    }

    //draw by insufficient material
    int straightMovingPieces = __builtin_popcountll(history.front().filesAndRanks);
    int bishops = __builtin_popcountll(history.front().diagonals & ~history.front().filesAndRanks);
    int knights = __builtin_popcountll(history.front().knights);
    int pawns = __builtin_popcountll(history.front().pawns);

    if(straightMovingPieces == 0 && pawns == 0) {
        //king vs king + bishop and king vs king + knight are drawn positions
        if(knights + bishops <= 1)
            return true;


        uint64_t lightSquares = 0xaa55aa55aa55aa55;
        uint64_t darkSquares = 0x55aa55aa55aa55aa;

        if(bishops == 2) {
            //if the bishops are on squares of the same color, and are belong to different players.
            if(     (((bool) (lightSquares & history.front().diagonals)) != ((bool) (darkSquares & history.front().diagonals)))
                &&  (__builtin_popcountll(history.front().diagonals & history.front().ownPieces) == 1)) {

                    return true;
                }
        }
    }

    return false;
}

int Game::getHalfMoveClock() {
    return history.front().halfMoveClock;
}

bool Game::whiteToMove() {
    return this->whitesTurn;
}

bool Game::moveLegal(Game::Move move) {
    Game::Move moveBuffer[343];
    int numOfMoves = this->getLegalMoves(moveBuffer);
    for(int i = 0; i < numOfMoves; i++) {
        if(moveBuffer[i] == move)
            return true;
    }
    return false;
}

//executes the given move
void Game::playMove(Move move) {
    //printInternalRepresentation();

    Position newPos;

    int moveMode = move.flags & 0xf0;

    //copy castling rights and remove them later, if necessary
    newPos.castlingRights = history.front().castlingRights;

    uint64_t moveToMask = getBitboard(move.to);
    uint64_t moveFromMask = getBitboard(move.from);

    uint64_t newOwnPieces = (history.front().ownPieces & ~moveFromMask) | moveToMask;

    if(history.front().pawns & moveFromMask) {
        
        //remove pawn from former square
        newPos.pawns = history.front().pawns & ~moveFromMask;

        //kings won't be affected
        newPos.kings = history.front().kings;


        if(history.front().enpassantFile != -1 && (
                (this->whitesTurn && move.to == history.front().enpassantFile + 16) ||
                (!this->whitesTurn && move.to == history.front().enpassantFile + 40))) {
            /*add pawn to the new field*/
            newPos.pawns |= moveToMask;
            /*remove en passant captured pawn*/
            if(this->whitesTurn) {
                newPos.pawns &= ~(moveToMask << 8);
            } else {
                newPos.pawns &= ~(moveToMask >> 8);
            }

            //no other piece can be captured
            newPos.diagonals = history.front().diagonals;
            newPos.filesAndRanks = history.front().filesAndRanks;
            newPos.knights = history.front().knights;

            newPos.enpassantFile = -1;  //a en passant capture can't be the first move of a pawn
        } else if(move.flags) {

            switch(move.flags) {
                case 2: //promote to knight
                    
                    //add knight to the destination square
                    newPos.knights = history.front().knights | moveToMask;
                    //remove potentially captured pieces
                    newPos.diagonals = history.front().diagonals & ~moveToMask;
                    newPos.filesAndRanks = history.front().filesAndRanks & ~moveToMask;
                    //enemy pawns can no be on the enemy base line, so they can't be captured here
                    break;

                case 3: //promote to bishop
                    //add bishop
                    newPos.diagonals = history.front().diagonals | moveToMask;
                    //remove potentially captured pieces
                    newPos.knights = history.front().knights & ~moveToMask;
                    newPos.filesAndRanks = history.front().filesAndRanks & ~moveToMask;
                    //enemy pawns can no be on the enemy base line, so they can't be captured here

                    break;
                case 4: //promote to rook
                    //add rook
                    newPos.filesAndRanks = history.front().filesAndRanks | moveToMask;
                    //remove potentially captured pieces
                    newPos.diagonals = history.front().diagonals & ~moveToMask;
                    newPos.knights = history.front().knights & ~moveToMask;
                
                    break;
                case 5: //promote to queen
                    //add queen
                    newPos.filesAndRanks = history.front().filesAndRanks | moveToMask;
                    newPos.diagonals = history.front().diagonals | moveToMask;
                    //remove pontentially captured pieces
                    newPos.knights = history.front().knights & ~moveToMask;

                    break;
            }
            newPos.enpassantFile = -1; //promoting moves can't be two square moves.
        } else { //normal pawn move
            /*add pawn to the new field*/
            newPos.pawns |= moveToMask;
            /*remove any potentially captured pieces*/
            newPos.knights = history.front().knights & ~moveToMask;
            newPos.diagonals = history.front().diagonals & ~moveToMask;
            newPos.filesAndRanks = history.front().filesAndRanks & ~moveToMask;

            //add en passant flag if the pawn moves two squares and there is an enemy pawn next to it
            if(((move.from - move.to == 16) || (move.from - move.to == -16)) 
                        && (((shift<WEST>(getBitboard(move.to)) | shift<EAST>(getBitboard(move.to))) & history.front().pawns & ~history.front().ownPieces))) {
                
                newPos.enpassantFile = move.to & 7;
            } else {
                newPos.enpassantFile = -1;
            }
        }
        newPos.halfMoveClock = 0;
    } else {
        //non pawn move
        newPos.enpassantFile = -1;

        if((history.front().diagonals | history.front().filesAndRanks | history.front().knights | history.front().pawns) & moveToMask) {
            //capture move
            newPos.halfMoveClock = 0;
        } else {
            //reversible move
            newPos.halfMoveClock = history.front().halfMoveClock + 1;
        }


        if(history.front().knights & moveFromMask) { //knight move
            //move knight
            newPos.knights = (history.front().knights & ~moveFromMask) | moveToMask;
            
            //remove potentially captured pieces
            newPos.diagonals = history.front().diagonals & ~moveToMask;
            newPos.filesAndRanks = history.front().filesAndRanks & ~moveToMask;
            newPos.pawns = history.front().pawns & ~moveToMask;
            //kings won't be affected
            newPos.kings = history.front().kings;
        
        } else if(history.front().kings & moveFromMask) { //king move
            //move king
            newPos.kings = (history.front().kings & ~moveFromMask) | moveToMask;
            //remove potentially captured pieces
            newPos.knights = history.front().knights & ~moveToMask;
            newPos.diagonals = history.front().diagonals & ~moveToMask;
            newPos.filesAndRanks = history.front().filesAndRanks & ~moveToMask;
            newPos.pawns = history.front().pawns & ~moveToMask;


            if(this->whitesTurn) {
                //remove castling rights
                newPos.castlingRights &= 0x0c;

                //castle if castling move is detected
                if(move.from - move.to == 2) { //queen side
                    newPos.filesAndRanks &= ~getBitboard(56);
                    newPos.filesAndRanks |= getBitboard(59);
                    newOwnPieces |= getBitboard(59);
                } else if(move.from - move.to == -2) { //king side
                    newPos.filesAndRanks &= ~getBitboard(63);
                    newPos.filesAndRanks |= getBitboard(61);
                    newOwnPieces |= getBitboard(61);
                }
            } else {
                //remove castling rights
                newPos.castlingRights &= 0x03;

                //perfrom castling if castling move is detected
                if(move.from - move.to == 2) { //queen side
                    newPos.filesAndRanks &= ~getBitboard(0);
                    newPos.filesAndRanks |= getBitboard(3);
                    newOwnPieces |= getBitboard(3);
                } else if(move.from - move.to == -2) { //king side
                    newPos.filesAndRanks &= ~getBitboard(7);
                    newPos.filesAndRanks |= getBitboard(5);
                    newOwnPieces |= getBitboard(5);
                }
            }

        } else if(history.front().diagonals & ~history.front().filesAndRanks & moveFromMask) { //bishop move
            //move bishop
            newPos.diagonals = (history.front().diagonals & ~moveFromMask) | moveToMask;
            //remove potentially captured pieces
            newPos.knights = history.front().knights & ~moveToMask;
            newPos.filesAndRanks = history.front().filesAndRanks & ~moveToMask;
            newPos.pawns = history.front().pawns & ~moveToMask;
            //kings won't be affected
            newPos.kings = history.front().kings;

        } else if(history.front().diagonals & history.front().filesAndRanks & moveFromMask) { //queen move
            //move queen
            newPos.diagonals = (history.front().diagonals & ~moveFromMask) | moveToMask;
            newPos.filesAndRanks = (history.front().filesAndRanks & ~moveFromMask) | moveToMask;

            //remove potentially captured pieces
            newPos.knights = history.front().knights & ~moveToMask;
            newPos.pawns = history.front().pawns & ~moveToMask;
            //kings won't be affected
            newPos.kings = history.front().kings;

        } else { //rook move
            //move rook
            newPos.filesAndRanks = (history.front().filesAndRanks & ~moveFromMask) | moveToMask;
            //remove potentially captured pieces
            newPos.knights = history.front().knights & ~moveToMask;
            newPos.diagonals = history.front().diagonals & ~moveToMask;
            newPos.pawns = history.front().pawns & ~moveToMask;
            //kings won't be affected
            newPos.kings = history.front().kings;
        }
    }

    //remove castling rights if rook moved or captured
    if(move.from == 0 || move.to == 0) {
        newPos.castlingRights &= ~(1 << 2);
    } 
    if(move.from == 7 || move.to == 7) {
        newPos.castlingRights &= ~(1 << 3);
    }
    if(move.from == 56 || move.to == 56) {
        newPos.castlingRights &= ~(1 << 0);
    }
    if(move.from == 63 || move.to == 63) {
        newPos.castlingRights &= ~(1 << 1);
    } 


    
    newPos.ownPieces = (newPos.diagonals | newPos.filesAndRanks | newPos.knights | newPos.pawns | newPos.kings) & ~newOwnPieces;
    this->history.push_front(newPos);
    if(!this->whitesTurn)
        this->fullMoveClock ++;

    this->whitesTurn = !this->whitesTurn;
}

//undoes the last made move
void Game::undo() {
    history.pop_front();
    this->fullMoveClock -= this->whitesTurn;

    this->whitesTurn = !this->whitesTurn;
}

//returns wether the given move is a capture move
bool Game::isCapture(Move move) {

    //check if there is a piece on the target square of the move. In that case the move is definitely a capture
    if(getBitboard(move.to) & (history.front().filesAndRanks | history.front().diagonals | history.front().knights | history.front().pawns))
        return true;;
    
    //check for en passant capture
    if(history.front().enpassantFile != -1) {
        if(this->whitesTurn) {
            return (move.to - 16 == history.front().enpassantFile) && (getBitboard(move.from) & history.front().pawns);
        } else {
            return (move.to - 40 == history.front().enpassantFile) && (getBitboard(move.from) & history.front().pawns);
        }
    }
    return false;
}

template <char direction>
uint64_t Game::getSquaresUntilBlocker(char square, uint64_t blockingSquare) {
    if(direction == NORTH || direction == NORTH_EAST || direction == WEST || direction == NORTH_WEST) {
        return ~(blockingSquare - 1) & getRayInDirection<direction>(square);
    } else if(direction == EAST) {
        return ((blockingSquare << 1) - 1) & getRayInDirection<EAST>(square);
    } else if (direction == SOUTH_EAST) {
        return ((blockingSquare << 9) - 1) & getRayInDirection<SOUTH_EAST>(square);
    } else if (direction == SOUTH) {
        return ((blockingSquare << 8) - 1) & getRayInDirection<SOUTH>(square);
    } else if (direction == SOUTH_WEST) {
         return ((blockingSquare << 7) - 1) & getRayInDirection<SOUTH_WEST>(square);
    }
}


template<char direction>
uint64_t Game::getFirstBlockerInDirection(char square, uint64_t occ) {
    uint64_t blocker = getRayInDirection<direction>(square) & occ;
    if(direction == NORTH || direction == NORTH_EAST || direction == WEST || direction == NORTH_WEST) { //negative directions
        if(blocker)
            return ((uint64_t) 0x8000000000000000) >> __builtin_clzll(blocker);
        else
            return 0;
    } else { //positive directions
        return blocker & (~blocker + 1);
    }
}


bool Game::wouldKingBeInCheck(char kingSquare) {
    
    bool attackedByKnight = getKnightMoveSquares(kingSquare) & (history.front().knights & ~history.front().ownPieces);
    bool attackedByPawn = getPawnAttacks(kingSquare, !this->whitesTurn) & (history.front().pawns & ~history.front().ownPieces);
    //kings can't be on adjacent squares, we therefore check for an "attack" by the enemy king
    bool attackedByKing = getKingMoveSquares(kingSquare) & (history.front().kings & ~history.front().ownPieces);

    if(attackedByKnight | attackedByPawn | attackedByKing)
        return true;
    
    //our own king cannot block a attackRay in this context, since he then still would be in check
    uint64_t occupyingPieces = history.front().diagonals | history.front().filesAndRanks | history.front().knights | history.front().pawns | (history.front().kings & ~history.front().ownPieces);

    uint64_t attackSquares;

    attackSquares = getFirstBlockerInDirection<NORTH>(kingSquare, occupyingPieces)
                  | getFirstBlockerInDirection<SOUTH>(kingSquare, occupyingPieces)
                  | getFirstBlockerInDirection<WEST>(kingSquare, occupyingPieces)
                  | getFirstBlockerInDirection<EAST>(kingSquare, occupyingPieces);
 
    if(attackSquares & (history.front().filesAndRanks & ~history.front().ownPieces))
        return true;


    attackSquares = getFirstBlockerInDirection<SOUTH_WEST>(kingSquare, occupyingPieces)
                  | getFirstBlockerInDirection<SOUTH_EAST>(kingSquare, occupyingPieces)
                  | getFirstBlockerInDirection<NORTH_EAST>(kingSquare, occupyingPieces)
                  | getFirstBlockerInDirection<NORTH_WEST>(kingSquare, occupyingPieces);
    
    if(attackSquares & (history.front().diagonals & ~history.front().ownPieces))
        return true;

    return false;
}


int Game::getLegalMoves(Game::Move *moveBuffer) {
    bool tmp;
    return getLegalMoves<true>(tmp, moveBuffer);
}

int Game::getLegalMoves(bool& kingInCheck, Game::Move *moveBuffer) {
    return getLegalMoves<true>(kingInCheck, moveBuffer);
}

int Game::getNumOfMoves(bool& kingInCheck) {
    return getLegalMoves<false>(kingInCheck, 0);
}

//helper functions

template<bool returnMoves>
void inline generatePromotions(char from, char to, int& numOfMoves, Game::Move *moveBuffer) {
    if(returnMoves) {
        //store four moves in the move array, one for every promotion option
        #pragma gcc unroll 4
        for(int j = 0; j < 4; j ++) {
            moveBuffer[numOfMoves+j] = Game::Move(from, to, j+2);
        }
    }
    //increment the move counter by four
    numOfMoves += 4;
}

template<bool returnMoves>
void inline generatePawnMove(char from, char to, int& numOfMoves, Game::Move *moveBuffer) {
    if(to > 55 || to < 8) {
        generatePromotions<returnMoves>(from, to, numOfMoves, moveBuffer);
    } else {
        if(returnMoves) {
            moveBuffer[numOfMoves] = Game::Move(from, to);
        }
        numOfMoves ++;
    }
}

template<bool returnMoves>
void inline generatePromotionPawnMoves(uint64_t to, char fromSquareOffset, int& numOfMoves, Game::Move *moveBuffer) {
    if(returnMoves) {
        //while there are still target
        while(to) {
            //get the current target square
            char square = __builtin_ctzll(to);
            //remove it from the target squares list
            to &= ~getBitboard(square);

            generatePromotions<returnMoves>(square + fromSquareOffset, square, numOfMoves, moveBuffer);
        }
    } else {
        numOfMoves += 4*__builtin_popcountll(to);
    }
}

template<bool returnMoves>
void inline generateNonPromotionPawnMoves(uint64_t to, char fromSquareOffset, int& numOfMoves, Game::Move *moveBuffer) {
    if(returnMoves) {
        while(to) {
            //get the current target square
            char square = __builtin_ctzll(to);
            //remove it from the target squares list
            to &= ~getBitboard(square);

            moveBuffer[numOfMoves] = Game::Move(square + fromSquareOffset, square);

            numOfMoves++;
        }
    } else {
        numOfMoves += __builtin_popcountll(to);
    }
}

template<bool returnMoves>
void Game::generatePawnMoves(uint64_t to, char fromSquareOffset, int& numOfMoves, Game::Move *moveBuffer) {

    //split the target squares into promotion and no-promotion moves
    uint64_t baseRanks = ((uint64_t) 0xff) | (((uint64_t) 0xff) << 56);
    uint64_t promotingMoves = baseRanks & to;
    uint64_t nonPromotingMoves = ~baseRanks & to;


    generatePromotionPawnMoves<returnMoves>(promotingMoves, fromSquareOffset, numOfMoves, moveBuffer);
    generateNonPromotionPawnMoves<returnMoves>(nonPromotingMoves, fromSquareOffset, numOfMoves, moveBuffer);
}

template <bool returnMoves>
void inline generateMoves(char from, uint64_t to, int& numOfMoves, Game::Move *moveBuffer) {
    if(returnMoves) {
        //while there are still target squares, for which moves need to be generated
        while(to) {
            //get the current target square
            char square = __builtin_ctzll(to);
            //remove it from the target squares list
            to &= ~getBitboard(square);
            //store the move to the move array
            moveBuffer[numOfMoves] = Game::Move(from, square);
            //increment the moves counter
            numOfMoves++;
        }
    } else {
        numOfMoves += __builtin_popcountll(to);
    }
}
    

template<char direction>
void Game::lookForCheck(uint64_t& checkBlockingSquares, uint64_t occupiedSquares, char kingSquare) {
    //depending on the direction pieces that move straight or pieces that move diagonally can give check
    uint64_t checkGivingPieces;
    if(direction == NORTH || direction == SOUTH || direction == EAST || direction == WEST) 
        checkGivingPieces = ~history.front().ownPieces & history.front().filesAndRanks;
    else
        checkGivingPieces = ~history.front().ownPieces & history.front().diagonals;

    //if the king is in check from the given direction all moves must either block the check or capture the checking piece
    uint64_t attackingPiece = getFirstBlockerInDirection<direction>(kingSquare, occupiedSquares) & checkGivingPieces;
    if(attackingPiece)
        checkBlockingSquares &= getSquaresUntilBlocker<direction>(kingSquare, attackingPiece);
}

uint64_t Game::getCheckBlockingSquares() {
    uint64_t blockSquares = ~((uint64_t) 0);

    uint64_t occupiedSquares = history.front().kings | history.front().knights | history.front().filesAndRanks | history.front().diagonals | history.front().pawns;

    char kingSquare = __builtin_ctzll(history.front().kings & history.front().ownPieces);

    uint64_t attackingKnight = getKnightMoveSquares(kingSquare) & history.front().knights & ~history.front().ownPieces;
    if(attackingKnight)
        blockSquares = attackingKnight;

    uint64_t attackingPawns = getPawnAttacks(kingSquare, !this->whitesTurn) & (history.front().pawns & ~history.front().ownPieces);
    if(attackingPawns)
        blockSquares &= attackingPawns;

    lookForCheck<NORTH>(blockSquares, occupiedSquares, kingSquare);
    lookForCheck<SOUTH>(blockSquares, occupiedSquares, kingSquare);
    lookForCheck<EAST>(blockSquares, occupiedSquares, kingSquare);
    lookForCheck<WEST>(blockSquares, occupiedSquares, kingSquare);
    lookForCheck<NORTH_EAST>(blockSquares, occupiedSquares, kingSquare);
    lookForCheck<NORTH_WEST>(blockSquares, occupiedSquares, kingSquare);
    lookForCheck<SOUTH_EAST>(blockSquares, occupiedSquares, kingSquare);
    lookForCheck<SOUTH_WEST>(blockSquares, occupiedSquares, kingSquare);

    return blockSquares;
}


//if there is a pinned piece in the given direction, from the perspective of the king, all legal moves for this pinned piece will be generated
template<char direction, bool returnMoves>
inline void Game::checkForPins(char& kingSquare, uint64_t& occupiedSquares, uint64_t& targetSquares, uint64_t& straightPiecesToMove, uint64_t& diagonalPiecesToMove, uint64_t& pawnsToMove, uint64_t& knightsToMove, Game::Move *moveBuffer, int& numOfMoves) {
    
    uint64_t firstBlocker = getFirstBlockerInDirection<direction>(kingSquare, occupiedSquares);
    
    bool diagonalDirection = (direction == NORTH_EAST || direction == SOUTH_EAST || direction == SOUTH_WEST || direction == NORTH_WEST);

    if(firstBlocker & history.front().ownPieces) {

        uint64_t secondBlocker = getFirstBlockerInDirection<direction>(kingSquare, occupiedSquares & ~firstBlocker);

        if(secondBlocker & ~history.front().ownPieces & (diagonalDirection ? history.front().diagonals : history.front().filesAndRanks)) {
            //there is a piece pinned in the direction specified in the template parameter
            //we will now check if there are square the pinned piece can still move to.


            if(direction == NORTH || direction == SOUTH) {
                if(firstBlocker & history.front().filesAndRanks) {
                    //rook and queen can move in south north direction
                    //remove the piece from the lists of pieces to examine:
                    straightPiecesToMove &= ~firstBlocker;
                    diagonalPiecesToMove &= ~firstBlocker; //the piece could be a queen
                    //the pinned piece can move between the king and the pinning piece, but staying on the same square is not a valid move
                    uint64_t moveTargets = getSquaresUntilBlocker<direction>(kingSquare, secondBlocker) & ~firstBlocker & targetSquares;
                    generateMoves<returnMoves>(__builtin_ctzll(firstBlocker), moveTargets, numOfMoves, moveBuffer);
                } else if(firstBlocker & history.front().pawns) {
                    //pawns can also move north or south, depending on if they are black or white
                    //However, pawn moves in north or south direction can not be capture moves,
                    //but only one square moves, or, if they haven't moved yet, two-square moves.
                    pawnsToMove &= ~firstBlocker;
                    uint64_t moveTargets;
                    if(this->whitesTurn) {
                        moveTargets = (firstBlocker >> 8) & ~occupiedSquares;
                        moveTargets |= (((firstBlocker >> 8) & ~occupiedSquares) >> 8) & ~occupiedSquares & (((uint64_t) 0xff) << 32);
                    } else {
                        moveTargets = (firstBlocker << 8) & ~occupiedSquares;
                        moveTargets |= (((firstBlocker << 8) & ~occupiedSquares) << 8) & ~occupiedSquares & (((uint64_t) 0xff) << 24);
                    }
                    
                    generateMoves<returnMoves>(__builtin_ctzll(firstBlocker), moveTargets & targetSquares, numOfMoves, moveBuffer);
                } else {
                    //knights and bishops can't move at all if they are pinned in north or south direction,
                    //so we just remove them from the lists of pieces to be examined
                    knightsToMove &= ~firstBlocker;
                    diagonalPiecesToMove &= ~firstBlocker; 
                }
            } else if(direction == EAST || direction == WEST) {
                if(firstBlocker & history.front().filesAndRanks) {
                    //rooks and queens can move in east west direction
                    straightPiecesToMove &= ~firstBlocker;
                    diagonalPiecesToMove &= ~firstBlocker; //the piece could be a queen.
                    //the piece can move between the king and the pinning piece, but staying on the same square is not a valid move
                    uint64_t moveTargets = getSquaresUntilBlocker<direction>(kingSquare, secondBlocker) & ~firstBlocker & targetSquares;
                    generateMoves<returnMoves>(__builtin_ctzll(firstBlocker), moveTargets, numOfMoves, moveBuffer);
                } else {
                    //pawns, knights and bishops are not able to move in east west direction, and therefore can not move if pinned in this direction
                    diagonalPiecesToMove &= ~firstBlocker;
                    knightsToMove &= ~firstBlocker;
                    pawnsToMove &= ~firstBlocker;
                }
            } else { //diagonal direction
                if(firstBlocker & history.front().diagonals) {
                    //bishops and queens are still able to move in a diagonal direction if pinned from that direction
                    diagonalPiecesToMove &= ~firstBlocker;
                    straightPiecesToMove &= ~firstBlocker; //the piece could be a queen
                    uint64_t moveTargets = getSquaresUntilBlocker<direction>(kingSquare, secondBlocker) & ~firstBlocker & targetSquares;
                    generateMoves<returnMoves>(__builtin_ctzll(firstBlocker), moveTargets, numOfMoves, moveBuffer);
                } else if(firstBlocker & history.front().pawns) {
                    //the only move a pawn that is pinned along a diagonal can possibly make is capturing the pinning Piece.
                    
                    pawnsToMove &= ~firstBlocker;
                    if( ((direction == NORTH_EAST || direction == NORTH_WEST) && this->whitesTurn) || //a white pawn can only capture north west or norht east
                        ((direction == SOUTH_EAST || direction == SOUTH_WEST) && !this->whitesTurn)) { // a black pawn can only capture south west or south east

                        uint64_t pawnTarget = shift<direction>(firstBlocker) & secondBlocker & targetSquares;
                        if(pawnTarget) {
                            generatePawnMove<returnMoves>(__builtin_ctzll(firstBlocker), __builtin_ctzll(secondBlocker), numOfMoves, moveBuffer);
                        }
                    }    
                } else {
                    //knights and rooks can't move in the diagonal direction when pinned, so we just remove them from the list of pieces that still have to be examined
                    straightPiecesToMove &= ~firstBlocker;
                    knightsToMove &= ~firstBlocker;
                }
            }
        }
    }
}



template <bool returnMoves>
int Game::getLegalMoves(bool& kingInCheck, Game::Move *moveBuffer) {
    int numOfMoves = 0;

    char kingSquare = __builtin_ctzll(history.front().kings & history.front().ownPieces);

    //target squares are squares that block checks if the king is in check and squares that are not occupied by our own pieces
    uint64_t targetSquares = getCheckBlockingSquares();
    kingInCheck = (targetSquares != ~((uint64_t) 0));
    targetSquares &= ~history.front().ownPieces;

    uint64_t occupiedSquares = history.front().diagonals | history.front().filesAndRanks | history.front().kings | history.front().pawns | history.front().knights;

    //variables to keep track of pieces for which we haven't generated moves yet. 
    uint64_t pawnsToMove = history.front().pawns & history.front().ownPieces;
    uint64_t knightsToMove = history.front().knights & history.front().ownPieces;
    uint64_t diagonalPiecesToMove = history.front().diagonals & history.front().ownPieces;
    uint64_t straightPiecesToMove = history.front().filesAndRanks & history.front().ownPieces;


    //check for pinned pieces in all directions
    checkForPins<NORTH, returnMoves>     (kingSquare, occupiedSquares, targetSquares, straightPiecesToMove, diagonalPiecesToMove, pawnsToMove, knightsToMove, moveBuffer, numOfMoves);
    checkForPins<NORTH_EAST, returnMoves>(kingSquare, occupiedSquares, targetSquares, straightPiecesToMove, diagonalPiecesToMove, pawnsToMove, knightsToMove, moveBuffer, numOfMoves);
    checkForPins<EAST, returnMoves>      (kingSquare, occupiedSquares, targetSquares, straightPiecesToMove, diagonalPiecesToMove, pawnsToMove, knightsToMove, moveBuffer, numOfMoves);
    checkForPins<SOUTH_EAST, returnMoves>(kingSquare, occupiedSquares, targetSquares, straightPiecesToMove, diagonalPiecesToMove, pawnsToMove, knightsToMove, moveBuffer, numOfMoves);
    checkForPins<SOUTH, returnMoves>     (kingSquare, occupiedSquares, targetSquares, straightPiecesToMove, diagonalPiecesToMove, pawnsToMove, knightsToMove, moveBuffer, numOfMoves);
    checkForPins<SOUTH_WEST, returnMoves>(kingSquare, occupiedSquares, targetSquares, straightPiecesToMove, diagonalPiecesToMove, pawnsToMove, knightsToMove, moveBuffer, numOfMoves);
    checkForPins<WEST, returnMoves>      (kingSquare, occupiedSquares, targetSquares, straightPiecesToMove, diagonalPiecesToMove, pawnsToMove, knightsToMove, moveBuffer, numOfMoves);
    checkForPins<NORTH_WEST, returnMoves>(kingSquare, occupiedSquares, targetSquares, straightPiecesToMove, diagonalPiecesToMove, pawnsToMove, knightsToMove, moveBuffer, numOfMoves);


    //knight moves
    while(knightsToMove) {
        char knightSquare = __builtin_ctzll(knightsToMove);
        knightsToMove &= ~getBitboard(knightSquare);

        uint64_t moveTargets = targetSquares & getKnightMoveSquares(knightSquare);

        generateMoves<returnMoves>(knightSquare, moveTargets, numOfMoves, moveBuffer);

    }

    //pawn moves
    if(this->whitesTurn) {
        //get a bit map of all target squares of pawn capture move to the north east
        uint64_t captureSquares = shift<NORTH_EAST>(pawnsToMove) & occupiedSquares & targetSquares;

        generatePawnMoves<returnMoves>(captureSquares, +7, numOfMoves, moveBuffer);

        //get a bit map of all squares on which a pawn can capture when moving north west
        captureSquares = shift<NORTH_WEST>(pawnsToMove) & occupiedSquares & targetSquares;

        generatePawnMoves<returnMoves>(captureSquares, +9, numOfMoves, moveBuffer);

        //a bitmap of all squares reachable by one square non capture pawn moves
        uint64_t nonCaptureSquares = shift<NORTH>(pawnsToMove) & ~occupiedSquares & targetSquares;

        generatePawnMoves<returnMoves>(nonCaptureSquares, +8, numOfMoves, moveBuffer);

        //a bitmap of all squares reachable by two square moves
        uint64_t twoSquareMoves = shift<NORTH>(shift<NORTH>(pawnsToMove) & ~occupiedSquares) & ~occupiedSquares & targetSquares & (((uint64_t) 0xff) << 32);

        generateNonPromotionPawnMoves<returnMoves>(twoSquareMoves, + 16, numOfMoves, moveBuffer);

    } else {
        uint64_t captureSquares = shift<SOUTH_EAST>(pawnsToMove) & occupiedSquares & targetSquares;

        generatePawnMoves<returnMoves>(captureSquares, -9, numOfMoves, moveBuffer);


        captureSquares = shift<SOUTH_WEST>(pawnsToMove) & occupiedSquares & targetSquares;

        generatePawnMoves<returnMoves>(captureSquares, -7, numOfMoves, moveBuffer);


        uint64_t nonCaptureSquares = shift<SOUTH>(pawnsToMove) & ~occupiedSquares & targetSquares;

        generatePawnMoves<returnMoves>(nonCaptureSquares, -8, numOfMoves, moveBuffer);


        uint64_t twoSquareMoves = shift<SOUTH>(shift<SOUTH>(pawnsToMove) & ~occupiedSquares) & ~occupiedSquares & targetSquares & (((uint64_t) 0xff) << 24);

        generateNonPromotionPawnMoves<returnMoves>(twoSquareMoves, - 16, numOfMoves, moveBuffer);
    }

    //diagonal moves
    while(diagonalPiecesToMove) {
        char currentPiece = __builtin_ctzll(diagonalPiecesToMove);
        diagonalPiecesToMove &= ~getBitboard(currentPiece);
        uint64_t blockerNorthWest = getFirstBlockerInDirection<NORTH_WEST>(currentPiece, occupiedSquares | 0xff | 0x0101010101010101);
        uint64_t blockerNorthEast = getFirstBlockerInDirection<NORTH_EAST>(currentPiece, occupiedSquares | 0xff | 0x8080808080808080);
        uint64_t blockerSouthEast = getFirstBlockerInDirection<SOUTH_EAST>(currentPiece, occupiedSquares | (((uint64_t) 0xff) << 56) | 0x8080808080808080);
        uint64_t blockerSouthWest = getFirstBlockerInDirection<SOUTH_WEST>(currentPiece, occupiedSquares | (((uint64_t) 0xff) << 56) | 0x0101010101010101);

        uint64_t rayNorthWest = getSquaresUntilBlocker<NORTH_WEST>(currentPiece, blockerNorthWest);
        uint64_t rayNorthEast = getSquaresUntilBlocker<NORTH_EAST>(currentPiece, blockerNorthEast);
        uint64_t raySouthEast = getSquaresUntilBlocker<SOUTH_EAST>(currentPiece, blockerSouthEast);
        uint64_t raySouthWest = getSquaresUntilBlocker<SOUTH_WEST>(currentPiece, blockerSouthWest);

        uint64_t rays = rayNorthEast | rayNorthWest | raySouthEast | raySouthWest;
        uint64_t blocker = blockerNorthEast | blockerNorthWest | blockerSouthEast | blockerSouthWest;

        uint64_t moveTargets = rays & ~(blocker & history.front().ownPieces) & targetSquares;
        
        generateMoves<returnMoves>(currentPiece, moveTargets, numOfMoves, moveBuffer);

    }

    //straight moves
    while(straightPiecesToMove) {
        char currentPiece = __builtin_ctzll(straightPiecesToMove);
        straightPiecesToMove &= ~getBitboard(currentPiece);
        uint64_t blockerNorth = getFirstBlockerInDirection<NORTH>(currentPiece, occupiedSquares | 0xff);
        uint64_t blockerEast = getFirstBlockerInDirection<EAST>(currentPiece, occupiedSquares | 0x8080808080808080);
        uint64_t blockerSouth = getFirstBlockerInDirection<SOUTH>(currentPiece, occupiedSquares | (((uint64_t) 0xff) << 56));
        uint64_t blockerWest = getFirstBlockerInDirection<WEST>(currentPiece, occupiedSquares | 0x0101010101010101);

        uint64_t rayNorth = getSquaresUntilBlocker<NORTH>(currentPiece, blockerNorth);
        uint64_t rayEast = getSquaresUntilBlocker<EAST>(currentPiece, blockerEast);
        uint64_t raySouth = getSquaresUntilBlocker<SOUTH>(currentPiece, blockerSouth);
        uint64_t rayWest = getSquaresUntilBlocker<WEST>(currentPiece, blockerWest);

        uint64_t rays = rayEast | rayNorth | raySouth | rayWest;
        uint64_t blocker = blockerEast | blockerNorth | blockerSouth | blockerWest;

        uint64_t moveTargets = rays & ~(blocker & history.front().ownPieces) & targetSquares;

        generateMoves<returnMoves>(currentPiece, moveTargets, numOfMoves, moveBuffer);
    }

    //king moves
    uint64_t kingMoveSquares = getKingMoveSquares(kingSquare) & ~history.front().ownPieces;
    while(kingMoveSquares) {
        char target = __builtin_ctzll(kingMoveSquares);
        if(!wouldKingBeInCheck(target)) {
            if(returnMoves) {
                moveBuffer[numOfMoves] = Game::Move(kingSquare, target);
            }
            numOfMoves++;
        }
        kingMoveSquares &= ~getBitboard(target);
    }

    //en passant
    if(history.front().enpassantFile != -1) {
        
        char capturedPawnSquare;
        char targetSquare;
        if(this->whitesTurn) {
            capturedPawnSquare = history.front().enpassantFile + 24;
            targetSquare = history.front().enpassantFile + 16;
        } else {
            capturedPawnSquare = history.front().enpassantFile + 32;
            targetSquare = history.front().enpassantFile + 40;
        }

        #pragma gcc unroll 2
        for(int i = 0; i < 2; i++) {
            uint64_t capturingPawn;
            if(i == 0) { 
                //if there is a pawn west of the pawn to be captured, its square will be set in this bit board.
                capturingPawn = shift<WEST>(getBitboard(capturedPawnSquare)) & history.front().ownPieces & history.front().pawns; 
            } else {
                capturingPawn = shift<EAST>(getBitboard(capturedPawnSquare)) & history.front().ownPieces & history.front().pawns;
            }

            if(capturingPawn) {
                //save the actual values for later
                uint64_t tmpPawns = history.front().pawns;
                uint64_t tmpOwnPieces = history.front().ownPieces;

                //temporarily perform the en passant capture
                history.front().ownPieces &= ~capturingPawn;
                history.front().pawns &= ~capturingPawn;
                history.front().ownPieces |= getBitboard(targetSquare);
                history.front().pawns |= getBitboard(targetSquare);
                history.front().pawns &= ~getBitboard(capturedPawnSquare);

                //if the king is not in check after the en passant capture, add the move to the move list.
                if(!wouldKingBeInCheck(kingSquare)) {
                    if(returnMoves) {
                        moveBuffer[numOfMoves] = Game::Move(__builtin_ctzll(capturingPawn), targetSquare);
                    }
                    numOfMoves ++;
                }
                //restore the modified values
                history.front().ownPieces = tmpOwnPieces;
                history.front().pawns = tmpPawns;
            }
        }
    }

    //castling
    if(!kingInCheck) {
        //this array contains the squares that the king will cross during castling, which therefore must not be attacked by enemy pieces
        const char squaresNotToBeChecked[4][2] = {
                {58, 59}, //long castling white
                {61, 62}, //short castling white
                {2, 3}, //long castling black
                {5, 6} //short castling black
        };
        //this holds a bitboard for every castling type containing the squares that must no be occupied when castling
        const uint64_t squaresNotToBeOccupied[4] = {
            ((uint64_t) 7) << 57,
            ((uint64_t) 3) << 61,
            ((uint64_t) 7) << 1,
            ((uint64_t) 3) << 5
        };

        const char fromSquares[4] = {60, 60, 4, 4};
        const char toSquares[4] = {58, 62, 2, 6};

        //go through the four possible castling moves
        #pragma gcc unroll 4
        for(int i = 0; i < 4; i++) {
            if((this->whitesTurn && (i == 0 || i == 1)) || (!this->whitesTurn && (i == 2 || i == 3))) { //only two castling moves are available to each player
                if(history.front().castlingRights & (1 << i)) { //check if the player has the corresponding castling right
                    if(     (occupiedSquares & squaresNotToBeOccupied[i]) == 0  //no pieces between rook and king
                            && !wouldKingBeInCheck(squaresNotToBeChecked[i][0]) //check if the king moves through or ends up at an attacked square
                            && !wouldKingBeInCheck(squaresNotToBeChecked[i][1])) { 

                        if(returnMoves) {
                            //if requested write the castling move to moveBuffer
                            moveBuffer[numOfMoves] = Move(fromSquares[i], toSquares[i]);
                        }
                        numOfMoves++;
                    }
                }
            }
        }
    }
    
    return numOfMoves;
}


void Game::printBitBoard(uint64_t board) {
    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            std::cout << " " << ((board >> i*8+j) & ((uint64_t )1));
        }
        std::cout << std::endl;
    }
}

void Game::printInternalRepresentation() {
    std::cout << "pawns:" << std::endl;
    printBitBoard(this->history.front().pawns);
    std::cout << "knights:" << std::endl;
    printBitBoard(this->history.front().knights);
    std::cout << "diagonals:" << std::endl;
    printBitBoard(this->history.front().diagonals);
    std::cout << "filesAndRanks:" << std::endl;
    printBitBoard(this->history.front().filesAndRanks);
    std::cout << "kings:" << std::endl;
    printBitBoard(this->history.front().kings);
    std::cout << "ownPieces:" << std::endl;
    printBitBoard(this->history.front().ownPieces);
    std::cout << "castlingRights: " << std::endl;
    if(this->history.front().castlingRights & 1)
        std::cout << "white long" << std::endl;
    if(this->history.front().castlingRights & 2)
        std::cout << "white short" << std::endl;
    if(this->history.front().castlingRights & 4)
        std::cout << "black long" << std::endl;
    if(this->history.front().castlingRights & 8)
        std::cout << "black short" << std::endl;
    
    //std::cout << "file of en passant pawn: " << (this->history.front().enpassantFile == -1 ? "none" : std::string(1, (char)(history.front().enpassantFile + 97))) << std::endl;

    std::cout << "next to move: " << (this->whitesTurn ? "white" : "black") << std::endl;
    std::cout << "half moves since capture or pawn advance: " << (int) this->history.front().halfMoveClock << std::endl;
    std::cout << "currently in full move: " << this->fullMoveClock << std::endl;
    
}

int Game::getLeafEvaluation() {
    bool kingInCheck;
    int numOfMoves = getNumOfMoves(kingInCheck);
    return getLeafEvaluation(kingInCheck, numOfMoves);
}

int Game::getLeafEvaluation(bool kingInCheck, int numOfMoves) {
    
    short material  = 300 * __builtin_popcountll(history.front().knights & history.front().ownPieces)
                    + 100 * __builtin_popcountll(history.front().pawns & history.front().ownPieces)
                    + 300 * __builtin_popcountll(history.front().diagonals & ~history.front().filesAndRanks & history.front().ownPieces)
                    + 500 * __builtin_popcountll(~history.front().diagonals & history.front().filesAndRanks & history.front().ownPieces)
                    + 900 * __builtin_popcountll(history.front().diagonals & history.front().filesAndRanks & history.front().ownPieces)
                    - 300 * __builtin_popcountll(history.front().knights & ~history.front().ownPieces)
                    - 100 * __builtin_popcountll(history.front().pawns & ~history.front().ownPieces)
                    - 300 * __builtin_popcountll(history.front().diagonals & ~history.front().filesAndRanks & ~history.front().ownPieces)
                    - 500 * __builtin_popcountll(~history.front().diagonals & history.front().filesAndRanks & ~history.front().ownPieces)
                    - 900 * __builtin_popcountll(history.front().diagonals & history.front().filesAndRanks & ~history.front().ownPieces);

    if(kingInCheck) {
        return material - 20;
    } else {
        //king is not in check -> null move is legal
        
        //save some values
        char tmpEnPassant = history.front().enpassantFile;
        uint64_t tmpOwnPieces = history.front().ownPieces;

        //perform a null move. A null move just switches the player that has to play
        history.front().ownPieces = (history.front().kings | history.front().knights | history.front().diagonals | history.front().filesAndRanks | history.front().pawns) & ~(history.front().ownPieces);
        history.front().enpassantFile = -1;
        this->whitesTurn = !this->whitesTurn;

        //get the number of moves the opponent could make in the current position
        bool tmp;
        short opponentNumOfMoves = this->getNumOfMoves(tmp);
        
        //restore the position with the previously saved values
        history.front().ownPieces = tmpOwnPieces;
        history.front().enpassantFile = tmpEnPassant;
        this->whitesTurn = !this->whitesTurn;


        return material + numOfMoves - opponentNumOfMoves;
    }
}

uint64_t Game::getPositionHash() {
    uint64_t hash = 0;

    //iterate through the 6 piece types
    #pragma unroll 6
    for(int i = 0; i < 6; i++) {
        //first add own pieces, then opponent pieces
        #pragma unroll 2
        for(int j = 0; j < 2; j++) {
            bool blacksTurn = (j == this->whitesTurn);
            uint64_t pieceBoard;
            switch (i) {
                case 0:
                    pieceBoard = history.front().pawns;
                    break;
                case 1:
                    pieceBoard = history.front().knights;
                    break;
                case 2: 
                    pieceBoard = history.front().diagonals & ~history.front().filesAndRanks;
                    break;
                case 3:
                    pieceBoard = history.front().filesAndRanks & ~history.front().diagonals;
                    break;
                case 4:
                    pieceBoard = history.front().filesAndRanks & history.front().diagonals;
                    break;
                case 5:
                    pieceBoard = history.front().kings;
                    break;
            }
            if(j == 0) {
                pieceBoard &= history.front().ownPieces;
            } else {
                pieceBoard &= ~history.front().ownPieces;
            }

            while(pieceBoard) {
                char square = __builtin_ctzll(pieceBoard);
                pieceBoard &= ~getBitboard(square);

                hash ^= zobristPieces[6*64*blacksTurn+64*i+square];
            }
            
        }   
    }
    hash ^= zobristCastlingRights[history.front().castlingRights];
    if(history.front().enpassantFile != -1)
        hash ^= zobristEnPassat[history.front().enpassantFile];

    hash ^= zobristPlayerToMove[this->whitesTurn];
    
    return hash;
}


char Game::getPieceOnSquare(char square) {
    uint64_t mask = getBitboard(square);
    if(history.front().pawns & mask)
        return PAWN;
    if(history.front().knights & mask)
        return KNIGHT;
    if(history.front().diagonals & ~history.front().filesAndRanks & mask)
        return BISHOP;
    if(history.front().diagonals & mask)
        return QUEEN;
    if(history.front().filesAndRanks & mask)
        return ROOK;
    if(history.front().kings & mask)
        return KING;

    return NO_PIECE;
}