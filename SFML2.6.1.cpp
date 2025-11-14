// tic_tac_toe_neon_complete.cpp
// g++ tic_tac_toe_neon_complete.cpp -lsfml-graphics -lsfml-window -lsfml-system -std=c++17 -O2

#include <SFML/Graphics.hpp>
#include <array>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <string>
#include <iostream>

using namespace sf;
using namespace std;

constexpr int WINDOW_W = 600;
constexpr int WINDOW_H = 720;
constexpr int BOARD_SIZE = 3;
constexpr int GAP = 14;
constexpr int BOARD_PIX = WINDOW_W;
constexpr int CELL_PIX = (BOARD_PIX - (BOARD_SIZE + 1) * GAP) / BOARD_SIZE;
constexpr int BOARD_TOP = 120;
constexpr int FOOTER_H = WINDOW_H - (BOARD_TOP + GAP + BOARD_SIZE * CELL_PIX + GAP);

enum class Piece { Empty = 0, X = 1, O = 2 };
enum class Difficulty { Easy = 1, Medium = 2, Hard = 3 };

static mt19937 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());

struct Game {
    array<Piece, 9> board{};
    string player1Name = "Player 1";
    string player2Name = "Player 2";
    Piece player1Piece = Piece::X;
    Piece player2Piece = Piece::O;
    Piece turnPiece = Piece::X;           // whose piece is the currently active turn
    bool currentTurnIsAI = false;         // true when it's AI's turn
    bool finished = false;
    Piece winner = Piece::Empty;
    vector<int> winLine;
    Difficulty difficulty = Difficulty::Medium;
    bool humanVsAI = true;
    bool playerFirst = true;
    Game() { board.fill(Piece::Empty); }
    void reset() {
        board.fill(Piece::Empty);
        finished = false;
        winner = Piece::Empty;
        winLine.clear();
        // set turnPiece and currentTurnIsAI according to playerFirst and symbol assignment
        if (playerFirst) {
            turnPiece = player1Piece;
            currentTurnIsAI = (player1Piece == player2Piece) ? false : false; // playerFirst => player's turn
        }
        else {
            turnPiece = player2Piece;
            currentTurnIsAI = humanVsAI; // if AI goes first and it's vs AI -> true
        }
        // More explicit:
        if (playerFirst) { turnPiece = player1Piece; currentTurnIsAI = false; }
        else { turnPiece = player2Piece; currentTurnIsAI = humanVsAI; }
    }
};

// board helpers
inline int idx(int r, int c) { return r * BOARD_SIZE + c; }
inline int rowOf(int i) { return i / BOARD_SIZE; }
inline int colOf(int i) { return i % BOARD_SIZE; }
static const int LINES[8][3] = {
    {0,1,2},{3,4,5},{6,7,8},
    {0,3,6},{1,4,7},{2,5,8},
    {0,4,8},{2,4,6}
};
bool isBoardFull(const array<Piece, 9>& b) { for (auto& p : b) if (p == Piece::Empty) return false; return true; }
vector<int> emptyIndices(const array<Piece, 9>& b) { vector<int> r; for (int i = 0; i < 9; i++) if (b[i] == Piece::Empty) r.push_back(i); return r; }

void checkFinish(Game& g) {
    g.finished = false; g.winner = Piece::Empty; g.winLine.clear();
    for (auto& line : LINES) {
        Piece a = g.board[line[0]];
        if (a == Piece::Empty) continue;
        if (a == g.board[line[1]] && a == g.board[line[2]]) {
            g.finished = true; g.winner = a; g.winLine = { line[0], line[1], line[2] }; return;
        }
    }
    if (isBoardFull(g.board)) { g.finished = true; g.winner = Piece::Empty; }
}

// ---------------- AI ----------------
int easyAIMove(const Game& g) {
    auto e = emptyIndices(g.board);
    if (e.empty()) return -1;
    uniform_int_distribution<int> d(0, (int)e.size() - 1);
    return e[d(rng)];
}

