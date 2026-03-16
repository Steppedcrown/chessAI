#include "Chess.h"
#include <array>
#include <limits>
#include <cmath>

namespace {

inline bool isWhiteTag(int tag) { return tag > 0 && tag < 128; }
inline bool isBlackTag(int tag) { return tag >= 128; }
inline bool isSameColor(int tag, int player) { return player == 0 ? isWhiteTag(tag) : isBlackTag(tag); }
inline bool isOpponentColor(int tag, int player) { return player == 0 ? isBlackTag(tag) : isWhiteTag(tag); }
inline int pieceTypeFromTag(int tag) {
    if (tag == 0) return 0;
    if (tag < 128) return tag;
    return tag - 128;
}

inline bool onBoard(int x, int y) { return x >= 0 && x < 8 && y >= 0 && y < 8; }
inline int squareIndex(int x, int y) { return y * 8 + x; }

std::array<int, 64> buildBoardArray(Chess *chess)
{
    std::array<int, 64> board;
    board.fill(0);
    Grid *grid = chess->getGrid();
    grid->forEachSquare([&](ChessSquare *sq, int x, int y) {
        Bit *bit = sq->bit();
        int idx = squareIndex(x, y);
        if (!bit) return;
        board[idx] = bit->gameTag();
    });
    return board;
}

int findKingSquare(const std::array<int, 64> &board, int player)
{
    int targetTag = (player == 0) ? King : 128 + King;
    for (int i = 0; i < 64; ++i) {
        if (board[i] == targetTag) return i;
    }
    return -1;
}

bool isSquareAttacked(const std::array<int, 64> &board, int square, int attacker)
{
    int tx = square % 8;
    int ty = square / 8;

    // Pawn attacks (white pawns attack "up" (+y), black pawns attack "down" (-y))
    if (attacker == 0) {
        int dy = 1;
        for (int dx : {-1, 1}) {
            int sx = tx + dx;
            int sy = ty + dy;
            if (!onBoard(sx, sy)) continue;
            int si = squareIndex(sx, sy);
            if (board[si] == Pawn) return true;
        }
    } else {
        int dy = -1;
        for (int dx : {-1, 1}) {
            int sx = tx + dx;
            int sy = ty + dy;
            if (!onBoard(sx, sy)) continue;
            int si = squareIndex(sx, sy);
            if (board[si] == 128 + Pawn) return true;
        }
    }

    // Knight attacks
    const int knightOff[8][2] = {{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
    for (auto &o : knightOff) {
        int sx = tx + o[0];
        int sy = ty + o[1];
        if (!onBoard(sx, sy)) continue;
        int si = squareIndex(sx, sy);
        int tag = board[si];
        if (tag == (attacker == 0 ? Knight : 128 + Knight)) return true;
    }

    // King attacks
    const int kingOff[8][2] = {{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1},{-1,0},{-1,1}};
    for (auto &o : kingOff) {
        int sx = tx + o[0];
        int sy = ty + o[1];
        if (!onBoard(sx, sy)) continue;
        int si = squareIndex(sx, sy);
        int tag = board[si];
        if (tag == (attacker == 0 ? King : 128 + King)) return true;
    }

    // Sliding pieces
    const int rookDirs[4][2] = {{0,1},{0,-1},{1,0},{-1,0}};
    const int bishopDirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};

    auto checkRay = [&](int dx, int dy, int rookTag, int bishopTag, int queenTag) {
        int sx = tx + dx;
        int sy = ty + dy;
        while (onBoard(sx, sy)) {
            int si = squareIndex(sx, sy);
            int tag = board[si];
            if (tag != 0) {
                if (tag == rookTag || tag == bishopTag || tag == queenTag) return true;
                break;
            }
            sx += dx;
            sy += dy;
        }
        return false;
    };

    int rookTag = (attacker == 0 ? Rook : 128 + Rook);
    int bishopTag = (attacker == 0 ? Bishop : 128 + Bishop);
    int queenTag = (attacker == 0 ? Queen : 128 + Queen);

    for (auto &d : rookDirs) {
        if (checkRay(d[0], d[1], rookTag, -1, queenTag)) return true;
    }
    for (auto &d : bishopDirs) {
        if (checkRay(d[0], d[1], -1, bishopTag, queenTag)) return true;
    }

    return false;
}

bool isInCheck(const std::array<int, 64> &board, int player)
{
    int kingSquare = findKingSquare(board, player);
    if (kingSquare < 0) return false;
    return isSquareAttacked(board, kingSquare, 1 - player);
}

std::array<int, 64> applyMoveToBoard(const std::array<int, 64> &board, const BitMove &move)
{
    auto next = board;
    next[move.to] = next[move.from];
    next[move.from] = 0;
    if (move.isCastle) {
        // Also move the rook
        if (move.from == 4 && move.to == 6)       { next[5] = next[7]; next[7] = 0; }  // white kingside
        else if (move.from == 4 && move.to == 2)  { next[3] = next[0]; next[0] = 0; }  // white queenside
        else if (move.from == 60 && move.to == 62){ next[61] = next[63]; next[63] = 0; }// black kingside
        else if (move.from == 60 && move.to == 58){ next[59] = next[56]; next[56] = 0; }// black queenside
    }
    if (move.isEnPassant) {
        // Remove the captured pawn: same file as destination, same rank as source
        int capturedSq = (move.from / 8) * 8 + (move.to % 8);
        next[capturedSq] = 0;
    }
    return next;
}

bool isLegalMove(Chess *chess, const BitMove &move)
{
    int player = chess->getCurrentPlayer()->playerNumber();
    auto board = buildBoardArray(chess);
    auto next = applyMoveToBoard(board, move);
    return !isInCheck(next, player);
}

bool hasLegalMove(Chess *chess, int player)
{
    auto board = buildBoardArray(chess);

    for (int from = 0; from < 64; ++from) {
        int tag = board[from];
        if (!isSameColor(tag, player)) continue;
        int type = pieceTypeFromTag(tag);
        int fx = from % 8;
        int fy = from / 8;

        auto tryMove = [&](int tx, int ty) {
            if (!onBoard(tx, ty)) return false;
            int to = squareIndex(tx, ty);
            int dest = board[to];
            if (isSameColor(dest, player)) return false;
            auto next = applyMoveToBoard(board, BitMove(from, to, static_cast<ChessPiece>(type)));
            if (!isInCheck(next, player)) return true;
            return false;
        };

        if (type == Pawn) {
            int dir = (player == 0 ? 1 : -1);
            // push
            if (onBoard(fx, fy + dir) && board[squareIndex(fx, fy + dir)] == 0) {
                if (tryMove(fx, fy + dir)) return true;
            }
            for (int dx : {-1, 1}) {
                int tx = fx + dx;
                int ty = fy + dir;
                if (!onBoard(tx, ty)) continue;
                int tgt = board[squareIndex(tx, ty)];
                if (tgt != 0 && isOpponentColor(tgt, player)) {
                    if (tryMove(tx, ty)) return true;
                }
            }
        } else if (type == Knight) {
            const int offsets[8][2] = {{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
            for (auto &o : offsets) {
                if (tryMove(fx + o[0], fy + o[1])) return true;
            }
        } else if (type == Bishop) {
            const int dirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
            for (auto &d : dirs) {
                int tx = fx + d[0];
                int ty = fy + d[1];
                while (onBoard(tx, ty)) {
                    int dest = board[squareIndex(tx, ty)];
                    if (isSameColor(dest, player)) break;
                    if (tryMove(tx, ty)) return true;
                    if (dest != 0) break;
                    tx += d[0];
                    ty += d[1];
                }
            }
        } else if (type == Rook) {
            const int dirs[4][2] = {{0,1},{0,-1},{1,0},{-1,0}};
            for (auto &d : dirs) {
                int tx = fx + d[0];
                int ty = fy + d[1];
                while (onBoard(tx, ty)) {
                    int dest = board[squareIndex(tx, ty)];
                    if (isSameColor(dest, player)) break;
                    if (tryMove(tx, ty)) return true;
                    if (dest != 0) break;
                    tx += d[0];
                    ty += d[1];
                }
            }
        } else if (type == Queen) {
            const int dirs[8][2] = {{0,1},{0,-1},{1,0},{-1,0},{1,1},{1,-1},{-1,1},{-1,-1}};
            for (auto &d : dirs) {
                int tx = fx + d[0];
                int ty = fy + d[1];
                while (onBoard(tx, ty)) {
                    int dest = board[squareIndex(tx, ty)];
                    if (isSameColor(dest, player)) break;
                    if (tryMove(tx, ty)) return true;
                    if (dest != 0) break;
                    tx += d[0];
                    ty += d[1];
                }
            }
        } else if (type == King) {
            const int offsets[8][2] = {{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1},{-1,0},{-1,1}};
            for (auto &o : offsets) {
                if (tryMove(fx + o[0], fy + o[1])) return true;
            }
        }
    }

    return false;
}

} // namespace

Chess::Chess()
{
    _grid = new Grid(8, 8);
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");

    _canCastleKingSide[0] = _canCastleKingSide[1] = true;
    _canCastleQueenSide[0] = _canCastleQueenSide[1] = true;
    _enPassantSquare = -1;

    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");

    _moveList.clear();
    _moveCount = 0;

    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    // NOT PART OF THIS ASSIGNMENT BUT OTHER THINGS THAT CAN BE IN A FEN STRING
    // ARE BELOW
    // 2: active color (W or B)
    // 3: castling availability (KQkq or -)
    // 4: en passant target square (in algebraic notation, or -)
    // 5: halfmove clock (number of halfmoves since the last capture or pawn advance)

    // Extract piece placement portion (stop at first space for full FEN strings)
    std::string placement = fen.substr(0, fen.find(' '));

    // Parse castling availability (3rd FEN field) and en passant target (4th FEN field)
    size_t sp1 = fen.find(' ');
    if (sp1 != std::string::npos) {
        size_t sp2 = fen.find(' ', sp1 + 1);
        if (sp2 != std::string::npos) {
            size_t sp3 = fen.find(' ', sp2 + 1);
            std::string castling = fen.substr(sp2 + 1, sp3 == std::string::npos ? std::string::npos : sp3 - sp2 - 1);
            _canCastleKingSide[0]  = castling.find('K') != std::string::npos;
            _canCastleQueenSide[0] = castling.find('Q') != std::string::npos;
            _canCastleKingSide[1]  = castling.find('k') != std::string::npos;
            _canCastleQueenSide[1] = castling.find('q') != std::string::npos;

            if (sp3 != std::string::npos) {
                size_t sp4 = fen.find(' ', sp3 + 1);
                std::string ep = fen.substr(sp3 + 1, sp4 == std::string::npos ? std::string::npos : sp4 - sp3 - 1);
                if (ep.size() >= 2 && ep[0] != '-') {
                    int epFile = ep[0] - 'a';
                    int epRank = ep[1] - '1';
                    _enPassantSquare = epRank * 8 + epFile;
                } else {
                    _enPassantSquare = -1;
                }
            }
        }
    }

    int rankIdx = 0; // 0 = rank 8 (black's back rank), 7 = rank 1 (white's back rank)
    int fileIdx = 0; // 0 = file a, 7 = file h

    for (char c : placement) {
        if (c == '/') {
            rankIdx++;
            fileIdx = 0;
        } else if (c >= '1' && c <= '8') {
            fileIdx += (c - '0'); // skip empty squares
        } else {
            int x = fileIdx;
            int y = 7 - rankIdx; // FEN rank 8 maps to grid y=7, rank 1 to y=0

            int playerNumber;
            ChessPiece piece;

            switch (c) {
                case 'P': playerNumber = 0; piece = Pawn;   break;
                case 'N': playerNumber = 0; piece = Knight; break;
                case 'B': playerNumber = 0; piece = Bishop; break;
                case 'R': playerNumber = 0; piece = Rook;   break;
                case 'Q': playerNumber = 0; piece = Queen;  break;
                case 'K': playerNumber = 0; piece = King;   break;
                case 'p': playerNumber = 1; piece = Pawn;   break;
                case 'n': playerNumber = 1; piece = Knight; break;
                case 'b': playerNumber = 1; piece = Bishop; break;
                case 'r': playerNumber = 1; piece = Rook;   break;
                case 'q': playerNumber = 1; piece = Queen;  break;
                case 'k': playerNumber = 1; piece = King;   break;
                default: fileIdx++; continue;
            }

            Bit* bit = PieceForPlayer(playerNumber, piece);
            bit->setGameTag(playerNumber == 0 ? piece : 128 + piece);
            ChessSquare* square = _grid->getSquare(x, y);
            square->setBit(bit);
            bit->setParent(square);
            bit->moveTo(square->getPosition());
            fileIdx++;
        }
    }
}

void Chess::onBitPickedUp(Bit& bit, BitHolder& src)
{
    ChessSquare* srcSquare = static_cast<ChessSquare*>(&src);
    int srcIndex = srcSquare->getSquareIndex();
    
    // Get all available moves and highlight destinations for this piece
    std::vector<BitMove> allMoves = generateAllMoves();
    for (const auto& move : allMoves) {
        if (move.from == srcIndex) {
            ChessSquare* dstSquare = _grid->getSquare(move.to % 8, move.to / 8);
            dstSquare->setValidMove(true);
            dstSquare->setHighlighted(true);
        }
    }
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };

    char movedPiece = bit.gameTag() < 128 ? wpieces[bit.gameTag()] : bpieces[bit.gameTag()-128];
    if (movedPiece != '0') {
        _moveList.push_back(movedPiece);
        _moveCount = static_cast<int>(_moveList.size());
    }

    int pieceType = bit.gameTag() & 0x7F;
    int player    = (bit.gameTag() >= 128) ? 1 : 0;
    ChessSquare* srcSq = static_cast<ChessSquare*>(&src);
    ChessSquare* dstSq = static_cast<ChessSquare*>(&dst);

    int srcCol = srcSq->getColumn();
    int dstCol = dstSq->getColumn();
    int srcRow = srcSq->getRow();
    int dstRow = dstSq->getRow();

    if (pieceType == King) {
        // Revoke all castling rights for this player
        _canCastleKingSide[player]  = false;
        _canCastleQueenSide[player] = false;

        // If king moved 2 squares horizontally it was a castle — move the rook too
        if (srcCol == 4 && abs(dstCol - srcCol) == 2) {
            int rookFromCol = (dstCol == 6) ? 7 : 0;
            int rookToCol   = (dstCol == 6) ? 5 : 3;
            ChessSquare* rookSrcSq = _grid->getSquare(rookFromCol, dstRow);
            ChessSquare* rookDstSq = _grid->getSquare(rookToCol,   dstRow);
            Bit* rook = rookSrcSq->bit();
            if (rook) {
                // Change parent BEFORE setBit(nullptr) so BitHolder::setBit
                // won't delete the rook (it checks parent ownership first).
                rook->setParent(rookDstSq);
                rookSrcSq->setBit(nullptr);
                rookDstSq->setBit(rook);
                rook->moveTo(rookDstSq->getPosition());
            }
        }
    } else if (pieceType == Rook) {
        // Revoke castling right for the rook that just moved
        int homeRow = (player == 0) ? 0 : 7;
        if (srcRow == homeRow) {
            if (srcCol == 7) _canCastleKingSide[player]  = false;
            if (srcCol == 0) _canCastleQueenSide[player] = false;
        }
    } else if (pieceType == Pawn) {
        // En passant capture: pawn moved diagonally to the en passant target square
        if (srcCol != dstCol && dstSq->getSquareIndex() == _enPassantSquare) {
            // The captured pawn sits on the same file as dst, same rank as src
            ChessSquare* capturedSq = _grid->getSquare(dstCol, srcRow);
            capturedSq->destroyBit();
        }
    }

    // Update en passant square: set it only on a double pawn push, clear otherwise
    if (pieceType == Pawn && abs(dstRow - srcRow) == 2) {
        _enPassantSquare = ((srcRow + dstRow) / 2) * 8 + srcCol;
    } else {
        _enPassantSquare = -1;
    }

    Game::bitMovedFromTo(bit, src, dst);
}

void Chess::clearBoardHighlights()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->setValidMove(false);
        square->setHighlighted(false);
    });
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

