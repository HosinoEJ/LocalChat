#include <iostream>
#include <fstream>

int main() {
    std::ofstream file("data.json");  // 創建一個 JSON 文件
    if (file.is_open()) {
        // 這裡可以寫入空的 JSON 對象 "{}"，或者保持空檔案
        file << "{}";  
        file.close();
        std::cout << "JSON 文件已創建！" << std::endl;
    } else {
        std::cout << "無法創建檔案！" << std::endl;
    }

    return 0;
}