int evaluateBoard(const array<Piece, 9>& b, Piece aiPiece) {
    for (auto& line : LINES) {
        Piece a = b[line[0]];
        if (a == Piece::Empty) continue;
        if (a == b[line[1]] && a == b[line[2]]) return (a == aiPiece) ? 10 : -10;
    }
    return 0;
}

int minimax(array<Piece, 9>& b, bool maxing, int alpha, int beta, Piece aiPiece) {
    Piece humanPiece = (aiPiece == Piece::X) ? Piece::O : Piece::X;
    int score = evaluateBoard(b, aiPiece);
    if (score == 10 || score == -10) return score;
    if (isBoardFull(b)) return 0;
    if (maxing) {
        int best = -10000;
        for (int i = 0; i < 9; ++i) if (b[i] == Piece::Empty) {
            b[i] = aiPiece;
            best = max(best, minimax(b, false, alpha, beta, aiPiece));
            b[i] = Piece::Empty;
            alpha = max(alpha, best);
            if (beta <= alpha) break;
        }
        return best;
    }
    else {
        int best = 10000;
        for (int i = 0; i < 9; ++i) if (b[i] == Piece::Empty) {
            b[i] = humanPiece;
            best = min(best, minimax(b, true, alpha, beta, aiPiece));
            b[i] = Piece::Empty;
            beta = min(beta, best);
            if (beta <= alpha) break;
        }
        return best;
    }
}

int hardAIMove(Game& g) {
    Piece aiPiece = g.player2Piece; // AI always assigned to player2 in our flow
    int bestVal = -10000, bestIdx = -1;
    for (int i = 0; i < 9; ++i) if (g.board[i] == Piece::Empty) {
        g.board[i] = aiPiece;
        int val = minimax(g.board, false, -10000, 10000, aiPiece);
        g.board[i] = Piece::Empty;
        if (val > bestVal) { bestVal = val; bestIdx = i; }
    }
    if (bestIdx == -1) {
        auto e = emptyIndices(g.board);
        if (e.empty()) return -1;
        uniform_int_distribution<int> d(0, (int)e.size() - 1);
        return e[d(rng)];
    }
    return bestIdx;
}

int mediumAIMove(Game& g) {
    float r = uniform_real_distribution<float>(0.f, 1.f)(rng);
    if (r > 0.4f) return hardAIMove(g);
    return easyAIMove(g);
}

int chooseAIMove(Game& g) {
    if (g.finished) return -1;
    switch (g.difficulty) {
    case Difficulty::Easy: return easyAIMove(g);
    case Difficulty::Medium: return mediumAIMove(g);
    case Difficulty::Hard: return hardAIMove(g);
    default: return easyAIMove(g);
    }
}

// ---------------- UI Helpers ----------------
struct Button {
    RectangleShape box;
    Color baseColor, glowColor;
    bool hovered = false;
    int id = 0;
    string label;
    Button() = default;
    Button(float x, float y, float w, float h, Color base, Color glow, int _id, const string& text = "")
        : baseColor(base), glowColor(glow), id(_id), label(text)
    {
        box.setSize(Vector2f(w, h));
        box.setOrigin(w / 2.f, h / 2.f);
        box.setPosition(x, y);
        box.setFillColor(base);
        box.setOutlineThickness(4.f);
        box.setOutlineColor(glow);
    }
    void draw(RenderWindow& win, float t, const Font& font, int charSize = 18) {
        int alpha = 90 + int(70 * abs(sin(t * 3.5f)));
        if (hovered) box.setOutlineColor(Color(glowColor.r, glowColor.g, glowColor.b, alpha));
        else box.setOutlineColor(glowColor);
        win.draw(box);
        if (!label.empty()) {
            Text txt(label, font, charSize);
            txt.setStyle(Text::Bold);
            FloatRect tb = txt.getLocalBounds();
            txt.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
            txt.setPosition(box.getPosition());
            win.draw(txt);
        }
    }
    bool contains(Vector2i mp) const { return box.getGlobalBounds().contains(Vector2f((float)mp.x, (float)mp.y)); }
};

