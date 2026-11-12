#define NOMINMAX
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <windows.h>
#include <psapi.h>

#ifdef DELETE
#undef DELETE
#endif

#include "ejson/ejson.h"
#include "json.hpp"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#pragma comment(lib, "psapi.lib")

using namespace ejson;

// 获取当前进程的真实物理内存占用 (MB)
double GetPhysicalMemoryMB() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / 1024.0 / 1024.0;
    }
    return 0.0;
}

// 🚀 核心武器：强制清空进程工作集，把物理内存压到最低，防止页面缓存干扰
void FlushMemory() {
    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
    Sleep(100); // 给操作系统一点时间处理
}

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

void test_memory(size_t target_size, double raw_mb) {
    std::string batched, raw;
    int count = 0;
    raw = generate_json(target_size, batched, count);

    std::cout << "========================================================\n";
    std::cout << ">>> 数据块大小: " << target_size / 1024 << " KB (" << count << " 个对象, 原始大小 " << raw_mb << " MB) <<<\n";
    std::cout << "========================================================\n";
    std::cout << std::left << std::setw(15) << "Library"
        << " | " << std::setw(15) << "Extra Memory (MB)"
        << " | " << std::setw(15) << "Overhead (%)" << "\n";
    std::cout << std::string(55, '-') << "\n";

    // 封装 lambda，精确测量每个库的真实物理开销
    auto measure = [&](const char* name, auto&& func) {
        FlushMemory(); // 强制让操作系统回收闲置内存
        double start = GetPhysicalMemoryMB();
        func();        // 执行解析/构建
        double end = GetPhysicalMemoryMB();

        double extra = end - start;
        if (extra < 0) extra = 0; // 防止微小抖动产生负数

        std::cout << std::left << std::setw(15) << name
            << " | " << std::setw(15) << std::fixed << std::setprecision(2) << extra
            << " | " << std::setw(15) << std::fixed << std::setprecision(1) << (extra / raw_mb * 100) << "%\n";
        };

    // 1. ejson
    measure("ejson", [&]() {
        jsonfile f; jsonvalue root = f.GetRootValue();
        root << EOBJECT; root << NPAIR("users"); root["users"] << EARRAY;
        root["users"] << JSTRING(batched.c_str());
        });

    // 2. nlohmann
    measure("nlohmann", [&]() {
        auto j = nlohmann::json::parse(raw);
        });

    // 3. RapidJSON
    measure("RapidJSON", [&]() {
        rapidjson::Document d;
        d.Parse(raw.c_str());
        });

    std::cout << "\n";
}

int main() {
    std::cout << ">>> 真实无水分 内存消耗对比 (已强制消除 OS 缓存干扰) <<<\n\n";
    test_memory(64 * 1024, 64.0 / 1024);
    test_memory(1024 * 1024, 1.0);
    test_memory(4096 * 1024, 4.0);
    return 0;
}