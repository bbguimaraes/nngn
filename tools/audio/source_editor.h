#ifndef NNGN_TOOLS_AUDIO_SOURCE_EDIT_H
#define NNGN_TOOLS_AUDIO_SOURCE_EDIT_H

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QPlainTextEdit>

#include "utils/utils.h"

namespace nngn {

class LuaSyntaxHighlighter final : public QSyntaxHighlighter {
    Q_OBJECT
public:
    LuaSyntaxHighlighter(QTextDocument *parent = nullptr);
private:
    struct category {
        std::vector<QRegularExpression> regexps = {};
        QTextCharFormat fmt = {};
        int group = 0;
    };
    void highlightBlock(const QString &text) final;
    std::vector<category> categories = {};
};

class SourceEditor final : public QPlainTextEdit {
    Q_OBJECT
public:
    NNGN_NO_COPY(SourceEditor)
    SourceEditor(QWidget *parent = nullptr);
    using QPlainTextEdit::QPlainTextEdit;
signals:
    void updated(QString s) const;
private:
    void keyPressEvent(QKeyEvent *e) final;
    void dropEvent(QDropEvent *e) final;
    LuaSyntaxHighlighter *highlighter = nullptr;
};

}

#endif
