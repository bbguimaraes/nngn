#include "edit.h"

#include <filesystem>

#include <QComboBox>
#include <QDial>
#include <QDockWidget>
#include <QSlider>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QPushButton>
#include <QSpacerItem>
#include <QSpinBox>
#include <QTabBar>
#include <QTimer>

#include "tools/utils.h"

#include "components.h"
#include "pcm.h"
#include "source_editor.h"

namespace {

auto create_controls(QSpinBox *rate) {
    auto *const controls = new QWidget;
    auto *const cl = new QFormLayout{controls};
    auto *const rate_label = new QLabel{"Rate (Hz)"};
    rate_label->setBuddy(rate);
    rate->setMaximum(INT_MAX);
    rate->setValue(44100);
    cl->addRow(rate_label, rate);
    auto *const preset_label = new QLabel{"Preset"};
    auto *const preset = new QComboBox;
    preset_label->setBuddy(preset);
    preset->setSizePolicy(
        QSizePolicy::Expanding, preset->sizePolicy().verticalPolicy());
    cl->addRow(preset_label, preset);
    auto *const gain_label = new QLabel{"Gain (%)"};
    auto *const gain = new QSlider{Qt::Horizontal};
    gain_label->setBuddy(gain);
    gain->setTickPosition(QSlider::TicksBothSides);
    gain->setTickInterval(20);
    gain->setMinimum(0);
    gain->setMaximum(100);
    gain->setValue(100);
    cl->addRow(gain_label, gain);
    cl->addItem(new QSpacerItem(0, 16));
    auto *const param_label = new QLabel{"Parameter: 0.00"};
    param_label->setAlignment(Qt::AlignHCenter);
    param_label->setSizePolicy(
        QSizePolicy::Expanding, preset->sizePolicy().verticalPolicy());
    auto *const param = new QDial;
    param->setMinimum(0);
    param->setMaximum(100);
    param->setValue(0);
    cl->addRow(param_label);
    cl->addRow(param);
    cl->addItem(new QSpacerItem(0, 16));
    auto *const layout = new QGridLayout;
    cl->addRow(layout);
    auto *const generate = new QPushButton{"Generate"};
    layout->addWidget(generate, 0, 0, 1, 2);
    auto *const stop = new QPushButton{"Stop"};
    layout->addWidget(stop, 1, 0);
    auto *const clear = new QPushButton{"Clear"};
    layout->addWidget(clear, 1, 1);
    auto *const loop = new QPushButton{"Loop"};
    loop->setCheckable(true);
    layout->addWidget(loop, 2, 0);
    auto *const monitor = new QPushButton{"Monitor"};
    monitor->setCheckable(true);
    layout->addWidget(monitor, 2, 1);
    auto *const rewind = new QPushButton{"Rewind"};
    rewind->setCheckable(true);
    rewind->setChecked(true);
    layout->addWidget(rewind, 3, 0);
    auto *const mute = new QPushButton{"Mute"};
    mute->setCheckable(true);
    layout->addWidget(mute, 3, 1);
    auto *const controls_dock = new QDockWidget{"Controls"};
    controls_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    controls_dock->setWidget(controls);
    return std::tuple{
        controls_dock, controls, layout,
        preset, gain, param_label, param,
        generate, stop, clear, loop, monitor, rewind, mute,
    };
}

auto create_bottom_row(QPlainTextEdit *error) {
    error->setReadOnly(true);
    auto *const pcm = new nngn::PCMWidget;
    pcm->setMinimumSize(320, 200);
    auto *const pcm_dock = new QDockWidget{"Display"};
    pcm_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    pcm_dock->setWidget(pcm);
    auto *const error_dock = new QDockWidget{"Errors"};
    error_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    error_dock->setWidget(error);
    return std::tuple{pcm_dock, pcm, error_dock};
}

auto create_components_dock(void) {
    auto *const window = new nngn::Components;
    auto *const scroll = new QScrollArea;
    scroll->setWidget(window);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setWidgetResizable(true);
    auto *const dock = new QDockWidget{"Components"};
    dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    dock->setWidget(scroll);
    dock->hide();
    return std::tuple{dock, window};
}

template<Qt::DockWidgetArea area>
auto add_dock(
    QMainWindow *window, QDockWidget *dock, QMenu *menu, const char *menu_name)
{
    auto *const action = menu->addAction(menu_name);
    action->setCheckable(true);
    QObject::connect(dock, &QDockWidget::visibilityChanged, [action](bool b) {
        action->blockSignals(true);
        action->setChecked(b);
        action->blockSignals(false);
    });
    window->addDockWidget(area, dock);
    QObject::connect(action, &QAction::changed, [window, dock] {
        const auto v = !dock->isVisible();
        dock->setVisible(v);
        if(v && window->dockWidgetArea(dock) == Qt::NoDockWidgetArea)
            window->addDockWidget(area, dock);
    });
}

void load_presets(QComboBox *b, QList<QString> *v) {
    const auto d =
        std::filesystem::current_path() / "tools" / "audio" / "presets";
    if(!std::filesystem::exists(d))
        return;
    std::vector<std::pair<QString, QString>> tmp = {};
    std::string prog_tmp = {};
    for(const auto &e: std::filesystem::directory_iterator{d}) {
        if(e.is_directory())
            continue;
        const auto p = e.path();
        const auto s = p.string();
        if(!nngn::read_file(s, &prog_tmp))
            continue;
        tmp.emplace_back(
            nngn::qstring_from_view(p.stem().string()),
            nngn::qstring_from_view(prog_tmp));
    }
    std::ranges::sort(tmp);
    v->reserve(static_cast<int>(tmp.size()));
    for(const auto &x : tmp) {
        b->addItem(x.first);
        v->push_back(x.second);
    }
}

}

