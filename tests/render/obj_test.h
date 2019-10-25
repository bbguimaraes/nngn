#ifndef NNGN_TESTS_RENDER_OBJ_H
#define NNGN_TESTS_RENDER_OBJ_H

#include <QTest>

class ObjTest : public QObject {
    Q_OBJECT
private slots:
    void parse_empty(void);
    void parse_comment(void);
    void parse_cr(void);
    void parse_ignored(void);
    void parse_space(void);
    void parse_vertex(void);
    void parse(void);
    void parse_faces(void);
};

#endif
