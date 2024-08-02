#include <iomanip>
#include <iostream>
#include <string>
#include <boost/any.hpp>
#include <boost/endian/conversion.hpp>
#include <nlohmann/json.hpp>
#include <cstring>
#include <random>
#include <array>
#include <cstdint>
#include <chrono>
#include <thread>
#include <winsock2.h>
#include <curl/curl.h>
#include <asio.hpp>
#include <algorithm> 
#include <openssl/sha.h> 

 using json = nlohmann::json;

int64_t getCurrentTimestamp() {
    // Получаем текущее время в секундах
    time_t seconds;

    seconds = time(NULL);

    return seconds;
}

uint64_t generateNonce() {
    // Создаем генератор случайных чисел
    std::random_device rd; // Используем аппаратный генератор случайных чисел
    std::mt19937_64 gen(rd()); // 64-битный генератор
    std::uniform_int_distribution<uint64_t> dis; // Равномерное распределение

    return dis(gen); // Генерируем случайное 64-битное число
}

// Функция обратного вызова для записи данных
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int get_current_height() {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://blockchain.info/latestblock");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            auto j = nlohmann::json::parse(readBuffer);
            return j["height"];
        }
    }
    return 0; // В случае ошибки
}

std::string get_my_ip() {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.ipify.org?format=json");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            auto j = nlohmann::json::parse(readBuffer);
            return j["ip"];
        }
    }
    return "0"; // В случае ошибки
}

int32_t get_version_protocol(const std::string& host, const std::string& port) {
    // URL, который будет храниться в переменной
    std::string url = "https://bitnodes.io/api/v1/nodes/" + host + "-" + port + "/";

    // Инициализация libcurl
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        // Установка URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Установка функции обратного вызова для обработки данных
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

        // Установка буфера для записи данных
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Выполнение запроса
        res = curl_easy_perform(curl);

        // Проверка на ошибки
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        else {
            // Парсинг JSON
            json parsedJson = json::parse(readBuffer);

            // Проверка наличия ключа "data" и извлечение первого элемента
            if (parsedJson.contains("data") && parsedJson["data"].is_array() && !parsedJson["data"].empty()) {
                json firstElement = parsedJson["data"][0]; // Извлечение первого элемента
                int32_t version = static_cast<int32_t>(std::stoi(firstElement.dump(4)));
                return version;
            }
            else {
                std::cout << "No data found or 'data' is not an array." << std::endl;
            }
        }

        // Очистка
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return 0;
}

// Функция для сериализации int32_t в буфер
void serializeInt32(std::vector<uint8_t>& buffer, int32_t value) {
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));         // младший байт
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));  // второй байт
    buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF)); // третий байт
    buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF)); // старший байт
}

// Функция для сериализации uint64_t в буфер
void serializeInt64(std::vector<uint8_t>& buffer, uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        buffer.push_back(static_cast<uint8_t>(value & 0xFF)); // Младший байт
        value >>= 8; // Сдвигаем на 8 бит
    }
}

// Функция для сериализации IP-адреса (IPv4)
void serializeIPAddress(std::vector<uint8_t>& buffer, const std::string& ip) {
    int a, b, c, d;
    sscanf_s(ip.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d);
    buffer.push_back(static_cast<uint8_t>(a));
    buffer.push_back(static_cast<uint8_t>(b));
    buffer.push_back(static_cast<uint8_t>(c));
    buffer.push_back(static_cast<uint8_t>(d));
}

// Функция для сериализации порта
void serializePort(std::vector<uint8_t>& buffer, uint16_t port) {
    buffer.push_back(static_cast<uint8_t>((port >> 8) & 0xFF)); // Старший байт
    buffer.push_back(static_cast<uint8_t>(port & 0xFF));        // Младший байт
}

// Функция для сериализации строки user_agent
void serializeUserAgent(std::vector<uint8_t>& buffer, const std::string& userAgent) {
    // Сериализуем длину строки как 8-битное целое число
    buffer.push_back(static_cast<uint8_t>(userAgent.size()));

    // Сериализуем саму строку
    buffer.insert(buffer.end(), userAgent.begin(), userAgent.end());
}

// Функция для сериализации bool
void serializeBool(std::vector<uint8_t>& buffer, bool value) {
    buffer.push_back(value ? 0x01 : 0x00);
}

// Функция для сериализации длины полезной нагрузки
std::uint32_t serializePayloadLength(std::size_t payloadLength) {
    if (payloadLength > 0xFFFFFFFF) { // Если длина больше 4 Гб
        return 0xFFFFFFFF;
    }
    else {
        return static_cast<std::uint32_t>(payloadLength);
    }
}

