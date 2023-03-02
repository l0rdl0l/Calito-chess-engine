#include "game.h"
#include "eval.h"
#include "bitboard.h"

using namespace Bitboard;


typedef ScorePair P;


Eval::Params defaultParameters = {//0: middle game, 1: end game

    //material values
    {0, 100, 315, 325, 500, 975},

    //piece square tables
    {
        //pawn
        {
            P(  0,  0), P(  0,  0), P(  0,  0), P(  0,  0), P(  0,  0), P(  0,  0), P(  0,  0), P(  0,  0), 
            P( 98,178), P(134,173), P( 61,158), P( 95,134), P( 68,147), P(126,132), P( 34,165), P(-11,187),
            P( -6, 94), P(  7,100), P( 26, 85), P( 31, 67), P( 65, 56), P( 56, 53), P( 25, 82), P(-20, 84), 
            P(-14, 32), P( 13, 24), P(  6, 13), P( 21,  5), P( 23, -2), P( 12,  4), P( 17, 17), P(-23, 17), 
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
    }
};


Eval::Params *Eval::params = &defaultParameters;



short Eval::evaluate(Game::Position* pos) {

    int gamePhase = 1 * __builtin_popcountll(pos->knights | (pos->diagonals & ~pos->filesAndRanks))
                  + 2 * __builtin_popcountll(pos->filesAndRanks & ~pos->diagonals)
                  + 4 * __builtin_popcountll(pos->filesAndRanks & pos->diagonals);

    uint64_t occupiedSquares = pos->pawns | pos->knights | pos->diagonals | pos->filesAndRanks | pos->kings;

    ScorePair score[2];
    score[0] = P(0);
    score[1] = P(0);

    uint64_t piecesBySide[2];
    piecesBySide[0] = pos->ownPieces;
    piecesBySide[1] = occupiedSquares & ~pos->ownPieces;

    #pragma gcc unroll 2
    for(int i = 0; i < 2; i++) {
        
        #pragma gcc unroll 5
        for(int j = PAWN; j < KING; j++) {
            uint64_t pieces;
            switch(j) {
                case PAWN:
                    pieces = pos->pawns;
                    break;
                case KNIGHT:
                    pieces = pos->knights;
                    break;
                case BISHOP:
                    pieces = pos->diagonals & ~pos->filesAndRanks;
                    break;
                case ROOK:
                    pieces = ~pos->diagonals & pos->filesAndRanks;
                    break;
                case QUEEN:
                    pieces = pos->diagonals & pos->filesAndRanks;
                    break;
                case KING:
                    pieces = pos->kings;
                    break;
            }

            if(i == 0) {
                pieces &= pos->ownPieces;
            } else {
                pieces &= ~pos->ownPieces;
            }

            score[i] += ScorePair(Eval::params->pieceValues[j]) * ScorePair(__builtin_popcountll(pieces));

            while(pieces) {
                char square = __builtin_ctzll(pieces);
                pieces &= ~getBitboard(square);

                if(pos->whitesTurn == i) {
                    square ^= 56;
                }

                score[i] += params->pieceSquare[j-1][square];
            }
        }
    }


    P totalScore = score[0] - score[1];

    return ((gamePhase * totalScore.mg) + ((24 - gamePhase) * totalScore.eg)) / 24;
}