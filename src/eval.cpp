#include "game.h"
#include "eval.h"
#include "bitboard.h"


uint64_t Eval::occupiedSquares;
uint64_t Eval::piecesByColor[2];
uint64_t Eval::kingRing[2];
short Eval::kingDanger[2];
uint64_t Eval::attackedByPawn[2];
uint64_t Eval::potentialOutpostSquares[2];

using namespace Bitboard;


typedef ScorePair P;


Eval::Params defaultParameters = {

    //material values
    {0, 100, 315, 325, 500, 975},

    //bishop pair
    P(10, 10),

    //piece square tables
    {
        //pawn
        {
            P(  0,  0), P(  0,  0), P(  0,  0), P(  0,  0), P(  0,  0), P(  0,  0), P(  0,  0), P(  0,  0), 
            P( 23, 28), P( 59, 23), P(-14,  8), P( 20,-16), P( -7, -3), P( 51,-18), P(-41, 15), P(-86, 37),
            P(-16, 54), P( -3, 60), P( 16, 45), P( 21, 27), P( 55, 16), P( 46, 13), P( 15, 42), P(-30, 44), 
            P(-14, 12), P( 13, 14), P(  6,  3), P( 21, -5), P( 23,-12), P( 12, -6), P( 17,  7), P(-23,  7), 
            P(-27, 13), P( -2,  9), P( -5, -3), P( 12, -7), P( 17, -7), P(  6, -8), P( 10,  3), P(-25, -1), 
            P(-26,  4), P( -4,  7), P( -4, -6), P(-10,  1), P(  3,  0), P(  3, -5), P( 33, -1), P(-12, -8), 
            P(-35, 13), P( -1,  8), P(-20,  8), P(-23, 10), P(-15, 13), P( 24,  0), P( 38,  2), P(-22, -7), 
            P(  0,  0), P(  0,  0), P(  0,  0), P(  0,  0), P(  0,  0), P(  0,  0), P(  0,  0), P(  0,  0), 
        }, 
        //knight
        {
            P(-167,-58), P(-89,-38), P(-34,-13), P(-49,-28), P( 61,-31), P(-97,-27), P(-15,-63), P(-107,-99), 
            P(-73,-25), P(-41, -8), P( 72,-25), P( 36, -2), P( 23, -9), P( 62,-25), P(  7,-24), P(-17,-52), 
            P(-47,-24), P( 60,-20), P( 37, 10), P( 65,  9), P( 84, -1), P(129, -9), P( 73,-19), P( 44,-41), 
            P( -9,-17), P( 17,  3), P( 19, 22), P( 53, 22), P( 37, 22), P( 69, 11), P( 18,  8), P( 22,-18), 
            P(-13,-18), P(  4, -6), P( 16, 16), P( 13, 25), P( 28, 16), P( 19, 17), P( 21,  4), P( -8,-18), 
            P(-23,-23), P( -9, -3), P( 12, -1), P( 10, 15), P( 19, 10), P( 17, -3), P( 25,-20), P(-16,-22), 
            P(-29,-42), P(-53,-20), P(-12,-10), P( -3, -5), P( -1, -2), P( 18,-20), P(-14,-23), P(-19,-44), 
            P(-105,-29), P(-21,-51), P(-58,-23), P(-33,-15), P(-17,-22), P(-28,-18), P(-19,-50), P(-23,-64), 
        },
        //bishop
        {
            P(-29,-14), P(  4,-21), P(-82,-11), P(-37, -8), P(-25, -7), P(-42, -9), P(  7,-17), P( -8,-24), 
            P(-26, -8), P( 16, -4), P(-18,  7), P(-13,-12), P( 30, -3), P( 59,-13), P( 18, -4), P(-47,-14), 
            P(-16,  2), P( 37, -8), P( 43,  0), P( 40, -1), P( 35, -2), P( 50,  6), P( 37,  0), P( -2,  4), 
            P( -4, -3), P(  5,  9), P( 19, 12), P( 50,  9), P( 37, 14), P( 37, 10), P(  7,  3), P( -2,  2), 
            P( -6, -6), P( 13,  3), P( 13, 13), P( 26, 19), P( 34,  7), P( 12, 10), P( 10, -3), P(  4, -9), 
            P(  0,-12), P( 15, -3), P( 15,  8), P( 15, 10), P( 14, 13), P( 27,  3), P( 18, -7), P( 10,-15), 
            P(  4,-14), P( 15,-18), P( 16, -7), P(  0, -1), P(  7,  4), P( 21, -9), P( 33,-15), P(  1,-27), 
            P(-33,-23), P( -3, -9), P(-14,-23), P(-21, -5), P(-13, -9), P(-12,-16), P(-39, -5), P(-21,-17), 
        },
        //rook
        {
            P( 32, 13), P( 42, 10), P( 32, 18), P( 51, 15), P( 63, 12), P(  9, 12), P( 31,  8), P( 43,  5), 
            P( 27, 11), P( 32, 13), P( 58, 13), P( 62, 11), P( 80, -3), P( 67,  3), P( 26,  8), P( 44,  3), 
            P( -5,  7), P( 19,  7), P( 26,  7), P( 36,  5), P( 17,  4), P( 45, -3), P( 61, -5), P( 16, -3), 
            P(-24,  4), P(-11,  3), P(  7, 13), P( 26,  1), P( 24,  2), P( 35,  1), P( -8, -1), P(-20,  2), 
            P(-36,  3), P(-26,  5), P(-12,  8), P( -1,  4), P(  9, -5), P( -7, -6), P(  6, -8), P(-23,-11), 
            P(-45, -4), P(-25,  0), P(-16, -5), P(-17, -1), P(  3, -7), P(  0,-12), P( -5, -8), P(-33,-16), 
            P(-44, -6), P(-16, -6), P(-20,  0), P( -9,  2), P( -1, -9), P( 11, -9), P( -6,-11), P(-71, -3), 
            P(-19, -9), P(-13,  2), P(  1,  3), P( 17, -1), P( 16, -5), P(  7,-13), P(-37,  4), P(-26,-20), 
        },
        //queen
        {
            P(-28, -9), P(  0, 22), P( 29, 22), P( 12, 27), P( 59, 27), P( 44, 19), P( 43, 10), P( 45, 20), 
            P(-24,-17), P(-39, 20), P( -5, 32), P(  1, 41), P(-16, 58), P( 57, 25), P( 28, 30), P( 54,  0), 
            P(-13,-20), P(-17,  6), P(  7,  9), P(  8, 49), P( 29, 47), P( 56, 35), P( 47, 19), P( 57,  9), 
            P(-27,  3), P(-27, 22), P(-16, 24), P(-16, 45), P( -1, 57), P( 17, 40), P( -2, 57), P(  1, 36), 
            P( -9,-18), P(-26, 28), P( -9, 19), P(-10, 47), P( -2, 31), P( -4, 34), P(  3, 39), P( -3, 23), 
            P(-14,-16), P(  2,-27), P(-11, 15), P( -2,  6), P( -5,  9), P(  2, 17), P( 14, 10), P(  5,  5), 
            P(-35,-22), P( -8,-23), P( 11,-30), P(  2,-16), P(  8,-16), P( 15,-23), P( -3,-36), P(  1,-32), 
            P( -1,-33), P(-18,-28), P( -9,-22), P( 10,-43), P(-15, -5), P(-25,-32), P(-31,-20), P(-50,-41), 
        },
        //king
        {
            P(-65,-74), P( 23,-35), P( 16,-18), P(-15,-18), P(-56,-11), P(-34, 15), P(  2,  4), P( 13,-17), 
            P( 29,-12), P( -1, 17), P(-20, 14), P( -7, 17), P( -8, 17), P( -4, 38), P(-38, 23), P(-29, 11), 
            P( -9, 10), P( 24, 17), P(  2, 23), P(-16, 15), P(-20, 20), P(  6, 45), P( 22, 44), P(-22, 13), 
            P(-17, -8), P(-20, 22), P(-12, 24), P(-27, 27), P(-30, 26), P(-25, 33), P(-14, 26), P(-36,  3), 
            P(-49,-18), P( -1, -4), P(-27, 21), P(-39, 24), P(-46, 27), P(-44, 23), P(-33,  9), P(-51,-11), 
            P(-14,-19), P(-14, -3), P(-22, 11), P(-46, 21), P(-44, 23), P(-30, 16), P(-15,  7), P(-27, -9), 
            P(  1,-27), P(  7,-11), P( -8,  4), P(-64, 13), P(-43, 14), P(-16,  4), P(  9, -5), P(  8,-17), 
            P(-15,-53), P( 36,-34), P( 12,-21), P(-54,-11), P(  8,-28), P(-28,-14), P( 24,-24), P( 14,-43), 
        },
    },
    
    //stack pawns
    P(-10, -15),

    //isolated pawn
    P(-10, -15),

    //passed pawn
    {P(10, 15), P(15, 20), P(20, 25), P(25, 35), P(50, 60), P(75 ,150)},

    //blocked pawn on bishop color
    P(-2, -6),

    //unblocked pawn on bishop color
    P(-1, -3),

    //king ring attackers
    {6, 5, 5, 8, 12},

    //king ring defenders
    {1, 3, 3, 5, 5},

    //king attack rays
    {
        //horizontal
        { 0, 0, 1, 1, 2, 2, 2},
        //diagonal
        {0, 3, 5, 7, 8, 9, 10},
        //vertical
        {0, 3, 4, 7, 8, 9, 10}
    },

    //end game scale down
    0,

    //mobility
    {
        {//knight
            P(-30,-30), P( -7, -7), P(  0,  0), P(  3,  3), P(  6,  6), P( 10, 10), P( 13, 13), P( 15, 15), P( 16, 16)
        },
        {//bishop
            P(-30,-30), P(-10,-10), P( -5, -5), P(  0,  0), P(  2,  2), P(  5,  5), P(  8,  8), P( 11, 11), P( 13, 13), P( 14, 14), 
            P( 15, 15), P( 16, 16), P( 16, 16), P( 16, 16)
        }, 
        {//rook
            P(-30,-30), P(-10,-10), P( -5, -5), P(  0,  0), P(  2,  2), P(  5,  5), P(  8,  8), P( 11, 11), P( 13, 13), P( 14, 14), 
            P( 15, 15), P( 16, 16), P( 16, 16), P( 16, 16), P( 16, 16)
        }, 
        {//queen
            P(-50,-50), P(-10,-10), P( -4, -4), P(  0,  0), P(  1,  1), P(  2,  2), P(  3,  3), P(  4,  4), P(  4,  4), P(  5,  5), 
            P(  5,  5), P(  6,  6), P(  6,  6), P(  6,  6), P(  7,  7), P(  7,  7), P(  7,  7), P(  7,  7), P(  8,  8), P(  8,  8), 
            P(  8,  8), P(  8,  8), P(  9,  9), P(  9,  9), P(  9,  9), P(  9,  9), P(  9,  9), P(  9,  9)
        }
    },

    //bishop outpost
    P(5, 5),

    //knight outpost
    P(7, 7),

    //rook on open file
    P(15, 7),

    //rook on half open file
    P(10, 7)
};


