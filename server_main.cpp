// ✅ 서버 코드 (server.cpp)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <vector>
#include <mysql/jdbc.h>
#include <fstream>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")

#define DB_HOST "tcp://127.0.0.1:3306"
#define DB_USER "root"
#define DB_PASS "0000"
#define DB_NAME "kdt_mini_project"

// 서버 로그 파일에 기록하는 함수
void logServerEvent(const std::string& filename, const std::string& content) {
    std::ofstream logFile(filename, std::ios::app);
    if (logFile.is_open()) {
        time_t now = time(0);
        char dt[26];
        ctime_s(dt, sizeof(dt), &now);
        dt[strlen(dt) - 1] = '\0';
        logFile << "[" << dt << "] " << content << std::endl;
    }
}

// 클라이언트 요청 처리 함수
void handleClient(SOCKET clientSocket) {
    std::vector<char> buffer(1024);
    bool loggedIn = false;
    int userId = -1;

    while (true) {
        int recvLen = recv(clientSocket, buffer.data(), buffer.size(), 0);
        if (recvLen <= 0) break;
        std::string msg(buffer.data(), recvLen);
        std::cout << "[클라이언트] " << msg << std::endl;
        logServerEvent("log_server.txt", "RECV: " + msg);

        try {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> conn(driver->connect(DB_HOST, DB_USER, DB_PASS));
            conn->setSchema(DB_NAME);

            if (msg.rfind("SIGNUP:", 0) == 0) {
                size_t p1 = msg.find(":", 7), p2 = msg.find(":", p1 + 1);
                std::string id = msg.substr(7, p1 - 7);
                std::string name = msg.substr(p1 + 1, p2 - p1 - 1);
                std::string pw = msg.substr(p2 + 1);

                auto pstmt = conn->prepareStatement("INSERT INTO users (user_id, username, password, status, created_at) VALUES (?, ?, ?, 'active', NOW())");
                pstmt->setInt(1, std::stoi(id));
                pstmt->setString(2, name);
                pstmt->setString(3, pw);
                pstmt->execute();

                std::string res = "Signup Success";
                send(clientSocket, res.c_str(), res.length(), 0);
                logServerEvent("log_server.txt", "SEND: " + res);
            }
            else if (msg.rfind("LOGIN:", 0) == 0) {
                size_t pos1 = msg.find(":", 6), pos2 = msg.find(":", pos1 + 1);
                std::string id = msg.substr(6, pos1 - 6);
                std::string pw = msg.substr(pos1 + 1, pos2 - pos1 - 1);

                auto pstmt = conn->prepareStatement("SELECT * FROM users WHERE username = ? AND password = ?");
                pstmt->setString(1, id);
                pstmt->setString(2, pw);
                auto res = pstmt->executeQuery();

                if (res->next()) {
                    userId = res->getInt("user_id");
                    std::string reply = "Login Success";
                    send(clientSocket, reply.c_str(), reply.length(), 0);
                    logServerEvent("log_server.txt", "SEND: " + reply);
                    loggedIn = true;

                    auto sessionStmt = conn->prepareStatement("INSERT INTO user_sessions (user_id, login_time, ip_address) VALUES (?, NOW(), ?)");
                    sessionStmt->setInt(1, userId);
                    sessionStmt->setString(2, "127.0.0.1");
                    sessionStmt->execute();
                }
                else {
                    std::string reply = "Login Failed";
                    send(clientSocket, reply.c_str(), reply.length(), 0);
                    logServerEvent("log_server.txt", "SEND: " + reply);
                }
            }
            else if (msg.rfind("CHAT:", 0) == 0 && loggedIn) {
                std::string content = msg.substr(5);
                std::string echo = "ECHO: " + content;
                send(clientSocket, echo.c_str(), echo.length(), 0);
                logServerEvent("chat_server.txt", content);
                logServerEvent("log_server.txt", "SEND: " + echo);

                auto chatStmt = conn->prepareStatement("INSERT INTO message_log (sender_id, content, sent_at) VALUES (?, ?, NOW())");
                chatStmt->setInt(1, userId);
                chatStmt->setString(2, content);
                chatStmt->execute();
            }
            else if (msg.rfind("LOGOUT:", 0) == 0 && loggedIn) {
                loggedIn = false;
                std::string bye = "Logout Success";
                send(clientSocket, bye.c_str(), bye.length(), 0);
                logServerEvent("log_server.txt", "SEND: " + bye);

                auto logoutStmt = conn->prepareStatement("UPDATE user_sessions SET logout_time = NOW() WHERE user_id = ? AND logout_time IS NULL");
                logoutStmt->setInt(1, userId);
                logoutStmt->execute();
            }
            else if (msg == "EXIT") {
                break;
            }
            else {
                std::string unknown = "Unknown Command";
                send(clientSocket, unknown.c_str(), unknown.length(), 0);
                logServerEvent("log_server.txt", "SEND: " + unknown);
            }
        }
        catch (sql::SQLException& e) {
            std::cerr << "[DB ERROR] " << e.what() << std::endl;
            logServerEvent("log_server.txt", std::string("[DB ERROR] ") + e.what());
            std::string err = "DB Error";
            send(clientSocket, err.c_str(), err.length(), 0);
        }
    }

    closesocket(clientSocket);
    std::cout << "클라이언트 연결 종료됨.\n";
}

// 서버 메인 함수
int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);
    std::cout << "서버 시작됨 (12345 포트)\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket != INVALID_SOCKET) {
            std::thread t(handleClient, clientSocket);
            t.detach();
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
