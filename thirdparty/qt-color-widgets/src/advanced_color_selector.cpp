/**
 * \file advanced_color_selector.cpp
 * \brief Advanced combined color selector widget
 *
 * \author caryoscelus
 *
 * \copyright Copyright (C) 2017 caryoscelus
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <QLayout>
#include <QToolButton>
#include <QTabWidget>
#include <QEvent>
#include <QResizeEvent>
#include <QButtonGroup>
#include <QAction>
#include <QMenu>
#include <QDebug>

#include <memory>
#include <cmath>

#include "color_wheel.hpp"
#include "color_2d_slider.hpp"
#include "hue_slider.hpp"
#include "color_line_edit.hpp"
#include "swatch.hpp"
#include "component_color_selector.hpp"
#include "advanced_color_selector.hpp"

namespace color_widgets {

static const int HISTORY_COLUMNS = 12;

class HarmonyButton : public QWidget {
public:
    HarmonyButton(AdvancedColorSelector* parent, unsigned n) :
        parent(parent),
        n(n),
        widget(new ColorLineEdit(this))
    {
        widget->setPreviewColor(true);
        auto layout = new QHBoxLayout();
        layout->addWidget(widget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        setLayout(layout);
        widget->installEventFilter(this);
    }
    inline void setColor(const QColor& color) {
        widget->setColor(color);
    }
    inline void setReadOnly(bool ro) {
        widget->setReadOnly(ro);
    }
    void setSelected(bool active) {
        setStyleSheet(active ? "font-weight: bold" : "");
    }
    bool eventFilter(QObject* object, QEvent* event) override {
        if (dynamic_cast<ColorLineEdit*>(object))
        {
            if (event->type() == QEvent::MouseButtonPress)
            {
                parent->setHarmony(n);
            }
        }
        return false;
    }
private:
    AdvancedColorSelector* parent;
    unsigned n;
    ColorLineEdit* widget;
};

class AdvancedColorSelector::Private : public QObject
{
public:
    Private(AdvancedColorSelector* parent) :
        wheel(new ColorWheel()),
        rectangle(new Color2DSlider()),
        hue_slider(new HueSlider(Qt::Vertical)),
        rgb_chooser(new ComponentColorSelector()),
        color_history(new Swatch()),
        harmony_buttons(new QButtonGroup()),
        wheel_layout(new QVBoxLayout()),
        parent(parent)
    {
        addColorWidget(wheel);
        addColorWidget(rectangle);
        addColorWidget(hue_slider);
        addColorWidget(rgb_chooser);

        auto harmony_none = newToolButton(
            QIcon(":/color_widgets/harmony/none.png"),
            [this]() {
                wheel->clearHarmonies();
                updateColors();
            }
        );
        harmony_buttons->addButton(
            harmony_none
        );
        harmony_buttons->addButton(
            newToolButton(
                QIcon(":/color_widgets/harmony/complementary.png"),
                [this]() {
                    wheel->clearHarmonies();
                    wheel->addHarmony(0.5, false);
                    updateColors();
                }
            )
        );
        harmony_buttons->addButton(
            newToolButton(
                QIcon(":/color_widgets/harmony/analogus.png"),
                [this]() {
                    wheel->clearHarmonies();
                    auto a = wheel->addHarmony(0.125, true);
                    wheel->addSymmetricHarmony(a);
                    updateColors();
                }
            )
        );
        harmony_buttons->addButton(
            newToolButton(
                QIcon(":/color_widgets/harmony/tetradic.png"),
                [this]() {
                    wheel->clearHarmonies();
                    wheel->addHarmony(0.5, false);
                    auto a = wheel->addHarmony(0.125, true);
                    wheel->addOppositeHarmony(a);
                    updateColors();
                }
            )
        );
        auto main_layout = new QVBoxLayout();

        auto tabs_widget = new QTabWidget();
        main_layout->addWidget(tabs_widget);

        auto wheel_container_widget = new QWidget();
        wheel_container_widget->installEventFilter(this);
        wheel_widget = new QWidget(wheel_container_widget);
        wheel_layout->setContentsMargins(0, 0, 0, 0);
        wheel_layout->setSpacing(0);
        wheel_widget->setLayout(wheel_layout);

        form_button = new QToolButton(wheel_container_widget);
        form_button->setCheckable(true);
        form_button->resize(32, 32);
        form_button->setStyleSheet("border: 0px;");
        connect(form_button, &QToolButton::toggled, [this](bool square) {
            if (square)
            {
                form_button->setIcon(QIcon(":/color_widgets/harmony/triangle.png"));
                wheel->setDisplayFlags(ColorWheel::SHAPE_SQUARE | ColorWheel::ANGLE_FIXED);
            }
            else
            {
                form_button->setIcon(QIcon(":/color_widgets/harmony/rectangle.png"));
                wheel->setDisplayFlags(ColorWheel::SHAPE_TRIANGLE | ColorWheel::ANGLE_ROTATING);
            }
        });
        form_button->setChecked(true);

        for (auto button : harmony_buttons->buttons())
            button->setParent(wheel_container_widget);

        auto wheel_inner_container = new QWidget();
        auto wheel_inner_layout = new QVBoxLayout();
        wheel_inner_layout->setContentsMargins(0, 40, 20, 0);
        wheel_inner_layout->addWidget(wheel);
        wheel_inner_container->setLayout(wheel_inner_layout);
        wheel_layout->addWidget(wheel_inner_container, 1.0);

        tabs_widget->addTab(wheel_container_widget, tr("Wheel"));

        auto rectangle_layout = new QHBoxLayout();
        rectangle_layout->addWidget(rectangle);
        rectangle_layout->addWidget(hue_slider);
        auto rectangle_widget = new QWidget();
        rectangle_widget->setLayout(rectangle_layout);
        tabs_widget->addTab(rectangle_widget, tr("Rectangle"));

        main_layout->addWidget(rgb_chooser);

        main_layout->addWidget(color_history);
        main_layout->setStretchFactor(tabs_widget, 1);
        color_history->setForcedColumns(HISTORY_COLUMNS);
        color_history->setColorSizePolicy(Swatch::Minimum);
        color_history->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

        auto config_menu_button = new QToolButton();
        config_menu_button->setDefaultAction(new QAction(QIcon::fromTheme("configure"), "Configure"));
        config_menu_button->setPopupMode(QToolButton::InstantPopup);
        auto config_menu = new QMenu();
        auto enable_rgb_action = new QAction("RGB sliders");
        enable_rgb_action->setCheckable(true);
        connect(enable_rgb_action, &QAction::toggled, [this](bool show) {
            rgb_chooser->setVisible(show);
        });
        config_menu->addAction(enable_rgb_action);
        config_menu_button->setMenu(config_menu);
        tabs_widget->setCornerWidget(config_menu_button);
        enable_rgb_action->setChecked(true);

        parent->setLayout(main_layout);

        connect(color_history, &Swatch::colorSelected, this, &Private::setColor);
        connect(parent, &AdvancedColorSelector::colorChanged, [this]() {
            if (color() != color_history->selectedColor()) {
                color_history->setSelected(-1);
            }
        });
        connect(wheel, &ColorWheel::harmonyChanged, this, &Private::updateColors);
        harmony_none->setChecked(true);
        setHarmony(0);
    }
    ~Private() = default;
public:
    bool eventFilter(QObject* /*target*/, QEvent* event) override {
        if (event->type() == QEvent::Resize) {
            auto size = static_cast<QResizeEvent*>(event)->size();
            auto w = size.width();
            auto h = size.height();
            wheel_widget->setGeometry(0, 0, w, h);
            form_button->move(5, h-60);
            int i = 0;
            auto x_start = std::max(0.0, 0.95*w-40.0*3.5-std::max(w-h, 0)/2.0);
            auto y_start = std::max(h-w-60, 0)/2.0;
            for (auto button : harmony_buttons->buttons()) {
                auto j = i*std::sqrt(std::min(w, h)/256.0);
                button->move(x_start+j*48-j*j*4, y_start+j*j*8-j*2);
                ++i;
            }
        }
        return false;
    }

    /**
     * Adds color widget with proper signals & slots
     *
     * TODO: more info
     */
    void addColorWidget(QObject* widget) {
        widgets.push_back(widget);
        connect(widget, SIGNAL(colorChanged(QColor)), parent, SLOT(setBaseColor(QColor)));
    }
    void removeColorWidget(QObject* widget) {
        widgets.removeAll(widget);
        disconnect(widget, SIGNAL(colorChanged(QColor)), parent, SLOT(setBaseColor(QColor)));
    }
    template <typename F>
    QToolButton* newToolButton(QIcon const& icon, F callback) const {
        auto button = new QToolButton();
        button->setCheckable(true);
        button->resize(32, 32);
        button->setIcon(icon);
        button->setStyleSheet(
            "QToolButton { border: 0px; }\n"
            "QToolButton:hover { border: 1px solid palette(dark); }\n"
            "QToolButton:checked { border: 2px solid palette(dark); }"
        );
        connect(button, &QToolButton::toggled, callback);
        return button;
    }
    void setColor(QColor c) {
        auto baseHue = c.hueF() - color().hueF() + baseColor().hueF();
        baseHue -= std::floor(baseHue);
        setBaseColor(QColor::fromHsvF(baseHue, c.saturationF(), c.valueF()));
    }
    void setBaseColor(QColor c) {
        for (auto widget : widgets) {
            auto oldState = widget->blockSignals(true);
            QMetaObject::invokeMethod(widget, "setColor", Q_ARG(QColor, c));
            widget->blockSignals(oldState);
        }
        updateColors();
    }
    void updateColors() {
        auto count = wheel->harmonyCount();
        auto colors = wheel->harmonyColors();
        if (harmony_colors_layout == nullptr || (unsigned) harmony_colors_layout->count() != count) {
            harmony_colors_widget.reset(new QWidget());
            harmony_colors_layout = new QHBoxLayout();
            harmony_colors_widget->setLayout(harmony_colors_layout);
            harmony_colors_widget->setMaximumHeight(32);
            wheel_layout->addWidget(harmony_colors_widget.get());
            harmony_colors_widgets.clear();
            for (unsigned i = 0; i < count; ++i)
            {
                auto button = new HarmonyButton(parent, i);
                harmony_colors_layout->addWidget(button);
                harmony_colors_widgets.append(button);
            }
        }
        unsigned i = 0;
        for (auto widget : harmony_colors_widgets)
        {
            widget->setColor(colors[i]);
            widget->setReadOnly(true);
            widget->setSelected((int)i == selected_harmony);
            ++i;
        }
        Q_EMIT parent->colorChanged(color());
    }
    void setHarmony(int i) {
        if (i < 0 || i >= (int)wheel->harmonyCount())
            i = 0;
        selected_harmony = i;
        int j = 0;
        for (auto widget : harmony_colors_widgets)
        {
            widget->setSelected(j == selected_harmony);
            ++j;
        }
        Q_EMIT parent->colorChanged(color());
    }
    QColor color() const {
        auto i = selected_harmony;
        if (i < 0 || i >= (int)wheel->harmonyCount())
            i = 0;
        return wheel->harmonyColors()[i];
    }
    QColor baseColor() const {
        return wheel->color();
    }
