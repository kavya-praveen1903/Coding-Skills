// Compile: g++ student.cpp -o student.exe -std=c++17 -lraylib -lopengl32 -lgdi32 -lwinmm

#include "raylib.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cmath>

using std::string;
using std::vector;

// ---------- Config ----------
const string DATA_FILE = "students.csv";
const string REQUEST_FILE = "requests.txt";
const int DEFAULT_SUBJECTS = 3;
const int TARGET_W = 1920;
const int TARGET_H = 1080;

// ---------- Data ----------
struct Student {
    int roll = 0;
    string name;
    string password;
    vector<int> marks;
    double totalScore() const {
        double s = 0;
        for (int m : marks) s += m;
        return s;
    }
    double averageScore() const {
        if (marks.empty()) return 0.0;
        return totalScore() / marks.size();
    }
};

// ---------- CSV Helpers ----------
string join_marks(const vector<int>& m) {
    std::stringstream ss;
    for (size_t i = 0; i < m.size(); ++i) {
        if (i) ss << ';';
        ss << m[i];
    }
    return ss.str();
}
vector<int> parse_marks(const string &s) {
    vector<int> out;
    std::stringstream ss(s);
    string cur;
    while (getline(ss, cur, ';')) {
        if (!cur.empty()) try { out.push_back(std::stoi(cur)); } catch(...) {}
    }
    return out;
}
bool save_to_file(const vector<Student>& db) {
    std::ofstream f(DATA_FILE);
    if (!f) return false;
    f << "roll,name,password,marks\n";
    for (auto &s : db) {
        string name = s.name;
        if (name.find(',') != string::npos || name.find('"') != string::npos) {
            string tmp;
            for (char c : name) tmp += (c == '"') ? "\"\"" : string(1,c);
            name = "\"" + tmp + "\"";
        }
        string pwd = s.password;
        if (pwd.find(',') != string::npos || pwd.find('"') != string::npos) {
            string tmp;
            for (char c : pwd) tmp += (c == '"') ? "\"\"" : string(1,c);
            pwd = "\"" + tmp + "\"";
        }
        f << s.roll << "," << name << "," << pwd << "," << join_marks(s.marks) << "\n";
    }
    return true;
}
bool load_from_file(vector<Student>& db) {
    std::ifstream f(DATA_FILE);
    if (!f) return false;
    db.clear();
    string line;
    getline(f, line); // skip header
    while (getline(f, line)) {
        if (line.empty()) continue;
        vector<string> parts;
        string cur;
        bool inQuotes = false;
        for (size_t i=0;i<line.size();i++) {
            char c = line[i];
            if (c == '"') {
                if (inQuotes && i+1 < line.size() && line[i+1]=='"') { cur.push_back('"'); i++; }
                else inQuotes = !inQuotes;
            } else if (c==',' && !inQuotes) { parts.push_back(cur); cur.clear(); }
            else cur.push_back(c);
        }
        parts.push_back(cur);
        if (parts.size()<4) continue;
        Student s;
        try { s.roll = std::stoi(parts[0]); } catch(...) { continue; }
        s.name = parts[1];
        s.password = parts[2];
        s.marks = parse_marks(parts[3]);
        if ((int)s.marks.size() < DEFAULT_SUBJECTS) s.marks.resize(DEFAULT_SUBJECTS, 0);
        db.push_back(s);
    }
    return true;
}

// ---------- Requests ----------
void SaveRequest(int roll, const string &msg) {
    std::ofstream f(REQUEST_FILE, std::ios::app);
    if (!f) return;
    f << "Roll " << roll << ": " << msg << "\n";
}
vector<string> LoadRequests() {
    vector<string> out;
    std::ifstream f(REQUEST_FILE);
    if (!f) return out;
    string line;
    while (getline(f,line)) if (!line.empty()) out.push_back(line);
    return out;
}
void ClearRequests() {
    std::ofstream f(REQUEST_FILE,std::ios::trunc);
}

