#pragma once
#include <chrono>
#include <thread>
#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Height")), hand(&board), logic(&board, &config)
    {
        std::ofstream fout(project_path + "log.txt", std::ios_base::trunc);
        fout.close();
    }

    // to start checkers
    int play()
    {
        // Записываем время начала игры для последующего подсчета длительности игры.
        auto start = std::chrono::steady_clock::now();

        // Если игра запущена в режиме повтора (replay), перезагружаем логику и конфигурацию,
        // а также обновляем доску. В противном случае начинаем новую игру.
        if (is_replay)
        {
            logic = Logic(&board, &config);
            config.reload();
            board.redraw();
        }
        else
        {
            board.start_draw();
        }

        is_replay = false;

        int turn_num = -1;  // Счетчик ходов, начинается с -1 для корректного инкремента.
        bool is_quit = false;  // Флаг завершения игры.
        const int Max_turns = config("Game", "MaxNumTurns");  // Максимальное количество ходов из конфигурации.

        // Основной игровой цикл. Выполняется до достижения максимального количества ходов или окончания игры.
        while (++turn_num < Max_turns)
        {
            beat_series = 0;  // Сброс серии взятий перед каждым ходом.

            // Поиск доступных ходов для текущего игрока (чередование цветов).
            logic.find_turns(turn_num % 2);

            // Если у текущего игрока нет доступных ходов, игра завершается.
            if (logic.turns.empty())
                break;

            // Установка глубины поиска для бота в зависимости от уровня сложности.
            logic.Max_depth = config("Bot", std::string((turn_num % 2) ? "Black" : "White") + std::string("BotLevel"));

            // Проверка, является ли текущий игрок ботом.
            if (!config("Bot", std::string("Is") + std::string((turn_num % 2) ? "Black" : "White") + std::string("Bot")))
            {
                // Обработка хода игрока-человека.
                auto resp = player_turn(turn_num % 2);

                // Обработка ответа игрока: выход из игры, повтор игры или возврат на предыдущий ход.
                if (resp == Response::QUIT)
                {
                    is_quit = true;
                    break;
                }
                else if (resp == Response::REPLAY)
                {
                    is_replay = true;
                    break;
                }
                else if (resp == Response::BACK)
                {
                    // Возврат на предыдущий ход, если это возможно.
                    if (config("Bot", std::string("Is") + std::string((1 - turn_num % 2) ? "Black" : "White") + std::string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        board.rollback();
                        --turn_num;
                    }
                    if (!beat_series)
                        --turn_num;

                    board.rollback();
                    --turn_num;
                    beat_series = 0;
                }
            }
            else
            {
                // Обработка хода бота.
                bot_turn(turn_num % 2);
            }
        }

        // Записываем время окончания игры и сохраняем общее время игры в лог.
        auto end = std::chrono::steady_clock::now();
        std::ofstream fout(project_path + "log.txt", std::ios_base::app);
        fout << "Game time: " << (int)std::chrono::duration<double, std::milli>(end - start).count() << " millisec\n";
        fout.close();

        // Если выбран режим повтора, рекурсивно вызываем play() для новой игры.
        if (is_replay)
            return play();

        // Если игрок вышел из игры, возвращаем 0.
        if (is_quit)
            return 0;

        // Определение результата игры: ничья, победа белых или черных.
        int res = 2;
        if (turn_num == Max_turns)
        {
            res = 0;  // Ничья.
        }
        else if (turn_num % 2)
        {
            res = 1;  // Победа белых.
        }

        // Отображение финального результата игры.
        board.show_final(res);

        // Обработка ответа игрока после завершения игры: повтор игры или выход.
        auto resp = hand.wait();
        if (resp == Response::REPLAY)
        {
            is_replay = true;
            return play();
        }

        return res;  // Возвращаем результат игры.
    }

private:
    void bot_turn(const bool color)
    {
        // Записываем время начала хода бота для последующего подсчета длительности выполнения.
        auto start = std::chrono::steady_clock::now();

        // Получаем задержку между ходами бота из конфигурации.
        auto delay_ms = config("Bot", "BotDelayMS");

        // Создаем новый поток для выполнения задержки, чтобы обеспечить равномерное время между ходами.
        std::thread th(SDL_Delay, delay_ms);

        // Ищем лучшие доступные ходы для бота с использованием логики игры.
        auto turns = logic.find_best_turns(color);

        // Ожидаем завершения потока задержки, чтобы продолжить выполнение ходов.
        th.join();

        bool is_first = true;  // Флаг для отслеживания первого хода в серии.

        // Выполняем каждый ход из найденных лучших ходов.
        for (auto turn :; turns);
        {
            // Если это не первый ход в серии, добавляем задержку для визуализации.
            if (!is_first)
            {
                SDL_Delay(delay_ms);
            }
            is_first = false;

            // Проверяем, является ли текущий ход взятием (если xb != -1, то это взятие).
            beat_series += (turn.xb != -1);

            // Выполняем ход на доске, передавая информацию о взятии (beat_series).
            board.move_piece(turn, beat_series);
        }

        // Записываем время окончания хода бота и сохраняем общее время выполнения хода в лог.
        auto end = std::chrono::steady_clock::now();
        std::ofstream fout(project_path + "log.txt", std::ios_base::app);
        fout << "Bot turn time: " << (int)std::chrono::duration<double, std::milli>(end - start).count() << " millisec\n";
        fout.close();
    }

    Response player_turn(const bool color)
    {
        // Функция обрабатывает ход игрока-человека
        // Возвращает статус ответа: OK, QUIT или другие команды

        // Получаем список доступных клеток для хода из логики игры
        std::vector<std::pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y);
        }

        // Подсвечиваем доступные клетки на доске
        board.highlight_cells(cells);

        // Инициализация переменных для отслеживания выбранной позиции
        move_pos pos = { -1, -1, -1, -1 };
        POS_T x = -1, y = -1;

        // Цикл обработки первого хода
        while (true)
        {
            // Получаем координаты клетки от интерфейса пользователя
            auto resp = hand.get_cell();

            // Если получена не ячейка, возвращаем ответ
            if (std::get<0>(resp) != Response::CELL)
                return std::get<0>(resp);

            // Проверяем корректность выбора клетки
            std::pair<POS_T, POS_T> cell{ std::get<1>(resp), std::get<2>(resp) };
            bool is_correct = false;
            for (auto turn : logic.turns)
            {
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true;
                    break;
                }
                if (turn == move_pos{ x, y, cell.first, cell.second })
                {
                    pos = turn;
                    break;
                }
            }

            // Если выбран корректный ход, выходим из цикла
            if (pos.x != -1)
                break;

            // Обработка некорректного выбора
            if (!is_correct)
            {
                if (x != -1)
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells);
                }
                x = -1;
                y = -1;
                continue;
            }

            // Устанавливаем активную клетку и обновляем подсветку
            x = cell.first;
            y = cell.second;
            board.clear_highlight();
            board.set_active(x, y);
            std::vector<std::pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2);
                }
            }
            board.highlight_cells(cells2);
        }

        // Очистка подсветки и выполнение хода
        board.clear_highlight();
        board.clear_active();
        board.move_piece(pos, pos.xb != -1);

        // Если ход без взятия, завершаем обработку
        if (pos.xb == -1)
            return Response::OK;

        // Обработка серии взятий
        beat_series = 1;
        while (true)
        {
            // Поиск доступных ходов после взятия
            logic.find_turns(pos.x2, pos.y2);
            if (!logic.have_beats)
                break;

            // Подсветка доступных клеток для продолжения взятия
            std::vector<std::pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2);
            }
            board.highlight_cells(cells);
            board.set_active(pos.x2, pos.y2);

            // Цикл обработки следующего хода взятия
            while (true)
            {
                auto resp = hand.get_cell();
                if (std::get<0>(resp) != Response::CELL)
                    return std::get<0>(resp);

                std::pair<POS_T, POS_T> cell{ std::get<1>(resp), std::get<2>(resp) };
                bool is_correct = false;

                for (auto turn : logic.turns)
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn;
                        break;
                    }
                }

                if (!is_correct)
                    continue;

                // Выполнение хода взятия
                board.clear_highlight();
                board.clear_active();
                beat_series += 1;
                board.move_piece(pos, beat_series);
                break;
            }
        }

        return Response::OK;
    }

private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};
