
#include "search.h"
#include "engine.h"
#include "chess.hpp"

#include <cmath>
#include <list>
#include <ranges>


float evaluate_for_side(const chess::Board& board, chess::Color color){
    float w_eval = evaluate_for_white(board);
    if(color == chess::Color::WHITE){
        return w_eval;
    }else{
        return -1.0 * w_eval;
    }
}

float evaluate_for_white(const chess::Board& board){

    static const float PIECE_VALUES[] = {
        1,//pawn
        3,//knight
        3,//bishop
        5,//rook
        9,//queen
        10000,//king
        0
    };


    //white piece value
    chess::Bitboard white_bb = board.us(chess::Color::WHITE);
    float white_value = 0.0;
    while(white_bb){
        uint8_t sq = white_bb.pop();
        chess::Square square(sq);
        chess::Piece piece = board.at(square);
        chess::PieceType ptype = piece.type();

        white_value += PIECE_VALUES[static_cast<uint8_t>(ptype)];

        if(sq==static_cast<uint8_t>(chess::Square::underlying::SQ_D5) ||//center 4 of board 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_E5) || 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_D4) || 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_E4)){
                white_value += 0.1;
        }

        if(sq==static_cast<uint8_t>(chess::Square::underlying::SQ_C5) ||//adjacent to center 4 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_F5) || 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_C4) || 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_F4)){
                white_value += 0.05;
        }
    }

    //black piece value
    chess::Bitboard black_bb = board.us(chess::Color::BLACK);
    float black_value = 0.0;
    while(black_bb){
        uint8_t sq = black_bb.pop();
        chess::Square square(sq);
        chess::Piece piece = board.at(square);
        chess::PieceType ptype = piece.type();

        black_value+= PIECE_VALUES[static_cast<uint8_t>(ptype)];


        if(sq==static_cast<uint8_t>(chess::Square::underlying::SQ_D5) ||//center 4 of board 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_E5) || 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_D4) || 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_E4)){
                black_value += 0.1;
        }

        if(sq==static_cast<uint8_t>(chess::Square::underlying::SQ_C5) ||//adjacent to center 4 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_F5) || 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_C4) || 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_F4)){
                black_value += 0.05;
        }
    }

    return white_value - black_value;
}


float score_move_mvv_lva(chess::Board& board, chess::Move move){
     static const float PIECE_VALUES[] = {
        1,//pawn
        3,//knight
        3,//bishop
        5,//rook
        9,//queen
        10000,//king
        0//none
    };
    
    
    chess::Piece attacking_piece = board.at(move.from());
    chess::PieceType ptype_attacker = attacking_piece.type();
    float attacker_value = PIECE_VALUES[static_cast<uint8_t>(ptype_attacker)];
    
    chess::Piece victim_piece = board.at(move.to());
    chess::PieceType ptype_victim = victim_piece.type();
    float victim_value = PIECE_VALUES[static_cast<uint8_t>(ptype_victim)];

    return victim_value - attacker_value;
}

inline void append(chess::Movelist& dst, const chess::Movelist& src) {
    for (const auto& m : src) dst.add(m);   // src.begin()/end() are just Move* iterators
}


