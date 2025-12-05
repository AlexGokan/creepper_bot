

#include <iostream>
#include "chess.hpp"

using namespace chess;

int main() {
    std::cout << "Hello, World!" << std::endl;

    Board board = Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    Movelist moves;
    movegen::legalmoves(moves, board);

    for (const auto &move : moves) {
        std::cout << uci::moveToUci(move) << std::endl;
    }

    std::cout << "Hello world :3";

    return 0;
}