public:
    ColorWheel* wheel;
    Color2DSlider* rectangle;
    HueSlider* hue_slider;
    ComponentColorSelector* rgb_chooser;
    Swatch* color_history;
    QButtonGroup* harmony_buttons;
    QVBoxLayout* wheel_layout;
    EnabledWidgetsFlags enabled_widgets;
private:
    AdvancedColorSelector * const parent;
    QWidget* wheel_widget;
    QToolButton* form_button;
    QVector<QObject*> widgets;
    std::unique_ptr<QWidget> harmony_colors_widget = nullptr;
    QHBoxLayout* harmony_colors_layout = nullptr;
    QVector<HarmonyButton*> harmony_colors_widgets;
    int selected_harmony = 0;
};

AdvancedColorSelector::AdvancedColorSelector(QWidget* parent) :
    QWidget(parent),
    p(new Private(this))
{
}

AdvancedColorSelector::~AdvancedColorSelector()
{
}

QColor AdvancedColorSelector::color() const
{
    return p->color();
}

void AdvancedColorSelector::setColor(QColor c)
{
    // NOTE: QColors with different models compare unequal!
    if (c.toRgb() == p->color().toRgb())
        return;
    p->setColor(c);
    Q_EMIT colorChanged(color());
}

void AdvancedColorSelector::setBaseColor(QColor c)
{
    p->setBaseColor(c);
    Q_EMIT colorChanged(color());
}

void AdvancedColorSelector::setHarmony(unsigned harmony)
{
    p->setHarmony(harmony);
}

void AdvancedColorSelector::saveToHistory() {
    auto h = p->color_history;
    if (h->palette().colorAt(0) == color())
        return;
    while (h->palette().count() > HISTORY_COLUMNS-1) {
        h->palette().eraseColor(HISTORY_COLUMNS-1);
    }
    h->palette().insertColor(0, color());
    h->setSelected(0);
}

void AdvancedColorSelector::setEnabledWidgets(EnabledWidgetsFlags flags) {
    p->enabled_widgets = flags;
    if (RGBSliders & p->enabled_widgets) {
        p->rgb_chooser->show();
    } else {
        p->rgb_chooser->hide();
    }
}

} // namespace color_widgets
