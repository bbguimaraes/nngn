#ifndef NNGN_TEST_FONT_H
#define NNGN_TEST_FONT_H

#include <QTest>

class FontTest : public QObject {
    Q_OBJECT
private slots:
    void from_file();
    void from_file_err();
};

#endif
