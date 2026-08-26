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

// Color param index for multi-color styles, like StyleEditor does:
// valid if 0 <= index < getColorParamCount(), else fall back to main color.
int getColorParamIndex(TPaletteHandle *palette_handle, TColorStyle *style) {
  int index = palette_handle->getStyleParamIndex();
  return (0 <= index && index < style->getColorParamCount()) ? index : -1;
}

TPixel32 getStyleColor(TPaletteHandle *palette_handle, TColorStyle *style) {
  int index = getColorParamIndex(palette_handle, style);
  return index >= 0 ? style->getColorParamValue(index) : style->getMainColor();
}

void setStyleColor(TPaletteHandle *palette_handle, TColorStyle *style,
                   TPixel32 color) {
  int index = getColorParamIndex(palette_handle, style);
  if (index >= 0)
    style->setColorParamValue(index, color);
  else
    style->setMainColor(color);
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
          TPixel32 oldColor = getStyleColor(palette_handle, style);
          TPixel32 newColor = qColorToTPixel(c);
          newColor.m = oldColor.m;
          if (oldColor != newColor) {
            setStyleColor(palette_handle, style, newColor);
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
      wheel->setColor(tPixelToQColor(getStyleColor(palette_handle, palette->getStyle(styleIndex))));
      wheel->saveToHistory();
    };
    // palette_handle outlives the panel: pass wheel as context so Qt
    // disconnects when the wheel is destroyed (e.g. on room deletion)
    QObject::connect(
      palette_handle,
      &TPaletteHandle::colorStyleSwitched,
      wheel,
      update_wheel
    );
    QObject::connect(
      palette_handle,
      &TPaletteHandle::colorStyleChanged,
      wheel,
      update_wheel
    );
    QObject::connect(
      palette_handle,
      &TPaletteHandle::paletteSwitched,
      wheel,
      update_wheel
    );
    // initialize with the current style color (no signal has fired yet)
    update_wheel();
    panel->setWidget(wheel);
    panel->setMinimumWidth(200);
    panel->resize(340, 630);  // same default size as Style Editor
    panel->setIsMaximizable(false);
  }
} advancedColorSelectorFactory;

OpenFloatingPanel openAdvancedColorSelectorCommand(
  "MI_OpenAdvancedColorSelector",
  "AdvancedColorSelector",
  QObject::tr("Advanced Color Selector")
);
