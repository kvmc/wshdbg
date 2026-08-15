#pragma once
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
struct TestCase{const char* name;std::function<void()> fn;};
inline std::vector<TestCase>& tests(){static std::vector<TestCase> value;return value;}
struct TestRegistrar{TestRegistrar(const char* name,std::function<void()> fn){tests().push_back({name,std::move(fn)});}};
#define WSHDBG_CONCAT_INNER(a,b) a##b
#define WSHDBG_CONCAT(a,b) WSHDBG_CONCAT_INNER(a,b)
#define TEST(name) static void WSHDBG_CONCAT(test_,__LINE__)(); static TestRegistrar WSHDBG_CONCAT(reg_,__LINE__){name,WSHDBG_CONCAT(test_,__LINE__)}; static void WSHDBG_CONCAT(test_,__LINE__)()
#define REQUIRE(expr) do { if(!(expr)) throw std::runtime_error(std::string("requirement failed: ")+#expr); } while(0)