void show_content(std::vector<uint8_t>& content, std::string type) {
    // Выводим содержимое буфера
    std::cout << "Serialized " + type + " message: ";
    for (auto byte : content) {
        std::cout << std::hex << static_cast<int>(byte) << " ";
    } std::cout << std::endl;
    std::cout << std::dec << std::endl; // Возвращаемся к десятичной системе
}

// Функция для вычисления контрольной суммы
uint32_t calculateChecksum(const std::vector<uint8_t>& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];

    // Первое хеширование
    SHA256(data.data(), data.size(), hash);

    // Второе хеширование
    SHA256(hash, SHA256_DIGEST_LENGTH, hash);

    // Возвращаем первые 4 байта как контрольную сумму
    return *(reinterpret_cast<uint32_t*>(hash));
}


class BitcoinNode {
public:
    BitcoinNode(const std::string& host, const std::string& port)
        : endpoint(boost::asio::ip::make_address(host), std::stoi(port)),
        socket(io_service) {}


    void connect() {
        try {
            socket.connect(endpoint);
            std::cout << "Successfully connected to Bitcoin node at " << endpoint << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to connect to Bitcoin node: " << e.what() << std::endl;
        }
    }

    void sendVersion() {

        std::vector<uint8_t> version_message = createVersionMessage();
        show_content(version_message, "version_message");
        
        boost::system::error_code error;

        // Отправка сообщения через сокет
        boost::asio::write(socket, boost::asio::buffer(version_message), error);

        if (error) {
            std::cerr << "Failed to send version message: " << error.message() << std::endl;
        }
        else {
            std::cout << "Version message sent successfully." << std::endl;
        }

    }



    void waitForVerack() {
        std::vector<uint8_t> buffer(24); // Размер буфера для сообщения
        boost::system::error_code error;

        // Чтение сообщения из сокета
        size_t len = boost::asio::read(socket, boost::asio::buffer(buffer), error);

        if (error) {
            std::cerr << "Failed to read from socket: " << error.message() << std::endl;
            return;
        }

        // Проверка, является ли сообщение verack
        if (len >= 24 && buffer[0] == 0xD9 && buffer[1] == 0xB4 && buffer[2] == 0xBE && buffer[3] == 0xF9) {
            // Проверка команды
            std::string command(buffer.begin() + 4, buffer.begin() + 16);
            if (command == "verack") {
                std::cout << "Received verack message." << std::endl;
            }
        }
        else {
            std::cout << "Received unknown message." << std::endl;
            std::cout << "Received message: ";
            for (size_t i = 0; i < len; ++i) {
                std::cout << std::hex << static_cast<int>(buffer[i]) << " ";
            }

        }
    }


private:
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::endpoint endpoint;
    boost::asio::ip::tcp::socket socket;


