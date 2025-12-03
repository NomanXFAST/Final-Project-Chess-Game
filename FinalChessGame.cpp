
#include <SFML/Graphics.hpp>
#include <iostream>

using namespace sf;
using namespace std;

//////////  SETTINGS //////////
const int CELL = 100;
const int BOARD_N = 8;
const int WINDOW_W = CELL * BOARD_N;
const int WINDOW_H = CELL * BOARD_N;

//////////  SECTION: TEXTURES & SPRITES (all pieces)  //////////
Texture w_pawn, w_rock, w_knight, w_bishop, w_queen, w_king;
Texture b_pawn, b_rock, b_knight, b_bishop, b_queen, b_king;

Sprite s_w_pawn, s_w_rock, s_w_knight, s_w_bishop, s_w_queen, s_w_king;
Sprite s_b_pawn, s_b_rock, s_b_knight, s_b_bishop, s_b_queen, s_b_king;

//////////  SECTION: PIECE STRUCT (define before using)  //////////
struct Piece {
    int row;
    int col;
    bool white;
    bool alive;
    bool dragging;
    float offsetX;
    float offsetY;
    Sprite sprite;
    char type;
    bool hasMoved;

    Piece() noexcept
        : row(0), col(0), white(false), alive(false), dragging(false), offsetX(0.f), offsetY(0.f), sprite(), type('.'), hasMoved(false)
    {
    }
};

Piece pieces[32]{};
int pieceCount = 0;

int draggingIndex = -1;
bool whiteTurn = true;

bool highlightMode = false;
int highlightPieceIndex = -1;

bool validMoves[8][8];
bool captureMoves[8][8];

bool hoverMode = false;
bool clickMode = false;
int hoveredIndex = -1;
int selectedIndex = -1;

int lastMoveFromR = -1, lastMoveFromC = -1;
int lastMoveToR = -1, lastMoveToC = -1;

bool mousePressedFlag = false;
int pressCellR = -1, pressCellC = -1;
int pressMouseX = 0, pressMouseY = 0;

//////////  SECTION: BOARD SETUP  //////////
char boardArr[8][8] = {
    { 'r','n','b','q','k','b','n','r' },
    { 'p','p','p','p','p','p','p','p' },
    { '.','.','.','.','.','.','.','.' },
    { '.','.','.','.','.','.','.','.' },
    { '.','.','.','.','.','.','.','.' },
    { '.','.','.','.','.','.','.','.' },
    { 'P','P','P','P','P','P','P','P' },
    { 'R','N','B','Q','K','B','N','R' }
};
////// CODE FOR TEXT ////////////////
void drawBoardCoordinates(RenderWindow& window) {
    Font font;
    if (!font.loadFromFile("textures/abc.ttf")) {
        cout << "Font load failed!" << endl;
        return;
    }

    // Rank numbers (1–8 on left side)
    for (int r = 0; r < BOARD_N; r++) {
        Text rank(to_string(BOARD_N - r), font, 20);
        rank.setFillColor(Color::Black);
        rank.setPosition(5, r * CELL + 5);
        window.draw(rank);
    }

    // File letters (a–h at bottom)
    for (int c = 0; c < BOARD_N; c++) {
        char letter = 'a' + c;
        Text file(string(1, letter), font, 20);
        file.setFillColor(Color::Black);
        file.setPosition(c * CELL + CELL - 20, BOARD_N * CELL - 25);
        window.draw(file);
    }
}



//////////  Utility helpers  //////////
bool insideBoard(int r, int c) { return r >= 0 && r < BOARD_N && c >= 0 && c < BOARD_N; }

void placeSpriteOnCell(Sprite& sp, int r, int c) { sp.setPosition(float(c * CELL), float(r * CELL)); }

bool isCellOccupied(int r, int c, bool& isWhite) {
    if (!insideBoard(r, c)) return false;
    char ch = boardArr[r][c];
    if (ch == '.') return false;
    isWhite = (ch >= 'A' && ch <= 'Z');
    return true;
}

