#define NOMINMAX
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <windows.h>
#include <psapi.h>

#ifdef DELETE
#undef DELETE
#endif

// ================= 切换测试目标 =================
// 每次编译请只取消注释一个库！
#define TEST_EJSON 
 //#define TEST_NLOHMANN
 //#define TEST_RAPIDJSON

#ifdef TEST_EJSON
#include "ejson/ejson.h"
#elif defined(TEST_NLOHMANN)
#include "json.hpp"
#elif defined(TEST_RAPIDJSON)
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"
#endif

#pragma comment(lib, "psapi.lib")
using namespace std;

long GetExeSize() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::ifstream file(buffer, std::ios::binary | std::ios::ate);
    return (long)file.tellg();
}

// 逼迫编译器实例化所有核心 API
void test_full_crud() {
    std::string raw = R"({"users":[{"id":1,"name":"A"},{"id":2,"name":"B"}]})";

#ifdef TEST_EJSON
    ejson::jsonfile f;
    ejson::jsonvalue root = f.GetRootValue();

    // 增 (Add)
    root << ejson::EOBJECT;
    root << NPAIR("users");
    root["users"] << ejson::EARRAY;
    root["users"] << ejson::NIPAIR;
    root["users"][1] << JSTRING(R"({"id":1,"name":"A"})");

    // 查 (Query) & 改 (Modify)
    int id; std::string name;
    root["users"][1]["id"] >> id;
    root["users"][1]["name"] << "Updated";
    root["users"][1]["name"] >> name;

    // 删 (Delete)
    root["users"][1] << ejson::DEL;

#elif defined(TEST_NLOHMANN)
    try {
        // 查 (Query) + 隐式转换
        auto j = nlohmann::json::parse(raw);
        int id = j["users"][0]["id"].get<int>();
        std::string name = j["users"][0]["name"].get<std::string>();

        // 改 (Modify)
        j["users"][0]["name"] = "Updated";

        // 增 (Add)
        j["users"].push_back({ {"id", 3}, {"name", "C"} });

        // 删 (Delete)
        j["users"].erase(1);

        // 序列化 (Serialize)
        std::string out1 = j.dump();
        std::string out2 = j.dump(4); // 格式化输出

        // 异常处理 (Exception)
        auto j2 = nlohmann::json::parse("{invalid}");
    }
    catch (const nlohmann::json::exception& e) {
        std::cerr << e.what();
    }

#elif defined(TEST_RAPIDJSON)
    rapidjson::Document d;
    d.Parse(raw.c_str());

    if (!d.HasParseError()) {
        // 查 (Query) & 改 (Modify)
        int id = d["users"][0]["id"].GetInt();
        d["users"][0]["name"].SetString("Updated", d.GetAllocator());

        // 增 (Add)
        rapidjson::Value newUser(rapidjson::kObjectType);
        newUser.AddMember("id", 3, d.GetAllocator());
        newUser.AddMember("name", "C", d.GetAllocator());
        d["users"].PushBack(newUser, d.GetAllocator());

        // 删 (Delete)
        auto& users = d["users"];
        for (rapidjson::SizeType i = 0; i < users.Size(); ++i) {
            if (users[i]["id"].GetInt() == 2) {
                users.Erase(users.Begin() + i);
                break;
            }
        }

        // 序列化 (Serialize)
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        d.Accept(writer);
        std::string out(buffer.GetString(), buffer.GetSize());
    }
    else {
        // 异常处理 (Error)
        std::string err = rapidjson::GetParseError_En(d.GetParseError());
    }
#endif
}

int main() {
    test_full_crud();

    std::cout << "\n===========================================\n";
    std::cout << "全功能 (CRUD + 序列化 + 异常) 编译体积测试\n";
    std::cout << "===========================================\n";
    std::cout << "编译生成的 .exe 磁盘体积: " << (GetExeSize() / 1024.0) << " KB\n";
    return 0;
}