namespace nngn {

Edit::Edit(void) {
    this->gen.set_rewind(true);
    auto *const menu_bar = this->menuBar();
    auto *const window_menu = menu_bar->addMenu("&Window");
    auto *const source_editor = new SourceEditor;
    this->editor = source_editor;
    this->editor->setFocus();
    const auto [components_dock, components_window] = create_components_dock();
    this->rate = new QSpinBox;
    const auto [
        controls_dock, controls, control_layout,
        preset, gain, param_label, param,
        generate_btn, stop, clear, loop, monitor, rewind, mute
    ] = create_controls(this->rate);
    this->error = new QPlainTextEdit;
    const auto [pcm_dock, pcm, error_dock] = create_bottom_row(this->error);
    auto *const pos_timer = new QTimer{this};
    pos_timer->start(10);
    this->setCentralWidget(this->editor);
    this->setDockNestingEnabled(true);
    add_dock<Qt::RightDockWidgetArea>(
        this, controls_dock, window_menu, "&Controls");
    add_dock<Qt::LeftDockWidgetArea>(
        this, components_dock, window_menu, "Com&ponents");
    add_dock<Qt::BottomDockWidgetArea>(
        this, pcm_dock, window_menu, "&Display");
    this->tabifyDockWidget(pcm_dock, error_dock);
    this->tab_bar = this->findChild<QTabBar*>();
    this->tab_bar->setCurrentIndex(0);
    load_presets(preset, &this->presets);
    if(!this->presets.empty())
        this->editor->setPlainText(this->presets[0]);
    QObject::connect(loop, &QPushButton::toggled, [this](bool b) {
        this->gen.set_loop(b) && b && this->generate();
    });
    QObject::connect(
        preset, QOverload<int>::of(&QComboBox::currentIndexChanged),
        [this](int i) { this->editor->setPlainText(this->presets[i]); });
    QObject::connect(
        gain, &QSlider::valueChanged,
        [this](int x) { this->gen.set_gain(static_cast<float>(x) / 100.0f); });
    QObject::connect(
        param, &QDial::valueChanged,
        [this](int x) { this->gen.set_param(static_cast<float>(x) / 100.0f); });
    QObject::connect(param, &QDial::valueChanged, [l = param_label](int x) {
        l->setText(QString::asprintf(
            "Parameter: %.2f", static_cast<double>(x) / 100));
    });
    QObject::connect(
        generate_btn, &QPushButton::pressed,
        std::bind_front(&Edit::generate, this));
    QObject::connect(
        stop, &QPushButton::pressed,
        std::bind_front(&Generator::stop, &this->gen));
    QObject::connect(
        clear, &QPushButton::pressed,
        std::bind_front(&Generator::stop, &this->gen));
    QObject::connect(clear, &QPushButton::pressed, pcm, &PCMWidget::clear);
    QObject::connect(monitor, &QPushButton::toggled, [this, p = param](bool s) {
        if(s) {
            const auto f = std::bind_front(&Edit::generate, this);
            this->editor_con = QObject::connect(
                this->editor, &SourceEditor::textChanged, f);
            this->param_con = QObject::connect(p, &QDial::valueChanged, f);
        } else {
            QObject::disconnect(std::exchange(this->editor_con, {}));
            QObject::disconnect(std::exchange(this->param_con, {}));
        }
    });
    QObject::connect(
        rewind, &QPushButton::toggled,
        std::bind_front(&Generator::set_rewind, &this->gen));
    QObject::connect(
        mute, &QPushButton::toggled,
        std::bind_front(&Generator::set_mute, &this->gen));
    mute->setChecked(false);
    QObject::connect(
        components_window, &Components::clicked,
        [e = this->editor](const QString &s) {
            e->insertPlainText(s);
            e->setFocus();
        });
    QObject::connect(
        this->editor, &SourceEditor::textChanged,
        std::bind_front(&QPushButton::setEnabled, generate_btn, true));
    QObject::connect(
        source_editor, &SourceEditor::updated,
        [b = generate_btn] { b->animateClick(); });
    QObject::connect(this, &Edit::updated, [p = pcm](auto v) { p->update(v); });
    QObject::connect(
        this, &Edit::updated,
        std::bind_front(&PCMWidget::set_pos, pcm, 1));
    QObject::connect(
        pos_timer, &QTimer::timeout,
        [&g = this->gen, p = pcm] { p->set_pos(g.pos()); });
}

bool Edit::init_generator(void) {
    if(!this->rate->isEnabled())
        return true;
    this->rate->setEnabled(false);
    return this->gen.init(
        static_cast<std::size_t>(this->rate->text().toULongLong()));
}

bool Edit::exec(void) {
    return this->gen.generate(this->editor->toPlainText());
}

bool Edit::generate(void) {
    if(this->init_generator() && this->exec()) {
        this->error->setPlainText("");
        this->tab_bar->setCurrentIndex(0);
        emit this->updated(this->gen.release_data());
        return true;
    } else {
        this->error->setPlainText(qstring_from_view(this->gen.error()));
        this->tab_bar->setCurrentIndex(1);
        return false;
    }
}

}
