
#include "chess.hpp"
#pragma once

class ChessEngine;


float evaluate_for_side(const chess::Board& board, chess::Color color);

float evaluate_for_white(const chess::Board& board);

chess::Move find_move(ChessEngine* engine, chess::Board board, int time_limit_ms, bool emit_info);