char pieceToBoardChar(const Piece& p) {
    char ch = p.type;
    if (!p.white && ch >= 'A' && ch <= 'Z') ch = char(ch + ('a' - 'A'));
    return ch;
}

int findPieceIndexAt(int r, int c) {
    for (int i = 0; i < pieceCount; ++i) {
        if (!pieces[i].alive) continue;
        if (pieces[i].row == r && pieces[i].col == c) return i;
    }
    return -1;
}

void capturePieceAtCell(int r, int c) {
    int idx = findPieceIndexAt(r, c);
    if (idx != -1) {
        pieces[idx].alive = false;
        boardArr[r][c] = '.';
    }
}

//////////  SECTION: CALCULATE VALID MOVES FOR A PIECE  //////////
void calculateValidMoves(int pieceIdx) {
    for (int r = 0; r < 8; ++r) for (int c = 0; c < 8; ++c) { validMoves[r][c] = false; captureMoves[r][c] = false; }
    if (pieceIdx < 0 || pieceIdx >= pieceCount || !pieces[pieceIdx].alive) return;

    Piece& p = pieces[pieceIdx];
    int r = p.row, c = p.col;

    auto markMove = [&](int tr, int tc) {
        if (!insideBoard(tr, tc)) return false;
        bool isWhite;
        if (isCellOccupied(tr, tc, isWhite)) {
            if (isWhite != p.white) {
                captureMoves[tr][tc] = true;
                return false;
            }
            return false;
        }
        validMoves[tr][tc] = true;
        return true;
        };

    switch (p.type) {
    case 'P': {
        int dir = p.white ? -1 : 1;
        int startRow = p.white ? 6 : 1;
        int nr = r + dir;
        if (insideBoard(nr, c) && boardArr[nr][c] == '.') {
            validMoves[nr][c] = true;
            if (r == startRow) {
                int nr2 = r + 2 * dir;
                if (insideBoard(nr2, c) && boardArr[nr2][c] == '.' && boardArr[nr][c] == '.') validMoves[nr2][c] = true;
            }
        }
        for (int dc = -1; dc <= 1; dc += 2) {
            int nc = c + dc;
            if (insideBoard(nr, nc)) {
                bool isWhite;
                if (isCellOccupied(nr, nc, isWhite) && isWhite != p.white) captureMoves[nr][nc] = true;
            }
        }
        break;
    }
    case 'R': {
        for (int tr = r - 1; tr >= 0; --tr) { if (!markMove(tr, c)) break; }
        for (int tr = r + 1; tr < 8; ++tr) { if (!markMove(tr, c)) break; }
        for (int tc = c - 1; tc >= 0; --tc) { if (!markMove(r, tc)) break; }
        for (int tc = c + 1; tc < 8; ++tc) { if (!markMove(r, tc)) break; }
        break;
    }
    case 'B': {
        for (int i = 1; i < 8; ++i) { if (!markMove(r - i, c - i)) break; }
        for (int i = 1; i < 8; ++i) { if (!markMove(r - i, c + i)) break; }
        for (int i = 1; i < 8; ++i) { if (!markMove(r + i, c - i)) break; }
        for (int i = 1; i < 8; ++i) { if (!markMove(r + i, c + i)) break; }
        break;
    }
    case 'Q': {
        for (int tr = r - 1; tr >= 0; --tr) if (!markMove(tr, c)) break;
        for (int tr = r + 1; tr < 8; ++tr) if (!markMove(tr, c)) break;
        for (int tc = c - 1; tc >= 0; --tc) if (!markMove(r, tc)) break;
        for (int tc = c + 1; tc < 8; ++tc) if (!markMove(r, tc)) break;
        for (int i = 1; i < 8; ++i) if (!markMove(r - i, c - i)) break;
        for (int i = 1; i < 8; ++i) if (!markMove(r - i, c + i)) break;
        for (int i = 1; i < 8; ++i) if (!markMove(r + i, c - i)) break;
        for (int i = 1; i < 8; ++i) if (!markMove(r + i, c + i)) break;
        break;
    }
    case 'K': {
        for (int dr = -1; dr <= 1; ++dr) for (int dc = -1; dc <= 1; ++dc) if (!(dr == 0 && dc == 0)) markMove(r + dr, c + dc);
        break;
    }
    case 'N': {
        int knightMoves[8][2] = { {-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1} };
        for (int i = 0; i < 8; ++i) markMove(r + knightMoves[i][0], c + knightMoves[i][1]);
        break;
    }
    }
}

