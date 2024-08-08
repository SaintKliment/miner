#pragma once

// Функция для сериализации int32_t в буфер
void serializeInt32(std::vector<uint8_t>& buffer, int32_t value);

// Функция для сериализации uint64_t в буфер
void serializeuInt64(std::vector<uint8_t>& buffer, uint64_t value);

// Функция для сериализации int64_t в буфер
void serializeInt64(std::vector<uint8_t>& buffer, int64_t value);

// Функция для сериализации IP-адреса (IPv4)
void serializeIPAddress(std::vector<uint8_t>& buffer, const std::string& ip);

// Функция для сериализации порта
void serializePort(std::vector<uint8_t>& buffer, uint16_t port);

void serializeAddress(std::vector<uint8_t>& buffer, const std::string& ip, const uint16_t port);

// Функция для сериализации строки user_agent
void serializeUserAgent(std::vector<uint8_t>& buffer, const std::string& userAgent);

// Функция для сериализации bool
void serializeBool(std::vector<uint8_t>& buffer, bool value);

void serializeMagicNumber(std::vector<uint8_t>& buffer);

void serializeCommand(std::vector<uint8_t>& buffer, const std::string& command);

void  serializePayloadLength(std::vector<uint8_t>& versionMessage, uint32_t& payloadLength);

void serializeChecksum(std::vector<uint8_t>& versionMessage, uint32_t checksum);