float negamax_ab(ChessEngine* engine, chess::Board& board, uint16_t ply, int depth, float a, float b, long long deadline_ns){

    /*
    TODO's
        move ordering
        fixing board.inCheck() with stand-pat: if you're in check, standing pat isn't valid, so you will miss the one quiet move that allows escape. Only an issue at the horizons, but should be fixed
    
    */

    if((++engine->nodes_evaluated & 2047) == 0){//only check this every 2048 nodes
        long long time_now_ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        if(time_now_ns > deadline_ns){
            engine->stop_search = true;
        } 
    }

    if(engine->stop_search){
        return 0.0;
    }
   


    const float MATED_VAL = -10000000.0 + ply;
    const float DRAW_VAL = 0.0;

    if(board.isHalfMoveDraw()){
        return board.getHalfMoveDrawType().first == chess::GameResultReason::CHECKMATE ? MATED_VAL : DRAW_VAL;
    }
    if(board.isRepetition() || board.isInsufficientMaterial()){
        return DRAW_VAL;
    }


    float side_mult = board.sideToMove() == chess::Color::WHITE ? 1.0f : -1.0f;

    if(depth <= 0){
        if(depth < -8){//don't infinitely search a super long forced chain
            return side_mult * evaluate_for_white(board);
        }

        if(board.inCheck()){
            chess::Movelist evasions;
            chess::movegen::legalmoves(evasions,board);

            if(evasions.empty()){
                return MATED_VAL;
            }

            float value = -INFINITY;
            for(const auto& move : evasions){
                board.makeMove(move);
                float score = -negamax_ab(engine,board,ply+1,depth-1,-b,-a,deadline_ns);
                board.unmakeMove(move);

                if(engine->stop_search){
                    return 0.0;
                }

                value = std::max(value,score);
                a = std::max(a,value);
                if(a >= b){
                    break;
                }

            }
            return value;
        }
        
        //we aren't in check, so stand-pat is valid
        float stand_pat = side_mult * evaluate_for_white(board);
        if(stand_pat >= b){
            return stand_pat;
        }if(stand_pat > a){
            a = stand_pat;
        }

        chess::Movelist captures;
        chess::movegen::legalmoves<chess::movegen::MoveGenType::CAPTURE>(captures, board);
        //TODO: order moves
        

        float value = stand_pat;// fail-soft floor
        for (const auto& move : captures) {
            board.makeMove(move);
            float score = -negamax_ab(engine, board, ply + 1, depth - 1, -b, -a, deadline_ns);
            board.unmakeMove(move);
            if(engine->stop_search){
                return 0.0;
            }
            value = std::max(value,score);
            a = std::max(a, value);
            if (a >= b) break;
        }
        return value;

    }
    



    float value = -INFINITY;

    chess::Movelist captures;
    chess::Movelist noncaptures;
    chess::Movelist semi_ordered;

    chess::movegen::legalmoves<chess::movegen::MoveGenType::CAPTURE>(captures,board);
    chess::movegen::legalmoves<chess::movegen::MoveGenType::QUIET  >(noncaptures,board);
    
    
    append(semi_ordered,captures);
    append(semi_ordered,noncaptures);
 
    //TODO: order moves according to mvv_lva


    if(semi_ordered.empty()){
        return board.inCheck() ? MATED_VAL : DRAW_VAL;
    }


    for(const auto &move: semi_ordered){
        board.makeMove(move);
        float score = -negamax_ab(engine, board, ply + 1, depth-1, -1*b, -1*a, deadline_ns);
        board.unmakeMove(move);

        if(engine->stop_search){
            return 0.0;
        }

        value = std::max(value,score);
        a = std::max(a,value);
        if(a >= b){
            break;
        }
    }

    

    return value;

}


chess::Move iterative_deepening_search(ChessEngine* engine, chess::Board& board, chess::Movelist move_list, uint16_t min_depth, uint16_t max_depth, long long deadline_ns){
    engine->stop_search = false;
    engine->nodes_evaluated = 0;

    chess::Move best_move = move_list[0];

    for(size_t depth = min_depth; depth <= max_depth; ++depth){
        float best_eval = -INFINITY;
        chess::Move iter_best = chess::Move::NO_MOVE;

        for(const auto& move : move_list){
            board.makeMove(move);
            float eval = -negamax_ab(engine,board,0,depth-1,-INFINITY,-best_eval,deadline_ns);
            board.unmakeMove(move);

            if(engine->stop_search){
                break;
            }

            if(eval > best_eval){
                best_eval = eval;
                iter_best = move;
            }
        }

        if(engine->stop_search){
            break;
        }

        best_move = iter_best;
        if(engine->debug){
            std::cout << "info string: depth=" << depth << " | move=" << chess::uci::moveToUci(iter_best) << std::endl;  
        }

        for(int i=0; i<move_list.size(); ++i){
            if(move_list[i] == iter_best){
                for(int j=i; j>0; --j){
                    move_list[j] = move_list[j-1];//shift everything over 1 slot to the right
                }
                move_list[0] = iter_best;//then insert the best move at the beginning
                break;
            }
        }


    }
    
    
    return best_move;
}


