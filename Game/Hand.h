#pragma once 
#include <tuple>
#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// Класс Hand представляет обработку пользовательского ввода для игрового поля
class Hand
{
public:
    // Конструктор принимает указатель на объект игрового поля
    Hand(Board* board) : board(board)
    {
    }

    // Метод для получения координат выбранной ячейки и типа ответа
    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent;  // Структура для хранения событий окна
        Response resp = Response::OK;
        int x = -1, y = -1;  // Координаты мыши
        int xc = -1, yc = -1;  // Вычисленные координаты ячейки

        // Основной цикл обработки событий
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:  // Обработка закрытия окна
                    resp = Response::QUIT;
                    break;

                case SDL_MOUSEBUTTONDOWN:  // Обработка нажатия мыши
                    x = windowEvent.motion.x;
                    y = windowEvent.motion.y;

                    // Вычисляем координаты ячейки
                    xc = int(y / (board->H / 10) - 1);
                    yc = int(x / (board->W / 10) - 1);

                    // Проверяем специальные зоны интерфейса
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        resp = Response::BACK;  // Кнопка "Назад"
                    }
                    else if (xc == -1 && yc == 8)
                    {
                        resp = Response::REPLAY;  // Кнопка "Переиграть"
                    }
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        resp = Response::CELL;  // Выбрана ячейка на доске
                    }
                    else
                    {
                        xc = -1;  // Сброс координат при ошибочном клике
                        yc = -1;
                    }
                    break;

                case SDL_WINDOWEVENT:  // Обработка изменения размера окна
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        board->reset_window_size();
                        break;
                    }
                }

                // Выход из цикла при получении значимого ответа
                if (resp != Response::OK)
                    break;
            }
        }
        return { resp, xc, yc };  // Возвращаем результат
    }

    // Метод ожидания пользовательского действия
    Response wait() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;

        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:  // Закрытие окна
                    resp = Response::QUIT;
                    break;

                case SDL_WINDOWEVENT_SIZE_CHANGED:  // Изменение размера окна
                    board->reset_window_size();
                    break;

                case SDL_MOUSEBUTTONDOWN:  // Нажатие мыши
                {
                    int x = windowEvent.motion.x;
                    int y = windowEvent.motion.y;

                    // Вычисляем координаты ячейки
                    int xc = int(y / (board->H / 10) - 1);
                    int yc = int(x / (board->W / 10) - 1);

                    // Проверяем кнопку "Переиграть"
                    if (xc == -1 && yc == 8)
                        resp = Response::REPLAY;
                }
                break;
                }

                // Выход из цикла при получении значимого ответа
                if (resp != Response::OK)
                    break;
            }
        }
        return resp;
    }

private:
    Board* board;  // Указатель на объект игрового поля
};
