#include "eval.h"

int evaluate_simple(const chess::Board& board){
    //evaluates the board from white's perspective

    static const int PIECE_VALUES[] = {
        100,
        300,
        300,
        500,
        900,
        100000,
        0
    };

   
    chess::Bitboard bb_white = board.us(chess::Color::WHITE);
    chess::Bitboard bb_black = board.them(chess::Color::WHITE);

    int white_material = 0;
    int black_material = 0;

    while(bb_white){
        uint8_t sq = bb_white.pop();
        
        if(sq==static_cast<uint8_t>(chess::Square::underlying::SQ_D5) ||//center 4 of board 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_E5) || 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_D4) || 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_E4)){
                white_material += 20;
        }

        if(sq==static_cast<uint8_t>(chess::Square::underlying::SQ_C5) ||//adjacent to center 4 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_F5) || 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_C4) || 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_F4)){
                white_material += 10;
        }

        chess::Square square(sq);
        chess::Piece piece = board.at(square);
        chess::PieceType ptype = piece.type();

        white_material += PIECE_VALUES[static_cast<std::uint8_t>(ptype)];
        
    }

    while(bb_black){
        uint8_t sq = bb_black.pop();

        if(sq==static_cast<uint8_t>(chess::Square::underlying::SQ_D5) ||//center 4 of board 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_E5) || 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_D4) || 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_E4)){
                black_material += 20;
        }

        if(sq==static_cast<uint8_t>(chess::Square::underlying::SQ_C5) ||//adjacent to center 4 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_F5) || 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_C4) || 
           sq==static_cast<uint8_t>(chess::Square::underlying::SQ_F4)){
                black_material += 10;
        }

        chess::Square square(sq);
        chess::Piece piece = board.at(square);
        chess::PieceType ptype = piece.type();

        black_material += PIECE_VALUES[static_cast<std::uint8_t>(ptype)];
    }

    return white_material - black_material;    
}