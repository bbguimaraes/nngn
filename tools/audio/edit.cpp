#include "edit.h"

#include <filesystem>

#include <QComboBox>
#include <QDial>
#include <QDockWidget>
#include <QSlider>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpacerItem>
#include <QSpinBox>
#include <QTabBar>

#include "pcm.h"
#include "source_editor.h"

namespace {

QString qstring(std::string_view s) {
    return QString::fromUtf8(s.data(), static_cast<int>(s.size()));
}

void load_presets(QComboBox *b, std::vector<std::string> *v) {
    const auto d =
        std::filesystem::current_path() / "tools" / "audio" / "presets";
    if(!std::filesystem::exists(d))
        return;
    std::vector<std::pair<std::string, std::string>> tmp = {};
    std::string prog_tmp = {};
    for(const auto &e: std::filesystem::directory_iterator{d}) {
        if(e.is_directory())
            continue;
        const auto p = e.path();
        const auto s = p.string();
        if(!nngn::read_file(s, &prog_tmp))
            continue;
        tmp.emplace_back(p.stem().string(), std::move(prog_tmp));
    }
    std::ranges::sort(tmp);
    v->reserve(tmp.size());
    for(auto &x : tmp) {
        b->addItem(qstring(x.first));
        v->emplace_back(std::move(x.second));
    }
}

}

namespace nngn {

Edit::Edit(void) {
    auto *const rate_label = new QLabel{"Rate (Hz)"};
    this->rate = new QSpinBox;
    auto *const preset_label = new QLabel{"Preset"};
    auto *const preset = new QComboBox;
    auto *const gain_label = new QLabel{"Gain (%)"};
    auto *const gain = new QSlider{Qt::Horizontal};
    auto *const param_label = new QLabel{"Parameter: 0.00"};
    this->param = new QDial; //FloatWidget;
    this->loop = new QPushButton{"Loop"};
    auto *const monitor = new QPushButton{"Monitor"};
    auto *const rewind = new QPushButton{"Rewind"};
    auto *const generate_btn = new QPushButton{"Generate"};
    this->error = new QPlainTextEdit;
    auto *const pcm = new PCMWidget;
    rate_label->setBuddy(this->rate);
    rate->setMaximum(INT_MAX);
    rate->setValue(44100);
    preset_label->setBuddy(preset);
    preset->setSizePolicy(
        QSizePolicy::Expanding, preset->sizePolicy().verticalPolicy());
    load_presets(preset, &this->presets);
    gain_label->setBuddy(gain);
    gain->setTickPosition(QSlider::TicksBothSides);
    gain->setTickInterval(20);
    gain->setMinimum(0);
    gain->setMaximum(100);
    gain->setValue(100);
    param_label->setAlignment(Qt::AlignHCenter);
    param_label->setSizePolicy(
        QSizePolicy::Expanding, preset->sizePolicy().verticalPolicy());
    this->param->setMinimum(0);
    this->param->setMaximum(100);
    this->param->setValue(0);
    this->loop->setCheckable(true);
    monitor->setCheckable(true);
    rewind->setCheckable(true);
    rewind->setChecked(true);
    this->gen.set_rewind(true);
    this->error->setReadOnly(true);
    pcm->setMinimumSize(320, 200);
    auto *const controls = new QWidget;
    auto *const control_layout = new QGridLayout;
    control_layout->addWidget(this->loop, 0, 0);
    control_layout->addWidget(monitor, 0, 1);
    control_layout->addWidget(rewind, 1, 0);
    control_layout->addWidget(generate_btn, 1, 1);
    auto *const cl = new QFormLayout{controls};
    cl->addRow(rate_label, this->rate);
    cl->addRow(preset_label, preset);
    cl->addRow(gain_label, gain);
    cl->addItem(new QSpacerItem(0, 16));
    cl->addRow(param_label);
    cl->addRow(this->param);
    cl->addItem(new QSpacerItem(0, 16));
    cl->addRow(control_layout);
    auto *const source_editor = new SourceEditor;
    this->editor = source_editor;
    if(!this->presets.empty())
        source_editor->setPlainText(qstring(this->presets[0]));
    source_editor->setFocus();
    this->setDockNestingEnabled(true);
    this->setCentralWidget(source_editor);
    auto *const controls_dock = new QDockWidget{"Controls"};
    controls_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    controls_dock->setWidget(controls);
    this->addDockWidget(Qt::RightDockWidgetArea, controls_dock);
    auto *const pcm_dock = new QDockWidget{"Display"};
    pcm_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    pcm_dock->setWidget(pcm);
    this->addDockWidget(Qt::BottomDockWidgetArea, pcm_dock);
    auto *const error_dock = new QDockWidget{"Errors"};
    error_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    error_dock->setWidget(this->error);
    this->tabifyDockWidget(pcm_dock, error_dock);
    this->tab_bar = this->findChild<QTabBar*>();
    this->tab_bar->setCurrentIndex(0);
    QObject::connect(this->loop, &QPushButton::toggled, [this](bool b) {
        this->gen.set_loop(b) && b && this->generate();
    });
    QObject::connect(
        preset, QOverload<int>::of(&QComboBox::currentIndexChanged),
        [this](int i) {
            this->editor->setPlainText(
                qstring(this->presets[static_cast<std::size_t>(i)]));
        });
    QObject::connect(
        gain, &QSlider::valueChanged,
        [this](int x) {
            if(this->gen.set_gain(static_cast<float>(x) / 100.0f))
                emit this->updated(std::move(this->gen.release_data()));
        });
    QObject::connect(
        this->param, &QDial::valueChanged, [this, param_label](int x) {
            param_label->setText(QString::asprintf(
                "Parameter: %.2f", static_cast<double>(x) / 100));
            this->generate();
        });
    QObject::connect(
        monitor, &QPushButton::toggled,
        [this, c = QMetaObject::Connection{}](bool s) mutable {
            if(s)
                c = QObject::connect(
                    this->editor, &SourceEditor::textChanged,
                    [this] { this->generate(); });
            else
                QObject::disconnect(std::exchange(c, {}));
        });
    QObject::connect(
        rewind, &QPushButton::toggled,
        [this](bool s) { this->gen.set_rewind(s); });
    QObject::connect(
        generate_btn, &QPushButton::pressed,
        [this] { this->generate(); });
    QObject::connect(
        source_editor, &SourceEditor::textChanged,
        [generate_btn] { generate_btn->setEnabled(true); });
    QObject::connect(
        source_editor, &SourceEditor::updated,
        [generate_btn] { generate_btn->animateClick(); });
    QObject::connect(
        this, &Edit::updated,
        [pcm](auto v) { pcm->update(v); });
}

bool Edit::init_generator(void) {
    if(!rate->isEnabled())
        return true;
    rate->setEnabled(false);
    return this->gen.init(static_cast<std::size_t>(rate->text().toULongLong()));
}

bool Edit::exec(void) {
    const auto prog = this->editor->toPlainText();
    const auto param_val = static_cast<float>(this->param->value()) / 100.0f;
    return this->gen.exec(prog, param_val)
        && this->gen.generate();
}

bool Edit::generate(void) {
    if(this->init_generator() && this->exec()) {
        this->error->setPlainText("");
        this->tab_bar->setCurrentIndex(0);
        emit this->updated(std::move(this->gen.release_data()));
        return true;
    } else {
        this->error->setPlainText(qstring(this->gen.error()));
        this->tab_bar->setCurrentIndex(1);
        return false;
    }
}

}
