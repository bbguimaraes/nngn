#include "source_editor.h"

#include <QKeyEvent>

#include "../utils.h"

using namespace std::string_view_literals;
using namespace nngn::literals;

namespace nngn {

SourceEditor::SourceEditor(QWidget *p) : QPlainTextEdit(p) {
    QPalette pal = this->palette();
    pal.setColor(QPalette::Base, Qt::black);
    this->setPalette(pal);
    QFont font = {};
    font.setFamily("monospace");
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    this->setFont(font);
    const auto space = QFontMetrics(font).horizontalAdvance(' ');
    this->setCursorWidth(space);
    this->setTabStopDistance(4 * space);
    this->highlighter = new LuaSyntaxHighlighter{this->document()};
}

void SourceEditor::keyPressEvent(QKeyEvent *e) {
    switch(e->key()) {
    case Qt::Key_Return:
       if(!(e->modifiers() & Qt::ControlModifier))
           break;
        e->accept();
        emit this->updated(this->toPlainText());
        return;
    case Qt::Key_Tab:
        if(e->modifiers())
            break;
        e->accept();
        this->insertPlainText("    ");
        return;
    }
    return QPlainTextEdit::keyPressEvent(e);
}

void SourceEditor::dropEvent(QDropEvent *e) {
    QPlainTextEdit::dropEvent(e);
    this->setFocus();
}

LuaSyntaxHighlighter::LuaSyntaxHighlighter(QTextDocument *p)
    : QSyntaxHighlighter(p)
{
    QTextCharFormat keyword_fmt = {};
    keyword_fmt.setFontWeight(QFont::Bold);
    QTextCharFormat sec_keyword_fmt = {};
    sec_keyword_fmt.setForeground(Qt::gray);
    QTextCharFormat literal_fmt = {};
    literal_fmt.setFontWeight(QFont::Bold);
    literal_fmt.setForeground(Qt::darkGray);
    this->categories.push_back({
        .regexps = {
            R"(\band\b)"_qre,   R"(\bbreak\b)"_qre,  R"(\bdo\b)"_qre,
            R"(\belse\b)"_qre,  R"(\belseif\b)"_qre, R"(\bend\b)"_qre,
            R"(\bfalse\b)"_qre, R"(\bfor\b)"_qre,    R"(\bfunction\b)"_qre,
            R"(\bgoto\b)"_qre,  R"(\bif\b)"_qre,     R"(\bin\b)"_qre,
            R"(\blocal\b)"_qre, R"(\bnil\b)"_qre,    R"(\bnot\b)"_qre,
            R"(\bor\b)"_qre,    R"(\brepeat\b)"_qre, R"(\breturn\b)"_qre,
            R"(\bthen\b)"_qre,  R"(\btrue\b)"_qre,   R"(\buntil\b)"_qre,
            R"(\bwhile\b)"_qre, "{"_qre, "}"_qre,
        },
        .fmt = keyword_fmt,
        .group = 0,
    });
    this->categories.push_back({
        .regexps = {
            R"(\blocal\s+\w+\s*(<const>))"_qre,
            R"(\blocal\s+\w+\s*(<close>))"_qre,
        },
        .fmt = sec_keyword_fmt,
        .group = 1,
    });
    this->categories.push_back({
        .regexps = {
            R"("[^"]*")"_qre,
            R"(\b(?:)"
                R"(\d+(?:\.(?:\d+)?)?)"
                "|"
                R"(\.\d+)"
            R"()\b)"_qre,
        },
        .fmt = literal_fmt,
        .group = 0,
    });
}

void LuaSyntaxHighlighter::highlightBlock(const QString &text) {
    for(const auto &[v, fmt, group] : this->categories)
        for(const auto &regexp : v) {
            auto i = regexp.globalMatch(text);
            while(i.hasNext()) {
                const auto m = i.next();
                this->setFormat(
                    m.capturedStart(group), m.capturedLength(group), fmt);
            }
        }
}

}
