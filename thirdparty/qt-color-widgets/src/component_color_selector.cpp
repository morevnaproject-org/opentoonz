/**
 * \file component_color_selector.cpp
 * \brief Color selector based on few widgets for each component
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
#include "component_color_selector.hpp"

#include "gradient_slider.hpp"

#include <QBoxLayout>

namespace color_widgets {

class ComponentColorSelector::Private
{
private:
    ComponentColorSelector * const w;

public:
    Private(ComponentColorSelector *widget)
        : w(widget)
        , red_slider(new GradientSlider())
        , green_slider(new GradientSlider())
        , blue_slider(new GradientSlider())
    {
        auto layout = new QVBoxLayout();
        layout->addWidget(red_slider);
        layout->addWidget(green_slider);
        layout->addWidget(blue_slider);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        red_slider->setFirstColor("#000000");
        red_slider->setLastColor("#ff0000");
        red_slider->setMaximum(255);
        green_slider->setFirstColor("#000000");
        green_slider->setLastColor("#00ff00");
        green_slider->setMaximum(255);
        blue_slider->setFirstColor("#000000");
        blue_slider->setLastColor("#0000ff");
        blue_slider->setMaximum(255);

        auto send_signal = [this]() {
            Q_EMIT w->colorChanged(color());
        };
        connect(red_slider, &QSlider::valueChanged, send_signal);
        connect(green_slider, &QSlider::valueChanged, send_signal);
        connect(blue_slider, &QSlider::valueChanged, send_signal);

        w->setLayout(layout);
    }

    QColor color() const {
        return QColor(red_slider->value(),
                      green_slider->value(),
                      blue_slider->value());
    }

    void setColor(QColor c) {
        red_slider->setValue(c.red());
        green_slider->setValue(c.green());
        blue_slider->setValue(c.blue());
    }

private:
    GradientSlider *red_slider;
    GradientSlider *green_slider;
    GradientSlider *blue_slider;
};

ComponentColorSelector::ComponentColorSelector(QWidget* parent) :
    QWidget(parent),
    p(new Private(this))
{
}

ComponentColorSelector::~ComponentColorSelector()
{
}

QColor ComponentColorSelector::color() const {
    return p->color();
}

void ComponentColorSelector::setColor(QColor c) {
    p->setColor(c);
}

} // namespace color_widgets