bool loadPreferredFont(Font& font) {
    const vector<string> cand = {
        "arial.ttf", "Arial.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "/usr/share/fonts/truetype/msttcorefonts/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
    };
    for (auto& p : cand) if (font.loadFromFile(p)) return true;
    return false;
}

// drawing board pieces
Vector2f cellTopLeft(int index) {
    int r = rowOf(index), c = colOf(index);
    float x = GAP + c * (CELL_PIX + GAP);
    float y = BOARD_TOP + GAP + r * (CELL_PIX + GAP);
    return { x, y };
}

int mousePosToIndex(const Vector2i& mp) {
    float boardLeft = 0.f, boardTop = BOARD_TOP + GAP;
    if (mp.x < boardLeft + GAP || mp.x >= boardLeft + GAP + BOARD_SIZE * (CELL_PIX + GAP)) return -1;
    if (mp.y < boardTop || mp.y >= boardTop + BOARD_SIZE * (CELL_PIX + GAP)) return -1;
    for (int r = 0; r < BOARD_SIZE; r++) for (int c = 0; c < BOARD_SIZE; c++) {
        float x = GAP + c * (CELL_PIX + GAP);
        float y = BOARD_TOP + GAP + r * (CELL_PIX + GAP);
        if (FloatRect(x, y, CELL_PIX, CELL_PIX).contains((float)mp.x, (float)mp.y)) return idx(r, c);
    }
    return -1;
}

void drawCellBackground(RenderWindow& win, float x, float y, float w, float h, const Color& fill, const Color& outline, float outlineThickness = 3.f) {
    RectangleShape rect(Vector2f(w, h));
    rect.setPosition(x, y);
    rect.setFillColor(fill);
    rect.setOutlineThickness(outlineThickness);
    rect.setOutlineColor(outline);
    win.draw(rect);
}

void drawX(RenderWindow& win, float cx, float cy, float size, Color color, float time, bool pulse) {
    if (pulse) {
        int alpha = 100 + int(120 * abs(sin(time * 4.f)));
        RectangleShape g1(Vector2f(size, 16)), g2(Vector2f(size, 16));
        g1.setOrigin(size / 2, 8); g2.setOrigin(size / 2, 8); g1.setPosition(cx, cy); g2.setPosition(cx, cy);
        g1.setRotation(45); g2.setRotation(-45);
        g1.setFillColor(Color(color.r, color.g, color.b, alpha));
        g2.setFillColor(Color(color.r, color.g, color.b, alpha));
        win.draw(g1); win.draw(g2);
    }
    RectangleShape r1(Vector2f(size, 10)), r2(Vector2f(size, 10));
    r1.setOrigin(size / 2, 5); r2.setOrigin(size / 2, 5); r1.setPosition(cx, cy); r2.setPosition(cx, cy);
    r1.setRotation(45); r2.setRotation(-45);
    r1.setFillColor(color); r2.setFillColor(color);
    win.draw(r1); win.draw(r2);
}

void drawO(RenderWindow& win, float cx, float cy, float size, Color color, float time, bool pulse) {
    if (pulse) {
        CircleShape glow(size / 2 + 8);
        glow.setOrigin(size / 2 + 8, size / 2 + 8);
        glow.setPosition(cx, cy);
        int alpha = 100 + int(110 * abs(sin(time * 4.f)));
        glow.setFillColor(Color(color.r, color.g, color.b, alpha));
        win.draw(glow);
    }
    CircleShape circ(size / 2 - 8);
    circ.setOrigin(size / 2 - 8, size / 2 - 8);
    circ.setPosition(cx, cy);
    circ.setFillColor(Color::Transparent);
    circ.setOutlineThickness(10);
    circ.setOutlineColor(color);
    win.draw(circ);
}

