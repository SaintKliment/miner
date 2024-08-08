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

#include "serialize.h"

 using json = nlohmann::json;

 // Функция обратного вызова для записи данных
 static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
     userp->append((char*)contents, size * nmemb);
     return size * nmemb;
 }


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


void show_content(std::vector<uint8_t>& content, std::string type) {
    // Выводим содержимое буфера
    std::cout << "Serialized " + type + " message: ";
    for (auto byte : content) {
        std::cout << std::hex << static_cast<int>(byte) << " ";
    } std::cout << std::endl;
    std::cout << std::dec << std::endl; // Возвращаемся к десятичной системе
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
            std::string command(buffer.begin() + 4, buffer.begin() + 16);
            if (command == "verack") {
                std::cout << "Received verack message." << std::endl;
            }
            else if (command == "reject") {
                // Обработка сообщения reject
                if (len >= 24) {
                    uint8_t code = buffer[18]; // Код отклонения
                    uint8_t message_length = buffer[19]; // Длина сообщения отклонения

                    std::string reject_message(buffer.begin() + 20, buffer.begin() + 20 + message_length);
                    std::cout << "Received reject message. Code: " << static_cast<int>(code)
                        << ", Message: " << reject_message << std::endl;
                }
                else {
                    std::cout << "Received incomplete reject message." << std::endl;
                }
            }
        }
        else {
            std::cout << "Received unknown message." << std::endl;
            std::cout << "Received message: ";
            for (size_t i = 0; i < len; ++i) {
                std::cout << std::hex << static_cast<int>(buffer[i]) << " ";
            }
            std::cout << std::dec << std::endl; // Сброс формата вывода на десятичный
        }
    }


private:
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::endpoint endpoint;
    boost::asio::ip::tcp::socket socket;


    std::vector<uint8_t> createVersionMessage() {
        struct NetAddr {
            std::string ip; // IPv6 адрес (можно использовать IPv4 в виде IPv6)
            uint16_t port;               // Порт

            NetAddr() : ip{}, port(0) {}
        };

        int32_t version;             // Версия протокола 4 байта
        uint64_t services;           // Услуги 8 байт
        int64_t timestamp;           // Время в формате UNIX 8 байт
        NetAddr addr_recv;           // Адрес получателя 26 байт
        NetAddr addr_from;           // Адрес отправителя 26 байт
        uint64_t nonce;              // Случайное число 8 байт
        std::string user_agent;      // Агент пользователя x байт
        int32_t start_height;        // Высота блока 4 байт
        bool relay;                  // Указывает, должен ли удаленный узел анонсировать транзакции 1 байт

        std::string host = endpoint.address().to_string();
        unsigned short u_port = endpoint.port();
        std::string port = std::to_string(u_port);

        version = get_version_protocol(host, port);
        services = 0xFFFFFFFFFFFFFFFF;
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


        // Сериализация services в 8 байт
        serializeuInt64(versionBuffer, services);


        // Сериализация timestamp в 8 байт
        serializeInt64(versionBuffer, timestamp);
        show_content(versionBuffer, "timestamp");


        // Сериализация IP-адреса
        // serializeIPAddress(versionBuffer, addr_recv.ip);
        // Сериализация порта
        // serializePort(versionBuffer, addr_recv.port);
        // show_content(versionBuffer, "ip and port recv");

        std::vector<uint8_t> net_addr_recv;
        serializeAddress(net_addr_recv, addr_recv.ip, addr_recv.port);

        versionBuffer.insert(versionBuffer.end(), net_addr_recv.begin(), net_addr_recv.end());

        show_content(net_addr_recv, "ip and port recv");
        show_content(versionBuffer, "ip and port recv");
        show_content(net_addr_recv, "ip and port recv");

        // Сериализация IP-адреса
        // serializeIPAddress(versionBuffer, addr_from.ip);
        // Сериализация порта
        // serializePort(versionBuffer, addr_from.port);

        std::vector<uint8_t> net_addr_from;
        serializeAddress(net_addr_from, addr_from.ip, addr_from.port);
        
        versionBuffer.insert(versionBuffer.end(), net_addr_from.begin(), net_addr_from.end());

        show_content(net_addr_from, "ip and port from");
        show_content(versionBuffer, "ip and port from");
        show_content(net_addr_from, "ip and port from");


        // Сериализация nonce в 8 байт
        serializeuInt64(versionBuffer, nonce);
        show_content(versionBuffer, "nonce");


        // Сериализация user_agent
        serializeUserAgent(versionBuffer, user_agent);
        show_content(versionBuffer, "user agent");
        

        // Сериализуем высоту
        serializeInt32(versionBuffer, start_height);
        show_content(versionBuffer, "height");
        

        // Сериализация bool
        serializeBool(versionBuffer, relay);
        
        std::vector<uint8_t> versionMessage;

        // Формирование заголовка сообщения
        serializeMagicNumber(versionMessage);

        serializeCommand(versionMessage, "version");

        // Сериализация длины полезной нагрузки
        uint32_t payloadLength = versionBuffer.size(); 
        serializePayloadLength(versionMessage, payloadLength);


        // uint32_t checksum = calculateChecksum(versionBuffer);

        // Сериализация контрольной суммы
        // serializeChecksum(versionMessage, checksum);

        // Добавляем полезную нагрузку из versionBuffer
        versionMessage.insert(versionMessage.end(), versionBuffer.begin(), versionBuffer.end());

        return versionMessage;
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
    std::string host = "172.65.15.46"; //mini_str.substr(0, mini_str.length() - 5);
    std::string port = "8333"; //mini_str.substr(mini_str.length() - 4);

    Node_Address node_data = {host, port};

    BitcoinNode node(node_data.host, node_data.port);

    node.connect();
    node.sendVersion();
    node.waitForVerack();
    
    
    return 0;
} 