chess::Move book_entry_to_move(ChessEngine* engine, const chess::Movelist move_list, const BookEntry& e){
    //we need this since the opening book stores castling, en-passants etc as illegal moves
    for(const chess::Move& m : move_list){
        if(m.from() == e.move_source && m.to() == e.move_dest){
            return m;
        }
    }

    return chess::Move::NO_MOVE;

}

chess::Move get_book_move(ChessEngine* engine, chess::Board board, chess::Movelist& move_list){
    auto zhash = board.hash();
    auto it = engine->opening_book.find(zhash);
    if(it != engine->opening_book.end()){//we found the board position in the opening book
        int move_idx = 0;

        
        std::uniform_real_distribution<double> distribution(0.0,1.0);
        double r = distribution(engine->rng);

        for(size_t i = 0; i<engine->opening_book[zhash].size(); i++){
            if(engine->opening_book[zhash][i].cum_weight > r){
                move_idx = i;
                break;
            }
        }
        

        BookEntry e = engine->opening_book[zhash][move_idx];

        //chess::Square source = engine->opening_book[zhash][move_idx].move_source;
        //chess::Square dest   = engine->opening_book[zhash][move_idx].move_dest;

        chess::Move m = book_entry_to_move(engine,move_list,e);
        //chess::Move m = chess::Move::make<chess::Move::NORMAL>(source,dest);

        if(engine->debug){
            std::cout << "info string: ----->  Found in table!   " << chess::uci::moveToUci(m) << "(" << e.normalized_weight << ")" << std::endl;
        }

        return m;

    }else{
        //the board position wasn't in our book
        //change engine state, return no-move to allow for search
        engine->in_book = false;
        
    }
    return chess::Move::NO_MOVE;
}

chess::Move find_move(ChessEngine* engine, chess::Board board, int time_limit_ms, bool emit_info){
    
    chess::Movelist moves;//we need these to check book moves against, so generate at the very top
    chess::movegen::legalmoves(moves,board);
    
    //check if we are in the book
    chess::Move book_move = chess::Move::NO_MOVE;
    if(engine->in_book){
        book_move = get_book_move(engine,board,moves);
    }

    if(book_move != chess::Move::NO_MOVE){
        return book_move;
    }

    engine->stop_search = false;
    engine->nodes_evaluated = 0;

    
    
    long long start_time_ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();//this measures in nanoseconds
    long long deadline_ns = start_time_ns + static_cast<long long>(time_limit_ms)*1'000'000LL;

    int min_depth = 1;
    int max_depth = 8;
    chess::Move iter_deep_move = iterative_deepening_search(engine,board,moves,min_depth,max_depth,deadline_ns);

    if(emit_info){
        std::cout   << "info depth " << max_depth
                    << " nodes " << engine->nodes_evaluated
                    << " score cp " << "0.0"//I don't actually care about this, its just to make fastchess happy
                    << std::endl;
    }

    /*
    //old code - simple loop
    float best_eval = -INFINITY;
    chess::Move best_move = chess::Move::NO_MOVE;

    for(const auto &move : moves){
        board.makeMove(move);
        float eval = -1 * negamax_ab(engine, board, 0, 5, -2000000000, 2000000000, deadline_ns);
        board.unmakeMove(move);

        if(engine->debug){
            std::cout << "info string: ----->  " <<  chess::uci::moveToUci(move) << "      " << eval << std::endl;
        }

        if(eval > best_eval){
            best_eval = eval;
            best_move = move;
        }
        

    }
    return best_move;

    */

    return iter_deep_move;


}