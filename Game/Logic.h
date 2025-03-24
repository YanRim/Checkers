#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9;

class Logic
{
public:
    /**
     * Конструктор класса Logic.
     * @param board Указатель на объект доски.
     * @param config Указатель на объект конфигурации.
     */
    Logic(Board* board, Config* config) : board(board), config(config)
    {
        rand_eng = std::default_random_engine(
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType");
        optimization = (*config)("Bot", "Optimization");
    }

    /**
     * Выполняет ход на доске.
     * @param mtx Текущее состояние доски.
     * @param turn Ход, который нужно выполнить.
     * @return Новое состояние доски после выполнения хода.
     */
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        if (turn.xb != -1)
            mtx[turn.xb][turn.yb] = 0; // Удаляем побитую фигуру
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2; // Превращаем пешку в дамку
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y]; // Перемещаем фигуру
        mtx[turn.x][turn.y] = 0; // Очищаем исходную позицию
        return mtx;
    }

    /**
     * Рассчитывает оценку текущего состояния доски.
     * @param mtx Текущее состояние доски.
     * @param first_bot_color Цвет бота (true - белые, false - черные).
     * @return Оценка состояния доски (чем меньше значение, тем лучше для бота).
     */
    double calc_score(const vector<vector<POS_T>>& mtx, const bool first_bot_color) const
    {
        double w = 0, wq = 0, b = 0, bq = 0;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1); // Считаем белые пешки
                wq += (mtx[i][j] == 3); // Считаем белые дамки
                b += (mtx[i][j] == 2); // Считаем черные пешки
                bq += (mtx[i][j] == 4); // Считаем черные дамки
                if (scoring_mode == "NumberAndPotential")
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i); // Потенциал белых пешек
                    b += 0.05 * (mtx[i][j] == 2) * (i); // Потенциал черных пешек
                }
            }
        }
        if (!first_bot_color)
        {
            swap(b, w);
            swap(bq, wq);
        }
        if (w + wq == 0)
            return INF;
        if (b + bq == 0)
            return 0;
        int q_coef = 4;
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5;
        }
        return (b + bq * q_coef) / (w + wq * q_coef);
    }

    /**
     * Находит все возможные ходы для заданного цвета.
     * @param color Цвет игрока.
     */
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board());
    }

    /**
     * Находит все возможные ходы для фигуры на позиции (x, y).
     * @param x Координата x фигуры.
     * @param y Координата y фигуры.
     */
    void find_turns(const POS_T x, const POS_T y)
    {
        find_turns(x, y, board->get_board());
    }

private:
    /**
     * Находит все возможные ходы для заданного цвета на заданной доске.
     * @param color Цвет игрока.
     * @param mtx Текущее состояние доски.
     */
    void find_turns(const bool color, const vector<vector<POS_T>>& mtx)
    {
        vector<move_pos> res_turns;
        bool have_beats_before = false;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (mtx[i][j] && mtx[i][j] % 2 != color)
                {
                    find_turns(i, j, mtx);
                    if (have_beats && !have_beats_before)
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns;
        shuffle(turns.begin(), turns.end(), rand_eng);
        have_beats = have_beats_before;
    }

    /**
     * Находит все возможные ходы для фигуры на позиции (x, y) на заданной доске.
     * @param x Координата x фигуры.
     * @param y Координата y фигуры.
     * @param mtx Текущее состояние доски.
     */
    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>>& mtx)
    {
        turns.clear();
        have_beats = false;
        POS_T type = mtx[x][y];
        // Проверяем возможность взятия
        switch (type)
        {
        case 1:
        case 2:
            // Проверяем пешки
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2;
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb);
                }
            }
            break;
        default:
            // Проверяем дамки
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1;
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                        {
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                            {
                                break;
                            }
                            xb = i2;
                            yb = j2;
                        }
                        if (xb != -1 && xb != i2)
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }
        // Проверяем другие ходы
        if (!turns.empty())
        {
            have_beats = true;
            return;
        }
        switch (type)
        {
        case 1:
        case 2:
            // Проверяем пешки
        {
            POS_T i = ((type % 2) ? x - 1 : x + 1);
            for (POS_T j = y - 1; j <= y + 1; j += 2)
            {
                if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                    continue;
                turns.emplace_back(x, y, i, j);
            }
            break;
        }
        default:
            // Проверяем дамки
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                            break;
                        turns.emplace_back(x, y, i2, j2);
                    }
                }
            }
            break;
        }
    }

public:
    vector<move_pos> turns; // Список доступных ходов
    bool have_beats; // Флаг наличия взятий
    int Max_depth; // Максимальная глубина поиска

private:
    default_random_engine rand_eng; // Генератор случайных чисел
    string scoring_mode; // Тип оценочной функции
    string optimization; // Тип оптимизации
    vector<move_pos> next_move; // Следующий ход
    vector<int> next_best_state; // Состояние следующего лучшего хода
    Board* board; // Указатель на объект доски
    Config* config; // Указатель на объект конфигурации
};
