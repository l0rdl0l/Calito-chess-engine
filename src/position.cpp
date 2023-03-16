#include <vector>
#include <array>
#include <list>
#include <cctype>

#include <iostream>

#include "position.h"
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


Position::Position(std::string fen) {

    int index = 0;
    int rank = 0;
    int file = 0;

    for(int i = PAWN; i <= KING; i++) {
        this->pieces[i] = 0;
    }

    for(int i = 0; i < 64; i++) {
        this->squares[i] = NO_PIECE;
    }

    Position::HistoryObject obj;
    obj.castlingRights = 0;
    obj.enpassantFile = -1;
    this->history.push_back(obj);

    this->ownPieces = 0;
    this->occupied = 0;
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
            char piece;
            if(toupper(c) == 'P') {
                piece = PAWN;
            } else if(toupper(c) == 'N') {
                piece = KNIGHT;
            } else if(toupper(c) == 'K') {
                piece = KING;
            } else if(toupper(c) == 'B') {
                piece = BISHOP;
            } else if(toupper(c) == 'R') {
                piece = ROOK;
            } else if(toupper(c) == 'Q') {
                piece = QUEEN;
            }
            this->pieces[piece] |= getBitboard(8*rank+file);
            if(c == toupper(c))
                this->ownPieces |= getBitboard(8*rank+file);
            this->occupied |= getBitboard(8*rank+file);
            this->squares[8*rank+file] = piece;
            file++;
            
        }
        index ++;
    }

    index ++;

    if(fen.at(index) == 'b') {
        this->ownPieces = this->occupied & ~this->ownPieces;
        this->whitesTurn = false;
    }

    index += 2;
    while((c = fen.at(index)) != ' ') {
        if(c == 'k')
            history.front().castlingRights |= 1 << BLACK_CASTLE_KINGSIDE;
        if(c == 'q')
            history.front().castlingRights |= 1 << BLACK_CASTLE_QUEENSIDE;
        if(c == 'K')
            history.front().castlingRights |= 1 << WHITE_CASTLE_KINGSIDE;
        if(c == 'Q') {
            history.front().castlingRights |= 1 << WHITE_CASTLE_QUEENSIDE;
        }
        index++;
    }

    index ++;
    if((c = fen.at(index)) != '-') {
        history.front().enpassantFile = c - 97;
        index++;
    }
    index += 2;

    history.front().halfMoveClock = 0;
    while((c = fen.at(index)) != ' ') {
        history.front().halfMoveClock *= 10;
        history.front().halfMoveClock += c - 0x30;
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


bool Position::isPositionDraw(int distanceToRoot) {

    //draw by fifty move rule
    if(history.front().halfMoveClock >= 100) 
        return true;


    int repetitions = 0;
    
    std::list<Position::HistoryObject>::iterator it = history.begin();
    for(int i = 2; i < history.size() && i <= history.front().halfMoveClock; i += 2) {
        it++;
        it++;

        if(it->hash == history.front().hash) {
            repetitions++;

            //the position occured inside the currently searched line and is therefore evaluated as draw
            if(i <= distanceToRoot) {
                return true;
            }
            //draw by threefold repetition
            if(repetitions >= 2) {
                return true;
            }
        }
    }

    //draw by insufficient material
    int majorPieces = __builtin_popcountll(pieces[ROOK] | pieces[QUEEN]);
    int bishops = __builtin_popcountll(pieces[BISHOP]);
    int knights = __builtin_popcountll(pieces[KNIGHT]);
    int pawns = __builtin_popcountll(pieces[PAWN]);

    if(majorPieces == 0 && pawns == 0) {
        //king vs king + bishop and king vs king + knight are drawn positions
        if(knights + bishops <= 1)
            return true;


        if(bishops == 2) {
            //if the bishops are on squares of the same color, and belong to different players.
            if(     (((bool) (lightSquares & pieces[BISHOP])) != ((bool) (darkSquares & pieces[BISHOP])))
                &&  (__builtin_popcountll(pieces[BISHOP] & ownPieces) == 1)) {

                    return true;
                }
        }
    }

    return false;
}

bool Position::moveLegal(Move move) {
    Move moveBuffer[343];
    int numOfMoves = this->getLegalMoves(moveBuffer);
    for(int i = 0; i < numOfMoves; i++) {
        if(moveBuffer[i] == move)
            return true;
    }
    return false;
}

void Position::makeMove(Move move) {

    HistoryObject obj;
    obj.capturedPiece = squares[move.to];
    obj.castlingRights = history.front().castlingRights;
    obj.enpassantFile = -1;
    obj.halfMoveClock = history.front().halfMoveClock + 1;
    obj.oldOccupied = occupied;
    obj.oldOwnPieces = ownPieces;
    obj.lastMove = move;

    char movedPiece = squares[move.from];

    uint64_t moveToMask = getBitboard(move.to);
    uint64_t moveFromMask = getBitboard(move.from);

    ownPieces = occupied & ~ownPieces;

    if(obj.capturedPiece != NO_PIECE) {
        pieces[obj.capturedPiece] &= ~moveToMask;
        obj.halfMoveClock = 0;
        ownPieces &= ~moveToMask;
    }

    pieces[movedPiece] &= ~moveFromMask;
    pieces[movedPiece] |= moveToMask;
    occupied &= ~moveFromMask;
    occupied |= moveToMask;
    squares[move.to] = movedPiece;
    squares[move.from] = NO_PIECE;

    if(move.specialMove) {
        if(move.specialMove == 1) {
            /*remove en passant captured pawn*/
            if(whitesTurn) {
                pieces[PAWN] &= ~shift<SOUTH>(moveToMask);
                occupied &= ~shift<SOUTH>(moveToMask);
                ownPieces &= ~shift<SOUTH>(moveToMask);
                squares[move.to + 8] = NO_PIECE;
            } else {
                pieces[PAWN] &= ~shift<NORTH>(moveToMask);
                occupied &= ~shift<NORTH>(moveToMask);
                ownPieces &= ~shift<NORTH>(moveToMask);
                squares[move.to - 8] = NO_PIECE;
            }
        } else if (move.specialMove == 2) {

            pieces[ROOK] &= ~getBitboard(CASTLE_ROOK_ORIGIN[move.flag]);
            occupied &= ~getBitboard(CASTLE_ROOK_ORIGIN[move.flag]);
            squares[CASTLE_ROOK_ORIGIN[move.flag]] = NO_PIECE;

            pieces[ROOK] |= getBitboard(CASTLE_ROOK_DEST[move.flag]);
            occupied |= getBitboard(CASTLE_ROOK_DEST[move.flag]);
            squares[CASTLE_ROOK_DEST[move.flag]] = ROOK;

        } else { //promotion move
            pieces[move.flag] |= moveToMask;
            pieces[PAWN] &= ~moveToMask;
            squares[move.to] = move.flag;
        }

        obj.halfMoveClock = 0;

    } 

    if(movedPiece == PAWN) {

        //add en passant flag if the pawn moves two squares and there is an enemy pawn next to it
        if(((move.from - move.to == 16) || (move.from - move.to == -16)) 
                && (((shift<WEST>(getBitboard(move.to)) | shift<EAST>(getBitboard(move.to))) & pieces[PAWN] & ownPieces))) {
            
            obj.enpassantFile = move.to & 7;
        }
        
        obj.halfMoveClock = 0;
    }

    if(movedPiece == KING) {
        if(whitesTurn) {
            obj.castlingRights &= ~((1 << WHITE_CASTLE_KINGSIDE) | (1 << WHITE_CASTLE_QUEENSIDE));
        } else {
            obj. castlingRights &= ~((1 << BLACK_CASTLE_KINGSIDE) | (1 << BLACK_CASTLE_QUEENSIDE));
        }
    }


    //remove castling rights if rook moved or captured
    for(int i = WHITE_CASTLE_KINGSIDE; i <= BLACK_CASTLE_QUEENSIDE; i++) {
        char rookSquare = CASTLE_ROOK_ORIGIN[i];
        if(move.to == rookSquare || move.from == rookSquare) {
            obj.castlingRights &= ~(1 << i);
        }
    }

    if(!whitesTurn)
        fullMoveClock++;

    whitesTurn = !whitesTurn;

    history.push_front(obj); 

    history.front().hash = calculateHash();
}

//reverts the last move
void Position::undo() {
    Move move = history.front().lastMove;

    uint64_t moveToMask = getBitboard(move.to);
    uint64_t moveFromMask = getBitboard(move.from);

    char capturedPiece = history.front().capturedPiece;
    char movedPiece = squares[move.to];

    whitesTurn = !whitesTurn;

    if(!whitesTurn)
        fullMoveClock --;

    if(move.specialMove) {
        if(move.specialMove == 1) {
            /*remove add passant captured pawn*/
            if(whitesTurn) {
                pieces[PAWN] |= shift<SOUTH>(moveToMask);
                occupied |= shift<SOUTH>(moveToMask);
                squares[move.to + 8] = PAWN;
            } else {
                pieces[PAWN] |= shift<NORTH>(moveToMask);
                occupied |= shift<NORTH>(moveToMask);
                squares[move.to - 8] = PAWN;
            }
        } else if(move.specialMove == 2) {
            pieces[ROOK] |= getBitboard(CASTLE_ROOK_ORIGIN[move.flag]);
            occupied |= getBitboard(CASTLE_ROOK_ORIGIN[move.flag]);
            squares[CASTLE_ROOK_ORIGIN[move.flag]] = ROOK;

            pieces[ROOK] &= ~getBitboard(CASTLE_ROOK_DEST[move.flag]);
            occupied &= ~getBitboard(CASTLE_ROOK_DEST[move.flag]);
            squares[CASTLE_ROOK_DEST[move.flag]] = NO_PIECE;
        } else {
            //remove promoted piece
            pieces[move.flag] &= ~moveToMask;

            movedPiece = PAWN;
        }
    }

    pieces[movedPiece] |= moveFromMask;
    pieces[movedPiece] &= ~moveToMask;
    squares[move.from] = movedPiece;
    squares[move.to] = capturedPiece;

    if(history.front().capturedPiece != NO_PIECE)
        pieces[capturedPiece] |= moveToMask;


    ownPieces = history.front().oldOwnPieces;
    occupied = history.front().oldOccupied;

    history.pop_front();
}

//determines wether the given move is a capture move
bool Position::isCapture(Move move) {
    return (move.specialMove == 1) || (squares[move.to] != NO_PIECE);
}

bool Position::wouldKingBeInCheck(char kingSquare) {
    
    bool attackedByKnight = getKnightAttacks(kingSquare) & (pieces[KNIGHT] & ~ownPieces);
    bool attackedByPawn = getPawnAttacks(kingSquare, whitesTurn ? WHITE : BLACK) & (pieces[PAWN] & ~ownPieces);
    //kings can't be on adjacent squares, we therefore check for an "attack" by the enemy king
    bool attackedByKing = getKingAttacks(kingSquare) & (pieces[KING] & ~ownPieces);

    if(attackedByKnight | attackedByPawn | attackedByKing)
        return true;
    
    //our own king cannot block a attackRay in this context, since he then still would be in check
    uint64_t occupyingPieces = occupied & ~(pieces[KING] & ownPieces);

    uint64_t attackSquares;

    attackSquares = getFirstBlockerInDirection<NORTH>(kingSquare, occupyingPieces)
                  | getFirstBlockerInDirection<SOUTH>(kingSquare, occupyingPieces)
                  | getFirstBlockerInDirection<WEST>(kingSquare, occupyingPieces)
                  | getFirstBlockerInDirection<EAST>(kingSquare, occupyingPieces);
 
    if(attackSquares & ((pieces[ROOK] | pieces[QUEEN]) & ~ownPieces))
        return true;


    attackSquares = getFirstBlockerInDirection<SOUTH_WEST>(kingSquare, occupyingPieces)
                  | getFirstBlockerInDirection<SOUTH_EAST>(kingSquare, occupyingPieces)
                  | getFirstBlockerInDirection<NORTH_EAST>(kingSquare, occupyingPieces)
                  | getFirstBlockerInDirection<NORTH_WEST>(kingSquare, occupyingPieces);
    
    if(attackSquares & ((pieces[BISHOP] | pieces[QUEEN]) & ~ownPieces))
        return true;

    return false;
}

bool Position::ownKingInCheck() {
    return wouldKingBeInCheck(__builtin_ctzll(pieces[KING] & ownPieces));
}

int Position::getLegalMoves(Move *moveBuffer) {
    bool tmp;
    return getLegalMoves(tmp, moveBuffer);
}


//helper functions

void inline generatePromotions(char from, char to, int& numOfMoves, Move *moveBuffer) {

    //store four moves in the move array, one for every promotion option
    #pragma gcc unroll 4
    for(int j = KNIGHT; j <= QUEEN; j ++) {
        moveBuffer[numOfMoves+j-KNIGHT] = Move(from, to, 3, j);
    }

    //increment the move counter by four
    numOfMoves += 4;
}

void inline generatePawnMove(char from, char to, int& numOfMoves, Move *moveBuffer) {
    if(to > 55 || to < 8) {
        generatePromotions(from, to, numOfMoves, moveBuffer);
        
    } else {
        moveBuffer[numOfMoves] = Move(from, to);
        numOfMoves ++;
    }
}


void inline generateNonPromotionPawnMoves(uint64_t to, char fromSquareOffset, int& numOfMoves, Move *moveBuffer) {
    foreach(to, [&] (char square) {
        moveBuffer[numOfMoves] = Move(square + fromSquareOffset, square);
        numOfMoves++;
    });
}


void inline generatePawnMoves(uint64_t to, char fromSquareOffset, int& numOfMoves, Move *moveBuffer) {

    //split the target squares into promotion and no-promotion moves
    uint64_t baseRanks = ((uint64_t) 0xff) | (((uint64_t) 0xff) << 56);
    uint64_t promotingMoves = baseRanks & to;
    uint64_t nonPromotingMoves = ~baseRanks & to;

    foreach(promotingMoves, [&] (char square) {
        generatePromotions(square + fromSquareOffset, square, numOfMoves, moveBuffer);
    });

    generateNonPromotionPawnMoves(nonPromotingMoves, fromSquareOffset, numOfMoves, moveBuffer);
}

void inline generateMoves(char from, uint64_t to, int& numOfMoves, Move *moveBuffer) {
    foreach(to, [&] (char square) {
        moveBuffer[numOfMoves] = Move(from, square);
        numOfMoves++;
    });
}
    

template<char direction>
void Position::lookForCheck(uint64_t& checkBlockingSquares, uint64_t occupiedSquares, char kingSquare) {
    //depending on the direction pieces that move straight or pieces that move diagonally can give check
    uint64_t checkGivingPieces;
    if(direction == NORTH || direction == SOUTH || direction == EAST || direction == WEST) 
        checkGivingPieces = ~ownPieces & (pieces[QUEEN] | pieces[ROOK]);
    else
        checkGivingPieces = ~ownPieces & (pieces[QUEEN] | pieces[BISHOP]);

    //if the king is in check from the given direction all moves must either block the check or capture the checking piece
    uint64_t attackingPiece = getFirstBlockerInDirection<direction>(kingSquare, occupiedSquares) & checkGivingPieces;
    if(attackingPiece)
        checkBlockingSquares &= getSquaresUntilBlocker<direction>(kingSquare, attackingPiece);
}

uint64_t Position::getCheckBlockingSquares() {
    uint64_t blockSquares = ~((uint64_t) 0);

    char kingSquare = __builtin_ctzll(pieces[KING] & ownPieces);

    uint64_t attackingKnight = getKnightAttacks(kingSquare) & pieces[KNIGHT] & ~ownPieces;
    if(attackingKnight)
        blockSquares = attackingKnight;

    uint64_t attackingPawns = getPawnAttacks(kingSquare, !this->whitesTurn) & (pieces[PAWN] & ~ownPieces);
    if(attackingPawns)
        blockSquares &= attackingPawns;

    lookForCheck<NORTH>(blockSquares, occupied, kingSquare);
    lookForCheck<SOUTH>(blockSquares, occupied, kingSquare);
    lookForCheck<EAST>(blockSquares, occupied, kingSquare);
    lookForCheck<WEST>(blockSquares, occupied, kingSquare);
    lookForCheck<NORTH_EAST>(blockSquares, occupied, kingSquare);
    lookForCheck<NORTH_WEST>(blockSquares, occupied, kingSquare);
    lookForCheck<SOUTH_EAST>(blockSquares, occupied, kingSquare);
    lookForCheck<SOUTH_WEST>(blockSquares, occupied, kingSquare);

    return blockSquares;
}


//if there is a pinned piece in the given direction, from the perspective of the king, all legal moves for this pinned piece will be generated
template<char direction>
void Position::checkForPins(uint64_t& pinnedPieces, uint64_t targetSquares, Move *moveBuffer, int& numOfMoves) {

    char kingSquare = __builtin_ctzll(pieces[KING] & ownPieces);
    
    uint64_t firstBlocker = getFirstBlockerInDirection<direction>(kingSquare, occupied);
    
    bool diagonalDirection = (direction == NORTH_EAST || direction == SOUTH_EAST || direction == SOUTH_WEST || direction == NORTH_WEST);

    if(firstBlocker & ownPieces) {

        uint64_t secondBlocker = getFirstBlockerInDirection<direction>(kingSquare, occupied & ~firstBlocker);

        uint64_t possiblePinner;
        if(diagonalDirection)
            possiblePinner = pieces[BISHOP] | pieces[QUEEN];
        else {
            possiblePinner = pieces[ROOK] | pieces[QUEEN];
        }

        if(secondBlocker & ~ownPieces & possiblePinner) {
            //there is a piece pinned in the direction specified in the template parameter
            //we will now check if there are square the pinned piece can still move to.

            pinnedPieces |= firstBlocker;

            if(diagonalDirection) {

                if(firstBlocker & (pieces[QUEEN] | pieces[BISHOP])) {
                    //bishops and queens are still able to move in a diagonal direction if pinned from that direction
                    uint64_t moveTargets = getSquaresUntilBlocker<direction>(kingSquare, secondBlocker) & ~firstBlocker & targetSquares;
                    generateMoves(__builtin_ctzll(firstBlocker), moveTargets, numOfMoves, moveBuffer);

                } else if(firstBlocker & pieces[PAWN]) {
                    //the only move a pawn that is pinned along a diagonal can possibly make is capturing the pinning Piece.
                    
                    if( ((direction == NORTH_EAST || direction == NORTH_WEST) && this->whitesTurn) || //a white pawn can only capture north west or norht east
                        ((direction == SOUTH_EAST || direction == SOUTH_WEST) && !this->whitesTurn)) { // a black pawn can only capture south west or south east

                        uint64_t pawnTarget = shift<direction>(firstBlocker) & secondBlocker & targetSquares;
                        if(pawnTarget) {
                            generatePawnMove(__builtin_ctzll(firstBlocker), __builtin_ctzll(secondBlocker), numOfMoves, moveBuffer);
                        }
                    }    
                }
            } else {
                if(firstBlocker & (pieces[ROOK] | pieces[QUEEN])) {
                    //the pinned piece can move between the king and the pinning piece, but staying on the same square is not a valid move
                    uint64_t moveTargets = getSquaresUntilBlocker<direction>(kingSquare, secondBlocker) & ~firstBlocker & targetSquares;
                    generateMoves(__builtin_ctzll(firstBlocker), moveTargets, numOfMoves, moveBuffer);
                } else if((firstBlocker & pieces[PAWN]) && (direction == NORTH || direction == SOUTH)) {
                    
                    uint64_t moveTargets;
                    if(this->whitesTurn) {
                        moveTargets = shift<NORTH>(firstBlocker) & ~occupied;
                        moveTargets |= shift<NORTH>(moveTargets) & ~occupied & (((uint64_t) 0xff) << 32);
                    } else {
                        moveTargets = shift<SOUTH>(firstBlocker) & ~occupied;
                        moveTargets |= shift<SOUTH>(moveTargets) & ~occupied & (((uint64_t) 0xff) << 24);
                    }
                    moveTargets &= targetSquares;

                    generateMoves(__builtin_ctzll(firstBlocker), moveTargets, numOfMoves, moveBuffer);
                }
            }
        }
    }
}


int Position::getLegalMoves(bool& kingInCheck, Move *moveBuffer) {
    int numOfMoves = 0;

    char kingSquare = __builtin_ctzll(pieces[KING] & ownPieces);

    //target squares are squares that block checks if the king is in check and squares that are not occupied by our own pieces
    uint64_t targetSquares = getCheckBlockingSquares();
    kingInCheck = (targetSquares != ~((uint64_t) 0));
    targetSquares &= ~ownPieces;

    uint64_t pinnedPieces = 0;
   
    //check for pinned pieces in all directions
    checkForPins<NORTH>     (pinnedPieces, targetSquares, moveBuffer, numOfMoves);
    checkForPins<NORTH_EAST>(pinnedPieces, targetSquares, moveBuffer, numOfMoves);
    checkForPins<EAST>      (pinnedPieces, targetSquares, moveBuffer, numOfMoves);
    checkForPins<SOUTH_EAST>(pinnedPieces, targetSquares, moveBuffer, numOfMoves);
    checkForPins<SOUTH>     (pinnedPieces, targetSquares, moveBuffer, numOfMoves);
    checkForPins<SOUTH_WEST>(pinnedPieces, targetSquares, moveBuffer, numOfMoves);
    checkForPins<WEST>      (pinnedPieces, targetSquares, moveBuffer, numOfMoves);
    checkForPins<NORTH_WEST>(pinnedPieces, targetSquares, moveBuffer, numOfMoves);

    //knight moves
    foreach(pieces[KNIGHT] & ownPieces & ~pinnedPieces, [&] (char square) {
        uint64_t moveTargets = targetSquares & getKnightAttacks(square);

        generateMoves(square, moveTargets, numOfMoves, moveBuffer);
    });

    //pawn moves
    uint64_t unpinnedPawns = this->pieces[PAWN] & ownPieces & ~pinnedPieces;

    if(this->whitesTurn) {
        //get a bit map of all target squares of pawn capture move to the north east
        uint64_t captureSquares = shift<NORTH_EAST>(unpinnedPawns) & occupied & targetSquares;

        generatePawnMoves(captureSquares, +7, numOfMoves, moveBuffer);

        //get a bit map of all squares on which a pawn can capture when moving north west
        captureSquares = shift<NORTH_WEST>(unpinnedPawns) & occupied & targetSquares;

        generatePawnMoves(captureSquares, +9, numOfMoves, moveBuffer);

        //a bitmap of all squares reachable by one square non capture pawn moves
        uint64_t nonCaptureSquares = shift<NORTH>(unpinnedPawns) & ~occupied & targetSquares;

        generatePawnMoves(nonCaptureSquares, +8, numOfMoves, moveBuffer);

        //a bitmap of all squares reachable by two square moves
        uint64_t twoSquareMoves = shift<NORTH>(shift<NORTH>(unpinnedPawns) & ~occupied) & ~occupied & targetSquares & (((uint64_t) 0xff) << 32);

        generateNonPromotionPawnMoves(twoSquareMoves, + 16, numOfMoves, moveBuffer);

    } else {
        uint64_t captureSquares = shift<SOUTH_EAST>(unpinnedPawns) & occupied & targetSquares;

        generatePawnMoves(captureSquares, -9, numOfMoves, moveBuffer);


        captureSquares = shift<SOUTH_WEST>(unpinnedPawns) & occupied & targetSquares;

        generatePawnMoves(captureSquares, -7, numOfMoves, moveBuffer);


        uint64_t nonCaptureSquares = shift<SOUTH>(unpinnedPawns) & ~occupied & targetSquares;

        generatePawnMoves(nonCaptureSquares, -8, numOfMoves, moveBuffer);


        uint64_t twoSquareMoves = shift<SOUTH>(shift<SOUTH>(unpinnedPawns) & ~occupied) & ~occupied & targetSquares & (((uint64_t) 0xff) << 24);

        generateNonPromotionPawnMoves(twoSquareMoves, - 16, numOfMoves, moveBuffer);
    }

    //diagonal moves
    foreach((pieces[BISHOP] | pieces[QUEEN]) & ownPieces & ~pinnedPieces, [&] (char square) {
        uint64_t moveTargets = getSlidingPieceAttacks<BISHOP>(square, occupied) & targetSquares;
        
        generateMoves(square, moveTargets, numOfMoves, moveBuffer);
    });

    //straight moves
    foreach((pieces[ROOK] | pieces[QUEEN]) & ownPieces & ~pinnedPieces, [&] (char square) {
        uint64_t moveTargets = getSlidingPieceAttacks<ROOK>(square, occupied) & targetSquares;
        generateMoves(square, moveTargets, numOfMoves, moveBuffer);
    });

    //king moves
    foreach(getKingAttacks(kingSquare) & ~ownPieces, [&] (char square) { 
        if(!wouldKingBeInCheck(square)) {
            moveBuffer[numOfMoves] = Move(kingSquare, square);
            numOfMoves++;
        }
    });

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
                capturingPawn = shift<WEST>(getBitboard(capturedPawnSquare)) & ownPieces & pieces[PAWN]; 
            } else {
                capturingPawn = shift<EAST>(getBitboard(capturedPawnSquare)) & ownPieces & pieces[PAWN];
            }

            if(capturingPawn) {
                //save the actual values for later
                uint64_t tmpPawns = pieces[PAWN];
                uint64_t tmpOwnPieces = ownPieces;
                uint64_t tmpOccupied = occupied;

                //temporarily perform the en passant capture
                ownPieces &= ~capturingPawn;
                pieces[PAWN] &= ~capturingPawn;
                ownPieces |= getBitboard(targetSquare);

                pieces[PAWN] |= getBitboard(targetSquare);
                pieces[PAWN] &= ~getBitboard(capturedPawnSquare);
                occupied |= getBitboard(targetSquare);
                occupied &= ~getBitboard(capturedPawnSquare);
                occupied &= ~capturingPawn;

                //if the king is not in check after the en passant capture, add the move to the move list.
                if(!wouldKingBeInCheck(kingSquare)) {
                    moveBuffer[numOfMoves] = Move(__builtin_ctzll(capturingPawn), targetSquare, 1, 0);
                    numOfMoves ++;
                }
                //restore the modified values
                ownPieces = tmpOwnPieces;
                occupied = tmpOccupied;
                pieces[PAWN] = tmpPawns;
            }
        }
    }

    //castling
    if(!kingInCheck) {
        //this array contains the squares that the king will cross during castling, which therefore must not be attacked by enemy pieces
        const char squaresNotToBeChecked[4][2] = {
            {61, 62}, //short castling white
            {58, 59}, //long castling white
            {5, 6}, //short castling black
            {2, 3} //long castling black
        };
        //this holds a bitboard for every castling type containing the squares that must no be occupied when castling
        const uint64_t squaresNotToBeOccupied[4] = {
            ((uint64_t) 3) << 61,
            ((uint64_t) 7) << 57,
            ((uint64_t) 3) << 5,
            ((uint64_t) 7) << 1
            
        };

        char fromSquares[4] = {60, 60, 4, 4};
        char toSquares[4] = {62, 58, 6, 2};

        //go through the four possible castling moves
        #pragma gcc unroll 4
        for(int i = WHITE_CASTLE_KINGSIDE; i <= BLACK_CASTLE_QUEENSIDE; i++) {
            if((this->whitesTurn && (i == WHITE_CASTLE_KINGSIDE || i == WHITE_CASTLE_QUEENSIDE)) || (!this->whitesTurn && (i == BLACK_CASTLE_KINGSIDE || i == BLACK_CASTLE_QUEENSIDE))) { //only two castling moves are available to each player
                
                if(history.front().castlingRights & (1 << i)) { //check if the player has the corresponding castling right
                    if(     (occupied & squaresNotToBeOccupied[i]) == 0  //no pieces between rook and king
                            && !wouldKingBeInCheck(squaresNotToBeChecked[i][0]) //check if the king moves through or ends up at an attacked square
                            && !wouldKingBeInCheck(squaresNotToBeChecked[i][1])) { 


                        moveBuffer[numOfMoves] = Move(fromSquares[i], toSquares[i], 2, i);
                        numOfMoves++;
                    }
                }
            }
        }
    }
    
    return numOfMoves;
}