Eval::Params *Eval::params = &defaultParameters;


template<char color>
ScorePair Eval::evaluateMaterial(Game::Position*pos) {
    ScorePair score = __builtin_popcountll(pos->pawns & piecesByColor[color]) * params->pieceValues[PAWN]
                    + __builtin_popcountll(pos->knights & piecesByColor[color]) * params->pieceValues[KNIGHT]
                    + __builtin_popcountll(~pos->filesAndRanks & pos->diagonals & piecesByColor[color]) * params->pieceValues[BISHOP]
                    + __builtin_popcountll(pos->filesAndRanks & ~pos->diagonals & piecesByColor[color]) * params->pieceValues[ROOK]
                    + __builtin_popcountll(pos->filesAndRanks & pos->diagonals & piecesByColor[color]) * params->pieceValues[QUEEN];

    if (__builtin_popcountll(~pos->filesAndRanks & pos->diagonals & piecesByColor[color]) >= 2) {
        score += params->bishopPair;
    }
    return score;
}

template<char color>
ScorePair Eval::evaluatePawns(Game::Position* pos) {

    ScorePair score;

    uint64_t ourPawns = piecesByColor[color] & pos->pawns;

    if(color == WHITE) {
        attackedByPawn[WHITE] = shift<NORTH_EAST>(ourPawns) | shift<NORTH_WEST>(ourPawns);
    } else {
        attackedByPawn[BLACK] = shift<SOUTH_EAST>(ourPawns) | shift<SOUTH_WEST>(ourPawns);
    }

    uint64_t squaresInFrontOfPawns;
    if(color == WHITE) {
        squaresInFrontOfPawns = ourPawns >> 8;
        squaresInFrontOfPawns |= squaresInFrontOfPawns >> 8;
        squaresInFrontOfPawns |= squaresInFrontOfPawns >> 16;
        squaresInFrontOfPawns |= squaresInFrontOfPawns >> 32;
    } else {
        squaresInFrontOfPawns = ourPawns << 8;
        squaresInFrontOfPawns |= squaresInFrontOfPawns << 8;
        squaresInFrontOfPawns |= squaresInFrontOfPawns << 16;
        squaresInFrontOfPawns |= squaresInFrontOfPawns << 32;
    }

    potentialOutpostSquares[!color] = ~(shift<WEST>(squaresInFrontOfPawns) | shift<EAST>(squaresInFrontOfPawns));

    //stacked pawns
    uint64_t stackedPawns = squaresInFrontOfPawns & ourPawns;
    score += params->stackedPawns * P(__builtin_popcountll(stackedPawns));

    //isolated pawns
    uint64_t pawnFiles;
    if(color == WHITE) {
        pawnFiles = squaresInFrontOfPawns & 0xff;
    } else {
        pawnFiles = squaresInFrontOfPawns >> 56;
    }

    uint64_t isolatedPawnFiles = pawnFiles & ~(pawnFiles << 1) & ~(pawnFiles >> 1);

    uint64_t stackedPawnFiles = stackedPawns;
    stackedPawnFiles |= stackedPawnFiles >> 8;
    stackedPawnFiles |= stackedPawnFiles >> 16;
    stackedPawnFiles |= stackedPawnFiles >> 32;

    isolatedPawnFiles &= ~stackedPawnFiles;

    score += params->isolatedPawns * P(__builtin_popcountll(isolatedPawnFiles));

    //enemy passed pawns
    //to calculate our passed pawns, we need to know the squares in front of enemy pawns. Since we already calculated the
    //squares in front of our pawns, we just calculate enemy passed pawns and substract their evaluation from the score.

    uint64_t stoppable = squaresInFrontOfPawns | shift<WEST>(squaresInFrontOfPawns) | shift<EAST>(squaresInFrontOfPawns);
    uint64_t enemyPassedPawns = piecesByColor[!color] & pos->pawns & ~stoppable;

    foreach(enemyPassedPawns, [&](char square) {
        char rank = square / 8;
        if(!color == WHITE) {
            rank = 7 - rank;
        }
        score -= params->passedPawns[rank-1];
    });

    //pawns on square with same color as bishop

    uint64_t bishops = piecesByColor[color] & pos->diagonals & ~pos->filesAndRanks;
    
    uint64_t bishopColorSquares = 0;
    if(bishops & lightSquares)
        bishopColorSquares = lightSquares;
    if(bishops & darkSquares)
        bishopColorSquares |= darkSquares;

    uint64_t blockedPawns;
    if(color == WHITE) {
        blockedPawns = shift<SOUTH>(piecesByColor[!color] & pos->pawns) & ourPawns;
    } else {
        blockedPawns = shift<NORTH>(piecesByColor[!color] & pos->pawns) & ourPawns;
    }

    score += params->blockedPawnOnBishopColor * P(__builtin_popcountll(blockedPawns & bishopColorSquares));
    score += params->unblockedPawnOnBishopColor * P(__builtin_popcountll(ourPawns & ~blockedPawns & bishopColorSquares));

    //king ring attack and defense
    uint64_t kingRingAttackSquares[2];
    for(int i = WHITE; i <= BLACK; i++) {
        if(color == WHITE) {
            kingRingAttackSquares[i] = shift<SOUTH_WEST>(kingRing[i]) | shift<SOUTH_EAST>(kingRing[i]);
        } else {
            kingRingAttackSquares[i] = shift<NORTH_WEST>(kingRing[i]) | shift<NORTH_EAST>(kingRing[i]);
        }
    }

    kingDanger[color] -= params->kingRingDefender[PAWN] * __builtin_popcountll(ourPawns & kingRingAttackSquares[color]);
    kingDanger[!color] += params->kingRingAttacker[PAWN] * __builtin_popcountll(ourPawns & kingRingAttackSquares[!color]);

    return score;
}