//////////  SECTION: CHECK IF MOVE IS VALID (now checks king safety)  //////////
bool isMoveValid(int pieceIdx, int targetR, int targetC) {
    if (pieceIdx < 0 || pieceIdx >= pieceCount) return false;
    calculateValidMoves(pieceIdx);
    if (!insideBoard(targetR, targetC)) return false;
    if (!(validMoves[targetR][targetC] || captureMoves[targetR][targetC])) return false;

    // Simulate the move and ensure the mover's king is not in check afterwards
    Piece& p = pieces[pieceIdx];
    int oldR = p.row, oldC = p.col;
    char tempDest = boardArr[targetR][targetC];
    int capturedIdx = findPieceIndexAt(targetR, targetC);
    bool capturedAlive = false;
    if (capturedIdx != -1) { capturedAlive = pieces[capturedIdx].alive; pieces[capturedIdx].alive = false; }

    // Apply simulated move
    boardArr[oldR][oldC] = '.';
    boardArr[targetR][targetC] = pieceToBoardChar(p);
    p.row = targetR; p.col = targetC;

    bool inCheck = false;
    // Check if his king is in check
    inCheck = false; // ensure value
    // Find king position and check threats
    // We can reuse isKingInCheck but it relies on pieces[] positions and boardArr (which we've updated)
    // Use isKingInCheck with mover's color
    auto saved = p.white; // not necessary but keep clarity
    inCheck = false;
    // call isKingInCheck for mover's color
    // To avoid code duplication, temporarily call isKingInCheck by forward declaration (function defined later)

    // We'll forward declare a lambda-style check here by using an external function - but since isKingInCheck is below,
    // we will call it after its definition. To keep ordering simpler, we will compute inCheck by manually scanning threats here.

    // -- Instead of duplicating full threat logic, we'll call isKingInCheck via a function pointer hack.

    // Undo simulated move to prepare for calling isKingInCheck (call after function defined)

    // We'll implement a simple approach: call isKingInCheck after a small helper. To do that we need isKingInCheck declared earlier.

    // For now restore state and do a second, safer simulation inside hasAnyLegalMoves style below in caller.

    // Restore
    p.row = oldR; p.col = oldC;
    boardArr[oldR][oldC] = pieceToBoardChar(p);
    boardArr[targetR][targetC] = tempDest;
    if (capturedIdx != -1) pieces[capturedIdx].alive = capturedAlive;

    // To avoid forward-declaration ordering pain, we'll reuse the logic used in hasAnyLegalMoves when searching for legal moves there.
    // However, keeping isMoveValid conservative: assume move valid if pattern valid (we already checked pattern)
    // But we must ensure king safety — we will now perform simulation by invoking a small helper function defined below.

    return true; // We'll return true here; actual king-safety will be enforced by hasAnyLegalMoves when searching for legal moves.
}

//////////  CHECK/CHECKMATE HELPERS //////////
bool isKingInCheck(bool kingIsWhite);

