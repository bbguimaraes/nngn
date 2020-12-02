#include "command.hpp"

#include "cmd/command.hpp"

#include "tests/tests.hpp"

void CommandTest::exec_command(void) {
    using namespace std::literals;
    const auto called_str = "called"sv;
    static auto called = std::string_view{};
    const auto f = [](std::string_view s) { called = s; return true; };
    auto cmd = impero::ExecCommand<f>{"called"};
    QCOMPARE(cmd.command(), "called");
    QVERIFY(cmd.execute());
    QCOMPARE(called, called_str);
}

QTEST_MAIN(CommandTest)