void renderBoard(RenderWindow& win, const Game& g, float time, const Font& font) {
    RectangleShape bg(Vector2f(WINDOW_W, WINDOW_H)); bg.setFillColor(Color(28, 30, 40)); win.draw(bg);
    for (int i = 0; i < 9; i++) {
        Vector2f tl = cellTopLeft(i);
        drawCellBackground(win, tl.x, tl.y, (float)CELL_PIX, (float)CELL_PIX, Color(20, 22, 28), Color(68, 76, 90), 3.f);
        float cx = tl.x + CELL_PIX / 2.f, cy = tl.y + CELL_PIX / 2.f;
        bool pulse = (g.finished && find(g.winLine.begin(), g.winLine.end(), i) != g.winLine.end());
        if (g.board[i] == Piece::X) drawX(win, cx, cy, CELL_PIX * 0.6f, Color(255, 120, 110), time, pulse);
        else if (g.board[i] == Piece::O) drawO(win, cx, cy, CELL_PIX * 0.6f, Color(110, 190, 255), time, pulse);
    }
    RectangleShape footer(Vector2f(WINDOW_W, FOOTER_H));
    footer.setPosition(0, BOARD_TOP + GAP + BOARD_SIZE * (CELL_PIX + GAP));
    footer.setFillColor(Color(18, 20, 26));
    win.draw(footer);

    // If finished draw result text in footer
    Text res("", font, 20);
    res.setFillColor(Color::White);
    if (g.finished) {
        if (g.winner == Piece::Empty) res.setString("Draw - Click Restart to play again");
        else {
            string name = (g.winner == g.player1Piece) ? g.player1Name : g.player2Name;
            res.setString(name + " wins! Click Restart to play again");
        }
        FloatRect r = res.getLocalBounds();
        res.setPosition((WINDOW_W - r.width) / 2.f, footer.getPosition().y + 10);
        win.draw(res);
    }
}

// ---------------- Menus (blocking loops) ----------------
int playerTypeMenu(RenderWindow& win, const Font& font) {
    vector<Button> btns;
    btns.emplace_back(WINDOW_W / 2.f, WINDOW_H / 2.f - 60, 300, 90, Color(30, 30, 40), Color(0, 180, 255), 1, "Human vs Human");
    btns.emplace_back(WINDOW_W / 2.f, WINDOW_H / 2.f + 60, 300, 90, Color(30, 30, 40), Color(255, 100, 100), 2, "Human vs AI");
    Clock clock;
    while (win.isOpen()) {
        Event ev;
        while (win.pollEvent(ev)) {
            if (ev.type == Event::Closed) win.close();
            if (ev.type == Event::MouseButtonPressed && ev.mouseButton.button == Mouse::Left) {
                Vector2i mp = Mouse::getPosition(win);
                for (auto& b : btns) if (b.contains(mp)) return b.id;
            }
        }
        Vector2i mp = Mouse::getPosition(win);
        for (auto& b : btns) b.hovered = b.contains(mp);
        float t = clock.getElapsedTime().asSeconds();
        win.clear(Color(20, 22, 28));
        for (auto& b : btns) b.draw(win, t, font, 20);
        win.display();
    }
    return 1;
}

