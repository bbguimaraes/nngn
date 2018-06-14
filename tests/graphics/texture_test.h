#ifndef NNGN_TEST_TEXTURE_H
#define NNGN_TEST_TEXTURE_H

#include <filesystem>
#include <QTest>

#include "graphics/graphics.h"

class TextureTest : public QObject {
    Q_OBJECT
    static std::filesystem::path data_file, data_alpha_file;
    static std::byte data[nngn::Graphics::TEXTURE_SIZE];
    static std::byte data_alpha[nngn::Graphics::TEXTURE_SIZE];
private slots:
    void initTestCase();
    void constructor();
    void read();
    void load_data_test();
    void load();
    void load_alpha();
    void load_err();
    void load_ids();
    void load_max();
    void load_cache();
    void remove();
};

#endif
