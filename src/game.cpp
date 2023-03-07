#include <vector>
#include <array>
#include <list>
#include <cctype>

#include <iostream>

#include "game.h"
#include "bitboard.h"
#include "move.h"
#include "constants.h"

using namespace Bitboard;


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


Game::Position::Position(std::string fen) {
    int index = 0;
    int rank = 0;
    int file = 0;

    this->diagonals = 0;
    this->filesAndRanks = 0;
    this->kings = 0;
    this->knights = 0;
    this->pawns = 0;
    this->ownPieces = 0;
    this->castlingRights = 0;
    this->enpassantFile = -1;

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
                this->pawns |= getBitboard(8*rank+file);
            } else if(toupper(c) == 'N') {
                this->knights |= getBitboard(8*rank+file);
            } else if(toupper(c) == 'K') {
                this->kings |= getBitboard(8*rank+file);
            } else if(toupper(c) == 'B') {
                this->diagonals |= getBitboard(8*rank+file);
            } else if(toupper(c) == 'R') {
                this->filesAndRanks |= getBitboard(8*rank+file);
            } else if(toupper(c) == 'Q') {
                this->filesAndRanks |= getBitboard(8*rank+file);
                this->diagonals |= getBitboard(8*rank+file);
            }
            bool whitePiece = (c == toupper(c));
            this->ownPieces |= ((uint64_t) whitePiece) << (8*rank+file);
            file++;
            
        }
        index ++;
    }

    index ++;

    if(fen.at(index) == 'b') {
        this->ownPieces = (this->pawns | this->diagonals | this->filesAndRanks | this->knights | this->kings) & ~this->ownPieces;
        this->whitesTurn = false;
    }

    index += 2;
    while((c = fen.at(index)) != ' ') {
        if(c == 'k')
            this->castlingRights |= 1 << 3;
        if(c == 'q')
            this->castlingRights |= 1 << 2;
        if(c == 'K')
            this->castlingRights |= 1 << 1;
        if(c == 'Q') {
            this->castlingRights |= 1 << 0;
        }
        index++;
    }

    index ++;
    if((c = fen.at(index)) != '-') {
        this->enpassantFile = c - 97;
        index++;
    }
    index += 2;

    this->halfMoveClock = 0;
    while((c = fen.at(index)) != ' ') {
        this->halfMoveClock *= 10;
        this->halfMoveClock += c - 0x30;
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
}

//game starts with the given position
Game::Game(std::string fen) {  
    this->history.push_back(Position(fen));
    this->pos = &history.back();
}


