#include <iostream>
#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <sstream>
#include <cstdint>
#include <winsock2.h>
#include <ws2tcpip.h>

void serializeInt32(std::vector<uint8_t>& buffer, int32_t value) {
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));         // ������� ����
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));  // ������ ����
    buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF)); // ������ ����
    buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF)); // ������� ����
}


void serializeuInt64(std::vector<uint8_t>& buffer, uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        buffer.push_back(static_cast<uint8_t>(value & 0xFF)); // ������� ����
        value >>= 8; // �������� �� 8 ���
    }
}


void serializeInt64(std::vector<uint8_t>& buffer, int64_t value) {
    for (int i = 0; i < 8; ++i) {
        buffer.push_back(static_cast<uint8_t>(value & 0xFF)); // ������� ����
        value >>= 8; // �������� �� 8 ���
    }
}


void serializeAddress(std::vector<uint8_t>& buffer, const std::string& ip, const uint16_t port) {
  
    // ��������, ��� ����� ���������� ����� ��� �������� 26 ����
    buffer.resize(26);

    // �������� ������ 10 ���� ������
    std::memset(buffer.data(), 0, 10);

    // ����������� IP-����� � �������� ������
    struct in_addr addr;
    if (inet_pton(AF_INET, ip.c_str(), &addr) != 1) {
        throw std::invalid_argument("Invalid IP address format");
    }

    // ��������� IP-����� � ����� (����� 11-14)
    std::memcpy(buffer.data() + 10, &addr, sizeof(addr)); // 4 ����� ��� IP

    // ����������� ���� � ������� ������� ������ � �������� � ����� (����� 15-16)
    uint16_t net_port = htons(port); // ����������� ���� � ������� ������� ������
    std::memcpy(buffer.data() + 14, &net_port, sizeof(net_port)); // 2 ����� ��� �����

    // �������� ��������� 10 ���� ������ (����� 17-26)
    std::memset(buffer.data() + 16, 0, 10);


}


// ������� ��� ������������ ������ user_agent
void serializeUserAgent(std::vector<uint8_t>& buffer, const std::string& userAgent) {
    // �������� �����
    if (userAgent.size() > 256) {
        throw std::length_error("User agent length exceeds maximum allowed size (256 characters)");
    }
    // ���������� ����� userAgent � ������ ����
    uint8_t length = static_cast<uint8_t>(userAgent.size());
    buffer.push_back(length); // ��������� ����� � ����� ������

    // ���������� ��� userAgent � ���������� �����
    for (char c : userAgent) {
        buffer.push_back(static_cast<uint8_t>(c)); // ��������� ������ ������
    }
}

// ������� ��� ������������ bool
void serializeBool(std::vector<uint8_t>& buffer, bool value) {
    buffer.push_back(value ? 0x01 : 0x00);
}


void serializeMagicNumber(std::vector<uint8_t>& buffer) {
    uint32_t magicNumber = 0xf9beb4d9; // ���������� �����
    buffer.push_back(magicNumber & 0xFF);         // ������� ����
    buffer.push_back((magicNumber >> 8) & 0xFF);  // ������ ����
    buffer.push_back((magicNumber >> 16) & 0xFF); // ������ ����
    buffer.push_back((magicNumber >> 24) & 0xFF); // ������� ����
}


void serializeCommand(std::vector<uint8_t>& buffer, const std::string& command) {
    std::string paddedCommand = command;
    paddedCommand.resize(12, '\0'); // ���������� ������ �� 12 ����
    buffer.insert(buffer.end(), paddedCommand.begin(), paddedCommand.end());
}


// ������� ��� ������������ ����� �������� ��������
void  serializePayloadLength(std::vector<uint8_t>& versionMessage, uint32_t& payloadLength) {
    versionMessage.insert(versionMessage.begin() + 16, payloadLength & 0xFF);         // ������� ����
    versionMessage.insert(versionMessage.begin() + 16 + 1, (payloadLength >> 8) & 0xFF);  // ������ ����
    versionMessage.insert(versionMessage.begin() + 16 + 2, (payloadLength >> 16) & 0xFF); // ������ ����
    versionMessage.insert(versionMessage.begin() + 16 + 3, (payloadLength >> 24) & 0xFF); // ������� ����
}

void serializeChecksum(std::vector<uint8_t>& versionMessage, uint32_t checksum) {
    versionMessage.push_back(checksum & 0xFF);         // ������� ����
    versionMessage.push_back((checksum >> 8) & 0xFF);  // ������ ����
    versionMessage.push_back((checksum >> 16) & 0xFF); // ������ ����
    versionMessage.push_back((checksum >> 24) & 0xFF); // ������� ����
}