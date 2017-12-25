#include <advanced_color_selector.hpp>

#include "floatingpanelcommand.h"
#include "pane.h"
#include "tapp.h"
#include <toonz/palettecontroller.h>
#include "tpalette.h"
#include <toonz/tpalettehandle.h>
#include <toonz/imagestyles.h>

#include <QDebug>
// #include "advanced_color_selector.h"

using color_widgets::AdvancedColorSelector;

TPixelRGBM32 qColorToTPixel(QColor c) {
  return { c.red(), c.green(), c.blue(), c.alpha() };
}

QColor tPixelToQColor(TPixelRGBM32 pix) {
  return { pix.r, pix.g, pix.b, pix.m };
}

class AdvancedColorSelectorFactory final : public TPanelFactory {
public:
  AdvancedColorSelectorFactory() : TPanelFactory("AdvancedColorSelector") {}
  void initialize(TPanel *panel) override {
    auto wheel = new AdvancedColorSelector(panel);
    auto palette_controller = TApp::instance()->getPaletteController();
    auto palette_handle = palette_controller->getCurrentPalette();
    QObject::connect(
      wheel,
      &AdvancedColorSelector::colorChanged,
      [palette_handle](QColor c){
        if (TColorStyle *style = palette_handle->getStyle()) {
          TPixel32 oldColor = style->getMainColor();
          TPixel32 newColor = qColorToTPixel(c);
          newColor.m = oldColor.m;
          if (oldColor != newColor) {
            style->setMainColor(newColor);
            palette_handle->notifyColorStyleChanged(true);
          }
        }
      }
    );
    auto update_wheel = [palette_handle, wheel]() {
      auto palette = palette_handle->getPalette();
      auto styleIndex = palette_handle->getStyleIndex();
      if (!palette || styleIndex < 0)
        return;
      wheel->setColor(tPixelToQColor(palette->getStyle(styleIndex)->getMainColor()));
      wheel->saveToHistory();
    };
    QObject::connect(
      palette_handle,
      &TPaletteHandle::colorStyleSwitched,
      update_wheel
    );
    QObject::connect(
      palette_handle,
      &TPaletteHandle::colorStyleChanged,
      update_wheel
    );
    QObject::connect(
      palette_handle,
      &TPaletteHandle::paletteSwitched,
      update_wheel
    );
    panel->setWidget(wheel);
    panel->setIsMaximizable(false);
  }
} advancedColorSelectorFactory;

OpenFloatingPanel openAdvancedColorSelectorCommand(
  "MI_OpenAdvancedColorSelector",
  "AdvancedColorSelector",
  QObject::tr("Advanced Color Selector")
);