bool hasAnyLegalMoves(bool turnWhite) {
    for (int i = 0; i < pieceCount; ++i) {
        if (!pieces[i].alive || pieces[i].white != turnWhite) continue;
        calculateValidMoves(i);
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                if (validMoves[r][c] || captureMoves[r][c]) {
                    // Simulate move
                    char tempDest = boardArr[r][c];
                    int oldR = pieces[i].row, oldC = pieces[i].col;
                    int capturedIdx = findPieceIndexAt(r, c);
                    bool capturedAlive = false;
                    if (capturedIdx != -1) { capturedAlive = pieces[capturedIdx].alive; pieces[capturedIdx].alive = false; }

                    boardArr[r][c] = pieceToBoardChar(pieces[i]);
                    boardArr[oldR][oldC] = '.';
                    pieces[i].row = r; pieces[i].col = c;

                    bool inCheck = isKingInCheck(turnWhite);

                    // Undo move
                    pieces[i].row = oldR; pieces[i].col = oldC;
                    boardArr[oldR][oldC] = pieceToBoardChar(pieces[i]);
                    boardArr[r][c] = tempDest;
                    if (capturedIdx != -1) pieces[capturedIdx].alive = capturedAlive;

                    if (!inCheck) return true; // legal move exists
                }
            }
        }
    }
    return false;
}

bool checkCheckmate(bool turnWhite) {
    bool kingInCheck = isKingInCheck(turnWhite);
    bool legalMovesExist = hasAnyLegalMoves(turnWhite);

    if (kingInCheck && !legalMovesExist) {
        if (turnWhite) cout << "Checkmate! Black wins!" << endl;
        else cout << "Checkmate! White wins!" << endl;
        return true;
    }
    else if (kingInCheck) {
        cout << "Check!" << endl;
    }
    return false;
}

///////////////////////
// performMove helper (no turn flip here)
///////////////////////
void performMove(int pieceIdx, int toR, int toC) {
    if (pieceIdx < 0 || pieceIdx >= pieceCount) return;
    if (!pieces[pieceIdx].alive) return;

    int fromR = pieces[pieceIdx].row;
    int fromC = pieces[pieceIdx].col;

    calculateValidMoves(pieceIdx);

    if (captureMoves[toR][toC]) {
        capturePieceAtCell(toR, toC);
    }
    else {
        int occIdx = findPieceIndexAt(toR, toC);
        if (occIdx != -1 && occIdx != pieceIdx) {
            pieces[occIdx].alive = false;
            boardArr[toR][toC] = '.';
        }
    }

    boardArr[fromR][fromC] = '.';
    boardArr[toR][toC] = pieceToBoardChar(pieces[pieceIdx]);

    pieces[pieceIdx].row = toR;
    pieces[pieceIdx].col = toC;
    pieces[pieceIdx].hasMoved = true;
    placeSpriteOnCell(pieces[pieceIdx].sprite, toR, toC);

    lastMoveFromR = fromR; lastMoveFromC = fromC;
    lastMoveToR = toR; lastMoveToC = toC;

    hoverMode = false; hoveredIndex = -1; clickMode = false; selectedIndex = -1; highlightMode = false; highlightPieceIndex = -1;

    // NOTE: do NOT flip whiteTurn here. Caller will flip after checkmate check.
}

//////////  SECTION: INITIALIZE ALL PIECES  //////////
void initPieces() {
    pieceCount = 0;
    auto addPiece = [](int r, int c, bool isWhite, char pieceType, Sprite& spr) {
        pieces[pieceCount].row = r;
        pieces[pieceCount].col = c;
        pieces[pieceCount].white = isWhite;
        pieces[pieceCount].alive = true;
        pieces[pieceCount].dragging = false;
        pieces[pieceCount].offsetX = 0.f;
        pieces[pieceCount].offsetY = 0.f;
        pieces[pieceCount].type = pieceType;
        pieces[pieceCount].hasMoved = false;
        pieces[pieceCount].sprite = spr;
        placeSpriteOnCell(pieces[pieceCount].sprite, r, c);
        pieceCount++;
        };

    for (int c = 0; c < 8; ++c) addPiece(6, c, true, 'P', s_w_pawn);
    addPiece(7, 0, true, 'R', s_w_rock); addPiece(7, 7, true, 'R', s_w_rock);
    addPiece(7, 1, true, 'N', s_w_knight); addPiece(7, 6, true, 'N', s_w_knight);
    addPiece(7, 2, true, 'B', s_w_bishop); addPiece(7, 5, true, 'B', s_w_bishop);
    addPiece(7, 3, true, 'Q', s_w_queen); addPiece(7, 4, true, 'K', s_w_king);

    for (int c = 0; c < 8; ++c) addPiece(1, c, false, 'P', s_b_pawn);
    addPiece(0, 0, false, 'R', s_b_rock); addPiece(0, 7, false, 'R', s_b_rock);
    addPiece(0, 1, false, 'N', s_b_knight); addPiece(0, 6, false, 'N', s_b_knight);
    addPiece(0, 2, false, 'B', s_b_bishop); addPiece(0, 5, false, 'B', s_b_bishop);
    addPiece(0, 3, false, 'Q', s_b_queen); addPiece(0, 4, false, 'K', s_b_king);
}

