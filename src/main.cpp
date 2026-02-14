#include <ncursesw/ncurses.h>
#include <locale.h>
#include <chrono>
#include <string>
#include <cmath>

using namespace std::chrono;

const int PADDLE_HEIGHT = 4;
const int WIN_SCORE = 1;
const int BALL_SIZE = 2;

const float INITIAL_SPEED = 0.30f;
const float SPEED_INCREMENT = 1.0f;
const float MAX_SPEED = 1.2f;

// Menu para começar ou sair
int menu() {
    const char* options[] = {"[ PLAY ]", "[ EXIT ]"};
    int choice = 0;
    int n_options = sizeof(options) / sizeof(options[0]);

    const char* title[] = {
        "                                                                   ",
        "░█████████    ░██████   ░███    ░██   ░██████   ░██████   ░██████  ",
        "░██     ░██  ░██   ░██  ░████   ░██  ░██   ░██ ░██   ░██ ░██   ░██ ",
        "░██     ░██ ░██     ░██ ░██░██  ░██ ░██              ░██ ░██       ",
        "░█████████  ░██     ░██ ░██ ░██ ░██ ░██  █████   ░█████  ░███████  ",
        "░██         ░██     ░██ ░██  ░██░██ ░██     ██  ░██      ░██   ░██ ",
        "░██          ░██   ░██  ░██   ░████  ░██  ░███ ░██       ░██   ░██ ",
        "░██           ░██████   ░██    ░███   ░█████░█ ░████████  ░██████  ",
        "                                                                   ",
        "       -----------------------------------------------------       ",
        "                           THE CLASSIC                            ",
        "       -----------------------------------------------------       ",

    };

    int titleLines = 12;

    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);

    // ===== BOX MAIOR =====
    int winHeight = 23;
    int winWidth  = 90;

    int startY = (maxY - winHeight) / 2;
    int startX = (maxX - winWidth) / 2;

    WINDOW* menuWin = newwin(winHeight, winWidth, startY, startX);
    keypad(menuWin, TRUE);

    while (true) {
        werase(menuWin);
        box(menuWin, 0, 0);

        // ===== DESENHA ASCII =====
        wattron(menuWin, COLOR_PAIR(1) | A_BOLD);

        for (int i = 0; i < titleLines; i++) {
            int lineLen = wcswidth((const wchar_t*)L"", 0); // dummy para evitar warning

            // Converter UTF-8 → wide char
            wchar_t wline[512];
            mbstowcs(wline, title[i], 512);

            int visualWidth = wcswidth(wline, wcslen(wline));

            int textX = (winWidth - visualWidth) / 2;

            mvwprintw(menuWin, 2 + i, textX, "%s", title[i]);
        }

        wattroff(menuWin, COLOR_PAIR(1) | A_BOLD);

        // ===== OPÇÕES =====
        for (int i = 0; i < n_options; i++) {

            wchar_t wopt[128];
            mbstowcs(wopt, options[i], 128);

            int visualWidth = wcswidth(wopt, wcslen(wopt));
            int textX = (winWidth - visualWidth) / 2;

            if (i == choice)
                wattron(menuWin, A_REVERSE | A_BOLD);

            mvwprintw(menuWin, 15 + i * 2, textX, "%s", options[i]);

            if (i == choice)
                wattroff(menuWin, A_REVERSE | A_BOLD);
        }

        std::string footer = "MIT, Copyright (c) 2026 nothingburguer. All Rights Reserved.";

        int footerY = winHeight - 3;
        int textX = (winWidth - footer.size()) / 2;

        mvwprintw(menuWin, footerY, textX, "%s", footer.c_str());

        wrefresh(menuWin);

        int ch = wgetch(menuWin);

        switch (ch) {
            case KEY_UP:
            case 'w':
                choice = (choice - 1 + n_options) % n_options;
                break;
            case KEY_DOWN:
            case 's':
                choice = (choice + 1) % n_options;
                break;
            case '\n':
            case KEY_ENTER:
                delwin(menuWin);
                return choice;
        }
    }
}