std::array<int, 64> Chess::getBoardArray() const
{
    return buildBoardArray(const_cast<Chess*>(this));
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor == currentPlayer) return true;
    return false;
}

void Chess::buildBitboards(int player, BitboardElement& occupied, BitboardElement& friendly, BitboardElement& enemy)
{
    _grid->forEachSquare([&](ChessSquare* sq, int x, int y) {
        if (sq->bit()) {
            int idx = y * 8 + x;
            occupied |= (1ULL << idx);
            if ((sq->bit()->gameTag() & 128) == (player * 128))
                friendly |= (1ULL << idx);
            else
                enemy |= (1ULL << idx);
        }
    });
}

BitboardElement Chess::getPawnMoves(ChessSquare* src, int player)
{
    BitboardElement occupied(0), friendly(0), enemy(0);
    buildBitboards(player, occupied, friendly, enemy);

    int col = src->getColumn();
    int row = src->getRow();
    int idx = row * 8 + col;

    BitboardElement moves(0);

    if (player == 0) {
        // White pawns move toward increasing row (rank 1 -> rank 8, y=1 -> y=7)
        int pushIdx = idx + 8;
        if (pushIdx < 64 && !(occupied.getData() & (1ULL << pushIdx))) {
            moves |= (1ULL << pushIdx);
            // Double push only from starting row (row 1)
            if (row == 1) {
                int doublePushIdx = idx + 16;
                if (!(occupied.getData() & (1ULL << doublePushIdx))) {
                    moves |= (1ULL << doublePushIdx);
                }
            }
        }
        // Diagonal captures
        if (col > 0 && (enemy.getData() & (1ULL << (idx + 7))))
            moves |= (1ULL << (idx + 7));
        if (col < 7 && (enemy.getData() & (1ULL << (idx + 9))))
            moves |= (1ULL << (idx + 9));
    } else {
        // Black pawns move toward decreasing row (rank 8 -> rank 1, y=6 -> y=0)
        int pushIdx = idx - 8;
        if (pushIdx >= 0 && !(occupied.getData() & (1ULL << pushIdx))) {
            moves |= (1ULL << pushIdx);
            // Double push only from starting row (row 6)
            if (row == 6) {
                int doublePushIdx = idx - 16;
                if (!(occupied.getData() & (1ULL << doublePushIdx))) {
                    moves |= (1ULL << doublePushIdx);
                }
            }
        }
        // Diagonal captures
        if (col > 0 && (enemy.getData() & (1ULL << (idx - 9))))
            moves |= (1ULL << (idx - 9));
        if (col < 7 && (enemy.getData() & (1ULL << (idx - 7))))
            moves |= (1ULL << (idx - 7));
    }

    return moves;
}