void Position::printInternalRepresentation() {
    std::cout << "pawns:" << std::endl;
    printBitBoard(this->pieces[PAWN]);
    std::cout << "knights:" << std::endl;
    printBitBoard(this->pieces[KNIGHT]);
    std::cout << "bishops:" << std::endl;
    printBitBoard(this->pieces[BISHOP]);
    std::cout << "rooks:" << std::endl;
    printBitBoard(this->pieces[ROOK]);
    std::cout << "queens:" << std::endl;
    printBitBoard(this->pieces[QUEEN]);
    std::cout << "kings:" << std::endl;
    printBitBoard(this->pieces[KING]);
    std::cout << "ownPieces:" << std::endl;
    printBitBoard(this->ownPieces);

    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            std::cout << (char) (squares[i*8+j] + 0x31) << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "castlingRights: " << std::endl;
    if(this->history.front().castlingRights & 1)
        std::cout << "white long" << std::endl;
    if(this->history.front().castlingRights & 2)
        std::cout << "white short" << std::endl;
    if(this->history.front().castlingRights & 4)
        std::cout << "black long" << std::endl;
    if(this->history.front().castlingRights & 8)
        std::cout << "black short" << std::endl;
    
    std::cout << "file of en passant pawn: " << (this->history.front().enpassantFile == -1 ? "none" : std::string(1, (char)(history.front().enpassantFile + 97))) << std::endl;

    std::cout << "next to move: " << (this->whitesTurn ? "white" : "black") << std::endl;
    std::cout << "half moves since capture or pawn advance: " << (int) this->history.front().halfMoveClock << std::endl;
    std::cout << "currently in full move: " << this->fullMoveClock << std::endl;
    
}


uint64_t Position::calculateHash() {
    uint64_t hash = 0;

    uint64_t piecesByColor[2];
    piecesByColor[!(whitesTurn)] = ownPieces;
    piecesByColor[whitesTurn] = occupied & ~ownPieces;

    //iterate through the 6 piece types
    #pragma unroll 6
    for(int i = PAWN; i <= KING; i++) {
        //first add own pieces, then opponent pieces
        #pragma unroll 2
        for(int j = WHITE; j <= BLACK; j++) {

            foreach(pieces[i] & piecesByColor[j], [&] (char square) {
                hash ^= zobristPieces[6*64*j+64*i+square];
            });
            
        }   
    }
    hash ^= zobristCastlingRights[history.front().castlingRights];
    if(history.front().enpassantFile != -1)
        hash ^= zobristEnPassat[history.front().enpassantFile];

    hash ^= zobristPlayerToMove[whitesTurn];
    
    return hash;
}

uint64_t Position::getPositionHash() {
    return history.front().hash;
}

char Position::getPieceOnSquare(char square) {
    return squares[square];
}