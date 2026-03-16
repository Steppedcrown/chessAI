#include "ChessAI.h"
#include "Chess.h"
#include <limits>
#include <algorithm>

// ---- piece-square tables (rank 8 at top, rank 1 at bottom) -----------------

const int ChessAI::PST_PAWN[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

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

// ---- local helpers ----------------------------------------------------------

namespace {
inline bool onBoard(int x, int y)      { return x >= 0 && x < 8 && y >= 0 && y < 8; }
inline bool isFriendly(int tag, int p) { return p == 0 ? (tag > 0 && tag < 128) : (tag >= 128); }
inline bool isEnemy(int tag, int p)    { return p == 0 ? (tag >= 128) : (tag > 0 && tag < 128); }
inline int  typeOf(int tag)            { return tag >= 128 ? tag - 128 : tag; }
} // namespace

// ---- isSquareAttacked -------------------------------------------------------

bool ChessAI::isSquareAttacked(const std::array<int,64>& board, int square, int attacker)
{
    int tx = square % 8, ty = square / 8;

    // Pawn attacks
    int pDir = (attacker == 0) ? 1 : -1;
    for (int dx : {-1, 1}) {
        int sx = tx + dx, sy = ty + pDir;
        if (onBoard(sx, sy) && board[sy*8+sx] == (attacker == 0 ? Pawn : 128 + Pawn))
            return true;
    }

    // Knight attacks
    const int kn[8][2] = {{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
    int knightTag = attacker == 0 ? Knight : 128 + Knight;
    for (auto& o : kn)
        if (onBoard(tx+o[0], ty+o[1]) && board[(ty+o[1])*8+(tx+o[0])] == knightTag)
            return true;

    // King attacks
    const int kg[8][2] = {{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1},{-1,0},{-1,1}};
    int kingTag = attacker == 0 ? King : 128 + King;
    for (auto& o : kg)
        if (onBoard(tx+o[0], ty+o[1]) && board[(ty+o[1])*8+(tx+o[0])] == kingTag)
            return true;

    // Sliding pieces
    int rookTag   = attacker == 0 ? Rook   : 128 + Rook;
    int bishopTag = attacker == 0 ? Bishop : 128 + Bishop;
    int queenTag  = attacker == 0 ? Queen  : 128 + Queen;

    auto checkRay = [&](int dx, int dy, bool rq, bool bq) {
        int x = tx+dx, y = ty+dy;
        while (onBoard(x, y)) {
            int t = board[y*8+x];
            if (t) {
                if (rq && (t == rookTag || t == queenTag))   return true;
                if (bq && (t == bishopTag || t == queenTag)) return true;
                break;
            }
            x += dx; y += dy;
        }
        return false;
    };

    const int straight[4][2] = {{0,1},{0,-1},{1,0},{-1,0}};
    const int diagonal[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
    for (auto& d : straight) if (checkRay(d[0],d[1],true,false))  return true;
    for (auto& d : diagonal) if (checkRay(d[0],d[1],false,true))  return true;

    return false;
}

// ---- isInCheck --------------------------------------------------------------

bool ChessAI::isInCheck(const std::array<int,64>& board, int player)
{
    int kingTag = player == 0 ? King : 128 + King;
    for (int sq = 0; sq < 64; ++sq)
        if (board[sq] == kingTag)
            return isSquareAttacked(board, sq, 1 - player);
    return false;
}

// ---- applyMove --------------------------------------------------------------

ChessAI::State ChessAI::applyMove(const State& s, const BitMove& move)
{
    State next = s;
    next.board[move.to]   = next.board[move.from];
    next.board[move.from] = 0;

    // Move the rook for castling
    if (move.isCastle) {
        if      (move.from == 4  && move.to == 6)  { next.board[5]  = next.board[7];  next.board[7]  = 0; }
        else if (move.from == 4  && move.to == 2)  { next.board[3]  = next.board[0];  next.board[0]  = 0; }
        else if (move.from == 60 && move.to == 62) { next.board[61] = next.board[63]; next.board[63] = 0; }
        else if (move.from == 60 && move.to == 58) { next.board[59] = next.board[56]; next.board[56] = 0; }
    }

    // Remove the captured pawn for en passant
    if (move.isEnPassant)
        next.board[(move.from / 8) * 8 + (move.to % 8)] = 0;

    // Auto-promote pawns reaching the back rank (queen)
    int toY = move.to / 8;
    if (move.piece == Pawn && (toY == 7 || toY == 0))
        next.board[move.to] = (s.sideToMove == 0) ? Queen : 128 + Queen;

    // Update castling rights for the moving side
    if (move.piece == King) {
        next.canCastleKingSide[s.sideToMove]  = false;
        next.canCastleQueenSide[s.sideToMove] = false;
    } else if (move.piece == Rook) {
        int homeRow = s.sideToMove == 0 ? 0 : 7;
        if (move.from / 8 == homeRow) {
            if (move.from % 8 == 7) next.canCastleKingSide[s.sideToMove]  = false;
            if (move.from % 8 == 0) next.canCastleQueenSide[s.sideToMove] = false;
        }
    }

    // Revoke opponent's castling right if their rook is captured on its home square
    int opp = 1 - s.sideToMove;
    if (move.to / 8 == (opp == 0 ? 0 : 7)) {
        if (move.to % 8 == 7) next.canCastleKingSide[opp]  = false;
        if (move.to % 8 == 0) next.canCastleQueenSide[opp] = false;
    }

    // Set en passant target square after a double pawn push
    if (move.piece == Pawn && abs((int)(move.to/8) - (int)(move.from/8)) == 2)
        next.enPassantSquare = ((move.from/8 + move.to/8) / 2) * 8 + (move.from % 8);
    else
        next.enPassantSquare = -1;

    next.sideToMove = opp;
    return next;
}

// ---- generateMoves ----------------------------------------------------------

std::vector<BitMove> ChessAI::generateMoves(const State& state)
{
    std::vector<BitMove> moves;
    int p = state.sideToMove;

    // Only keep moves that don't leave our own king in check
    auto addIfLegal = [&](BitMove m) {
        if (!isInCheck(applyMove(state, m).board, p))
            moves.push_back(m);
    };

    for (int sq = 0; sq < 64; ++sq) {
        int tag = state.board[sq];
        if (!isFriendly(tag, p)) continue;

        int type = typeOf(tag);
        int x = sq % 8, y = sq / 8;

        if (type == Pawn) {
            int dir      = p == 0 ? 1 : -1;
            int startRow = p == 0 ? 1 : 6;

            // Single push
            int ny = y + dir;
            if (onBoard(x, ny) && state.board[ny*8+x] == 0) {
                addIfLegal(BitMove(sq, ny*8+x, Pawn));
                // Double push from starting rank
                int ny2 = y + 2*dir;
                if (y == startRow && state.board[ny2*8+x] == 0)
                    addIfLegal(BitMove(sq, ny2*8+x, Pawn));
            }
            // Diagonal captures
            for (int dx : {-1, 1}) {
                int nx = x + dx;
                if (onBoard(nx, ny) && isEnemy(state.board[ny*8+nx], p))
                    addIfLegal(BitMove(sq, ny*8+nx, Pawn));
            }
            // En passant
            if (state.enPassantSquare >= 0) {
                int epx = state.enPassantSquare % 8, epy = state.enPassantSquare / 8;
                if (epy == y + dir && abs(epx - x) == 1) {
                    BitMove m(sq, state.enPassantSquare, Pawn);
                    m.isEnPassant = true;
                    addIfLegal(m);
                }
            }

        } else if (type == Knight) {
            const int offs[8][2] = {{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
            for (auto& o : offs) {
                int nx = x+o[0], ny = y+o[1];
                if (onBoard(nx, ny) && !isFriendly(state.board[ny*8+nx], p))
                    addIfLegal(BitMove(sq, ny*8+nx, Knight));
            }

        } else if (type == Bishop || type == Rook || type == Queen) {
            ChessPiece cp = static_cast<ChessPiece>(type);
            const int straight[4][2] = {{0,1},{0,-1},{1,0},{-1,0}};
            const int diagonal[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
            bool doStraight = (type == Rook  || type == Queen);
            bool doDiagonal = (type == Bishop || type == Queen);
            auto slide = [&](const int dirs[][2], int n) {
                for (int i = 0; i < n; ++i) {
                    int nx = x+dirs[i][0], ny = y+dirs[i][1];
                    while (onBoard(nx, ny)) {
                        int dest = state.board[ny*8+nx];
                        if (isFriendly(dest, p)) break;
                        addIfLegal(BitMove(sq, ny*8+nx, cp));
                        if (dest) break;
                        nx += dirs[i][0]; ny += dirs[i][1];
                    }
                }
            };
            if (doStraight) slide(straight, 4);
            if (doDiagonal) slide(diagonal, 4);

        } else if (type == King) {
            const int offs[8][2] = {{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1},{-1,0},{-1,1}};
            for (auto& o : offs) {
                int nx = x+o[0], ny = y+o[1];
                if (onBoard(nx, ny) && !isFriendly(state.board[ny*8+nx], p))
                    addIfLegal(BitMove(sq, ny*8+nx, King));
            }

            // Castling (king must be on its home square and not in check)
            int home = p == 0 ? 4 : 60;
            if (sq == home && !isInCheck(state.board, p)) {
                int rookTag = p == 0 ? Rook : 128 + Rook;

                // Kingside
                if (state.canCastleKingSide[p]) {
                    int rSq = p==0?7:63, mid = p==0?5:61, dst = p==0?6:62;
                    if (state.board[rSq]==rookTag && !state.board[mid] && !state.board[dst] &&
                        !isSquareAttacked(state.board, mid, 1-p)) {
                        BitMove m(sq, dst, King); m.isCastle = true; addIfLegal(m);
                    }
                }

                // Queenside
                if (state.canCastleQueenSide[p]) {
                    int rSq = p==0?0:56, mid = p==0?3:59, dst = p==0?2:58, bsq = p==0?1:57;
                    if (state.board[rSq]==rookTag && !state.board[mid] && !state.board[dst] &&
                        !state.board[bsq] && !isSquareAttacked(state.board, mid, 1-p)) {
                        BitMove m(sq, dst, King); m.isCastle = true; addIfLegal(m);
                    }
                }
            }
        }
    }

    return moves;
}

// ---- evaluate ---------------------------------------------------------------

int ChessAI::evaluate(const State& state)
{
    int score = 0;
    for (int sq = 0; sq < 64; ++sq) {
        int tag = state.board[sq];
        if (!tag) continue;
        int pp    = tag >= 128 ? 1 : 0;
        int ptype = tag >= 128 ? tag - 128 : tag;
        int sign  = pp == state.sideToMove ? 1 : -1;
        score += sign * pieceValue(ptype);
        // White flips rank; black uses square directly
        int pstIdx = pp == 0 ? (7 - sq/8)*8 + (sq%8) : sq;
        score += sign * pstValue(ptype, pstIdx);
    }
    return score;
}

// ---- move ordering ----------------------------------------------------------

// MVV-LVA: prefer capturing high-value pieces with low-value pieces.
// Promotions and en passant captures are also boosted.
int ChessAI::moveScore(const BitMove& move, const State& state)
{
    int score = 0;

    // Regular capture: 10 * victim value - attacker value
    int victim = state.board[move.to];
    if (victim) {
        int vtype = victim >= 128 ? victim - 128 : victim;
        int atype = state.board[move.from] >= 128 ? state.board[move.from] - 128 : state.board[move.from];
        score += 10 * pieceValue(vtype) - pieceValue(atype);
    }

    // En passant: pawn captures pawn
    if (move.isEnPassant)
        score += 10 * pieceValue(Pawn) - pieceValue(Pawn);

    // Promotion: reward the material gain
    int toY = move.to / 8;
    if (move.piece == Pawn && (toY == 7 || toY == 0))
        score += pieceValue(Queen) - pieceValue(Pawn);

    return score;
}

void ChessAI::orderMoves(std::vector<BitMove>& moves, const State& state)
{
    std::sort(moves.begin(), moves.end(), [&](const BitMove& a, const BitMove& b) {
        return moveScore(a, state) > moveScore(b, state);
    });
}

// ---- negamax ----------------------------------------------------------------

int ChessAI::negamax(const State& state, int depth, int alpha, int beta)
{
    if (depth == 0) return evaluate(state);

    auto moves = generateMoves(state);

    if (moves.empty()) {
        if (isInCheck(state.board, state.sideToMove)) return -20000 - depth;
        return 0; // stalemate
    }

    orderMoves(moves, state);

    for (const auto& move : moves) {
        int score = -negamax(applyMove(state, move), depth - 1, -beta, -alpha);
        if (score >= beta) return beta; // beta cutoff
        alpha = std::max(alpha, score);
    }
    return alpha;
}

// ---- getBestMove ------------------------------------------------------------

BitMove ChessAI::getBestMove(const State& state, int depth)
{
    BitMove best;
    int alpha = std::numeric_limits<int>::min() + 1;
    int beta  = std::numeric_limits<int>::max();

    auto moves = generateMoves(state);
    orderMoves(moves, state);

    for (const auto& move : moves) {
        int score = -negamax(applyMove(state, move), depth - 1, -beta, -alpha);
        if (score > alpha) {
            alpha = score;
            best  = move;
        }
    }
    return best;
}

// ---- scoring helpers --------------------------------------------------------

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

// ---- fromChess --------------------------------------------------------------

ChessAI::State ChessAI::fromChess(Chess* chess)
{
    State s;
    s.board                = chess->getBoardArray();
    s.canCastleKingSide[0] = chess->getCanCastleKingSide(0);
    s.canCastleKingSide[1] = chess->getCanCastleKingSide(1);
    s.canCastleQueenSide[0]= chess->getCanCastleQueenSide(0);
    s.canCastleQueenSide[1]= chess->getCanCastleQueenSide(1);
    s.enPassantSquare      = chess->getEnPassantSquare();
    s.sideToMove           = chess->getCurrentPlayer()->playerNumber();
    return s;
}