// ---------- Text Field (placeholder + caret) ----------
struct TextField {
    string text;
    Rectangle rect;
    bool active = false;
    int maxLen = 256;
    int blinkTimer = 0;
    string placeholder = "";
    bool passwordMode = false;
};
void ProcessTextField(TextField &tf) {
    if (!tf.active) { tf.blinkTimer = (tf.blinkTimer+1)%60; return; }
    int key = GetCharPressed();
    while (key > 0) {
        if ((key>=32)&&(key<=125)&&(tf.text.size()<tf.maxLen)) tf.text.push_back((char)key);
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && !tf.text.empty()) tf.text.pop_back();
    tf.blinkTimer = (tf.blinkTimer+1)%60;
}
void DrawTextField(const TextField &tf, int fontSize, Color bgColor = Fade(GRAY,0.9f)) {
    DrawRectangleRec(tf.rect, tf.active?Fade(LIGHTGRAY,0.95f):bgColor);
    DrawRectangleLinesEx(tf.rect,2,BLACK);
    int pad = std::max(6, fontSize/3);
    string display = tf.text;
    if (tf.passwordMode) display = string(tf.text.size(), '*');
    if (tf.text.empty() && !tf.active && !tf.placeholder.empty()) {
        DrawText(tf.placeholder.c_str(), (int)tf.rect.x+pad, (int)tf.rect.y+pad, fontSize, Fade(DARKGRAY,0.6f));
    } else {
        int availW = (int)tf.rect.width - pad*2;
        string shown = display;
        while(!shown.empty() && MeasureText(shown.c_str(), fontSize) > availW) shown.erase(0,1);
        DrawText(shown.c_str(), (int)tf.rect.x+pad, (int)tf.rect.y+pad, fontSize, BLACK);
        if (tf.active && tf.blinkTimer < 30) {
            int caretX = (int)tf.rect.x + pad + MeasureText(shown.c_str(), fontSize);
            DrawLine(caretX, (int)tf.rect.y+pad, caretX, (int)tf.rect.y+pad+fontSize, BLACK);
        }
    }
}