BitboardElement Chess::getJumpMoves(ChessSquare* src, int player, const int offsets[][2], int numOffsets)
{
    BitboardElement occupied(0), friendly(0), enemy(0);
    buildBitboards(player, occupied, friendly, enemy);

    int col = src->getColumn();
    int row = src->getRow();

    BitboardElement moves(0);

    for (int i = 0; i < numOffsets; i++) {
        int newCol = col + offsets[i][0];
        int newRow = row + offsets[i][1];
        if (newCol >= 0 && newCol < 8 && newRow >= 0 && newRow < 8) {
            int newIdx = newRow * 8 + newCol;
            if (!(friendly.getData() & (1ULL << newIdx)))
                moves |= (1ULL << newIdx);
        }
    }

    return moves;
}

BitboardElement Chess::getSlidingMoves(ChessSquare* src, int player, const int dirs[][2], int numDirs)
{
    BitboardElement occupied(0), friendly(0), enemy(0);
    buildBitboards(player, occupied, friendly, enemy);

    int col = src->getColumn();
    int row = src->getRow();

    BitboardElement moves(0);

    for (int d = 0; d < numDirs; d++) {
        int dc = dirs[d][0];
        int dr = dirs[d][1];
        int c = col + dc;
        int r = row + dr;

        while (c >= 0 && c < 8 && r >= 0 && r < 8) {
            int idx = r * 8 + c;
            if (friendly.getData() & (1ULL << idx)) break;       // blocked by own piece
            moves |= (1ULL << idx);
            if (occupied.getData() & (1ULL << idx)) break;       // captured enemy — stop ray
            c += dc;
            r += dr;
        }
    }

    return moves;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* srcSquare = static_cast<ChessSquare*>(&src);
    ChessSquare* dstSquare = static_cast<ChessSquare*>(&dst);
    
    int srcIndex = srcSquare->getSquareIndex();
    int dstIndex = dstSquare->getSquareIndex();
    
    // Check if this move is in the list of all available moves
    std::vector<BitMove> allMoves = generateAllMoves();
    for (const auto& move : allMoves) {
        if (move.from == srcIndex && move.to == dstIndex) {
            return true;
        }
    }
    
    return false;
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    int player = getCurrentPlayer()->playerNumber();
    auto board = buildBoardArray(this);
    bool inCheck = isInCheck(board, player);
    bool hasMove = !generateAllMoves().empty();

    if (!hasMove && inCheck) {
        // Checkmate: current player has no legal move and is in check
        return getPlayerAt(1 - player);
    }
    return nullptr;
}