bool Game::isPositionDraw(int distanceToRoot) {

    //draw by fifty move rule
    if(pos->halfMoveClock >= 100) 
        return true;


    int repetitions = 0;
    
    std::list<Game::Position>::iterator it = history.end();
    it --;
    for(int i = 2; i < history.size() && i <= pos->halfMoveClock; i += 2) {
        it--;
        it--;

        if(         it->castlingRights == pos->castlingRights
                &&  it->enpassantFile == pos->enpassantFile
                &&  it->diagonals == pos->diagonals
                &&  it->filesAndRanks == pos->filesAndRanks
                &&  it->kings == pos->kings
                &&  it->knights == pos->knights
                &&  it->ownPieces == pos->ownPieces
                &&  it->pawns == pos->pawns) {
                
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
    int straightMovingPieces = __builtin_popcountll(pos->filesAndRanks);
    int bishops = __builtin_popcountll(pos->diagonals & ~pos->filesAndRanks);
    int knights = __builtin_popcountll(pos->knights);
    int pawns = __builtin_popcountll(pos->pawns);

    if(straightMovingPieces == 0 && pawns == 0) {
        //king vs king + bishop and king vs king + knight are drawn positions
        if(knights + bishops <= 1)
            return true;


        if(bishops == 2) {
            //if the bishops are on squares of the same color, and belong to different players.
            if(     (((bool) (lightSquares & pos->diagonals)) != ((bool) (darkSquares & pos->diagonals)))
                &&  (__builtin_popcountll(pos->diagonals & pos->ownPieces) == 1)) {

                    return true;
                }
        }
    }

    return false;
}

bool Game::Position::moveLegal(Move move) {
    Move moveBuffer[343];
    int numOfMoves = this->getLegalMoves(moveBuffer);
    for(int i = 0; i < numOfMoves; i++) {
        if(moveBuffer[i] == move)
            return true;
    }
    return false;
}

uint64_t Game::Position::getPseudoLegalBishopMoves(char square, uint64_t occupiedSquares) {
    return getPseudoLegalBishopMoves(square, occupiedSquares, ownPieces);
}

uint64_t Game::Position::getPseudoLegalBishopMoves(char square, uint64_t occupiedSquares, uint64_t ownPieces) {
    return getBishopAttacks(square, occupiedSquares) & ~ownPieces;
}

uint64_t Game::Position::getBishopAttacks(char square, uint64_t occupiedSquares) {

    uint64_t rayNorthWest = getBlockedRay<NORTH_WEST, true>(square, occupiedSquares);
    uint64_t rayNorthEast = getBlockedRay<NORTH_EAST, true>(square, occupiedSquares);
    uint64_t raySouthEast = getBlockedRay<SOUTH_EAST, true>(square, occupiedSquares);
    uint64_t raySouthWest = getBlockedRay<SOUTH_WEST, true>(square, occupiedSquares);

    uint64_t attacks = rayNorthEast | rayNorthWest | raySouthEast | raySouthWest;

    return attacks;
}

uint64_t Game::Position::getPseudoLegalRookMoves(char square, uint64_t occupiedSquares) {
    return getPseudoLegalRookMoves(square, occupiedSquares, ownPieces);
}

uint64_t Game::Position::getPseudoLegalRookMoves(char square, uint64_t occupiedSquares, uint64_t ownPieces) {
    return getRookAttacks(square, occupiedSquares) & ~ownPieces;
}

uint64_t Game::Position::getRookAttacks(char square, uint64_t occupiedSquares) {
    
    uint64_t rayNorth = getBlockedRay<NORTH, true>(square, occupiedSquares);
    uint64_t rayEast = getBlockedRay<EAST, true>(square, occupiedSquares);
    uint64_t raySouth = getBlockedRay<SOUTH, true>(square, occupiedSquares);
    uint64_t rayWest = getBlockedRay<WEST, true>(square, occupiedSquares);

    uint64_t attacks = rayEast | rayNorth | raySouth | rayWest;

    return attacks;
}

//executes the given move
void Game::makeMove(Move move) {
    history.push_back(Position(pos, move));
    pos = &history.back();
}

Game::Position::Position(Game::Position *pos, Move move) {

    int moveMode = move.flags & 0xf0;

    //copy castling rights and remove them later, if necessary
    this->castlingRights = pos->castlingRights;

    uint64_t moveToMask = getBitboard(move.to);
    uint64_t moveFromMask = getBitboard(move.from);

    uint64_t newOwnPieces = (pos->ownPieces & ~moveFromMask) | moveToMask;

    if(pos->pawns & moveFromMask) {
        
        //remove pawn from former square
        this->pawns = pos->pawns & ~moveFromMask;

        //kings won't be affected
        this->kings = pos->kings;


        if(pos->enpassantFile != -1 && (
                (pos->whitesTurn && move.to == pos->enpassantFile + 16) ||
                (!pos->whitesTurn && move.to == pos->enpassantFile + 40))) {
            /*add pawn to the new field*/
            this->pawns |= moveToMask;
            /*remove en passant captured pawn*/
            if(pos->whitesTurn) {
                this->pawns &= ~(moveToMask << 8);
            } else {
                this->pawns &= ~(moveToMask >> 8);
            }

            //no other piece can be captured
            this->diagonals = pos->diagonals;
            this->filesAndRanks = pos->filesAndRanks;
            this->knights = pos->knights;

            this->enpassantFile = -1;  //a en passant capture can't be the first move of a pawn
        } else if(move.flags) {

            switch(move.flags) {
                case 2: //promote to knight
                    
                    //add knight to the destination square
                    this->knights = pos->knights | moveToMask;
                    //remove potentially captured pieces
                    this->diagonals = pos->diagonals & ~moveToMask;
                    this->filesAndRanks = pos->filesAndRanks & ~moveToMask;
                    //enemy pawns can no be on the enemy base line, so they can't be captured here
                    break;

                case 3: //promote to bishop
                    //add bishop
                    this->diagonals = pos->diagonals | moveToMask;
                    //remove potentially captured pieces
                    this->knights = pos->knights & ~moveToMask;
                    this->filesAndRanks = pos->filesAndRanks & ~moveToMask;
                    //enemy pawns can no be on the enemy base line, so they can't be captured here

                    break;
                case 4: //promote to rook
                    //add rook
                    this->filesAndRanks = pos->filesAndRanks | moveToMask;
                    //remove potentially captured pieces
                    this->diagonals = pos->diagonals & ~moveToMask;
                    this->knights = pos->knights & ~moveToMask;
                
                    break;
                case 5: //promote to queen
                    //add queen
                    this->filesAndRanks = pos->filesAndRanks | moveToMask;
                    this->diagonals = pos->diagonals | moveToMask;
                    //remove pontentially captured pieces
                    this->knights = pos->knights & ~moveToMask;

                    break;
            }
            this->enpassantFile = -1; //promoting moves can't be two square moves.
        } else { //normal pawn move
            /*add pawn to the new field*/
            this->pawns |= moveToMask;
            /*remove any potentially captured pieces*/
            this->knights = pos->knights & ~moveToMask;
            this->diagonals = pos->diagonals & ~moveToMask;
            this->filesAndRanks = pos->filesAndRanks & ~moveToMask;

            //add en passant flag if the pawn moves two squares and there is an enemy pawn next to it
            if(((move.from - move.to == 16) || (move.from - move.to == -16)) 
                        && (((shift<WEST>(getBitboard(move.to)) | shift<EAST>(getBitboard(move.to))) & pos->pawns & ~pos->ownPieces))) {
                
                this->enpassantFile = move.to & 7;
            } else {
                this->enpassantFile = -1;
            }
        }
        this->halfMoveClock = 0;
    } else {
        //non pawn move
        this->enpassantFile = -1;

        if((pos->diagonals | pos->filesAndRanks | pos->knights | pos->pawns) & moveToMask) {
            //capture move
            this->halfMoveClock = 0;
        } else {
            //reversible move
            this->halfMoveClock = pos->halfMoveClock + 1;
        }


        if(pos->knights & moveFromMask) { //knight move
            //move knight
            this->knights = (pos->knights & ~moveFromMask) | moveToMask;
            
            //remove potentially captured pieces
            this->diagonals = pos->diagonals & ~moveToMask;
            this->filesAndRanks = pos->filesAndRanks & ~moveToMask;
            this->pawns = pos->pawns & ~moveToMask;
            //kings won't be affected
            this->kings = pos->kings;
        
        } else if(pos->kings & moveFromMask) { //king move
            //move king
            this->kings = (pos->kings & ~moveFromMask) | moveToMask;
            //remove potentially captured pieces
            this->knights = pos->knights & ~moveToMask;
            this->diagonals = pos->diagonals & ~moveToMask;
            this->filesAndRanks = pos->filesAndRanks & ~moveToMask;
            this->pawns = pos->pawns & ~moveToMask;


            if(pos->whitesTurn) {
                //remove castling rights
                this->castlingRights &= 0x0c;

                //castle if castling move is detected
                if(move.from - move.to == 2) { //queen side
                    this->filesAndRanks &= ~getBitboard(56);
                    this->filesAndRanks |= getBitboard(59);
                    newOwnPieces |= getBitboard(59);
                } else if(move.from - move.to == -2) { //king side
                    this->filesAndRanks &= ~getBitboard(63);
                    this->filesAndRanks |= getBitboard(61);
                    newOwnPieces |= getBitboard(61);
                }
            } else {
                //remove castling rights
                this->castlingRights &= 0x03;

                //perfrom castling if castling move is detected
                if(move.from - move.to == 2) { //queen side
                    this->filesAndRanks &= ~getBitboard(0);
                    this->filesAndRanks |= getBitboard(3);
                    newOwnPieces |= getBitboard(3);
                } else if(move.from - move.to == -2) { //king side
                    this->filesAndRanks &= ~getBitboard(7);
                    this->filesAndRanks |= getBitboard(5);
                    newOwnPieces |= getBitboard(5);
                }
            }

        } else if(pos->diagonals & ~pos->filesAndRanks & moveFromMask) { //bishop move
            //move bishop
            this->diagonals = (pos->diagonals & ~moveFromMask) | moveToMask;
            //remove potentially captured pieces
            this->knights = pos->knights & ~moveToMask;
            this->filesAndRanks = pos->filesAndRanks & ~moveToMask;
            this->pawns = pos->pawns & ~moveToMask;
            //kings won't be affected
            this->kings = pos->kings;

        } else if(pos->diagonals & pos->filesAndRanks & moveFromMask) { //queen move
            //move queen
            this->diagonals = (pos->diagonals & ~moveFromMask) | moveToMask;
            this->filesAndRanks = (pos->filesAndRanks & ~moveFromMask) | moveToMask;

            //remove potentially captured pieces
            this->knights = pos->knights & ~moveToMask;
            this->pawns = pos->pawns & ~moveToMask;
            //kings won't be affected
            this->kings = pos->kings;

        } else { //rook move
            //move rook
            this->filesAndRanks = (pos->filesAndRanks & ~moveFromMask) | moveToMask;
            //remove potentially captured pieces
            this->knights = pos->knights & ~moveToMask;
            this->diagonals = pos->diagonals & ~moveToMask;
            this->pawns = pos->pawns & ~moveToMask;
            //kings won't be affected
            this->kings = pos->kings;
        }
    }

    //remove castling rights if rook moved or captured
    if(move.from == 0 || move.to == 0) {
        this->castlingRights &= ~(1 << 2);
    } 
    if(move.from == 7 || move.to == 7) {
        this->castlingRights &= ~(1 << 3);
    }
    if(move.from == 56 || move.to == 56) {
        this->castlingRights &= ~(1 << 0);
    }
    if(move.from == 63 || move.to == 63) {
        this->castlingRights &= ~(1 << 1);
    } 


    this->ownPieces = (this->diagonals | this->filesAndRanks | this->knights | this->pawns | this->kings) & ~newOwnPieces;
    
    if(!pos->whitesTurn) {
        this->fullMoveClock = pos->fullMoveClock+1;
    } else {
        this->fullMoveClock = pos->fullMoveClock;
    }

    this->whitesTurn = !pos->whitesTurn;
}

//reverts the last move
void Game::undo() {
    history.pop_back();
    pos = &history.back();
}

//determines wether the given move is a capture move
bool Game::Position::isCapture(Move move) {

    //check if there is a piece on the target square of the move. In that case the move is definitely a capture
    if(getBitboard(move.to) & (filesAndRanks | diagonals | knights | pawns))
        return true;;
    
    //check for en passant capture
    if(enpassantFile != -1) {
        if(this->whitesTurn) {
            return (move.to - 16 == enpassantFile) && (getBitboard(move.from) & pawns);
        } else {
            return (move.to - 40 == enpassantFile) && (getBitboard(move.from) & pawns);
        }
    }
    return false;
}


bool Game::Position::wouldKingBeInCheck(char kingSquare) {
    
    bool attackedByKnight = getKnightMoveSquares(kingSquare) & (knights & ~ownPieces);
    bool attackedByPawn = getPawnAttacks(kingSquare, !this->whitesTurn) & (pawns & ~ownPieces);
    //kings can't be on adjacent squares, we therefore check for an "attack" by the enemy king
    bool attackedByKing = getKingMoveSquares(kingSquare) & (kings & ~ownPieces);

    if(attackedByKnight | attackedByPawn | attackedByKing)
        return true;
    
    //our own king cannot block a attackRay in this context, since he then still would be in check
    uint64_t occupyingPieces = diagonals | filesAndRanks | knights | pawns | (kings & ~ownPieces);

    uint64_t attackSquares;

    attackSquares = getFirstBlockerInDirection<NORTH>(kingSquare, occupyingPieces)
                  | getFirstBlockerInDirection<SOUTH>(kingSquare, occupyingPieces)
                  | getFirstBlockerInDirection<WEST>(kingSquare, occupyingPieces)
                  | getFirstBlockerInDirection<EAST>(kingSquare, occupyingPieces);
 
    if(attackSquares & (filesAndRanks & ~ownPieces))
        return true;


    attackSquares = getFirstBlockerInDirection<SOUTH_WEST>(kingSquare, occupyingPieces)
                  | getFirstBlockerInDirection<SOUTH_EAST>(kingSquare, occupyingPieces)
                  | getFirstBlockerInDirection<NORTH_EAST>(kingSquare, occupyingPieces)
                  | getFirstBlockerInDirection<NORTH_WEST>(kingSquare, occupyingPieces);
    
    if(attackSquares & (diagonals & ~ownPieces))
        return true;

    return false;
}


int Game::Position::getLegalMoves(Move *moveBuffer) {
    bool tmp;
    return getLegalMoves<true>(tmp, moveBuffer);
}

int Game::Position::getLegalMoves(bool& kingInCheck, Move *moveBuffer) {
    return getLegalMoves<true>(kingInCheck, moveBuffer);
}

//helper functions

template<bool returnMoves>
void inline generatePromotions(char from, char to, int& numOfMoves, Move *moveBuffer) {
    if(returnMoves) {
        //store four moves in the move array, one for every promotion option
        #pragma gcc unroll 4
        for(int j = 0; j < 4; j ++) {
            moveBuffer[numOfMoves+j] = Move(from, to, j+2);
        }
    }
    //increment the move counter by four
    numOfMoves += 4;
}

template<bool returnMoves>
void inline generatePawnMove(char from, char to, int& numOfMoves, Move *moveBuffer) {
    if(to > 55 || to < 8) {
        generatePromotions<returnMoves>(from, to, numOfMoves, moveBuffer);
    } else {
        if(returnMoves) {
            moveBuffer[numOfMoves] = Move(from, to);
        }
        numOfMoves ++;
    }
}

template<bool returnMoves>
void inline generatePromotionPawnMoves(uint64_t to, char fromSquareOffset, int& numOfMoves, Move *moveBuffer) {
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
void inline generateNonPromotionPawnMoves(uint64_t to, char fromSquareOffset, int& numOfMoves, Move *moveBuffer) {
    if(returnMoves) {
        while(to) {
            //get the current target square
            char square = __builtin_ctzll(to);
            //remove it from the target squares list
            to &= ~getBitboard(square);

            moveBuffer[numOfMoves] = Move(square + fromSquareOffset, square);

            numOfMoves++;
        }
    } else {
        numOfMoves += __builtin_popcountll(to);
    }
}

template<bool returnMoves>
void generatePawnMoves(uint64_t to, char fromSquareOffset, int& numOfMoves, Move *moveBuffer) {

    //split the target squares into promotion and no-promotion moves
    uint64_t baseRanks = ((uint64_t) 0xff) | (((uint64_t) 0xff) << 56);
    uint64_t promotingMoves = baseRanks & to;
    uint64_t nonPromotingMoves = ~baseRanks & to;


    generatePromotionPawnMoves<returnMoves>(promotingMoves, fromSquareOffset, numOfMoves, moveBuffer);
    generateNonPromotionPawnMoves<returnMoves>(nonPromotingMoves, fromSquareOffset, numOfMoves, moveBuffer);
}

template <bool returnMoves>
void inline generateMoves(char from, uint64_t to, int& numOfMoves, Move *moveBuffer) {
    if(returnMoves) {
        //while there are still target squares, for which moves need to be generated
        while(to) {
            //get the current target square
            char square = __builtin_ctzll(to);
            //remove it from the target squares list
            to &= ~getBitboard(square);
            //store the move to the move array
            moveBuffer[numOfMoves] = Move(from, square);
            //increment the moves counter
            numOfMoves++;
        }
    } else {
        numOfMoves += __builtin_popcountll(to);
    }
}
    

template<char direction>
void Game::Position::lookForCheck(uint64_t& checkBlockingSquares, uint64_t occupiedSquares, char kingSquare) {
    //depending on the direction pieces that move straight or pieces that move diagonally can give check
    uint64_t checkGivingPieces;
    if(direction == NORTH || direction == SOUTH || direction == EAST || direction == WEST) 
        checkGivingPieces = ~ownPieces & filesAndRanks;
    else
        checkGivingPieces = ~ownPieces & diagonals;

    //if the king is in check from the given direction all moves must either block the check or capture the checking piece
    uint64_t attackingPiece = getFirstBlockerInDirection<direction>(kingSquare, occupiedSquares) & checkGivingPieces;
    if(attackingPiece)
        checkBlockingSquares &= getSquaresUntilBlocker<direction>(kingSquare, attackingPiece);
}

uint64_t Game::Position::getCheckBlockingSquares() {
    uint64_t blockSquares = ~((uint64_t) 0);

    uint64_t occupiedSquares = kings | knights | filesAndRanks | diagonals | pawns;

    char kingSquare = __builtin_ctzll(kings & ownPieces);

    uint64_t attackingKnight = getKnightMoveSquares(kingSquare) & knights & ~ownPieces;
    if(attackingKnight)
        blockSquares = attackingKnight;

    uint64_t attackingPawns = getPawnAttacks(kingSquare, !this->whitesTurn) & (pawns & ~ownPieces);
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
inline void Game::Position::checkForPins(char& kingSquare, uint64_t& occupiedSquares, uint64_t& targetSquares, uint64_t& straightPiecesToMove, uint64_t& diagonalPiecesToMove, uint64_t& pawnsToMove, uint64_t& knightsToMove, Move *moveBuffer, int& numOfMoves) {
    
    uint64_t firstBlocker = getFirstBlockerInDirection<direction>(kingSquare, occupiedSquares);
    
    bool diagonalDirection = (direction == NORTH_EAST || direction == SOUTH_EAST || direction == SOUTH_WEST || direction == NORTH_WEST);

    if(firstBlocker & ownPieces) {

        uint64_t secondBlocker = getFirstBlockerInDirection<direction>(kingSquare, occupiedSquares & ~firstBlocker);

        if(secondBlocker & ~ownPieces & (diagonalDirection ? diagonals : filesAndRanks)) {
            //there is a piece pinned in the direction specified in the template parameter
            //we will now check if there are square the pinned piece can still move to.


            if(direction == NORTH || direction == SOUTH) {
                if(firstBlocker & filesAndRanks) {
                    //rook and queen can move in south north direction
                    //remove the piece from the lists of pieces to examine:
                    straightPiecesToMove &= ~firstBlocker;
                    diagonalPiecesToMove &= ~firstBlocker; //the piece could be a queen
                    //the pinned piece can move between the king and the pinning piece, but staying on the same square is not a valid move
                    uint64_t moveTargets = getSquaresUntilBlocker<direction>(kingSquare, secondBlocker) & ~firstBlocker & targetSquares;
                    generateMoves<returnMoves>(__builtin_ctzll(firstBlocker), moveTargets, numOfMoves, moveBuffer);
                } else if(firstBlocker & pawns) {
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
                if(firstBlocker & filesAndRanks) {
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
                if(firstBlocker & diagonals) {
                    //bishops and queens are still able to move in a diagonal direction if pinned from that direction
                    diagonalPiecesToMove &= ~firstBlocker;
                    straightPiecesToMove &= ~firstBlocker; //the piece could be a queen
                    uint64_t moveTargets = getSquaresUntilBlocker<direction>(kingSquare, secondBlocker) & ~firstBlocker & targetSquares;
                    generateMoves<returnMoves>(__builtin_ctzll(firstBlocker), moveTargets, numOfMoves, moveBuffer);
                } else if(firstBlocker & pawns) {
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
int Game::Position::getLegalMoves(bool& kingInCheck, Move *moveBuffer) {
    int numOfMoves = 0;

    char kingSquare = __builtin_ctzll(kings & ownPieces);

    //target squares are squares that block checks if the king is in check and squares that are not occupied by our own pieces
    uint64_t targetSquares = getCheckBlockingSquares();
    kingInCheck = (targetSquares != ~((uint64_t) 0));
    targetSquares &= ~ownPieces;

    uint64_t occupiedSquares = diagonals | filesAndRanks | kings | pawns | knights;

    //variables to keep track of pieces for which we haven't generated moves yet. 
    uint64_t pawnsToMove = pawns & ownPieces;
    uint64_t knightsToMove = knights & ownPieces;
    uint64_t diagonalPiecesToMove = diagonals & ownPieces;
    uint64_t straightPiecesToMove = filesAndRanks & ownPieces;


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

        uint64_t moveTargets = getPseudoLegalBishopMoves(currentPiece, occupiedSquares) & targetSquares;
        
        generateMoves<returnMoves>(currentPiece, moveTargets, numOfMoves, moveBuffer);

    }

    //straight moves
    while(straightPiecesToMove) {
        char currentPiece = __builtin_ctzll(straightPiecesToMove);
        straightPiecesToMove &= ~getBitboard(currentPiece);
        
        uint64_t moveTargets = getPseudoLegalRookMoves(currentPiece, occupiedSquares) & targetSquares;

        generateMoves<returnMoves>(currentPiece, moveTargets, numOfMoves, moveBuffer);
    }

    //king moves
    uint64_t kingMoveSquares = getKingMoveSquares(kingSquare) & ~ownPieces;
    while(kingMoveSquares) {
        char target = __builtin_ctzll(kingMoveSquares);
        if(!wouldKingBeInCheck(target)) {
            if(returnMoves) {
                moveBuffer[numOfMoves] = Move(kingSquare, target);
            }
            numOfMoves++;
        }
        kingMoveSquares &= ~getBitboard(target);
    }

    //en passant
    if(enpassantFile != -1) {
        
        char capturedPawnSquare;
        char targetSquare;
        if(this->whitesTurn) {
            capturedPawnSquare = enpassantFile + 24;
            targetSquare = enpassantFile + 16;
        } else {
            capturedPawnSquare = enpassantFile + 32;
            targetSquare = enpassantFile + 40;
        }

        #pragma gcc unroll 2
        for(int i = 0; i < 2; i++) {
            uint64_t capturingPawn;
            if(i == 0) { 
                //if there is a pawn west of the pawn to be captured, its square will be set in this bit board.
                capturingPawn = shift<WEST>(getBitboard(capturedPawnSquare)) & ownPieces & pawns; 
            } else {
                capturingPawn = shift<EAST>(getBitboard(capturedPawnSquare)) & ownPieces & pawns;
            }

            if(capturingPawn) {
                //save the actual values for later
                uint64_t tmpPawns = pawns;
                uint64_t tmpOwnPieces = ownPieces;

                //temporarily perform the en passant capture
                ownPieces &= ~capturingPawn;
                pawns &= ~capturingPawn;
                ownPieces |= getBitboard(targetSquare);
                pawns |= getBitboard(targetSquare);
                pawns &= ~getBitboard(capturedPawnSquare);

                //if the king is not in check after the en passant capture, add the move to the move list.
                if(!wouldKingBeInCheck(kingSquare)) {
                    if(returnMoves) {
                        moveBuffer[numOfMoves] = Move(__builtin_ctzll(capturingPawn), targetSquare);
                    }
                    numOfMoves ++;
                }
                //restore the modified values
                ownPieces = tmpOwnPieces;
                pawns = tmpPawns;
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
                if(castlingRights & (1 << i)) { //check if the player has the corresponding castling right
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


void Game::Position::printInternalRepresentation() {
    std::cout << "pawns:" << std::endl;
    printBitBoard(this->pawns);
    std::cout << "knights:" << std::endl;
    printBitBoard(this->knights);
    std::cout << "diagonals:" << std::endl;
    printBitBoard(this->diagonals);
    std::cout << "filesAndRanks:" << std::endl;
    printBitBoard(this->filesAndRanks);
    std::cout << "kings:" << std::endl;
    printBitBoard(this->kings);
    std::cout << "ownPieces:" << std::endl;
    printBitBoard(this->ownPieces);
    std::cout << "castlingRights: " << std::endl;
    if(this->castlingRights & 1)
        std::cout << "white long" << std::endl;
    if(this->castlingRights & 2)
        std::cout << "white short" << std::endl;
    if(this->castlingRights & 4)
        std::cout << "black long" << std::endl;
    if(this->castlingRights & 8)
        std::cout << "black short" << std::endl;
    
    //std::cout << "file of en passant pawn: " << (this->enpassantFile == -1 ? "none" : std::string(1, (char)(enpassantFile + 97))) << std::endl;

    std::cout << "next to move: " << (this->whitesTurn ? "white" : "black") << std::endl;
    std::cout << "half moves since capture or pawn advance: " << (int) this->halfMoveClock << std::endl;
    std::cout << "currently in full move: " << this->fullMoveClock << std::endl;
    
}


uint64_t Game::Position::getPositionHash() {
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
                    pieceBoard = pawns;
                    break;
                case 1:
                    pieceBoard = knights;
                    break;
                case 2: 
                    pieceBoard = diagonals & ~filesAndRanks;
                    break;
                case 3:
                    pieceBoard = filesAndRanks & ~diagonals;
                    break;
                case 4:
                    pieceBoard = filesAndRanks & diagonals;
                    break;
                case 5:
                    pieceBoard = kings;
                    break;
            }
            if(j == 0) {
                pieceBoard &= ownPieces;
            } else {
                pieceBoard &= ~ownPieces;
            }

            while(pieceBoard) {
                char square = __builtin_ctzll(pieceBoard);
                pieceBoard &= ~getBitboard(square);

                hash ^= zobristPieces[6*64*blacksTurn+64*i+square];
            }
            
        }   
    }
    hash ^= zobristCastlingRights[castlingRights];
    if(enpassantFile != -1)
        hash ^= zobristEnPassat[enpassantFile];

    hash ^= zobristPlayerToMove[this->whitesTurn];
    
    return hash;
}

char Game::Position::getPieceOnSquare(char square) {
    uint64_t mask = getBitboard(square);
    if(pawns & mask)
        return PAWN;
    if(knights & mask)
        return KNIGHT;
    if(diagonals & ~filesAndRanks & mask)
        return BISHOP;
    if(diagonals & mask)
        return QUEEN;
    if(filesAndRanks & mask)
        return ROOK;
    if(kings & mask)
        return KING;

    return NO_PIECE;
}