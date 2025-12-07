// g++ quiz.cpp -o quiz.exe -L"C:\\raylib\\lib" -I"C:\\raylib\\include" -lraylib -lopengl32 -lgdi32 -lwinmm -std=c++17

#include "raylib.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

// -------------------------
// Data Structures
// -------------------------
struct Question;

struct Option {
    std::string text;
    bool correct;
    Option* next;
    Option(const std::string &t = "", bool c = false) : text(t), correct(c), next(nullptr) {}
};

struct Question {
    int id;
    std::string text;
    Option* options;
    Question* next;
    Question(int i = 0, const std::string &t = "")
        : id(i), text(t), options(nullptr), next(nullptr) {}
};

// -------------------------
// Linked List Helpers
// -------------------------
void addOption(Question* q, const std::string& text, bool correct) {
    Option* n = new Option(text, correct);
    if (!q->options) { q->options = n; return; }
    Option* p = q->options;
    while (p->next) p = p->next;
    p->next = n;
}

void appendQuestion(Question*& head, Question* node) {
    if (!head) { head = node; return; }
    Question* t = head;
    while (t->next) t = t->next;
    t->next = node;
}

void freeQuiz(Question* head) {
    while (head) {
        Option* o = head->options;
        while (o) {
            Option* temp = o;
            o = o->next;
            delete temp;
        }
        Question* qTemp = head;
        head = head->next;
        delete qTemp;
    }
}

// -------------------------
// Load from questions.txt
// -------------------------
Question* loadQuestionsFromFile(const std::string& filename) {
    std::ifstream fin(filename);
    if (!fin.is_open()) return nullptr;

    std::string line;
    std::vector<std::string> block;
    Question* head = nullptr;
    int idCounter = 1;

    auto flushBlock = [&]() {
        if (block.empty()) return;
        Question* q = new Question(idCounter++, block[0]);

        for (size_t i = 1; i < block.size(); i++) {
            size_t pos = block[i].find('|');
            if (pos == std::string::npos) continue;

            std::string txt = block[i].substr(0, pos);
            bool correct = (block[i].substr(pos + 1) == "1");
            addOption(q, txt, correct);
        }

        appendQuestion(head, q);
        block.clear();
    };

    while (std::getline(fin, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        bool blank = std::all_of(line.begin(), line.end(),
                                 [](char c){ return isspace((unsigned char)c); });
        if (blank) flushBlock();
        else block.push_back(line);
    }
    flushBlock();

    return head;
}

// -------------------------
// Drawing Helpers
// -------------------------
bool DrawRoundedButton(Rectangle rec, const std::string& text) {
    Vector2 mouse = GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, rec);

    Color base = hover ? Color{200,200,255,255} : Color{230,230,250,255};
    Color border = hover ? DARKBLUE : DARKPURPLE;

    DrawRectangleRounded(rec, 0.15f, 8, base);
    DrawRectangleRoundedLines(rec, 0.15f, 8, border);

    int fontSize = std::clamp((int)(rec.height * 0.40f), 22, 32);
    int w = MeasureText(text.c_str(), fontSize);
    DrawText(text.c_str(), rec.x + rec.width/2 - w/2,
             rec.y + rec.height/2 - fontSize/2, fontSize, BLACK);

    return hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

// Manual wrapped text (Raylib compatibility)
void DrawWrappedText(const std::string &text, float x, float y, float maxWidth, int fontSize, Color color) {
    std::istringstream ss(text);
    std::string word, line;
    float offsetY = 0;

    while (ss >> word) {
        std::string testLine = line.empty() ? word : (line + " " + word);
        int width = MeasureText(testLine.c_str(), fontSize);

        if (width > maxWidth) {
            DrawText(line.c_str(), x, y + offsetY, fontSize, color);
            offsetY += fontSize + 6;
            line = word;
        } else {
            line = testLine;
        }
    }

    if (!line.empty())
        DrawText(line.c_str(), x, y + offsetY, fontSize, color);
}

// Centered text
void DrawCenteredText(const std::string& text, float y, int sw, int size, Color col) {
    int w = MeasureText(text.c_str(), size);
    DrawText(text.c_str(), sw/2 - w/2, y, size, col);
}

// -------------------------
// MAIN
// -------------------------
int main() {
    Question* head = loadQuestionsFromFile("questions.txt");
    if (!head) {
        std::cout << "Could not open questions.txt\n";
        return 1;
    }

    InitWindow(1400, 900, "Quiz Engine");
    SetTargetFPS(60);

    bool inMenu = true;
    Question* current = head;
    int score = 0;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(Color{182,215,255,255});

        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        // ---------------- MENU ----------------
        if (inMenu) {
            DrawCenteredText("Dynamic Quiz Engine", sh * 0.18f, sw, 72, BLACK);

            Rectangle startBtn = { float(sw/2 - 160), float(sh/2 - 40), 320, 90 };
            if (DrawRoundedButton(startBtn, "Start Quiz")) {
                inMenu = false;
                current = head;
                score = 0;
            }

            EndDrawing();
            continue;
        }

        // ---------------- END SCREEN ----------------
        if (!current) {
            DrawCenteredText("Quiz Finished!", sh * 0.18f, sw, 72, BLACK);
            DrawCenteredText("Score: " + std::to_string(score),
                              sh * 0.35f, sw, 62, DARKGREEN);

            Rectangle restart = { float(sw/2 - 160), float(sh*0.60f), 320, 90 };
            if (DrawRoundedButton(restart, "Restart")) {
                current = head;
                score = 0;
            }

            Rectangle quit = { float(sw/2 - 160), float(sh*0.75f), 320, 90 };
            if (DrawRoundedButton(quit, "Quit")) break;

            EndDrawing();
            continue;
        }

        // ---------------- QUIZ PAGE ----------------
        DrawWrappedText(current->text, sw*0.08f, 50, sw*0.84f, 40, BLACK);

        float x = sw*0.08f;
        float y = 200;
        float w = sw*0.84f;
        float h = 80;
        float gap = 30;

        for (Option* o = current->options; o; o = o->next) {
            Rectangle r = {x, y, w, h};
            if (DrawRoundedButton(r, o->text)) {
                if (o->correct) score++;
                current = current->next;
            }
            y += h + gap;
        }

        // SKIP BUTTON
        Rectangle skipBtn = { float(sw - 240), float(sh - 120), 200, 70 };
        if (DrawRoundedButton(skipBtn, "Skip")) {
            current = current->next;
        }

        // SCORE BOTTOM-LEFT
        DrawText(TextFormat("Score: %d", score),
                 20, sh - 50, 32, DARKGREEN);

        EndDrawing();
    }

    freeQuiz(head);
    CloseWindow();
    return 0;
}
