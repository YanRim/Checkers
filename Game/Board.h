#pragma once
#include <iostream>
#include <fstream>
#include <vector>

#include "../Models/Move.h"
#include "../Models/Project_path.h"

#ifdef __APPLE__
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#else
#include <SDL.h>
#include <SDL_image.h>
#endif

using namespace std;

class Board
{
public:
    // Конструкторы класса
    Board() = default;  // Конструктор по умолчанию
    Board(const unsigned int W, const unsigned int H) : W(W), H(H)
    {
    }

    // Метод для инициализации и отрисовки начальной доски
    int start_draw()
    {
        // Инициализация SDL. Если не удалось, записываем ошибку в лог.
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        {
            print_exception("SDL_Init can't init SDL2 lib");
            return 1;
        }

        // Если размеры окна не заданы, определяем их автоматически на основе разрешения экрана
        if (W == 0 || H == 0)
        {
            SDL_DisplayMode dm;
            if (SDL_GetDesktopDisplayMode(0, &dm))
            {
                print_exception("SDL_GetDesktopDisplayMode can't get desctop display mode");
                return 1;
            }
            W = min(dm.w, dm.h);
            W -= W / 15;
            H = W;
        }

        // Создание окна игры
        win = SDL_CreateWindow("Checkers", 0, H / 30, W, H, SDL_WINDOW_RESIZABLE);
        if (win == nullptr)
        {
            print_exception("SDL_CreateWindow can't create window");
            return 1;
        }

        // Создание рендерера для отрисовки текстур
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (ren == nullptr)
        {
            print_exception("SDL_CreateRenderer can't create renderer");
            return 1;
        }

        // Загрузка текстур для игровых элементов
        board = IMG_LoadTexture(ren, board_path.c_str());
        w_piece = IMG_LoadTexture(ren, piece_white_path.c_str());
        b_piece = IMG_LoadTexture(ren, piece_black_path.c_str());
        w_queen = IMG_LoadTexture(ren, queen_white_path.c_str());
        b_queen = IMG_LoadTexture(ren, queen_black_path.c_str());
        back = IMG_LoadTexture(ren, back_path.c_str());
        replay = IMG_LoadTexture(ren, replay_path.c_str());

        // Проверка успешности загрузки текстур
        if (!board || !w_piece || !b_piece || !w_queen || !b_queen || !back || !replay)
        {
            print_exception("IMG_LoadTexture can't load main textures from " + textures_path);
            return 1;
        }

        // Получение размеров окна рендера и создание начальной матрицы доски
        SDL_GetRendererOutputSize(ren, &W, &H);
        make_start_mtx();
        rerender();  // Перерисовка доски
        return 0;
    }

    // Метод для сброса доски к начальному состоянию
    void redraw()
    {
        game_results = -1;  // Сброс результатов игры
        history_mtx.clear();  // Очистка истории ходов
        history_beat_series.clear();  // Очистка истории серий битья
        make_start_mtx();  // Создание начальной матрицы
        clear_active();  // Сброс активной клетки
        clear_highlight();  // Сброс выделенных клеток
    }

    // Метод для перемещения фигуры
    void move_piece(move_pos turn, const int beat_series = 0)
    {
        if (turn.xb != -1)
        {
            mtx[turn.xb][turn.yb] = 0;  // Удаление фигуры из старой позиции
        }
        move_piece(turn.x, turn.y, turn.x2, turn.y2, beat_series);  // Вызов основного метода перемещения
    }

    // Основной метод перемещения фигуры
    void move_piece(const POS_T i, const POS_T j, const POS_T i2, const POS_T j2, const int beat_series = 0)
    {
        if (mtx[i2][j2])
        {
            throw runtime_error("final position is not empty, can't move");
        }
        if (!mtx[i][j])
        {
            throw runtime_error("begin position is empty, can't move");
        }

        // Превращение обычной фигуры в дамку при достижении последней линии
        if ((mtx[i][j] == 1 && i2 == 0) || (mtx[i][j] == 2 && i2 == 7))
            mtx[i][j] += 2;

        mtx[i2][j2] = mtx[i][j];  // Перемещение фигуры
        drop_piece(i, j);  // Удаление фигуры из старой позиции
        add_history(beat_series);  // Добавление хода в историю
    }

    // Метод для удаления фигуры с доски
    void drop_piece(const POS_T i, const POS_T j)
    {
        mtx[i][j] = 0;  // Обнуление позиции
        rerender();  // Перерисовка доски
    }