template<char pieceType, char color>
ScorePair Eval::evaluatePiece(Game::Position* pos, char square) {

    char flippedSquare = square;
    if(color == BLACK) {
        flippedSquare ^= 56;
    }

    ScorePair score = params->pieceSquare[pieceType - 1][flippedSquare];

    if(pieceType == KING) {
        //initialize fields for king danger calculations
        kingRing[color] = getKingMoveSquares(__builtin_ctzll(piecesByColor[color] & pos->kings)) | getBitboard(square);
        kingDanger[color] = 0;

        kingDanger[color] += params->kingAttackRays[0][__builtin_popcountll(getBlockedRay<WEST, false>(square, piecesByColor[!color]))];
        kingDanger[color] += params->kingAttackRays[0][__builtin_popcountll(getBlockedRay<EAST, false>(square, piecesByColor[!color]))];
        kingDanger[color] += params->kingAttackRays[1][__builtin_popcountll(getBlockedRay<NORTH, false>(square, piecesByColor[!color]))];
        kingDanger[color] += params->kingAttackRays[1][__builtin_popcountll(getBlockedRay<SOUTH, false>(square, piecesByColor[!color]))];
        kingDanger[color] += params->kingAttackRays[2][__builtin_popcountll(getBlockedRay<NORTH_WEST, false>(square, piecesByColor[!color]))];
        kingDanger[color] += params->kingAttackRays[2][__builtin_popcountll(getBlockedRay<NORTH_EAST, false>(square, piecesByColor[!color]))];
        kingDanger[color] += params->kingAttackRays[2][__builtin_popcountll(getBlockedRay<SOUTH_WEST, false>(square, piecesByColor[!color]))];
        kingDanger[color] += params->kingAttackRays[2][__builtin_popcountll(getBlockedRay<SOUTH_EAST, false>(square, piecesByColor[!color]))];

    } else {

        if(pieceType != PAWN) {

            uint64_t attacks;
            if(pieceType == KNIGHT) {
                attacks = getKnightMoveSquares(square);
            } else if(pieceType == BISHOP) {
                attacks = pos->getBishopAttacks(square, occupiedSquares);
            } else if(pieceType == ROOK) {
                attacks = pos->getRookAttacks(square, occupiedSquares);
            } else if(pieceType == QUEEN) {
                attacks = pos->getRookAttacks(square, occupiedSquares) | pos->getBishopAttacks(square, occupiedSquares);
            }

            //is the piece defending our king?
            if(attacks & kingRing[color]) {
                kingDanger[color] -= params->kingRingDefender[pieceType-1];
            }

            //is the piece attacking the enemy king?
            if(attacks & kingRing[!color]) {
                kingDanger[!color] += params->kingRingAttacker[pieceType-1];
            }

            //mobility
            uint64_t moves = attacks & ~piecesByColor[color]; //pseudo legal moves

            //remove squares controlled by enemy pawns
            moves &= ~attackedByPawn[!color];

            score += params->mobility[pieceType-2][__builtin_popcountll(moves)];

            //outposts
            if(pieceType == BISHOP || pieceType == KNIGHT) {
                if(getBitboard(square) & attackedByPawn[color] & potentialOutpostSquares[color]) {
                    score += (pieceType == KNIGHT) ? params->knightOutpost : params->bishopOutpost;
                }
            }

            //open and half open file
            if(pieceType == ROOK) {
                uint64_t file = ((uint64_t) 0x0101010101010101) << (square % 8);
                if((pos->pawns & file) == 0) {
                    score += params->rookOnOpenFile;
                } else if((pos->pawns & piecesByColor[color] & file) == 0) {
                    score += params->rookOnHalfOpenFile;
                }
            }
        }
    }

    return score;
}



