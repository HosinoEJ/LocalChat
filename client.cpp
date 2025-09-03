#include <functional>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <thread>
#include <atomic>
#include <fcntl.h>
#include <fstream>
#include "json.hpp"
#include <locale>
//#include "sb.h"//WDF你tm來搞笑？
//#include <windows.h>

using namespace std;
using json = nlohmann::json;

#define PORT 8080
#define BUFFER_SIZE 1024

#define DEV_MOTD true

atomic<bool> keep_running(true);

/*
enum class SetServerResult {//定義枚舉結果
    SUCCESS
};*/



// 接收訊息的執行緒函式
void receive_thread(int sockfd,string& REMARK_ONLINE, json& config) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in fromaddr;
    socklen_t addrlen = sizeof(fromaddr);

    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    while (keep_running) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&fromaddr, &addrlen);

        if (bytes_received > 0) {
            string sender_ip = inet_ntoa(fromaddr.sin_addr);
            string received_message(buffer, bytes_received);

            string display_name;
            if (!config["REMARK"][sender_ip].is_null()) {
                display_name = config["REMARK"][sender_ip]; // JSON 裡的固定備註
            } else if (!REMARK_ONLINE.empty()) {
                display_name = REMARK_ONLINE; // 使用最新用戶備註
            } else {
                display_name = sender_ip; // 默認 IP
            }

            cout << "\n[\033[32m來自 " <<"\033[33m"<< display_name <<"\033[32m"<< " 的訊息\033[0m]: " << received_message << endl;
            cout << "> ";
            fflush(stdout);
        }

        this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// 函式：讓使用者輸入並設定伺服器 IP
string set_server_address(sockaddr_in& servaddr, string& ip, json& config) {
    string server_ip;
    cout << "請輸入對象IP或備註: ";
    getline(cin, server_ip);

    // 檢查是否輸入的是備註名稱
    for (auto& [real_ip, remark] : config["REMARK"].items()) {
        if (server_ip == remark) {
            server_ip = real_ip; // 把備註轉換成真實 IP
            break;
        }
    }

    // bro給自己發訊息
    if (server_ip.empty() || server_ip == "localhost" || server_ip == "127.0.0.1") {
        server_ip = "127.0.0.1";
        cout << "Bro,你tm想給你自己發訊息，好吧我原諒你…" << endl;
    }

    // 驗證 IP 格式
    if (inet_pton(AF_INET, server_ip.c_str(), &servaddr.sin_addr) <= 0) {
        perror("無效的 IP 地址");
        return string("false") + ip;
    }

    ip = server_ip;
    return string("true") + ip;
}


string RunningON(){
    string RunningON;

    #if defined(_WIN32) || defined(_WIN64)
        RunningON = "Windows";
    #elif defined(__ANDROID__)
        RunningON = "Android";
    #elif defined(__linux__)
        RunningON = "Linux";
    #elif defined(__APPLE__)
        RunningON = "macOS";
    #else
        RunningON = "ERROR";
    #endif

    return RunningON;
}

