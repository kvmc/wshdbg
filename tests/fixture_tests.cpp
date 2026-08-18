#include "test.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

std::string read_fixture(const std::filesystem::path& relative) {
    const auto path = std::filesystem::path{WSHDBG_SOURCE_DIR} / "tests" / "data" / relative;
    std::ifstream input(path, std::ios::binary);
    REQUIRE(input.good());
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

} // namespace

TEST("basic VBScript fixture is stable") {
    const auto text = read_fixture("basic/hello.vbs");
    REQUIRE(text.find("hello from vbscript fixture") != std::string::npos);
    REQUIRE(text.find("WScript.Echo") != std::string::npos);
}

TEST("basic JScript fixture is stable") {
    const auto text = read_fixture("basic/hello.js");
    REQUIRE(text.find("hello from jscript fixture") != std::string::npos);
    REQUIRE(text.find("WScript.Echo") != std::string::npos);
}

TEST("error fixtures preserve deterministic fault sites") {
    const auto syntax = read_fixture("errors/syntax_error.vbs");
    const auto runtime = read_fixture("errors/runtime_error.vbs");
    REQUIRE(syntax.find("value =\n") != std::string::npos);
    REQUIRE(runtime.find("1 / 0") != std::string::npos);
}

TEST("debugger fixtures cover breakpoints variables COM and WSH") {
    REQUIRE(read_fixture("breakpoints/simple.vbs").find("x = x + 1") != std::string::npos);
    REQUIRE(read_fixture("breakpoints/nested_calls.vbs").find("Inner value + 1") != std::string::npos);
    REQUIRE(read_fixture("variables/scalars.vbs").find("numberValue = 42") != std::string::npos);
    REQUIRE(read_fixture("com/create_object.vbs").find("CreateObject(\"WScript.Shell\")") != std::string::npos);
    REQUIRE(read_fixture("wsh/echo.vbs").find("fixture-output") != std::string::npos);
}
