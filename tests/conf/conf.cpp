#include "conf.hpp"

#include <sstream>

#include <QTemporaryFile>

#include "conf/conf.hpp"

#include "tests/tests.hpp"

void ConfigurationTest::from_file(void) {
    QTemporaryFile tmp;
    QVERIFY(tmp.open());
    constexpr const char content[] = "test command0\ntest command1\n";
    constexpr auto len = sizeof(content) - 1;
    QCOMPARE(tmp.write(content, len), len);
    QVERIFY(tmp.seek(0));
    const auto ret =
        impero::Configuration::from_file(tmp.fileName().toStdString());
    std::stringstream ss;
    for(auto &x : ret)
        ss << x.cmd << '\n';
    QCOMPARE(ss.str(), content);
}

QTEST_MAIN(ConfigurationTest)