Piece symbolMenu(RenderWindow& win, const Font& font) {
    vector<Button> btns;
    btns.emplace_back(WINDOW_W / 2.f, WINDOW_H / 2.f - 60, 200, 80, Color(30, 30, 40), Color(255, 100, 100), 1, "Play as X");
    btns.emplace_back(WINDOW_W / 2.f, WINDOW_H / 2.f + 60, 200, 80, Color(30, 30, 40), Color(100, 255, 100), 2, "Play as O");
    Clock clock;
    while (win.isOpen()) {
        Event ev;
        while (win.pollEvent(ev)) {
            if (ev.type == Event::Closed) win.close();
            if (ev.type == Event::MouseButtonPressed && ev.mouseButton.button == Mouse::Left) {
                Vector2i mp = Mouse::getPosition(win);
                for (auto& b : btns) if (b.contains(mp)) return (b.id == 1) ? Piece::X : Piece::O;
            }
        }
        Vector2i mp = Mouse::getPosition(win);
        for (auto& b : btns) b.hovered = b.contains(mp);
        float t = clock.getElapsedTime().asSeconds();
        win.clear(Color(20, 22, 28));
        for (auto& b : btns) b.draw(win, t, font, 20);
        win.display();
    }
    return Piece::X;
}

bool firstTurnMenu(RenderWindow& win, const Font& font, const string& p1, const string& p2, bool vsAI) {
    vector<Button> btns;
    btns.emplace_back(WINDOW_W / 2.f, WINDOW_H / 2.f - 60, 260, 80, Color(30, 30, 40), Color(255, 200, 0), 1, p1 + " first");
    string secondLabel = vsAI ? "AI first" : p2 + " first";
    btns.emplace_back(WINDOW_W / 2.f, WINDOW_H / 2.f + 60, 260, 80, Color(30, 30, 40), Color(255, 0, 100), 2, secondLabel);
    Clock clock;
    while (win.isOpen()) {
        Event ev;
        while (win.pollEvent(ev)) {
            if (ev.type == Event::Closed) win.close();
            if (ev.type == Event::MouseButtonPressed && ev.mouseButton.button == Mouse::Left) {
                Vector2i mp = Mouse::getPosition(win);
                for (auto& b : btns) if (b.contains(mp)) return b.id == 1;
            }
        }
        Vector2i mp = Mouse::getPosition(win);
        for (auto& b : btns) b.hovered = b.contains(mp);
        float t = clock.getElapsedTime().asSeconds();
        win.clear(Color(20, 22, 28));
        for (auto& b : btns) b.draw(win, t, font, 18);
        win.display();
    }
    return true;
}

Difficulty aiDifficultyMenu(RenderWindow& win, const Font& font) {
    vector<Button> btns;
    btns.emplace_back(WINDOW_W / 2.f, WINDOW_H / 2.f - 80, 220, 70, Color(30, 30, 40), Color(200, 200, 200), 1, "Easy");
    btns.emplace_back(WINDOW_W / 2.f, WINDOW_H / 2.f, 220, 70, Color(30, 30, 40), Color(100, 255, 100), 2, "Medium");
    btns.emplace_back(WINDOW_W / 2.f, WINDOW_H / 2.f + 80, 220, 70, Color(30, 30, 40), Color(255, 100, 100), 3, "Hard");
    Clock clock;
    while (win.isOpen()) {
        Event ev;
        while (win.pollEvent(ev)) {
            if (ev.type == Event::Closed) win.close();
            if (ev.type == Event::MouseButtonPressed && ev.mouseButton.button == Mouse::Left) {
                Vector2i mp = Mouse::getPosition(win);
                for (auto& b : btns) if (b.contains(mp)) return (b.id == 1) ? Difficulty::Easy : (b.id == 2) ? Difficulty::Medium : Difficulty::Hard;
            }
        }
        Vector2i mp = Mouse::getPosition(win);
        for (auto& b : btns) b.hovered = b.contains(mp);
        float t = clock.getElapsedTime().asSeconds();
        win.clear(Color(20, 22, 28));
        for (auto& b : btns) b.draw(win, t, font, 18);
        win.display();
    }
    return Difficulty::Medium;
}

