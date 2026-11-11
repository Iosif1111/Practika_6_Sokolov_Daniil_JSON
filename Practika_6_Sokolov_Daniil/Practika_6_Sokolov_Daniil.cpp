#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <clocale> 
#include <cstdlib> 

#ifdef _WIN32
#include <windows.h> 
#endif

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std::chrono_literals;

std::string Utf8ToConsole(const std::string& utf8_str)
{
#ifdef _WIN32
    if (utf8_str.empty()) return "";

    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return utf8_str;

    std::vector<wchar_t> wbuf(wlen);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, wbuf.data(), wlen);

    int clen = WideCharToMultiByte(CP_ACP, 0, wbuf.data(), -1, nullptr, 0, nullptr, nullptr);
    if (clen <= 0) return utf8_str;

    std::vector<char> cbuf(clen);
    WideCharToMultiByte(CP_ACP, 0, wbuf.data(), -1, cbuf.data(), clen, nullptr, nullptr);

    return std::string(cbuf.data());
#else
    return utf8_str;
#endif
}

enum TrafficLightsState
{
    RED,
    YELLOW,
    GREEN
};

struct Address
{
    std::string country;
    std::string city;
    std::string street;
    std::string coordinates;
};

class TrafficLight
{
private:
    int start_offset;

public:
    Address address;

    TrafficLight(Address addr) : address(addr)
    {
        start_offset = std::rand() % 4;
    }

    TrafficLightsState GetCurrentState();
    void GetInfo();
};

// Безопасная функция чтения без неявного приведения типов в .value()
TrafficLight LoadTrafficLightFromJson(const std::string& filename)
{
    std::ifstream file(filename);

    if (!file.is_open())
    {
        std::cerr << "\033[1;31mОшибка: Не удалось открыть файл " << filename << "\033[0m\n";
        return TrafficLight({ "Неизвестно", "Неизвестно", filename, "0.0, 0.0" });
    }

    try
    {
        json data = json::parse(file); // Используем явный json::parse вместо оператора >>

        Address addr;
        addr.country = data.contains("country") && data["country"].is_string()
            ? Utf8ToConsole(data["country"].get<std::string>()) : "Не указано";

        addr.city = data.contains("city") && data["city"].is_string()
            ? Utf8ToConsole(data["city"].get<std::string>()) : "Не указано";

        addr.street = data.contains("street") && data["street"].is_string()
            ? Utf8ToConsole(data["street"].get<std::string>()) : "Не указано";

        addr.coordinates = data.contains("coordinates") && data["coordinates"].is_string()
            ? Utf8ToConsole(data["coordinates"].get<std::string>()) : "0.0, 0.0";

        return TrafficLight(addr);
    }
    catch (const std::exception& e)
    {
        std::cerr << "\033[1;31mОшибка разбора JSON в файле " << filename << ": " << e.what() << "\033[0m\n";
        return TrafficLight({ "Ошибка JSON", "Ошибка JSON", filename, "0.0, 0.0" });
    }
}

TrafficLightsState TrafficLight::GetCurrentState()
{
    static auto start_time = std::chrono::system_clock::now();
    auto now = std::chrono::system_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
    long long total_seconds = duration.count();

    long long step = total_seconds / 23;
    int phase = (step + start_offset) % 4;

    if (phase == 0)
    {
        return TrafficLightsState::GREEN;
    }
    else if (phase == 1)
    {
        return TrafficLightsState::YELLOW;
    }
    else if (phase == 2)
    {
        return TrafficLightsState::RED;
    }
    else
    {
        return TrafficLightsState::YELLOW;
    }
}

void TrafficLight::GetInfo()
{
    std::cout << "Адрес светофора: "
        << address.country << ", г. "
        << address.city << ", "
        << address.street << " ("
        << address.coordinates << ")\n";

    TrafficLightsState state = GetCurrentState();

    std::cout << "Текущий свет светофора: ";

    switch (state)
    {
    case TrafficLightsState::RED:
        std::cout << "\033[1;31m[ КРАСНЫЙ ]\033[0m\n";
        break;
    case TrafficLightsState::YELLOW:
        std::cout << "\033[1;33m[ ЖЕЛТЫЙ ]\033[0m\n";
        break;
    case TrafficLightsState::GREEN:
        std::cout << "\033[1;32m[ ЗЕЛЕНЫЙ ]\033[0m\n";
        break;
    }
    std::cout << "----------------------------------------\n";
}

int main()
{
    std::setlocale(LC_ALL, "");
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    std::vector<TrafficLight> lights;
    lights.push_back(LoadTrafficLightFromJson("traffic_light_1.json"));
    lights.push_back(LoadTrafficLightFromJson("traffic_light_2.json"));
    lights.push_back(LoadTrafficLightFromJson("traffic_light_3.json"));

    std::cout << "=== Запуск мониторинга всех светофоров ===\n\n";

    while (true)
    {
        for (size_t i = 0; i < lights.size(); ++i)
        {
            std::cout << "Светофор #" << (i + 1) << ":\n";
            lights[i].GetInfo();
        }

        std::cout << "Ожидание 23 секунды до следующего опроса...\n";
        std::cout << "========================================\n\n";

        std::this_thread::sleep_for(23000ms);
    }

    return 0;
}