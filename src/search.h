
#include "chess.hpp"
#pragma once

class ChessEngine;

void simple_ab_search(ChessEngine& ce);

void playout(ChessEngine& ce, int depth_limit);

void playout_wrapper(ChessEngine& ce);
void playout_wrapper(ChessEngine& ce, int depth_limit);

int negamax_search(ChessEngine& ce, chess::Board& board, int depth, float alpha, float beta);
chess::Move negamax_caller(ChessEngine& ce, chess::Board& board, int depth, bool print_info, int time_limit_ms);


