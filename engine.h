#ifndef ENGINE_H
#define ENGINE_H

#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>
#include <cctype>
#include <thread>
#include <condition_variable>


#include "game.h"

class Engine {
    public:

        struct Options {
            std::vector<Game::Move> searchMoves;
            int64_t winc = 0;
            int64_t binc = 0;
            int64_t wtime = -1;
            int64_t btime = -1;
            int64_t maxNodes = -1;
            int64_t moveTime = -1;
            int movesToGo = -1;
            int maxDepth = -1;
            bool ponder = false; 
            bool searchInfinitely = false;
        };

        static void startAnalyzing(Game& game, Options& options);

        static void ponderHit();

        static void stopCalculation();

    private:
        static const int pvLength = 10;

        static const uint64_t connectionLagBuffer = 50;

        static const short maxMateDistance = 5000;

        static std::atomic<bool> ponder;
        static std::atomic<bool> stop;

        static bool searchAborted;

        static uint64_t nodesSearched;
        static Game::Move pv[pvLength];

        static Engine::Options options;

        static Game game;

        static std::chrono::time_point<std::chrono::steady_clock> executionStartTime;

        static std::thread workerThread;

        static std::thread timeController;
        static std::mutex timeThreadMutex;
        static std::condition_variable timingAbortCondition;
        static bool timingAbortFlag;

        static bool playAsWhite;

        static std::atomic<uint64_t> maxTimeInms;

        static void analyze();

        static int64_t getExecutionTimeInms();

        static void setTimer();

        static void clearTimer();

        static short searchWrapper(int depth);

        static Game::Move (*killerMoves)[2];

        static short search(short alpha, short beta, int depth, int distanceToRoot, bool pvNode, Game::Move *moveBuffer, bool searchRecaptures);

        static short getMateEvaluation(int depth);

        static short getMateDistanceFromEvaluation(int eval);

        static bool isMate(short evaluation);
};

#endif