// Name input with event handling (Enter confirms)
string nameInput(RenderWindow& win, const Font& font, const string& prompt) {
    RectangleShape box(Vector2f(360.f, 52.f));
    box.setOrigin(box.getSize().x / 2.f, box.getSize().y / 2.f);
    box.setPosition(WINDOW_W / 2.f, WINDOW_H / 2.f + 10);
    box.setFillColor(Color::Transparent);
    box.setOutlineThickness(2.f);
    string s;
    bool done = false;
    Clock clock;
    while (win.isOpen() && !done) {
        Event ev;
        while (win.pollEvent(ev)) {
            if (ev.type == Event::Closed) { win.close(); done = true; break; }
            if (ev.type == Event::TextEntered) {
                uint32_t c = ev.text.unicode;
                if (c == 13) { // Enter
                    if (!s.empty()) done = true;
                }
                else if (c == 8) { // backspace
                    if (!s.empty()) s.pop_back();
                }
                else if (c >= 32 && c < 127 && s.size() < 32) {
                    s.push_back((char)c);
                }
            }
            if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Enter) {
                if (!s.empty()) done = true;
            }
        }
        float t = clock.getElapsedTime().asSeconds();
        box.setOutlineColor(Color::White);
        win.clear(Color(20, 22, 28));
        Text p(prompt, font, 20); p.setFillColor(Color::White); p.setPosition(WINDOW_W / 2.f - 180, WINDOW_H / 2.f - 40);
        win.draw(p);
        win.draw(box);
        Text txt(s.empty() ? string("> ") : s, font, 20);
        txt.setFillColor(Color::White);
        txt.setPosition(box.getPosition().x - box.getSize().x / 2.f + 8, box.getPosition().y - box.getSize().y / 2.f + 8);
        win.draw(txt);
        win.display();
    }
    return s.empty() ? "Player" : s;
}

