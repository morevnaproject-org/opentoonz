#include <advanced_color_selector.hpp>

#include "floatingpanelcommand.h"
#include "pane.h"
#include "tapp.h"
#include <toonz/palettecontroller.h>
#include "tpalette.h"
#include <toonz/tpalettehandle.h>

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
        auto palette = palette_handle->getPalette();
        auto styleIndex = palette_handle->getStyleIndex();
        if (!palette || styleIndex < 0)
          return;
        palette->setStyle(styleIndex, qColorToTPixel(c)); 
//             if (palette->getPaletteName() != L"EmptyColorFieldPalette")
//               TUndoManager::manager()->add(new UndoPaletteChange(
//                   m_paletteHandle, styleIndex, *m_oldStyle, *m_editedStyle));
//           }
// 
//           setOldStyleToStyle(m_editedStyle.getPointer());
// 
//           // In case the frame is a keyframe, update it
//           if (palette->isKeyframe(styleIndex, palette->getFrame()))  // here
//             palette->setKeyframe(styleIndex, palette->getFrame());   //
// 
//           palette->setDirtyFlag(true);
//         }

        palette_handle->notifyColorStyleChanged(true);
      }
    );
    auto update_wheel = [palette_handle, wheel]() {
      auto palette = palette_handle->getPalette();
      auto styleIndex = palette_handle->getStyleIndex();
      if (!palette || styleIndex < 0)
        return;
      wheel->setColor(tPixelToQColor(palette->getStyle(styleIndex)->getMainColor()));
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
