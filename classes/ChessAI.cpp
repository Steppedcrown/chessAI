#include "ChessAI.h"
#include "Chess.h"

// ---------------------------------------------------------------------------
// Piece-square tables
// Written rank 8 → rank 1 (top to bottom), file a → h (left to right).
// A white piece at board square s uses index:  (7 - s/8)*8 + (s%8)
// A black piece at board square s uses index:  s
// This mirrors the table for black so both sides see symmetric incentives.
// ---------------------------------------------------------------------------

// Pawns: reward advancement and center control; penalise immobile center pawns
const int ChessAI::PST_PAWN[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,  // rank 8 (promotion rank — handled elsewhere)
    50, 50, 50, 50, 50, 50, 50, 50,  // rank 7
    10, 10, 20, 30, 30, 20, 10, 10,  // rank 6
     5,  5, 10, 25, 25, 10,  5,  5,  // rank 5
     0,  0,  0, 20, 20,  0,  0,  0,  // rank 4
     5, -5,-10,  0,  0,-10, -5,  5,  // rank 3
     5, 10, 10,-20,-20, 10, 10,  5,  // rank 2 (starting rank)
     0,  0,  0,  0,  0,  0,  0,  0   // rank 1
};

// Knights: strongly favour central squares, penalise rim
const int ChessAI::PST_KNIGHT[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

// Bishops: favour diagonals and open centre
const int ChessAI::PST_BISHOP[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

// Rooks: 7th rank is powerful; open d/e files; start on 1st rank corners
const int ChessAI::PST_ROOK[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};

// Queens: penalise early development; reward central-ish positions
const int ChessAI::PST_QUEEN[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

// King middlegame: stay safe behind pawns; reward castled positions
const int ChessAI::PST_KING_MG[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

// ---------------------------------------------------------------------------
// pieceValue — centipawn material values
// ---------------------------------------------------------------------------
int ChessAI::pieceValue(int pieceType)
{
    switch (pieceType) {
        case Pawn:   return 100;
        case Knight: return 320;
        case Bishop: return 330;
        case Rook:   return 500;
        case Queen:  return 900;
        case King:   return 20000;
        default:     return 0;
    }
}

// ---------------------------------------------------------------------------
// pstValue — positional bonus from the appropriate table
// ---------------------------------------------------------------------------
int ChessAI::pstValue(int pieceType, int pstIdx)
{
    switch (pieceType) {
        case Pawn:   return PST_PAWN[pstIdx];
        case Knight: return PST_KNIGHT[pstIdx];
        case Bishop: return PST_BISHOP[pstIdx];
        case Rook:   return PST_ROOK[pstIdx];
        case Queen:  return PST_QUEEN[pstIdx];
        case King:   return PST_KING_MG[pstIdx];
        default:     return 0;
    }
}

// ---------------------------------------------------------------------------
// fromChess — snapshot the live game into a self-contained State
// ---------------------------------------------------------------------------
ChessAI::State ChessAI::fromChess(Chess* chess)
{
    State s;
    s.board             = chess->getBoardArray();
    s.canCastleKingSide[0]  = chess->getCanCastleKingSide(0);
    s.canCastleKingSide[1]  = chess->getCanCastleKingSide(1);
    s.canCastleQueenSide[0] = chess->getCanCastleQueenSide(0);
    s.canCastleQueenSide[1] = chess->getCanCastleQueenSide(1);
    s.enPassantSquare   = chess->getEnPassantSquare();
    s.sideToMove        = chess->getCurrentPlayer()->playerNumber();
    return s;
}

// ---------------------------------------------------------------------------
// evaluate — static evaluation from sideToMove's perspective (centipawns)
// ---------------------------------------------------------------------------
int ChessAI::evaluate(const State& state)
{
    int score = 0;

    for (int sq = 0; sq < 64; ++sq) {
        int tag = state.board[sq];
        if (tag == 0) continue;

        int piecePlayer = (tag >= 128) ? 1 : 0;
        int pieceType   = (tag >= 128) ? tag - 128 : tag;
        int sign        = (piecePlayer == state.sideToMove) ? 1 : -1;

        score += sign * pieceValue(pieceType);

        // PST lookup: flip rank for white pieces, use square directly for black
        int pstIdx = (piecePlayer == 0) ? (7 - sq / 8) * 8 + (sq % 8) : sq;
        score += sign * pstValue(pieceType, pstIdx);
    }

    return score;
}