    // Метод для превращения фигуры в дамку
    void turn_into_queen(const POS_T i, const POS_T j)
    {
        if (mtx[i][j] == 0 || mtx[i][j] > 2)
        {
            throw runtime_error("can't turn into queen in this position");
        }
        mtx[i][j] += 2;  // Изменение типа фигуры на дамку
        rerender();  // Перерисовка доски
    }

    // Метод для получения текущего состояния доски
    vector<vector<POS_T>> get_board() const
    {
        return mtx;
    }

    // Метод для выделения клеток
    void highlight_cells(vector<pair<POS_T, POS_T>> cells)
    {
        for (auto pos : cells)
        {
            POS_T x = pos.first, y = pos.second;
            is_highlighted_[x][y] = 1;  // Установка флага выделения
        }
        rerender();  // Перерисовка доски
    }

    // Метод для очистки выделений
    void clear_highlight()
    {
        for (POS_T i = 0; i < 8; ++i)
        {
            is_highlighted_[i].assign(8, 0);  // Сброс флагов выделения
        }
        rerender();  // Перерисовка доски
    }

    // Метод для установки активной клетки
    void set_active(const POS_T x, const POS_T y)
    {
        active_x = x;
        active_y = y;
        rerender();  // Перерисовка доски
    }

    // Метод для сброса активной клетки
    void clear_active()
    {
        active_x = -1;
        active_y = -1;
        rerender();  // Перерисовка доски
    }

    // Метод для проверки, выделена ли клетка
    bool is_highlighted(const POS_T x, const POS_T y)
    {
        return is_highlighted_[x][y];
    }

    // Метод для отмены последнего хода
    void rollback()
    {
        auto beat_series = max(1, *(history_beat_series.rbegin()));
        while (beat_series-- && history_mtx.size() > 1)
        {
            history_mtx.pop_back();  // Удаление последних ходов из истории
            history_beat_series.pop_back();
        }
        mtx = *(history_mtx.rbegin());  // Восстановление предыдущего состояния доски
        clear_highlight();  // Сброс выделений
        clear_active();  // Сброс активной клетки
    }

    // Метод для отображения результата игры
    void show_final(const int res)
    {
        game_results = res;
        rerender();  // Перерисовка доски
    }

    // Метод для обновления размеров окна
    void reset_window_size()
    {
        SDL_GetRendererOutputSize(ren, &W, &H);
        rerender();  // Перерисовка доски
    }

    // Метод для завершения работы с SDL
    void quit()
    {
        SDL_DestroyTexture(board);
        SDL_DestroyTexture(w_piece);
        SDL_DestroyTexture(b_piece);
        SDL_DestroyTexture(w_queen);
        SDL_DestroyTexture(b_queen);
        SDL_DestroyTexture(back);
        SDL_DestroyTexture(replay);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
    }

    ~Board()
    {
        if (win)
            quit();  // Вызов метода завершения работы при уничтожении объекта
    }

private:
    // Метод для добавления текущего состояния доски в историю
    void add_history(const int beat_series = 0)
    {
        history_mtx.push_back(mtx);
        history_beat_series.push_back(beat_series);
    }

