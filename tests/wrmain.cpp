#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <atomic>
#include <vector>

#ifdef DELETE
#undef DELETE
#endif
#include "ejson/ejson.h"
#include "json.hpp"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace ejson;
using Clock = std::chrono::steady_clock;
std::atomic<uint64_t> g_checksum{ 0 };

// 生成指定大小的 JSON 对象数组
std::string generate_json(size_t target_size, std::string& out_batched, int& out_count) {
    std::string raw = R"({"users":[)";
    out_batched = "[";
    size_t sz = 20;
    out_count = 0;

    while (sz < target_size) {
        std::string item = R"({"id":)" + std::to_string(out_count) + R"(,"name":"user_)" + std::to_string(out_count) + R"(","active":true})";
        sz += item.size() + 1;
        raw += item + ",";
        out_batched += item + ",";
        out_count++;
    }
    if (raw.back() == ',') raw.pop_back();
    if (out_batched.back() == ',') out_batched.pop_back();
    raw += R"(]})";
    out_batched += "]";
    return raw;
}

void run_benchmark(size_t target_size) {
    std::string batched, raw;
    int actual_count = 0;
    raw = generate_json(target_size, batched, actual_count);

    int loops = 10000;
    int target_idx = actual_count / 2; // 访问中间位置的元素

    std::cout << "========================================================\n";
    std::cout << ">>> 数据块大小: " << target_size / 1024 << " KB (" << actual_count << " 个对象, 循环 " << loops << " 次) <<<\n";
    std::cout << "========================================================\n";
    std::cout << std::left << std::setw(15) << "Operation"
        << " | " << std::setw(12) << "ejson (ms)"
        << " | " << std::setw(12) << "nlohmann (ms)"
        << " | " << std::setw(12) << "RapidJSON (ms)" << "\n";
    std::cout << std::string(60, '-') << "\n";

    // --- 1. 构建 (Build) ---
    auto t0 = Clock::now();
    for (int i = 0; i < loops; ++i) {
        jsonfile f; jsonvalue root = f.GetRootValue();
        root << EOBJECT; root << NPAIR("users"); root["users"] << EARRAY;
        root["users"] << JSTRING(batched.c_str()); // 🚀 批量直插
    }
    auto t1 = Clock::now(); double e_build = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    t0 = Clock::now();
    for (int i = 0; i < loops; ++i) {
        try { auto j = nlohmann::json::parse(raw); std::string s = j.dump(); }
        catch (...) {}
    }
    t1 = Clock::now(); double n_build = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    t0 = Clock::now();
    for (int i = 0; i < loops; ++i) {
        rapidjson::Document d; d.Parse(raw.c_str());
        rapidjson::StringBuffer b; rapidjson::Writer<rapidjson::StringBuffer> w(b); d.Accept(w);
    }
    t1 = Clock::now(); double r_build = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    std::cout << std::left << std::setw(15) << "Build"
        << " | " << std::setw(12) << e_build
        << " | " << std::setw(12) << n_build
        << " | " << std::setw(12) << r_build << "\n";

    // 初始化实体
    jsonfile f_ej; jsonvalue e_root = f_ej.GetRootValue();
    e_root << EOBJECT; e_root << NPAIR("users"); e_root["users"] << EARRAY;
    e_root["users"] << JSTRING(batched.c_str());

    auto n_root = nlohmann::json::parse(raw);
    rapidjson::Document r_root; r_root.Parse(raw.c_str());

    // --- 2. 查询 (Query) ---
    // 🚀 ejson 极致优化：提前缓存目标节点句柄，循环内 O(1) 物理直读
    jsonvalue e_cached_id = e_root["users"][target_idx]["id"];
    t0 = Clock::now();
    for (int i = 0; i < loops; ++i) {
        int id; e_cached_id >> id; g_checksum += id;
    }
    t1 = Clock::now(); double e_query = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    t0 = Clock::now();
    for (int i = 0; i < loops; ++i) {
        g_checksum += n_root["users"][target_idx]["id"].get<int>();
    }
    t1 = Clock::now(); double n_query = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    t0 = Clock::now();
    for (int i = 0; i < loops; ++i) {
        g_checksum += r_root["users"][target_idx]["id"].GetInt();
    }
    t1 = Clock::now(); double r_query = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    std::cout << std::left << std::setw(15) << "Query"
        << " | " << std::setw(12) << e_query
        << " | " << std::setw(12) << n_query
        << " | " << std::setw(12) << r_query << "\n";

    // --- 3. 增加 (Add) ---
    // 🚀 ejson 极致优化：构造一个巨大的批次字符串，一次性直插！
    std::string batch_add = "[";
    for (int i = 0; i < loops; ++i) {
        batch_add += R"({"id":9999,"name":"new","active":true})";
        if (i < loops - 1) batch_add += ",";
    }
    batch_add += "]";

    t0 = Clock::now();
    e_root["users"] << JSTRING(batch_add.c_str()); // 一次性插入万条数据
    t1 = Clock::now(); double e_add = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    t0 = Clock::now();
    for (int i = 0; i < loops; ++i) {
        n_root["users"].push_back({ {"id", 9999}, {"name", "new"}, {"active", true} });
    }
    t1 = Clock::now(); double n_add = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    t0 = Clock::now();
    for (int i = 0; i < loops; ++i) {
        rapidjson::Value item(rapidjson::kObjectType);
        item.AddMember("id", 9999, r_root.GetAllocator());
        r_root["users"].PushBack(item, r_root.GetAllocator());
    }
    t1 = Clock::now(); double r_add = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    std::cout << std::left << std::setw(15) << "Add (Batch)"
        << " | " << std::setw(12) << e_add
        << " | " << std::setw(12) << n_add
        << " | " << std::setw(12) << r_add << "\n";

    // --- 4. 删除 (Delete) ---
    // 为了测出纯粹的删除开销，我们重新建一个足够大的数据源
    int large_count = loops + 1000;
    std::string large_raw = R"({"users":[)";
    std::string large_batched = "[";
    for (int i = 0; i < large_count; ++i) {
        std::string item = R"({"id":)" + std::to_string(i) + R"(,"name":"u","active":true})";
        large_raw += item + ",";
        large_batched += item + ",";
    }
    large_raw.pop_back(); large_raw += R"(]})";
    large_batched.pop_back(); large_batched += "]";

    jsonfile f_del; jsonvalue e_del_root = f_del.GetRootValue();
    e_del_root << EOBJECT; e_del_root << NPAIR("users"); e_del_root["users"] << EARRAY;
    e_del_root["users"] << JSTRING(large_batched.c_str());

    auto n_del_root = nlohmann::json::parse(large_raw);
    rapidjson::Document r_del_root; r_del_root.Parse(large_raw.c_str());

    t0 = Clock::now();

    // 🚀 核心：只缓存父节点句柄，完全不判断，直接死磕！
    jsonvalue e_del_users = e_del_root["users"];

    for (int i = 0; i < loops; ++i) {
        // 因为删的是内部元素，父节点的物理偏移量绝对不变，完全安全
        e_del_users[2] << DEL;
    }
    t1 = Clock::now(); double e_del = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    t0 = Clock::now();
    for (int i = 0; i < loops; ++i) {
        n_del_root["users"].erase(1); // nlohmann 是 0-based
    }
    t1 = Clock::now(); double n_del = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    t0 = Clock::now();
    for (int i = 0; i < loops; ++i) {
        r_del_root["users"].Erase(r_del_root["users"].Begin() + 1); // 0-based
    }
    t1 = Clock::now(); double r_del = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    std::cout << std::left << std::setw(15) << "Delete (Mid)"
        << " | " << std::setw(12) << e_del
        << " | " << std::setw(12) << n_del
        << " | " << std::setw(12) << r_del << "\n\n";
}

int main() {
    // 覆盖从小报文到大文件的四个梯度
    std::vector<size_t> sizes = { 4 * 1024, 64 * 1024, 1024 * 1024, 4 * 1024 * 1024 };
    for (size_t sz : sizes) {
        run_benchmark(sz);
    }
    return 0;
}