string LOC_NAME(){
    string lang = "unknow";
    if (RunningON() == "Linux") {
        locale loc("");
        string loc_name = loc.name(); // 例如 zh_TW.UTF-8
        size_t dot = loc_name.find('.');
        if (dot != string::npos)
            loc_name = loc_name.substr(0, dot); // 去掉 .UTF-8
        lang = loc_name;
    }

    
    ifstream langfile(lang);
    if(!langfile.is_open())
        lang = "en_US";
    return lang;
}
string CONFIG_LINK(){
    string path,lang;
    if (DEV_MOTD == true) {
        path = "config.json";
        lang = "lang/en.json";
        //cout<<getenv("HOME")<<"FUCK Linux";
        //cout<<string(getenv("HOME")) + "/.config/HosinoEJChat/config.json";
    } else {
        path = string(getenv("HOME")) + "/.config/HosinoEJChat/config.json";
        lang = string(getenv("HOME")) + "/.config/HosinoEJChat/lang/en.json";
    }
    ifstream file(path);
    if (!file.is_open())
        cerr << "ERROR:CONFIG FILE IS NOTHING IN: " << path << "!\n";//FUCK WINDOWS AND LINUX AND FUCK U MOTHER BEACH APPLE!!!!!!!!!!
    
    ifstream langfile(lang);
    if(!langfile.is_open())
        cerr << "ERROR:LANGUAGE JSON FILE IS NOTHING IN: " << path << "!\n";

    // 檢查檔案是否為空
    if (file.peek() == ifstream::traits_type::eof()) {
        cerr << "ERROR: CONFIG FILE IS BAD"<< "\n";
    }

    return path;
}
void Detection(){
    cout<<"Self-testing..."<<endl;
    CONFIG_LINK();
    if (DEV_MOTD == true) {
        cout<<"\033[31mPlease note! Debug mode is enabled! This is what developers use when debugging programs! It can cause all sorts of security issues! I don't know how the hell you got this version, so if you want to disable it, please download it again.\033[0m\n";//軟體都能下錯，真是雜魚~
        cout<<"\033[33mYou are running this program in a "<<RunningON()<<" environment"<<endl;
        cout<<"Configuration archive directory:"<<CONFIG_LINK()<<"\033[0m"<<endl;
    }
}

void REMARK(json& config) {
    int in;
    cin>>in;
}

// 主執行緒：處理發送訊息和使用者輸入
int main() {
    int sockfd;
    struct sockaddr_in servaddr, myaddr;
    string message;


    
    Detection();//自檢程序
 
    //cout<<CONFIG_LINK();
    ifstream file(CONFIG_LINK());
    ifstream lang_config(CONFIG_LINK()+"/lang/" + LOC_NAME() + ".json");

    json config,lang;
    file >> config;
    lang_config >> lang;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(PORT);
    myaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (const struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    // 設定伺服器地址結構
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);

    string REMARK_ONLINE;
    // 啟動接收訊息的獨立執行緒
    thread receiver(receive_thread, sockfd, ref(REMARK_ONLINE), ref(config));


    // 主迴圈
    while (true) {
        while (true) {
            string options;
            cin>>options;
            if(options == "chat")
                break;
            if(options == "STOP")
                goto end_program;
            if(options == "setting")
                cout<<"稍等，我還沒做這個功能";
            if(options == "0d000000007218964sbXJPFUCKU!")
                system("start https://google.com");
        }
        // 在每次循環開始時設定 IP
        //SetServerResult result = set_server_address(servaddr);
        string ip;
        string result = set_server_address(servaddr,ip,config);

        if(result== "false"){
            continue;
        }
        /*
        if (result == SetServerResult::INVALID_IP) {
            continue; // 只跳過當前迴圈，重新要求輸入
        }*/


        cout << "已設定伺服器 IP。現在可以發送訊息。輸入 'CHANGE_IP' 再次更改 IP，輸入REMARK將這個ip添加備注， 輸入'QUIT' 退出，輸入'STOP'關閉應用程式。" << endl;
        
        while (true) {
            cout << "> ";
            getline(cin, message);

            if (message == "QUIT") {
                goto end_program; // 使用 goto 來安全地跳出多層迴圈
            } else if (message == "CHANGE_IP") {
                cout << "正在更改伺服器 IP..." << endl;
                break; // 跳出內層迴圈，回到外層迴圈重新設定 IP
            } else if (message == "REMARK") {
                cout<<"請輸入將現在聊天的ip（"<<ip<<"）添加的備注：";
                cin>>REMARK_ONLINE;
                config["REMARK"][ip] = REMARK_ONLINE;

            }

            
            
            sendto(sockfd, message.c_str(), message.length(), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
        }
    }

end_program:
    cout << "正在退出..." << endl;
    keep_running = false;
    receiver.join();

    ofstream out(CONFIG_LINK());
    out << setw(4) << config << endl;
    out.close();

    close(sockfd);
    return 0;
}