int main() {
    RenderWindow window(VideoMode(WINDOW_W, WINDOW_H), "Tic-Tac-Toe Neon", Style::Close);
    window.setFramerateLimit(60);

    Font font;
    if (!loadPreferredFont(font)) {
        cerr << "Failed to load font; make sure a system font is available (arial.ttf etc.)\n";
        if (!font.loadFromFile("arial.ttf")) { /* proceed - maybe default */ }
    }

    Game game;
    bool restartRequested = true;

    // restart button top-right
    Button restartBtn(WINDOW_W - 90.f, BOARD_TOP / 2.f, 140.f, 40.f, Color(30, 30, 40), Color(200, 180, 40), 99, "Restart");

    Clock neonClock, aiClock;
    bool aiWaiting = false;

    auto getColorForPiece = [](Piece p) -> Color {
        if (p == Piece::X) return Color(255, 120, 110);
        if (p == Piece::O) return Color(110, 190, 255);
        return Color::White;
        };

    while (window.isOpen()) {
        // ------------------- MENUS -------------------
        if (restartRequested) {
            // Player type
            int playerChoice = playerTypeMenu(window, font);
            game.humanVsAI = (playerChoice == 2);

            // Player symbol
            game.player1Piece = symbolMenu(window, font);
            game.player2Piece = (game.player1Piece == Piece::X) ? Piece::O : Piece::X;

            // Who goes first
            game.player1Name = "Player 1";
            game.player2Name = game.humanVsAI ? "AI" : "Player 2";
            game.playerFirst = firstTurnMenu(window, font, game.player1Name, game.player2Name, game.humanVsAI);

            // Names
            game.player1Name = nameInput(window, font, "Enter Player 1 name:");
            if (!game.humanVsAI) game.player2Name = nameInput(window, font, "Enter Player 2 name:");
            else game.player2Name = "AI";

            // AI difficulty
            game.difficulty = game.humanVsAI ? aiDifficultyMenu(window, font) : Difficulty::Medium;

            // Final reset
            game.reset();
            if (game.playerFirst) { game.turnPiece = game.player1Piece; game.currentTurnIsAI = false; }
            else { game.turnPiece = game.player2Piece; game.currentTurnIsAI = game.humanVsAI; }

            restartRequested = false;
            aiWaiting = false;
        }

        // ------------------- EVENTS -------------------
        Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == Event::Closed) { window.close(); break; }

            if (ev.type == Event::MouseButtonPressed && ev.mouseButton.button == Mouse::Left) {
                Vector2i mp = Mouse::getPosition(window);

                // Restart button
                if (restartBtn.contains(mp)) { restartRequested = true; continue; }

                // Game finished click -> restart
                if (game.finished) { restartRequested = true; continue; }

                // Player turn click
                if (!game.currentTurnIsAI && !game.finished) {
                    int m = mousePosToIndex(mp);
                    if (m >= 0 && game.board[m] == Piece::Empty) {
                        game.board[m] = game.turnPiece;
                        checkFinish(game);
                        if (!game.finished) {
                            // Swap turn
                            Piece other = (game.turnPiece == game.player1Piece) ? game.player2Piece : game.player1Piece;
                            game.turnPiece = other;
                            game.currentTurnIsAI = (game.humanVsAI && other == game.player2Piece);
                        }
                    }
                }
            }
        }

        // Hover state
        restartBtn.hovered = restartBtn.contains(Mouse::getPosition(window));

        // ------------------- AI TURN -------------------
        if (!game.finished && game.humanVsAI && game.currentTurnIsAI) {
            if (!aiWaiting) { aiClock.restart(); aiWaiting = true; }
            if (aiClock.getElapsedTime().asSeconds() >= 0.18f) {
                int move = chooseAIMove(game);
                if (move >= 0) {
                    game.board[move] = game.turnPiece;
                    checkFinish(game);
                    if (!game.finished) {
                        game.turnPiece = (game.turnPiece == game.player1Piece) ? game.player2Piece : game.player1Piece;
                        game.currentTurnIsAI = false;
                    }
                }
                aiWaiting = false;
            }
        }

        // ------------------- RENDERING -------------------
        float t = neonClock.getElapsedTime().asSeconds();
        window.clear();
        renderBoard(window, game, t, font);

        // Player labels (top-left)
        Text p1(game.player1Name + " (" + string((game.player1Piece == Piece::X) ? "X" : "O") + ")", font, 18);
        p1.setFillColor(getColorForPiece(game.player1Piece));
        p1.setPosition(12, 12);
        window.draw(p1);

        Text p2(game.player2Name + " (" + string((game.player2Piece == Piece::X) ? "X" : "O") + ")", font, 18);
        p2.setFillColor(getColorForPiece(game.player2Piece));
        p2.setPosition(12, 36);
        window.draw(p2);

        // Current turn or winner (top-center)
        Text topText("", font, 30);
        if (game.finished) {
            if (game.winner == Piece::Empty) {
                topText.setString("DRAW!");
                topText.setFillColor(Color::White);
            }
            else {
                string name = (game.winner == game.player1Piece) ? game.player1Name : game.player2Name;
                topText.setString(name + " WINS!");
                topText.setFillColor(getColorForPiece(game.winner));
            }
        }
        else {
            string currName = (game.turnPiece == game.player1Piece) ? game.player1Name : game.player2Name;
            currName += " (" + string((game.turnPiece == Piece::X) ? "X" : "O") + ")";
            topText.setString(currName);
            Color c = getColorForPiece(game.turnPiece);
            int alpha = 150 + 100 * abs(sin(t * 4.f)); // neon pulse
            topText.setFillColor(Color(c.r, c.g, c.b, (sf::Uint8)alpha));
        }

        FloatRect r = topText.getLocalBounds();
        topText.setOrigin(r.left + r.width / 2.f, r.top + r.height / 2.f);
        topText.setPosition(WINDOW_W / 2.f, 60.f);
        window.draw(topText);

        // Restart button (top-right)
        restartBtn.draw(window, t, font, 18);

        window.display();
    }

    return 0;
}