bool Chess::checkForDraw()
{
    int player = getCurrentPlayer()->playerNumber();
    auto board = buildBoardArray(this);
    bool inCheck = isInCheck(board, player);
    bool hasMove = !generateAllMoves().empty();
    return (!hasMove && !inCheck);
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}

std::vector<BitMove> Chess::generateAllMoves()
{
    std::vector<BitMove> allMoves;
    int currentPlayer = getCurrentPlayer()->playerNumber();

    _grid->forEachSquare([&](ChessSquare* srcSquare, int srcX, int srcY) {
        Bit* bit = srcSquare->bit();
        if (!bit) return;

        // Check if this piece belongs to the current player
        int pieceColor = bit->gameTag() & 128;
        int currentPlayerColor = currentPlayer * 128;
        if (pieceColor != currentPlayerColor) return;

        // Get the piece type
        int pieceType = bit->gameTag() & 0x7F;
        BitboardElement moves(0);

        // Generate moves based on piece type
        if (pieceType == Pawn) {
            moves = getPawnMoves(srcSquare, currentPlayer);
        } else if (pieceType == Knight) {
            const int offsets[8][2] = {{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
            moves = getJumpMoves(srcSquare, currentPlayer, offsets, 8);
        } else if (pieceType == King) {
            const int offsets[8][2] = {{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1},{-1,0},{-1,1}};
            moves = getJumpMoves(srcSquare, currentPlayer, offsets, 8);
        } else if (pieceType == Bishop) {
            const int dirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
            moves = getSlidingMoves(srcSquare, currentPlayer, dirs, 4);
        } else if (pieceType == Rook) {
            const int dirs[4][2] = {{0,1},{0,-1},{1,0},{-1,0}};
            moves = getSlidingMoves(srcSquare, currentPlayer, dirs, 4);
        } else if (pieceType == Queen) {
            const int dirs[8][2] = {{0,1},{0,-1},{1,0},{-1,0},{1,1},{1,-1},{-1,1},{-1,-1}};
            moves = getSlidingMoves(srcSquare, currentPlayer, dirs, 8);
        }

        // Convert bitboard moves to BitMove objects
        int srcIndex = srcSquare->getSquareIndex();
        moves.forEachBit([&](int dstIndex) {
            BitMove candidate(srcIndex, dstIndex, static_cast<ChessPiece>(pieceType));
            if (isLegalMove(this, candidate)) {
                allMoves.emplace_back(candidate);
            }
        });

        // En passant capture for pawns
        if (pieceType == Pawn && _enPassantSquare >= 0) {
            int epX = _enPassantSquare % 8;
            int epY = _enPassantSquare / 8;
            int dir = (currentPlayer == 0) ? 1 : -1;
            if (epY == srcY + dir && abs(epX - srcX) == 1) {
                BitMove epMove(srcIndex, _enPassantSquare, Pawn);
                epMove.isEnPassant = true;
                if (isLegalMove(this, epMove)) allMoves.emplace_back(epMove);
            }
        }

        // Generate castling moves for the king
        if (pieceType == King) {
            auto board = buildBoardArray(this);
            int player = currentPlayer;
            int expectedKingSq = (player == 0) ? 4 : 60;

            if (srcIndex == expectedKingSq && !isInCheck(board, player)) {
                int rookTag = (player == 0) ? Rook : 128 + Rook;

                // Kingside: squares between king (e) and rook (h) must be empty; king passes f then lands g
                if (_canCastleKingSide[player]) {
                    int rookSq = (player == 0) ? 7  : 63;
                    int passSq = (player == 0) ? 5  : 61;  // f-file (king passes through)
                    int dstSq  = (player == 0) ? 6  : 62;  // g-file (king lands)
                    if (board[rookSq] == rookTag && board[passSq] == 0 && board[dstSq] == 0 &&
                        !isSquareAttacked(board, passSq, 1 - player)) {
                        BitMove cm(srcIndex, dstSq, King);
                        cm.isCastle = true;
                        if (isLegalMove(this, cm)) allMoves.emplace_back(cm);
                    }
                }

                // Queenside: b, c, d squares empty; king passes d then lands c
                if (_canCastleQueenSide[player]) {
                    int rookSq = (player == 0) ? 0  : 56;
                    int dSq    = (player == 0) ? 3  : 59;  // d-file (king passes through)
                    int dstSq  = (player == 0) ? 2  : 58;  // c-file (king lands)
                    int bSq    = (player == 0) ? 1  : 57;  // b-file (must be empty, rook path)
                    if (board[rookSq] == rookTag && board[dSq] == 0 && board[dstSq] == 0 && board[bSq] == 0 &&
                        !isSquareAttacked(board, dSq, 1 - player)) {
                        BitMove cm(srcIndex, dstSq, King);
                        cm.isCastle = true;
                        if (isLegalMove(this, cm)) allMoves.emplace_back(cm);
                    }
                }
            }
        }
    });

    return allMoves;
}