short Eval::evaluate(Game::Position* pos) {

    int gamePhase = 1 * __builtin_popcountll(pos->knights | (pos->diagonals & ~pos->filesAndRanks))
                  + 2 * __builtin_popcountll(pos->filesAndRanks & ~pos->diagonals)
                  + 4 * __builtin_popcountll(pos->filesAndRanks & pos->diagonals);
    
    occupiedSquares = pos->pawns | pos->knights | pos->diagonals | pos->filesAndRanks | pos->kings;

    piecesByColor[!(pos->whitesTurn)] = pos->ownPieces;
    piecesByColor[pos->whitesTurn] = occupiedSquares & ~pos->ownPieces;

    ScorePair score;

    //material
    score += evaluateMaterial<WHITE>(pos);
    score -= evaluateMaterial<BLACK>(pos);

    //kings
    score += evaluatePiece<KING, WHITE>(pos, __builtin_ctzll(piecesByColor[WHITE] & pos->kings));
    score -= evaluatePiece<KING, BLACK>(pos, __builtin_ctzll(piecesByColor[BLACK] & pos->kings));


    //pawn structure
    score += evaluatePawns<WHITE>(pos);
    score -= evaluatePawns<BLACK>(pos);


    //pawns (only for piece-square values)
    foreach(pos->pawns & piecesByColor[WHITE], [&](char square) {
        score += evaluatePiece<PAWN, WHITE>(pos, square);
    });

    foreach(pos->pawns & piecesByColor[BLACK], [&](char square) {
        score -= evaluatePiece<PAWN, BLACK>(pos, square);
    });

    //knights
    foreach(pos->knights & piecesByColor[WHITE], [&](char square) {
        score += evaluatePiece<KNIGHT, WHITE>(pos, square);
    });

    foreach(pos->knights & piecesByColor[BLACK], [&](char square) {
        score -= evaluatePiece<KNIGHT, BLACK>(pos, square);
    });

    //bishops
    foreach(pos->diagonals & ~pos->filesAndRanks & piecesByColor[WHITE], [&](char square) {
        score += evaluatePiece<BISHOP, WHITE>(pos, square);
    });

    foreach(pos->diagonals & ~pos->filesAndRanks & piecesByColor[BLACK], [&](char square) {
        score -= evaluatePiece<BISHOP, BLACK>(pos, square);
    });

    //rooks
    foreach(~pos->diagonals & pos->filesAndRanks & piecesByColor[WHITE], [&](char square) {
        score += evaluatePiece<ROOK, WHITE>(pos, square);
    });

    foreach(~pos->diagonals & pos->filesAndRanks & piecesByColor[BLACK], [&](char square) {
        score -= evaluatePiece<ROOK, BLACK>(pos, square);
    });

    //queens
    foreach(pos->diagonals & pos->filesAndRanks & piecesByColor[WHITE], [&](char square) {
        score += evaluatePiece<QUEEN, WHITE>(pos, square);
    });

    foreach(pos->diagonals & pos->filesAndRanks & piecesByColor[BLACK], [&](char square) {
        score -= evaluatePiece<QUEEN, BLACK>(pos, square);
    });
    
    //king danger
    ScorePair kingDangerScore[2];
    for(int i = 0; i < 2 ; i++) {
        kingDangerScore[i].mg = kingDanger[i];
        kingDangerScore[i].eg = (kingDanger[i] * params->endGameScaleDown) / 1024;
    }

    score += kingDangerScore[BLACK] - kingDangerScore[WHITE];

    short finalScore = ((gamePhase * score.mg) + ((24 - gamePhase) * score.eg)) / 24;

    if(!pos->whitesTurn) {
        finalScore = -finalScore;
    }

    return finalScore;
}