    // Метод для создания начальной матрицы доски
    void make_start_mtx()
    {
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                mtx[i][j] = 0;  // Обнуление всех клеток
                if (i < 3 && (i + j) % 2 == 1)
                    mtx[i][j] = 2;  // Расстановка черных фигур
                if (i > 4 && (i + j) % 2 == 1)
                    mtx[i][j] = 1;  // Расстановка белых фигур
            }
        }
        add_history();  // Добавление начального состояния в историю
    }

    // Метод для перерисовки доски
    void rerender()
    {
        // Очистка рендера и отрисовка доски
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, board, NULL, NULL);

        // Отрисовка фигур
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (!mtx[i][j])
                    continue;

                int wpos = W * (j + 1) / 10 + W / 120;
                int hpos = H * (i + 1) / 10 + H / 120;
                SDL_Rect rect{ wpos, hpos, W / 12, H / 12 };

                SDL_Texture* piece_texture;
                if (mtx[i][j] == 1)
                    piece_texture = w_piece;
                else if (mtx[i][j] == 2)
                    piece_texture = b_piece;
                else if (mtx[i][j] == 3)
                    piece_texture = w_queen;
                else
                    piece_texture = b_queen;

                SDL_RenderCopy(ren, piece_texture, NULL, &rect);
            }
        }

        // Отрисовка выделений
        SDL_SetRenderDrawColor(ren, 0, 255, 0, 0);
        const double scale = 2.5;
        SDL_RenderSetScale(ren, scale, scale);
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (!is_highlighted_[i][j])
                    continue;
                SDL_Rect cell{ int(W * (j + 1) / 10 / scale), int(H * (i + 1) / 10 / scale), int(W / 10 / scale),
                              int(H / 10 / scale) };
                SDL_RenderDrawRect(ren, &cell);
            }
        }

        // Отрисовка активной клетки
        if (active_x != -1)
        {
            SDL_SetRenderDrawColor(ren, 255, 0, 0, 0);
            SDL_Rect active_cell{ int(W * (active_y + 1) / 10 / scale), int(H * (active_x + 1) / 10 / scale),
                                 int(W / 10 / scale), int(H / 10 / scale) };
            SDL_RenderDrawRect(ren, &active_cell);
        }
        SDL_RenderSetScale(ren, 1, 1);

        // Отрисовка кнопок управления
        SDL_Rect rect_left{ W / 40, H / 40, W / 15, H / 15 };
        SDL_RenderCopy(ren, back, NULL, &rect_left);
        SDL_Rect replay_rect{ W * 109 / 120, H / 40, W / 15, H / 15 };
        SDL_RenderCopy(ren, replay, NULL, &replay_rect);

        // Отрисовка результата игры
        if (game_results != -1)
        {
            string result_path = draw_path;
            if (game_results == 1)
                result_path = white_path;
            else if (game_results == 2)
                result_path = black_path;
            SDL_Texture* result_texture = IMG_LoadTexture(ren, result_path.c_str());
            if (result_texture == nullptr)
            {
                print_exception("IMG_LoadTexture can't load game result picture from " + result_path);
                return;
            }
            SDL_Rect res_rect{ W / 5, H * 3 / 10, W * 3 / 5, H * 2 / 5 };
            SDL_RenderCopy(ren, result_texture, NULL, &res_rect);
            SDL_DestroyTexture(result_texture);
        }

        SDL_RenderPresent(ren);
        SDL_Delay(10);
        SDL_Event windowEvent;
        SDL_PollEvent(&windowEvent);
    }

    // Метод для записи ошибок в лог
    void print_exception(const string& text) {
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Error: " << text << ". " << SDL_GetError() << endl;
        fout.close();
    }

public:
    int W = 0;  // Ширина окна
    int H = 0;  // Высота окна
    vector<vector<vector<POS_T>>> history_mtx;  // История состояний доски

private:
    SDL_Window* win = nullptr;  // Указатель на окно SDL
    SDL_Renderer* ren = nullptr;  // Указатель на рендерер SDL
    SDL_Texture* board = nullptr;  // Текстура доски
    SDL_Texture* w_piece = nullptr;  // Текстура белой фигуры
    SDL_Texture* b_piece = nullptr;  // Текстура черной фигуры
    SDL_Texture* w_queen = nullptr;  // Текстура белой дамки
    SDL_Texture* b_queen = nullptr;  // Текстура черной дамки
    SDL_Texture* back = nullptr;  // Текстура кнопки "Назад"
    SDL_Texture* replay = nullptr;  // Текстура кнопки "Перезапуск"
    const string textures_path = project_path + "Textures/";  // Путь к текстурам
    const string board_path = textures_path + "board.png";  // Путь к текстуре доски
    const string piece_white_path = textures_path + "white_piece.png";  // Путь к текстуре белой фигуры
    const string piece_black_path = textures_path + "black_piece.png";  // Путь к текстуре черной фигуры
    const string queen_white_path = textures_path + "white_queen.png";  // Путь к текстуре белой дамки
    const string queen_black_path = textures_path + "black_queen.png";  // Путь к текстуре черной дамки
    const string back_path = textures_path + "back.png";  // Путь к текстуре кнопки "Назад"
    const string replay_path = textures_path + "replay.png";  // Путь к текстуре кнопки "Перезапуск"
    const string draw_path = textures_path + "draw.png";  // Путь к текстуре ничьей
    const string white_path = textures_path + "white_win.png";  // Путь к текстуре победы белых
    const string black_path = textures_path + "black_win.png";  // Путь к текстуре победы черных
    vector<vector<POS_T>> mtx = vector<vector<POS_T>>(8, vector<POS_T>(8, 0));  // Матрица доски
    vector<vector<int>> is_highlighted_ = vector<vector<int>>(8, vector<int>(8, 0));  // Матрица выделений
    POS_T active_x = -1;  // Координата X активной клетки
    POS_T active_y = -1;  // Координата Y активной клетки
    int game_results = -1;  // Результат игры (-1: игра продолжается, 0: ничья, 1: победа белых, 2: победа черных)
    vector<int> history_beat_series;  // История серий битья
};