    std::vector<uint8_t> createVersionMessage() {

        // Определение структуры для хранения данных сообщения version
        struct VersionMessage {
            int32_t version;           // Версия протокола
            uint64_t services;         // Сервисы
            uint64_t timestamp;        // Временная метка
            std::string addr_recv_ip;  // IP-адрес получателя
            uint16_t addr_recv_port;   // Порт получателя
            std::string addr_from_ip;  // IP-адрес отправителя
            uint16_t addr_from_port;    // Порт отправителя
            uint64_t nonce;            // Nonce
            std::string user_agent;    // User agent
            int32_t start_height;      // Начальная высота блока
            bool relay;                // Флаг реле
        };

        struct NetAddr {
            std::string ip; // IPv6 адрес (можно использовать IPv4 в виде IPv6)
            uint16_t port;               // Порт

            NetAddr() : ip{}, port(0) {}
        };

        int32_t version;             // Версия протокола
        uint64_t services;           // Услуги
        int64_t timestamp;           // Время в формате UNIX
        NetAddr addr_recv;           // Адрес получателя
        NetAddr addr_from;           // Адрес отправителя
        uint64_t nonce;              // Случайное число
        std::string user_agent;      // Агент пользователя
        int32_t start_height;        // Высота блока
        bool relay;                  // Указывает, должен ли удаленный узел анонсировать транзакции

        std::string host = endpoint.address().to_string();
        unsigned short port_ = endpoint.port();
        std::string port = std::to_string(port_);

        version = get_version_protocol(host, port);
        services = 0x1 | 0x2 | 0x4 | 0x8 | 0x10 | 0x20 | 0x40 | 0x80 | 0x100;
        timestamp = getCurrentTimestamp();
        addr_recv.ip = host;
        addr_recv.port = std::stoi(port);
        addr_from.ip = get_my_ip();
        addr_from.port = 8333;
        nonce = generateNonce(); // Генерируем nonce
        user_agent = "/MegaMiner:1.0/";
        start_height = get_current_height();
        relay = true;

        std::vector<uint8_t> versionBuffer;
        // Сериализуем версию
        serializeInt32(versionBuffer, version);


        std::vector<uint8_t> servicesBuffer;
        // Сериализация services в 8 байт
        serializeInt64(versionBuffer, services);


        std::vector<uint8_t> timestampBuffer;
        // Сериализация timestamp в 8 байт
        serializeInt64(versionBuffer, timestamp);


        std::vector<uint8_t> addr_recvBuffer;
        // Сериализация IP-адреса
        serializeIPAddress(versionBuffer, addr_recv.ip);
        // Сериализация порта
        serializePort(versionBuffer, addr_recv.port);


        std::vector<uint8_t> addr_fromBuffer;
        // Сериализация IP-адреса
        serializeIPAddress(versionBuffer, addr_from.ip);
        // Сериализация порта
        serializePort(versionBuffer, addr_from.port);


        std::vector<uint8_t> nonceBuffer;
        // Сериализация nonce в 8 байт
        serializeInt64(versionBuffer, nonce);


        std::vector<uint8_t> userBuffer;
        // Сериализация user_agent
        serializeUserAgent(versionBuffer, user_agent);
        

        std::vector<uint8_t> heightBuffer;
        // Сериализуем высоту
        serializeInt32(versionBuffer, start_height);
        

        std::vector<uint8_t> relayBuffer;
        // Сериализация bool
        serializeBool(versionBuffer, relay);
        

        // Формирование заголовка сообщения
        std::string magic_number = "f9beb4d9";
        std::string command = "version";

        const size_t command_size = 12;
        const size_t header_size = magic_number.size() + command_size;

        uint32_t payload_length = static_cast<uint32_t>(versionBuffer.size());// Вычисление контрольной суммы
        std::uint32_t length = serializePayloadLength(payload_length);

        uint32_t checksum = calculateChecksum(versionBuffer);

        std::vector<uint8_t> commandBytes(command_size, 0);
        std::copy(command.begin(), command.end(), commandBytes.begin());
        commandBytes.resize(command_size, 0);

        std::vector<uint8_t> message(header_size + payload_length);
        std::copy(magic_number.begin(), magic_number.end(), message.begin());
        std::copy(commandBytes.begin(), commandBytes.end(), message.begin() + magic_number.size());
        std::memcpy(message.data() + magic_number.size() + command_size, &length, sizeof(length));
        std::memcpy(message.data() + magic_number.size() + command_size + sizeof(length), &checksum, sizeof(checksum)); // Добавляем контрольную сумму
        std::copy(versionBuffer.begin(), versionBuffer.end(), message.begin() + header_size);

        return message;
    }
};

std::string getHostLink() {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    // Инициализация libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        // Установка URL
        curl_easy_setopt(curl, CURLOPT_URL, "https://bitnodes.io/api/v1/snapshots/");

        // Установка функции обратного вызова для обработки данных
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

        // Установка буфера для записи данных
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Выполнение запроса
        res = curl_easy_perform(curl);

        // Проверка на ошибки
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            return "-";
        }
        else {
            // Парсинг JSON
            json parsedJson = json::parse(readBuffer);
            std::string answer = parsedJson["results"][0]["url"];
            // Очистка
            curl_easy_cleanup(curl);

            curl_global_cleanup();
            return answer;
        }

        // Очистка
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

std::string getDataNode() {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;


    std::string url = getHostLink();

    
    // Инициализация libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        // Установка URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Установка функции обратного вызова для обработки данных
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

        // Установка буфера для записи данных
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Выполнение запроса
        res = curl_easy_perform(curl);

        // Проверка на ошибки
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        else {
            // Парсинг JSON
            json parsedJson = json::parse(readBuffer);
            // Получение объекта nodes
            json nodes = parsedJson["nodes"];

            auto firstNode = nodes.begin(); // Итератор на первый элемент
            json firstKey = firstNode.key(); // Значение первого элемента

            // Очистка
            curl_easy_cleanup(curl);

            curl_global_cleanup();

            return firstKey;

        }

        // Очистка
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return "0";

}


int main() {    

    struct Node_Address {
        std::string host;
        std::string port;
    };
    
    //std::string mini_str = getDataNode();
    std::string host = "150.136.4.102"; //mini_str.substr(0, mini_str.length() - 5);
    std::string port = "8333"; //mini_str.substr(mini_str.length() - 4);

    Node_Address node_data = {host, port};

    BitcoinNode node(node_data.host, node_data.port);

    node.connect();
    node.sendVersion();
    node.waitForVerack();
    
    
    return 0;
} 






