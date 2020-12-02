#ifndef IMPERO_TESTS_CONF_CONF_H
#define IMPERO_TESTS_CONF_CONF_H

#include <QTest>

class ConfigurationTest : public QObject {
    Q_OBJECT
private slots:
    void from_file(void);
};

#endif
