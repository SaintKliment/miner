#include <iostream>
#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <sstream>
#include <cstdint>
#include <winsock2.h>
#include <ws2tcpip.h>

void serializeInt32(std::vector<uint8_t>& buffer, int32_t value) {
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));         // младший байт
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));  // второй байт
    buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF)); // третий байт
    buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF)); // старший байт
}


void serializeuInt64(std::vector<uint8_t>& buffer, uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        buffer.push_back(static_cast<uint8_t>(value & 0xFF)); // Младший байт
        value >>= 8; // Сдвигаем на 8 бит
    }
}


void serializeInt64(std::vector<uint8_t>& buffer, int64_t value) {
    for (int i = 0; i < 8; ++i) {
        buffer.push_back(static_cast<uint8_t>(value & 0xFF)); // Младший байт
        value >>= 8; // Сдвигаем на 8 бит
    }
}


void serializeAddress(std::vector<uint8_t>& buffer, const std::string& ip, const uint16_t port) {
  
    // Убедимся, что буфер достаточно велик для хранения 26 байт
    buffer.resize(26);

    // Заполним первые 10 байт нулями
    std::memset(buffer.data(), 0, 10);

    // Преобразуем IP-адрес в двоичный формат
    struct in_addr addr;
    if (inet_pton(AF_INET, ip.c_str(), &addr) != 1) {
        throw std::invalid_argument("Invalid IP address format");
    }

    // Скопируем IP-адрес в буфер (байты 11-14)
    std::memcpy(buffer.data() + 10, &addr, sizeof(addr)); // 4 байта для IP

    // Преобразуем порт в сетевой порядок байтов и копируем в буфер (байты 15-16)
    uint16_t net_port = htons(port); // Преобразуем порт в сетевой порядок байтов
    std::memcpy(buffer.data() + 14, &net_port, sizeof(net_port)); // 2 байта для порта

    // Заполним последние 10 байт нулями (байты 17-26)
    std::memset(buffer.data() + 16, 0, 10);


}


// Функция для сериализации строки user_agent
void serializeUserAgent(std::vector<uint8_t>& buffer, const std::string& userAgent) {
    // Проверка длины
    if (userAgent.size() > 256) {
        throw std::length_error("User agent length exceeds maximum allowed size (256 characters)");
    }
    // Записываем длину userAgent в первый байт
    uint8_t length = static_cast<uint8_t>(userAgent.size());
    buffer.push_back(length); // Добавляем длину в конец буфера

    // Записываем сам userAgent в оставшиеся байты
    for (char c : userAgent) {
        buffer.push_back(static_cast<uint8_t>(c)); // Добавляем каждый символ
    }
}

// Функция для сериализации bool
void serializeBool(std::vector<uint8_t>& buffer, bool value) {
    buffer.push_back(value ? 0x01 : 0x00);
}


void serializeMagicNumber(std::vector<uint8_t>& buffer) {
    uint32_t magicNumber = 0xf9beb4d9; // Магическое число
    buffer.push_back(magicNumber & 0xFF);         // Младший байт
    buffer.push_back((magicNumber >> 8) & 0xFF);  // Второй байт
    buffer.push_back((magicNumber >> 16) & 0xFF); // Третий байт
    buffer.push_back((magicNumber >> 24) & 0xFF); // Старший байт
}


void serializeCommand(std::vector<uint8_t>& buffer, const std::string& command) {
    std::string paddedCommand = command;
    paddedCommand.resize(12, '\0'); // Заполнение нулями до 12 байт
    buffer.insert(buffer.end(), paddedCommand.begin(), paddedCommand.end());
}


// Функция для сериализации длины полезной нагрузки
void  serializePayloadLength(std::vector<uint8_t>& versionMessage, uint32_t& payloadLength) {
    versionMessage.insert(versionMessage.begin() + 16, payloadLength & 0xFF);         // Младший байт
    versionMessage.insert(versionMessage.begin() + 16 + 1, (payloadLength >> 8) & 0xFF);  // Второй байт
    versionMessage.insert(versionMessage.begin() + 16 + 2, (payloadLength >> 16) & 0xFF); // Третий байт
    versionMessage.insert(versionMessage.begin() + 16 + 3, (payloadLength >> 24) & 0xFF); // Старший байт
}

void serializeChecksum(std::vector<uint8_t>& versionMessage, uint32_t checksum) {
    versionMessage.push_back(checksum & 0xFF);         // Младший байт
    versionMessage.push_back((checksum >> 8) & 0xFF);  // Второй байт
    versionMessage.push_back((checksum >> 16) & 0xFF); // Третий байт
    versionMessage.push_back((checksum >> 24) & 0xFF); // Старший байт
}