// ---------- Button ----------
bool Button(const Rectangle &r, const char* label, int fontSize = 20) {
    Vector2 m = GetMousePosition();
    bool hover = (m.x>=r.x && m.x<=r.x+r.width && m.y>=r.y && m.y<=r.y+r.height);
    DrawRectangleRec(r, hover?Fade(SKYBLUE,0.95f):BLUE);
    DrawRectangleLinesEx(r, 2, BLACK);
    int tw = MeasureText(label, fontSize);
    DrawText(label, (int)(r.x + (r.width - tw)/2), (int)(r.y + (r.height-fontSize)/2), fontSize, WHITE);
    return hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

// ---------- Student List ----------
int DrawStudentList(const vector<Student>& db, const Rectangle &area, int &outOffset, int fontSize) {
    DrawRectangleRec(area, RAYWHITE);
    DrawRectangleLinesEx(area, 2, BLACK);
    int itemH = fontSize + 12;
    int visible = (int)area.height / itemH;
    int N = db.size();
    int maxOffset = std::max(0, N - visible);
    float wheel = GetMouseWheelMove();
    if (wheel != 0) { outOffset -= (int)wheel; if (outOffset < 0) outOffset = 0; if (outOffset > maxOffset) outOffset = maxOffset; }
    int start = outOffset;
    int y = (int)area.y;
    for (int i = 0; i < visible; ++i) {
        int idx = start + i;
        Rectangle item = { area.x, (float)y, area.width, (float)itemH-2 };
        if (idx < N) {
            Color bg = (i % 2 == 0) ? Fade(LIGHTGRAY, 0.35f) : Fade(LIGHTGRAY, 0.25f);
            DrawRectangleRec(item, bg);
            string line = std::to_string(db[idx].roll) + " | " + db[idx].name + " | Score:" + std::to_string((int)db[idx].totalScore());
            DrawText(line.c_str(), (int)item.x + 8, (int)item.y + 6, fontSize, BLACK);
            Vector2 m = GetMousePosition();
            if (m.x >= item.x && m.x <= item.x + item.width && m.y >= item.y && m.y <= item.y + item.height) {
                DrawRectangleLinesEx(item, 2, RED);
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return idx;
            }
        }
        y += itemH;
    }
    return -1;
}

// ---------- Utilities ----------
void activateOnly(TextField* which, const vector<TextField*> &allFields) {
    for (auto p : allFields) if (p) p->active = (p == which);
}

// ---------- Main ----------
int main() {
    const int screenW = TARGET_W;
    const int screenH = TARGET_H;
    InitWindow(screenW, screenH, "Student Result Management System");
    SetTargetFPS(60);

    vector<Student> db;
    load_from_file(db);

    enum Screen { SCR_MAIN, SCR_ADMIN_LOGIN, SCR_ADMIN_PANEL, SCR_STUDENT_LOGIN, SCR_STUDENT_PANEL, SCR_ADD_STUDENT, SCR_VIEW_STUDENTS, SCR_VIEW_REQUESTS } screen = SCR_MAIN;
    Screen prevScreen = SCR_MAIN;

    // Layout metrics (relative)
    const int titleFont = 36;
    const int labelFont = 20;
    const int inputFont = 20;
    const int smallFont = 16;
    const int btnFont = 20;

    // Persistent Text fields (rects updated each frame)
    TextField tfAdminUser{"", {0,0,0,0}, false, 256, 0, "Enter username", false};
    TextField tfAdminPass{"", {0,0,0,0}, false, 256, 0, "Enter password", true};
    TextField tfStudentRoll{"", {0,0,0,0}, false, 256, 0, "Enter roll no", false};
    TextField tfStudentPass{"", {0,0,0,0}, false, 256, 0, "Enter password", true};
    TextField tfRoll{"", {0,0,0,0}, false, 256, 0, "Enter Id Number", false};
    TextField tfName{"", {0,0,0,0}, false, 256, 0, "Student name", false};
    TextField tfPassword{"", {0,0,0,0}, false, 256, 0, "Set password", true};
    TextField tfSubCount{std::to_string(DEFAULT_SUBJECTS), {0,0,0,0}, false, 4, 0, "Subjects", false};
    TextField tfRequestMsg{"", {0,0,0,0}, false, 512, 0, "Type your request here...", false};
    TextField tfSearchRoll{"", {0,0,0,0}, false, 64, 0, "Search roll", false};
    vector<TextField> tfMarks;
    auto ensureMarksForCount = [&](int subCount) {
        if (subCount < 1) subCount = 1;
        if ((int)tfMarks.size() != subCount) {
            tfMarks.clear();
            for (int i = 0; i < subCount; ++i) {
                TextField m; m.text = "0"; m.rect = {0,0,0,0}; m.maxLen = 4; m.blinkTimer = 0; m.placeholder = "0"; m.passwordMode = false;
                tfMarks.push_back(m);
            }
        }
    };
    ensureMarksForCount(DEFAULT_SUBJECTS);

    int studentListOffset = 0;
    int selectedStudentIndex = -1;
    int loggedStudentIndex = -1;
    string infoMsg;
    bool adminAuthenticated = false;

    // Main loop
    while (!WindowShouldClose()) {
        // Compute layout areas (responsive)
        float leftW = screenW * 0.38f;
        float rightW = screenW * 0.58f;
        float margin = screenW * 0.03f;
        float topY = screenH * 0.06f;

        Rectangle adminArea = { margin, topY + 80, leftW - margin * 0.5f, 340 };
        Rectangle studentArea = adminArea;
        Rectangle formArea = { leftW + margin * 0.5f, topY + 60, rightW - margin, screenH - (topY + 120) };
        Rectangle listArea = { margin, topY + 80, leftW - margin * 0.5f, screenH - (topY + 160) };
        Rectangle reqArea = { margin, topY + 80, screenW - margin * 2, screenH - (topY + 160) };

        // Assign rects depending on screen (so clickable areas exist and are accurate)
        if (screen == SCR_ADMIN_LOGIN) {
            float bx = adminArea.x + 24;
            float by = adminArea.y + 24;
            float w = adminArea.width - 48;
            float h = 48;
            tfAdminUser.rect = { bx, by + labelFont + 6, w, h };
            tfAdminPass.rect = { bx, by + (labelFont + 6) + h + 18, w, h };
        } else if (screen == SCR_STUDENT_LOGIN) {
            float bx = studentArea.x + 24;
            float by = studentArea.y + 24;
            float w = studentArea.width - 48;
            float h = 48;
            tfStudentRoll.rect = { bx, by + labelFont + 6, w, h };
            tfStudentPass.rect = { bx, by + (labelFont + 6) + h + 18, w, h };
        } else if (screen == SCR_ADD_STUDENT) {
            float bx = formArea.x + 28;
            float by = formArea.y + 18;
            float w = formArea.width - 56;
            float smallH = 44;
            tfRoll.rect = { bx, by + labelFont + 6, w * 0.32f, smallH };
            tfName.rect = { bx, by + (labelFont + 6) + smallH + 12, w, smallH };
            tfPassword.rect = { bx, by + (labelFont + 6) + smallH*2 + 30, w * 0.45f, smallH };
            tfSubCount.rect = { bx + w * 0.47f + 12, by + (labelFont + 6) + smallH*2 + 30, w * 0.22f, smallH };
            int markCount = std::max(1, std::atoi(tfSubCount.text.c_str()));
            ensureMarksForCount(markCount);
            float marksStartY = by + (labelFont + 6) + smallH*3 + 42;
            float markW = w * 0.18f;
            for (int i = 0; i < (int)tfMarks.size(); ++i) {
                float mx = bx + (i % 4) * (markW + 22);
                float my = marksStartY + (i / 4) * (smallH + 12);
                tfMarks[i].rect = { mx, my + labelFont + 4, markW, smallH };
            }
        } else if (screen == SCR_VIEW_STUDENTS) {
            tfSearchRoll.rect = { listArea.x + 12, listArea.y - 52, listArea.width * 0.55f, 40 };
        } else if (screen == SCR_STUDENT_PANEL) {
            tfRequestMsg.rect = { formArea.x + 24, formArea.y + 20, formArea.width - 48, 140 };
        }

        // Build activeFields vector per current screen (important: per-screen!)
        vector<TextField*> activeFields;
        if (screen == SCR_ADMIN_LOGIN) activeFields = { &tfAdminUser, &tfAdminPass };
        else if (screen == SCR_STUDENT_LOGIN) activeFields = { &tfStudentRoll, &tfStudentPass };
        else if (screen == SCR_ADD_STUDENT) {
            activeFields = { &tfRoll, &tfName, &tfPassword, &tfSubCount };
            for (auto &m : tfMarks) activeFields.push_back(&m);
        }
        else if (screen == SCR_VIEW_STUDENTS) activeFields = { &tfSearchRoll };
        else if (screen == SCR_STUDENT_PANEL) activeFields = { &tfRequestMsg };
        else activeFields = {}; // main / admin panel / requests have none or handled fields

        // Handle mouse clicks: activate only fields for THIS screen
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 m = GetMousePosition();
            bool clicked = false;
            for (auto tf : activeFields) {
                if (!tf) continue;
                if (CheckCollisionPointRec(m, tf->rect)) {
                    activateOnly(tf, activeFields);
                    clicked = true;
                    break;
                }
            }
            if (!clicked) {
                // Clicked outside any field on this screen: deactivate all fields of this screen
                activateOnly(nullptr, activeFields);
            }
        }

        // Now process keyboard input only for activeFields
        for (auto tf : activeFields) ProcessTextField(*tf);

        // ---------- Draw ----------
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("Student Result Management System", (int)(screenW*0.5f) - MeasureText("Student Result Management System", titleFont)/2, (int)(topY - 10), titleFont, DARKBLUE);

        if (screen == SCR_MAIN) {
            float btnW = 360, btnH = 80;
            float cx = screenW * 0.5f;
            float by = screenH * 0.33f;
            if (Button({cx - btnW/2, by, btnW, btnH}, "Admin Login", 30)) { screen = SCR_ADMIN_LOGIN; tfAdminPass.text.clear(); }
            if (Button({cx - btnW/2, by + btnH + 32, btnW, btnH}, "Student Login", 30)) { screen = SCR_STUDENT_LOGIN; tfStudentRoll.text.clear(); tfStudentPass.text.clear(); }
            if (Button({cx - btnW/2, by + (btnH+32)*2, btnW, btnH}, "Exit", 30)) { break; }
            if (!infoMsg.empty()) DrawText(infoMsg.c_str(), 20, screenH - 36, smallFont, DARKGRAY);
        }

        else if (screen == SCR_ADMIN_LOGIN) {
            DrawRectangleLinesEx(adminArea, 2, Fade(DARKGRAY, 0.4f));
            DrawText("Admin Login", (int)adminArea.x + 12, (int)adminArea.y - 8, 28, DARKBLUE);
            DrawTextField(tfAdminUser, inputFont);

            DrawTextField(tfAdminPass, inputFont);

            float btnX = adminArea.x + 24;
            float btnY = adminArea.y + adminArea.height - 72;
            if (Button({btnX, btnY, 180, 48}, "Login", btnFont)) {
                if (tfAdminPass.text == "12345") { adminAuthenticated = true; screen = SCR_ADMIN_PANEL; infoMsg.clear(); }
                else infoMsg = "Invalid admin credentials";
            }
            if (Button({btnX + 200, btnY, 180, 48}, "Back", btnFont)) { screen = SCR_MAIN; infoMsg.clear(); }
            if (!infoMsg.empty()) DrawText(infoMsg.c_str(), (int)btnX, (int)(btnY - 28), smallFont, RED);
        }

        else if (screen == SCR_STUDENT_LOGIN) {
            DrawRectangleLinesEx(studentArea, 2, Fade(DARKGRAY, 0.4f));
            DrawText("Student Login", (int)studentArea.x + 12, (int)studentArea.y - 8, 28, DARKBLUE);
             DrawTextField(tfStudentRoll, inputFont);
            DrawTextField(tfStudentPass, inputFont);
            float bx = studentArea.x + 24;
            float btnY = studentArea.y + studentArea.height - 72;
            if (Button({bx, btnY, 180, 48}, "Login", btnFont)) {
                try {
                    int r = std::stoi(tfStudentRoll.text);
                    bool found = false;
                    for (int i = 0; i < (int)db.size(); ++i) {
                        if (db[i].roll == r && db[i].password == tfStudentPass.text) {
                            loggedStudentIndex = i; screen = SCR_STUDENT_PANEL; infoMsg.clear(); found = true; break;
                        }
                    }
                    if (!found) infoMsg = "Invalid roll or password";
                } catch (...) { infoMsg = "Invalid roll"; }
            }
            if (Button({bx + 200, btnY, 180, 48}, "Back", btnFont)) { screen = SCR_MAIN; infoMsg.clear(); }
            if (!infoMsg.empty()) DrawText(infoMsg.c_str(), (int)bx, (int)(btnY - 28), smallFont, RED);
        }

        else if (screen == SCR_ADMIN_PANEL) {
            float x = margin;
            float y = topY + 90;
            float w = leftW - margin;
            float h = 64;
            DrawText("Admin Panel", (int)x, (int)(topY + 40), 28, DARKBLUE);
            if (Button({x, y, w, h}, "Add/Edit Student", btnFont)) {
                tfRoll.text = ""; tfName.text = ""; tfPassword.text = ""; tfSubCount.text = std::to_string(DEFAULT_SUBJECTS);
                ensureMarksForCount(DEFAULT_SUBJECTS); prevScreen = screen; screen = SCR_ADD_STUDENT;
            }
            y += h + 18;
            if (Button({x, y, w, h}, "View Students", btnFont)) { screen = SCR_VIEW_STUDENTS; studentListOffset = 0; selectedStudentIndex = -1; }
            y += h + 18;
            if (Button({x, y, w, h}, "View Requests", btnFont)) { screen = SCR_VIEW_REQUESTS; }
            y += h + 18;
            if (Button({x, y, w, h}, "Logout", btnFont)) { screen = SCR_MAIN; adminAuthenticated = false; }
            if (!infoMsg.empty()) DrawText(infoMsg.c_str(), (int)(leftW + 20), (int)(screenH - 36), smallFont, DARKGRAY);
        }

        else if (screen == SCR_ADD_STUDENT) {
            DrawRectangleLinesEx(formArea, 2, Fade(DARKGRAY, 0.4f));
            DrawText("Add / Edit Student", (int)formArea.x + 8, (int)(formArea.y - 26), 28, DARKBLUE);
            DrawTextField(tfRoll, inputFont);
            DrawTextField(tfName, inputFont);
            DrawTextField(tfPassword, inputFont);
            for (int i = 0; i < 3; ++i) {
                DrawText(("Marks " + std::to_string(i+1)).c_str(), (int)tfMarks[i].rect.x, (int)(tfMarks[i].rect.y - labelFont - 6), labelFont, BLACK);
                DrawTextField(tfMarks[i], inputFont);
            }

            float btnY = formArea.y + formArea.height - 88;
            if (Button({formArea.x + 28, btnY, 180, 48}, "Save Student", btnFont)) {
                try {
                    Student s; s.roll = std::stoi(tfRoll.text); s.name = tfName.text; s.password = tfPassword.text;
                    s.marks.clear();
                    for (auto &m : tfMarks) s.marks.push_back(std::stoi(m.text));
                    if ((int)s.marks.size() < DEFAULT_SUBJECTS) s.marks.resize(DEFAULT_SUBJECTS, 0);
                    bool dup = false; int dupIdx = -1;
                    for (int i = 0; i < (int)db.size(); ++i) if (db[i].roll == s.roll) { dup = true; dupIdx = i; break; }
                    if (dup) db[dupIdx] = s; else db.push_back(s);
                    if (!save_to_file(db)) infoMsg = "Failed to save to disk"; else { infoMsg = "Saved"; screen = SCR_ADMIN_PANEL; }
                } catch (...) { infoMsg = "Invalid input"; }
            }
            if (Button({formArea.x + 220, btnY, 180, 48}, "Back", btnFont)) { screen = prevScreen; }

            if (!infoMsg.empty()) DrawText(infoMsg.c_str(), (int)(formArea.x + 420), (int)(btnY + 12), smallFont, infoMsg == "Saved" ? DARKGREEN : RED);
        }

        else if (screen == SCR_VIEW_STUDENTS) {
            DrawText("Students", (int)listArea.x, (int)(listArea.y - 36), 28, DARKBLUE);
            int sel = DrawStudentList(db, listArea, studentListOffset, smallFont);
            if (sel != -1) selectedStudentIndex = sel;

            if (selectedStudentIndex >= 0 && selectedStudentIndex < (int)db.size()) {
                Student &s = db[selectedStudentIndex];
                Rectangle infoR = { formArea.x, formArea.y + 20, formArea.width - 40, 360 };
                DrawRectangleRec(infoR, Fade(LIGHTGRAY, 0.18f));
                DrawRectangleLinesEx(infoR, 2, BLACK);
                DrawText(("Roll: " + std::to_string(s.roll)).c_str(), (int)infoR.x + 12, (int)infoR.y + 8, 22, BLACK);
                DrawText(("Name: " + s.name).c_str(), (int)infoR.x + 12, (int)infoR.y + 44, 20, BLACK);
                DrawText(("Password: " + s.password).c_str(), (int)infoR.x + 12, (int)infoR.y + 74, 18, BLACK);
                DrawText(("Total: " + std::to_string((int)s.totalScore())).c_str(), (int)infoR.x + 12, (int)infoR.y + 106, 18, BLACK);

                if (Button({ infoR.x + 12, infoR.y + 170, 180, 48 }, "Edit", btnFont)) {
                    tfRoll.text = std::to_string(s.roll); tfName.text = s.name; tfPassword.text = s.password;
                    tfSubCount.text = std::to_string((int)s.marks.size());
                    ensureMarksForCount((int)s.marks.size());
                    for (size_t i = 0; i < s.marks.size() && i < tfMarks.size(); ++i) tfMarks[i].text = std::to_string(s.marks[i]);
                    prevScreen = SCR_VIEW_STUDENTS;
                    screen = SCR_ADD_STUDENT;
                }
                if (Button({ infoR.x + 210, infoR.y + 170, 180, 48 }, "Delete", btnFont)) {
                    db.erase(db.begin() + selectedStudentIndex);
                    selectedStudentIndex = -1;
                    save_to_file(db);
                }
            }
            if (Button({ margin, screenH - 84, 180, 48 }, "Back", btnFont)) screen = SCR_ADMIN_PANEL;
        }

        else if (screen == SCR_VIEW_REQUESTS) {
            DrawText("Requests", (int)reqArea.x, (int)(reqArea.y - 36), 28, DARKBLUE);
            vector<string> reqs = LoadRequests();
            float y = reqArea.y + 12;
            float lineH = smallFont + 8;
            for (size_t i = 0; i < reqs.size(); ++i) {
                DrawText(reqs[i].c_str(), (int)reqArea.x + 8, (int)y, smallFont, BLACK);
                y += lineH;
                if (y > reqArea.y + reqArea.height - 80) break;
            }
            if (Button({ reqArea.x + 8, reqArea.y + reqArea.height - 72, 180, 48 }, "Clear All", btnFont)) { ClearRequests(); infoMsg = "Requests cleared"; }
            if (Button({ reqArea.x + 200, reqArea.y + reqArea.height - 72, 180, 48 }, "Back", btnFont)) screen = SCR_ADMIN_PANEL;
            if (!infoMsg.empty()) DrawText(infoMsg.c_str(), (int)(reqArea.x + 400), (int)(reqArea.y + reqArea.height - 64), smallFont, DARKGRAY);
        }

        else if (screen == SCR_STUDENT_PANEL) {
            if (loggedStudentIndex >= 0 && loggedStudentIndex < (int)db.size()) {
                Student &s = db[loggedStudentIndex];
                Rectangle studentInfo = { margin, topY + 110, leftW - margin*0.5f, 420 };
                DrawRectangleLinesEx(studentInfo, 2, Fade(DARKGRAY, 0.4f));
                DrawText(("Welcome, " + s.name).c_str(), (int)studentInfo.x + 12, (int)studentInfo.y + 8, 26, DARKBLUE);
                DrawText(("Roll: " + std::to_string(s.roll)).c_str(), (int)studentInfo.x + 12, (int)studentInfo.y + 46, 20, BLACK);
                float y = studentInfo.y + 86;
                for (size_t i = 0; i < s.marks.size(); ++i) {
                    DrawText(("Subject " + std::to_string(i+1) + ": " + std::to_string(s.marks[i])).c_str(), (int)studentInfo.x + 20, (int)y, 20, BLACK);
                    y += 36;
                }
                DrawText(("Total: " + std::to_string((int)s.totalScore())).c_str(), (int)studentInfo.x + 12, (int)y, 20, BLACK); y += 36;

                DrawRectangleLinesEx(formArea, 2, Fade(DARKGRAY, 0.4f));
                DrawText("Message", (int)tfRequestMsg.rect.x, (int)(tfRequestMsg.rect.y - labelFont - 6), labelFont, BLACK);
                DrawTextField(tfRequestMsg, smallFont);

                if (Button({ formArea.x + 28, formArea.y + 180, 180, 48 }, "Send Request", btnFont)) {
                    SaveRequest(s.roll, tfRequestMsg.text);
                    tfRequestMsg.text.clear();
                    infoMsg = "Request sent";
                }
                if (Button({ margin, screenH - 84, 180, 48 }, "Logout", btnFont)) { loggedStudentIndex = -1; screen = SCR_MAIN; infoMsg.clear(); }
                if (!infoMsg.empty()) DrawText(infoMsg.c_str(), (int)(formArea.x + 220), (int)(formArea.y + 184), smallFont, DARKGREEN);
            } else {
                loggedStudentIndex = -1; screen = SCR_MAIN;
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
