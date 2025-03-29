#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
public:
    Config()
    {
        reload();
    }

    // Функция reload() перезагружает конфигурационный файл settings.json
    // Она открывает файл настроек и парсит его содержимое в объект JSON config
    // Используется для обновления настроек без перезапуска программы
    void reload()
    {
        std::ifstream fin(project_path + "settings.json");
        fin >> config;
        fin.close();
    }

    // Оператор круглых скобок определён для удобного доступа к настройкам
    // Он позволяет получать значения из конфигурации по двум ключам:
    // - setting_dir (раздел настроек)
    // - setting_name (имя конкретной настройки)
    auto operator()(const string& setting_dir, const string& setting_name) const
    {
        return config[setting_dir][setting_name];
    }

private:
    json config;
};
