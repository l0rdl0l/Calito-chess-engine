#ifndef EVAL_H
#define EVAL_H

#include <iostream>

#include "position.h"

//evaluation
class ScorePair {
    public:
        short mg;
        short eg;

        ScorePair() {
            this->mg = 0;
            this->eg = 0;
        }

        ScorePair(int score) {
            this->mg = score;
            this->eg = score;
        }

        ScorePair(int mg, int eg) {
            this->mg = mg;
            this->eg = eg;
        }

        ScorePair operator+ (ScorePair obj) {
            return ScorePair(this->mg + obj.mg, this->eg + obj.eg);
        }
        ScorePair operator- (ScorePair obj) {
            return ScorePair(this->mg - obj.mg, this->eg - obj.eg);
        }
        ScorePair operator* (ScorePair obj) {
            return ScorePair(this->mg * obj.mg, this->eg * obj.eg);
        }
        ScorePair operator/ (ScorePair obj) {
            return ScorePair(this->mg / obj.mg, this->eg / obj.eg);
        }
        
        ScorePair& operator+= (ScorePair obj) {
            this->mg += obj.mg;
            this->eg += obj.eg;
            return *this;
        }
        ScorePair& operator-= (ScorePair obj) {
            this->mg -= obj.mg;
            this->eg -= obj.eg;
            return *this;
        }
        ScorePair& operator*= (ScorePair obj) {
            this->mg *= obj.mg;
            this->eg *= obj.eg;
            return *this;
        }
        ScorePair& operator/= (ScorePair obj) {
            this->mg /= obj.mg;
            this->eg /= obj.eg;
            return *this;
        }

        void print() {
            std::cout << this->mg << ", " << this->eg << std::endl;
        }
};

class Eval {
    public:
        struct Params {
            //material:
            short pieceValues[6]; //indexed by piece constants defined in constants.h (PAWN, KNIGHT, BISHOP, ROOK, QUEEN)

            ScorePair bishopPair;

            //piece-square combinations:
            ScorePair pieceSquare[6][64];
        
            //pawns:
            ScorePair stackedPawns;
            ScorePair isolatedPawns;
            ScorePair passedPawns[6]; //bonus for passed pawn on rank i. Starts with rank 1
            ScorePair blockedPawnOnBishopColor; //penalty for every pawn on the same color as our bishop, that is blocked by a pawn 
            ScorePair unblockedPawnOnBishopColor; //penalty for every pawn on the same coler as our bishop, that is not blocked by a pawn

            //king danger:
            short kingRingAttacker[5];
            short kingRingDefender[5];
            short kingAttackRays[3][7]; //potential lines of attack. First index: type of attack direction (horizontal, vertical, diagonal)
                                        //second index: distance from king to next friendly piece in the given direction
            short endGameScaleDown; //in the end game king safety is less important. endGameScor = (kingDanger * endGameScaleDown) /1024

            //mobility
            ScorePair mobility[4][28]; //pseudo legal moves. moves to squares controlled by enemy pawns don't count


            //other
            ScorePair bishopOutpost;
            ScorePair knightOutpost;
            ScorePair rookOnOpenFile;
            ScorePair rookOnHalfOpenFile;
        };

        static Params *params;

        static short evaluate(Position* pos);

    private:

        static uint64_t piecesByColor[2];
        static uint64_t kingRing[2];
        static short kingDanger[2];
        static uint64_t attackedByPawn[2];
        static uint64_t potentialOutpostSquares[2];

        template<char color>
        static ScorePair evaluateMaterial(Position*pos);

        template<char pieceType, char color>
        static ScorePair evaluatePiece(Position* pos, char square);

        template<char color>
        static ScorePair evaluatePawns(Position* pos);

};


#endif