int main() {
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    halfdelay(0);

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_WHITE, COLOR_BLACK);
    }

    int choice = menu();
    if (choice == 1) { // Sair
        endwin();
        return 0;
    }

    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);

    int paddle1Y = maxY / 2 - PADDLE_HEIGHT / 2;
    int paddle2Y = maxY / 2 - PADDLE_HEIGHT / 2;

    float ballX = maxX / 2.0f;
    float ballY = maxY / 2.0f;
    float ballVelX = INITIAL_SPEED;
    float ballVelY = 0.0f;

    int score1 = 0;
    int score2 = 0;

    const int FPS = 60;
    const int FRAME_DELAY = 1000 / FPS;

    auto lastFrameTime = high_resolution_clock::now();
    auto lastBallUpdate = high_resolution_clock::now();

    const int BALL_UPDATE_DELAY = 8; // ms entre updates da bola

    while (true) {
        getmaxyx(stdscr, maxY, maxX);

        auto now = high_resolution_clock::now();
        auto delta = duration_cast<milliseconds>(now - lastFrameTime).count();
        if (delta < FRAME_DELAY) {
            napms(1);
            continue;
        }
        lastFrameTime = now;


        // ===== INPUT =====
        int ch = getch();

        const int PADDLE_SPEED = 2;

        if (ch == 'w' && paddle1Y > 1)
            paddle1Y -= PADDLE_SPEED;

        if (ch == 's' && paddle1Y + PADDLE_HEIGHT < maxY - 1)
            paddle1Y += PADDLE_SPEED;

        if (ch == KEY_UP && paddle2Y > 1)
            paddle2Y -= PADDLE_SPEED;

        if (ch == KEY_DOWN && paddle2Y + PADDLE_HEIGHT < maxY - 1)
            paddle2Y += PADDLE_SPEED;

        if (ch == 'q') {
            endwin();
            return 0;
        }

        // ===== MOVIMENTO CONTÍNUO =====
        auto nowBall = high_resolution_clock::now();
        auto ballDelta = duration_cast<milliseconds>(nowBall - lastBallUpdate).count();

        if (ballDelta >= BALL_UPDATE_DELAY) {
            lastBallUpdate = nowBall;

            ballX += ballVelX;
            ballY += ballVelY;
        }

        // ===== COLISÃO TETO/CHÃO =====
        if (ballY <= 1) {
            ballY = 1;
            ballVelY = -ballVelY;
        }
        if (ballY + BALL_SIZE >= maxY - 1) {
            ballY = maxY - 1 - BALL_SIZE;
            ballVelY = -ballVelY;
        }

        // ===== COLISÃO RAQUETE 1 =====
        if (ballVelX < 0 && ballX <= 3) {
            if (ballY + BALL_SIZE >= paddle1Y &&
                ballY <= paddle1Y + PADDLE_HEIGHT) {

                float hitPos = ((ballY + BALL_SIZE/2.0f) - paddle1Y) / PADDLE_HEIGHT;
                float angle = (hitPos - 0.5f) * 2.0f;

                ballX = 3;
                ballVelX = fabs(ballVelX);
                ballVelY = angle * fabs(ballVelX);

                ballVelX *= SPEED_INCREMENT;
                ballVelY *= SPEED_INCREMENT;

                if (fabs(ballVelX) > MAX_SPEED)
                    ballVelX = (ballVelX > 0 ? 1 : -1) * MAX_SPEED;

                if (fabs(ballVelY) > MAX_SPEED)
                    ballVelY = (ballVelY > 0 ? 1 : -1) * MAX_SPEED;

                beep();
            }
        }

        // ===== COLISÃO RAQUETE 2 =====
        if (ballVelX > 0 && ballX + BALL_SIZE >= maxX - 3) {
            if (ballY + BALL_SIZE >= paddle2Y &&
                ballY <= paddle2Y + PADDLE_HEIGHT) {

                float hitPos = ((ballY + BALL_SIZE/2.0f) - paddle2Y) / PADDLE_HEIGHT;
                float angle = (hitPos - 0.5f) * 2.0f;

                ballX = maxX - 3 - BALL_SIZE;
                ballVelX = -fabs(ballVelX);
                ballVelY = angle * fabs(ballVelX);

                ballVelX *= SPEED_INCREMENT;
                ballVelY *= SPEED_INCREMENT;

                if (fabs(ballVelX) > MAX_SPEED)
                    ballVelX = (ballVelX > 0 ? 1 : -1) * MAX_SPEED;

                if (fabs(ballVelY) > MAX_SPEED)
                    ballVelY = (ballVelY > 0 ? 1 : -1) * MAX_SPEED;

                beep();
            }
        }

        // ===== PONTUAÇÃO =====
        if (ballX <= 1) {
            score2++;
            ballX = maxX / 2.0f;
            ballY = maxY / 2.0f;
            ballVelX = INITIAL_SPEED;
            ballVelY = 0.0f;
        }

        if (ballX + BALL_SIZE >= maxX - 1) {
            score1++;
            ballX = maxX / 2.0f;
            ballY = maxY / 2.0f;
            ballVelX = -INITIAL_SPEED;
            ballVelY = 0.0f;
        }

        // ===== DESENHO =====
        clear();
        attron(COLOR_PAIR(1));
        box(stdscr, 0, 0);

        int center = maxX / 2;
        for (int i = 1; i < maxY - 1; i++)
            mvaddch(i, center, ACS_VLINE);

        std::string leftScore  = " " + std::to_string(score1) + " ";
        std::string rightScore = " " + std::to_string(score2) + " ";

        mvprintw(0, center - leftScore.size(), "%s", leftScore.c_str());
        mvaddch(0, center, ACS_VLINE);
        mvprintw(0, center + 1, "%s", rightScore.c_str());

        // Raquete 1
        for (int i = 0; i < PADDLE_HEIGHT; i++)
            mvprintw(paddle1Y + i, 2, "█");

        // Raquete 2
        for (int i = 0; i < PADDLE_HEIGHT; i++)
            mvprintw(paddle2Y + i, maxX - 3, "█");

        // Bola
        attron(A_BOLD);
        for (int i = 0; i < BALL_SIZE; i++)
            for (int j = 0; j < BALL_SIZE; j++)
                mvprintw((int)ballY + i, (int)ballX + j, "█");
        attroff(A_BOLD);

        refresh();

        // ===== VITÓRIA =====
        if (score1 == WIN_SCORE || score2 == WIN_SCORE) {
            clear();
            mvprintw(maxY/2, maxX/2 - 10,
                score1 == WIN_SCORE ? "PLAYER 1 win!" : "PLAYER 2 win!");
            refresh();
            timeout(-1);
            getch();
            break;
        }
    }

    endwin();
    return 0;
}