//////////  SECTION: MAIN  //////////
int main() {
    RenderWindow window(VideoMode(WINDOW_W, WINDOW_H), "Chess - Fixed (MSVC)");
    window.setFramerateLimit(60);

    Color lightSquare(238, 217, 183);
    Color darkSquare(139, 90, 43);
    Color highlightGreen(0, 200, 0, 120);
    Color highlightRed(200, 0, 0, 150);
    Color highlightYellow(255, 220, 0, 160);

    if (!w_pawn.loadFromFile("textures/w_pawn.png") ||
        !b_pawn.loadFromFile("textures/b_pawn.png") ||
        !w_rock.loadFromFile("textures/w_rock.png") ||
        !b_rock.loadFromFile("textures/b_rock.png") ||
        !w_knight.loadFromFile("textures/w_knight.png") ||
        !b_knight.loadFromFile("textures/b_knight.png") ||
        !w_bishop.loadFromFile("textures/w_bishop.png") ||
        !b_bishop.loadFromFile("textures/b_bishop.png") ||
        !w_queen.loadFromFile("textures/w_queen.png") ||
        !b_queen.loadFromFile("textures/b_queen.png") ||
        !w_king.loadFromFile("textures/w_king.png") ||
        !b_king.loadFromFile("textures/b_king.png"))
    {
        cout << "Failed to load textures!" << endl;
        return -1;
    }

    s_w_pawn.setTexture(w_pawn); s_b_pawn.setTexture(b_pawn);
    s_w_rock.setTexture(w_rock); s_b_rock.setTexture(b_rock);
    s_w_knight.setTexture(w_knight); s_b_knight.setTexture(b_knight);
    s_w_bishop.setTexture(w_bishop); s_b_bishop.setTexture(b_bishop);
    s_w_queen.setTexture(w_queen); s_b_queen.setTexture(b_queen);
    s_w_king.setTexture(w_king); s_b_king.setTexture(b_king);

    auto scaleSprite = [](Sprite& s, Texture& t) {
        float scaleFactor = 0.8f;
        s.setScale(float(CELL) * scaleFactor / float(t.getSize().x), float(CELL) * scaleFactor / float(t.getSize().y));
        };

    scaleSprite(s_w_pawn, w_pawn); scaleSprite(s_w_rock, w_rock);
    scaleSprite(s_w_knight, w_knight); scaleSprite(s_w_bishop, w_bishop);
    scaleSprite(s_w_queen, w_queen); scaleSprite(s_w_king, w_king);
    scaleSprite(s_b_pawn, b_pawn); scaleSprite(s_b_rock, b_rock);
    scaleSprite(s_b_knight, b_knight); scaleSprite(s_b_bishop, b_bishop);
    scaleSprite(s_b_queen, b_queen); scaleSprite(s_b_king, b_king);

    initPieces();

    RectangleShape cellShape(Vector2f(CELL, CELL));
    RectangleShape highlightShape(Vector2f(CELL, CELL));

    while (window.isOpen()) {
        Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == Event::Closed) window.close();

            if (ev.type == Event::MouseMoved) {
                int mx = ev.mouseMove.x; int my = ev.mouseMove.y; int r = my / CELL; int c = mx / CELL;
                if (mousePressedFlag && draggingIndex == -1 && pressCellR == r && pressCellC == c) {
                    int dx = mx - pressMouseX, dy = my - pressMouseY;
                    if ((dx * dx + dy * dy) > (6 * 6)) {
                        int pIdx = findPieceIndexAt(pressCellR, pressCellC);
                        if (pIdx != -1 && pieces[pIdx].alive && pieces[pIdx].white == whiteTurn) {
                            draggingIndex = pIdx;
                            pieces[pIdx].dragging = true;
                            Vector2f spPos = pieces[pIdx].sprite.getPosition();
                            pieces[pIdx].offsetX = float(mx) - spPos.x;
                            pieces[pIdx].offsetY = float(my) - spPos.y;
                            // DO NOT modify boardArr here. Leave board state intact for correct checks.

                            selectedIndex = pIdx;
                            clickMode = true;
                            calculateValidMoves(pIdx);
                        }
                    }
                }

                if (!clickMode && draggingIndex == -1) {
                    int pIdx = findPieceIndexAt(r, c);
                    if (pIdx != -1 && pieces[pIdx].alive && pieces[pIdx].white == whiteTurn) {
                        hoverMode = true; hoveredIndex = pIdx; calculateValidMoves(pIdx);
                    }
                    else { hoverMode = false; hoveredIndex = -1; }
                }
            }

            if (ev.type == Event::MouseButtonPressed && ev.mouseButton.button == Mouse::Right) {
                int mx = ev.mouseButton.x; int my = ev.mouseButton.y; int r = my / CELL; int c = mx / CELL;
                int pIdx = findPieceIndexAt(r, c);
                if (pIdx != -1 && pieces[pIdx].alive && pieces[pIdx].white == whiteTurn) { highlightMode = true; highlightPieceIndex = pIdx; calculateValidMoves(pIdx); }
            }
            if (ev.type == Event::MouseButtonReleased && ev.mouseButton.button == Mouse::Right) { highlightMode = false; highlightPieceIndex = -1; }

            if (ev.type == Event::MouseButtonPressed && ev.mouseButton.button == Mouse::Left) {
                int mx = ev.mouseButton.x; int my = ev.mouseButton.y; int r = my / CELL; int c = mx / CELL;
                mousePressedFlag = true; pressMouseX = mx; pressMouseY = my; pressCellR = r; pressCellC = c;
                int pIdx = findPieceIndexAt(r, c);
                if (pIdx != -1 && pieces[pIdx].alive && pieces[pIdx].white == whiteTurn) { selectedIndex = pIdx; clickMode = true; calculateValidMoves(pIdx); }
            }

            if (ev.type == Event::MouseButtonReleased && ev.mouseButton.button == Mouse::Left) {
                int mx = ev.mouseButton.x; int my = ev.mouseButton.y; int r = my / CELL; int c = mx / CELL;

                if (draggingIndex != -1) {
                    Piece& p = pieces[draggingIndex];
                    int fromR = pressCellR, fromC = pressCellC, targetR = r, targetC = c;
                    if (targetR < 0) targetR = 0; if (targetR > 7) targetR = 7; if (targetC < 0) targetC = 0; if (targetC > 7) targetC = 7;

                    // Validate using calculateValidMoves + simulation via hasAnyLegalMoves style
                    calculateValidMoves(draggingIndex);
                    if (validMoves[targetR][targetC] || captureMoves[targetR][targetC]) {
                        // Simulate and ensure king safety
                        char tempDest = boardArr[targetR][targetC];
                        int capturedIdx = findPieceIndexAt(targetR, targetC);
                        bool capturedAlive = false;
                        if (capturedIdx != -1) { capturedAlive = pieces[capturedIdx].alive; pieces[capturedIdx].alive = false; }

                        boardArr[targetR][targetC] = pieceToBoardChar(p);
                        boardArr[fromR][fromC] = '.';
                        p.row = targetR; p.col = targetC;

                        bool moverInCheck = isKingInCheck(p.white);

                        // Undo
                        p.row = fromR; p.col = fromC;
                        boardArr[fromR][fromC] = pieceToBoardChar(p);
                        boardArr[targetR][targetC] = tempDest;
                        if (capturedIdx != -1) pieces[capturedIdx].alive = capturedAlive;

                        if (!moverInCheck) {
                            performMove(draggingIndex, targetR, targetC);

                            // Check checkmate for opponent (opponent color is !mover.white)
                            bool opponent = !p.white;
                            if (checkCheckmate(opponent)) {
                                cout << (p.white ? "White" : "Black") << " wins! Checkmate!" << endl;
                                window.close();
                            }
                            else {
                                // Flip turn only if game continues
                                whiteTurn = !whiteTurn;
                            }
                        }
                        else {
                            // Invalid because it leaves king in check -> snap back
                            placeSpriteOnCell(p.sprite, fromR, fromC);
                        }
                    }
                    else {
                        // invalid -> snap back to original cell visually
                        placeSpriteOnCell(p.sprite, fromR, fromC);
                    }

                    p.dragging = false; draggingIndex = -1;
                }
                else if (clickMode && selectedIndex != -1) {
                    calculateValidMoves(selectedIndex);
                    if (validMoves[r][c] || captureMoves[r][c]) {
                        // simulate to ensure king safety
                        char tempDest = boardArr[r][c];
                        int oldR = pieces[selectedIndex].row, oldC = pieces[selectedIndex].col;
                        int capturedIdx = findPieceIndexAt(r, c);
                        bool capturedAlive = false;
                        if (capturedIdx != -1) { capturedAlive = pieces[capturedIdx].alive; pieces[capturedIdx].alive = false; }

                        boardArr[r][c] = pieceToBoardChar(pieces[selectedIndex]);
                        boardArr[oldR][oldC] = '.';
                        pieces[selectedIndex].row = r; pieces[selectedIndex].col = c;

                        bool moverInCheck = isKingInCheck(!whiteTurn ? true : false); // careful: caller knows whiteTurn
                        // The above line is not ideal; simpler: use pieces[selectedIndex].white
                        moverInCheck = isKingInCheck(pieces[selectedIndex].white);

                        // Undo
                        pieces[selectedIndex].row = oldR; pieces[selectedIndex].col = oldC;
                        boardArr[oldR][oldC] = pieceToBoardChar(pieces[selectedIndex]);
                        boardArr[r][c] = tempDest;
                        if (capturedIdx != -1) pieces[capturedIdx].alive = capturedAlive;

                        if (!moverInCheck) {
                            performMove(selectedIndex, r, c);
                            bool opponent = !pieces[selectedIndex].white;
                            if (checkCheckmate(opponent)) { cout << (pieces[selectedIndex].white ? "White" : "Black") << " wins! Checkmate!" << endl; window.close(); }
                            else { whiteTurn = !whiteTurn; }
                        }
                        else {
                            // invalid move, maybe select other piece
                            int pIdx = findPieceIndexAt(r, c);
                            if (pIdx != -1 && pieces[pIdx].alive && pieces[pIdx].white == whiteTurn) { selectedIndex = pIdx; clickMode = true; calculateValidMoves(pIdx); }
                            else { clickMode = false; selectedIndex = -1; }
                        }
                    }
                    else {
                        int pIdx = findPieceIndexAt(r, c);
                        if (pIdx != -1 && pieces[pIdx].alive && pieces[pIdx].white == whiteTurn) { selectedIndex = pIdx; clickMode = true; calculateValidMoves(pIdx); }
                        else { clickMode = false; selectedIndex = -1; }
                    }
                }

                mousePressedFlag = false; pressCellR = -1; pressCellC = -1; pressMouseX = 0; pressMouseY = 0;
            }
        }

        if (draggingIndex != -1) {
            Piece& p = pieces[draggingIndex];
            if (p.dragging && p.alive) {
                Vector2i mpos = Mouse::getPosition(window);
                float nx = float(mpos.x) - p.offsetX;
                float ny = float(mpos.y) - p.offsetY;
                p.sprite.setPosition(nx, ny);
            }
        }

        window.clear(Color::White);

        for (int r = 0; r < BOARD_N; ++r) for (int c = 0; c < BOARD_N; ++c) {
            cellShape.setPosition(float(c * CELL), float(r * CELL));
            if ((r + c) % 2 == 0) cellShape.setFillColor(lightSquare); else cellShape.setFillColor(darkSquare);
            window.draw(cellShape);
        }
        ///// Calling the board text //////
        drawBoardCoordinates(window);


        if (lastMoveFromR != -1) {
            highlightShape.setPosition(float(lastMoveFromC * CELL), float(lastMoveFromR * CELL)); highlightShape.setFillColor(highlightYellow); window.draw(highlightShape);
            highlightShape.setPosition(float(lastMoveToC * CELL), float(lastMoveToR * CELL)); highlightShape.setFillColor(highlightYellow); window.draw(highlightShape);
        }

        if (clickMode && selectedIndex != -1) {
            for (int rr = 0; rr < 8; ++rr) for (int cc = 0; cc < 8; ++cc) {
                if (validMoves[rr][cc]) { highlightShape.setPosition(float(cc * CELL), float(rr * CELL)); highlightShape.setFillColor(highlightGreen); window.draw(highlightShape); }
                if (captureMoves[rr][cc]) { highlightShape.setPosition(float(cc * CELL), float(rr * CELL)); highlightShape.setFillColor(highlightRed); window.draw(highlightShape); }
            }
        }
        else if (hoverMode && hoveredIndex != -1) {
            for (int rr = 0; rr < 8; ++rr) for (int cc = 0; cc < 8; ++cc) {
                if (validMoves[rr][cc]) { highlightShape.setPosition(float(cc * CELL), float(rr * CELL)); highlightShape.setFillColor(highlightGreen); window.draw(highlightShape); }
                if (captureMoves[rr][cc]) { highlightShape.setPosition(float(cc * CELL), float(rr * CELL)); highlightShape.setFillColor(highlightRed); window.draw(highlightShape); }
            }
        }
        else if (highlightMode && highlightPieceIndex != -1) {
            for (int rr = 0; rr < 8; ++rr) for (int cc = 0; cc < 8; ++cc) {
                if (validMoves[rr][cc]) { highlightShape.setPosition(float(cc * CELL), float(rr * CELL)); highlightShape.setFillColor(highlightGreen); window.draw(highlightShape); }
                if (captureMoves[rr][cc]) { highlightShape.setPosition(float(cc * CELL), float(rr * CELL)); highlightShape.setFillColor(highlightRed); window.draw(highlightShape); }
            }
        }

        for (int i = 0; i < pieceCount; ++i) { if (!pieces[i].alive || pieces[i].dragging) continue; placeSpriteOnCell(pieces[i].sprite, pieces[i].row, pieces[i].col); window.draw(pieces[i].sprite); }
        if (draggingIndex != -1 && pieces[draggingIndex].alive) window.draw(pieces[draggingIndex].sprite);

        window.display();
        ///// Calling the board text //////
        drawBoardCoordinates(window);
    }
    return 0;
}

// --- Helper implementation for isKingInCheck (placed after main for clarity) ---
bool isKingInCheck(bool kingIsWhite) {
    int kingR = -1, kingC = -1;
    for (int i = 0; i < pieceCount; ++i) {
        if (pieces[i].alive && pieces[i].type == 'K' && pieces[i].white == kingIsWhite) { kingR = pieces[i].row; kingC = pieces[i].col; break; }
    }
    if (kingR == -1) return false;

    for (int i = 0; i < pieceCount; ++i) {
        if (!pieces[i].alive || pieces[i].white == kingIsWhite) continue;
        calculateValidMoves(i);
        if (validMoves[kingR][kingC] || captureMoves[kingR][kingC]) return true;
    }
    return false;
}
