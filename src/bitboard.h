#ifndef BITBOARD_H
#define BITBOARD_H


#include <cstdint>
#include <array>
#include <iostream>

#include "constants.h"

namespace Bitboard {

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


    //returns a bitboard with the only set bit at the given square
    inline uint64_t getBitboard(char square) {
        return (((uint64_t ) 1) << square);
    }

    template<char direction>
    uint64_t getRay(char square) {
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
    inline uint64_t shift(uint64_t square) {
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

    template <char direction>
    uint64_t getSquaresUntilBlocker(char square, uint64_t blockingSquare) {
        if(direction == NORTH || direction == NORTH_EAST || direction == WEST || direction == NORTH_WEST) {
            return ~(blockingSquare - 1) & getRay<direction>(square);
        } else if(direction == EAST) {
            return ((blockingSquare << 1) - 1) & getRay<EAST>(square);
        } else if (direction == SOUTH_EAST) {
            return ((blockingSquare << 9) - 1) & getRay<SOUTH_EAST>(square);
        } else if (direction == SOUTH) {
            return ((blockingSquare << 8) - 1) & getRay<SOUTH>(square);
        } else if (direction == SOUTH_WEST) {
            return ((blockingSquare << 7) - 1) & getRay<SOUTH_WEST>(square);
        }
    }


    template<char direction>
    uint64_t getFirstBlockerInDirection(char square, uint64_t occ) {
        uint64_t blocker = getRay<direction>(square) & occ;
        if(direction == NORTH || direction == NORTH_EAST || direction == WEST || direction == NORTH_WEST) { //negative directions
            return (((uint64_t) 0x8000000000000000) >> __builtin_clzll(blocker)) & blocker;
        } else { //positive directions
            return blocker & (~blocker + 1);
        }
    }

    template<char direction, bool includingBlocker>
    uint64_t getBlockedRay(char square, uint64_t occ) {

        uint64_t blockingSquare = getFirstBlockerInDirection<direction>(square, occ);

        if(direction == NORTH || direction == NORTH_EAST || direction == WEST || direction == NORTH_WEST) {
            
            uint64_t including = ~((blockingSquare - 1) & ((blockingSquare == 0) - 1)) & getRay<direction>(square);
            if(includingBlocker) {
                return including;
            } else {
                return including ^ blockingSquare;
            }
        } else {
            
            uint64_t excluding = (blockingSquare-1) & getRay<direction>(square);
            if(includingBlocker) {
                return excluding | blockingSquare;
            } else {
                return excluding;
            }
        }
    }

    void inline printBitBoard(uint64_t board) {
        for(int i = 0; i < 8; i++) {
            for(int j = 0; j < 8; j++) {
                std::cout << " " << ((board >> i*8+j) & ((uint64_t )1));
            }
            std::cout << std::endl;
        }
    }

    template <typename func>
    void inline foreach(uint64_t board, func f) {
        while(board) {
            char square = __builtin_ctzll(board);
            board &= ~getBitboard(square);

            f(square);
        }
    }

    uint64_t inline getPawnAttacks(char square, short color) {
        if(color == WHITE) {
            return shift<NORTH_WEST>(getBitboard(square)) | shift<NORTH_EAST>(getBitboard(square));
        } else {
            return shift<SOUTH_WEST>(getBitboard(square)) | shift<SOUTH_EAST>(getBitboard(square));
        }
    }

    uint64_t inline getKnightAttacks(char square) {
        return Bitboard::knightAttacks[square];
    }

    uint64_t inline getKingAttacks(char square) {
        return Bitboard::kingMovesLUT[square];
    }

    template<char pieceType>
    uint64_t inline getSlidingPieceAttacks(char square, uint64_t occupied) {
        if(pieceType == BISHOP) {
            uint64_t rayNorthWest = getBlockedRay<NORTH_WEST, true>(square, occupied);
            uint64_t rayNorthEast = getBlockedRay<NORTH_EAST, true>(square, occupied);
            uint64_t raySouthEast = getBlockedRay<SOUTH_EAST, true>(square, occupied);
            uint64_t raySouthWest = getBlockedRay<SOUTH_WEST, true>(square, occupied);

            return rayNorthEast | rayNorthWest | raySouthEast | raySouthWest;
        } else if(pieceType == ROOK) {
            uint64_t rayNorth = getBlockedRay<NORTH, true>(square, occupied);
            uint64_t rayEast = getBlockedRay<EAST, true>(square, occupied);
            uint64_t raySouth = getBlockedRay<SOUTH, true>(square, occupied);
            uint64_t rayWest = getBlockedRay<WEST, true>(square, occupied);

            return rayEast | rayNorth | raySouth | rayWest;
        } else {
            return getSlidingPieceAttacks<BISHOP>(square, occupied) | getSlidingPieceAttacks<ROOK>(square, occupied);
        }
    }

}
#endif