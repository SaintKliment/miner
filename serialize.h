#pragma once

// ������� ��� ������������ int32_t � �����
void serializeInt32(std::vector<uint8_t>& buffer, int32_t value);

// ������� ��� ������������ uint64_t � �����
void serializeuInt64(std::vector<uint8_t>& buffer, uint64_t value);

// ������� ��� ������������ int64_t � �����
void serializeInt64(std::vector<uint8_t>& buffer, int64_t value);

// ������� ��� ������������ IP-������ (IPv4)
void serializeIPAddress(std::vector<uint8_t>& buffer, const std::string& ip);

// ������� ��� ������������ �����
void serializePort(std::vector<uint8_t>& buffer, uint16_t port);

void serializeAddress(std::vector<uint8_t>& buffer, const std::string& ip, const uint16_t port);

// ������� ��� ������������ ������ user_agent
void serializeUserAgent(std::vector<uint8_t>& buffer, const std::string& userAgent);

// ������� ��� ������������ bool
void serializeBool(std::vector<uint8_t>& buffer, bool value);

void serializeMagicNumber(std::vector<uint8_t>& buffer);

void serializeCommand(std::vector<uint8_t>& buffer, const std::string& command);

void  serializePayloadLength(std::vector<uint8_t>& versionMessage, uint32_t& payloadLength);

void serializeChecksum(std::vector<uint8_t>& versionMessage, uint32_t checksum);