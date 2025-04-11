#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")

// 로그 파일에 메시지 기록하는 함수
void logToFile(const std::string& filename, const std::string& content) {
    std::ofstream logFile(filename, std::ios::app);
    if (logFile.is_open()) {
        time_t now = time(0);
        char dt[26];
        ctime_s(dt, sizeof(dt), &now); // 현재 시간 문자열 생성
        dt[strlen(dt) - 1] = '\0';     // 개행 문자 제거
        logFile << "[" << dt << "] " << content << std::endl;
    }
}

// 서버에 메시지를 보내고 응답을 받는 함수
void sendAndReceive(SOCKET sock, const std::string& msg) {
    send(sock, msg.c_str(), static_cast<int>(msg.length()), 0); // 메시지 전송
    logToFile("log_client.txt", "SEND: " + msg); // 로그 기록
    std::vector<char> buffer(1024);
    int len = recv(sock, buffer.data(), buffer.size(), 0); // 응답 수신
    if (len > 0) {
        std::string response(buffer.data(), len);
        std::cout << "[서버 응답] " << response << std::endl;
        logToFile("log_client.txt", "RECV: " + response);
        if (msg.rfind("CHAT:", 0) == 0) {
            logToFile("chat_history.txt", msg.substr(5)); // 채팅 메시지는 별도 기록
        }
    }
}

int main() {
    // Winsock 초기화
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    // 서버 주소 설정
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    // 서버 연결 시도
    connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));

    while (true) {
        std::cout << "\n[메뉴] 1. 회원가입 2. 로그인 3. 종료\n> ";
        int menu; std::cin >> menu; std::cin.ignore();

        if (menu == 1) { // 회원가입
            std::string id, name, pw;
            std::cout << "ID (숫자): "; std::getline(std::cin, id);
            std::cout << "이름: "; std::getline(std::cin, name);
            std::cout << "비밀번호: "; std::getline(std::cin, pw);
            sendAndReceive(sock, "SIGNUP:" + id + ":" + name + ":" + pw);
        }
        else if (menu == 2) { // 로그인
            std::string id, pw;
            std::cout << "아이디: "; std::getline(std::cin, id);
            std::cout << "비밀번호: "; std::getline(std::cin, pw);
            std::string msg = "LOGIN:" + id + ":" + pw + ":";
            send(sock, msg.c_str(), static_cast<int>(msg.length()), 0);

            std::vector<char> buffer(1024);
            int len = recv(sock, buffer.data(), buffer.size(), 0);
            std::string res(buffer.data(), len);
            std::cout << "[서버 응답] " << res << std::endl;
            logToFile("log_client.txt", "RECV: " + res);

            if (res == "Login Success") {
                while (true) {
                    std::cout << "\n[로그인 메뉴] 1. 채팅 2. 로그아웃 3. 종료\n> ";
                    int sub; std::cin >> sub; std::cin.ignore();
                    if (sub == 1) { // 채팅
                        std::string chat;
                        std::cout << "메시지: "; std::getline(std::cin, chat);
                        sendAndReceive(sock, "CHAT:" + chat);
                    }
                    else if (sub == 2) { // 로그아웃
                        sendAndReceive(sock, "LOGOUT:");
                        break;
                    }
                    else if (sub == 3) { // 종료
                        sendAndReceive(sock, "EXIT");
                        closesocket(sock); WSACleanup();
                        return 0;
                    }
                }
            }
        }
        else if (menu == 3) { // 전체 종료
            sendAndReceive(sock, "EXIT");
            break;
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
