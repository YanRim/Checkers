#pragma once  
#include <stdlib.h>  

// Определяем тип данных для координат 
typedef int8_t POS_T;

// Структура, представляющая ход в игре
struct move_pos
{
    POS_T x, y;             // Начальные координаты хода (откуда)
    POS_T x2, y2;           // Конечные координаты хода (куда)
    POS_T xb = -1, yb = -1; // Координаты побитой фигуры (по умолчанию -1, что означает "нет")

    // Конструктор для простого хода без побития
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2)
        : x(x), y(y), x2(x2), y2(y2)
    {
    }

    // Конструктор для хода с побитием
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    // Перегрузка оператора сравнения (==)
    // Проверяет равенство двух ходов по всем координатам
    bool operator==(const move_pos& other) const
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }

    // Перегрузка оператора неравенства (!=)
    // Использует уже определённый оператор ==
    bool operator!=(const move_pos& other) const
    {
        return !(*this == other);
    }
};
