#pragma once

#ifndef MYPAINT_HPP
#define MYPAINT_HPP

#include <string>

extern "C" {
  #include <mypaint-brush.h>
  #include <mypaint-surface.h>
}

namespace mypaint {
  class Brush;

  //=======================================================
  //
  // Surface
  //
  //=======================================================

  class Surface {
    friend class Brush;

    struct InternalSurface: public MyPaintSurface {
      Surface *m_owner;
    };

    InternalSurface m_surface;

    static int internalDrawDab(
        MyPaintSurface *self,
        float x, float y, float radius,
        float colorR, float colorG, float colorB,
        float opaque, float hardness,
        float alphaEraser,
        float aspectRatio, float angle,
        float lockAlpha,
        float colorize )
    {
      return static_cast<InternalSurface*>(self)->m_owner->drawDab(
        x, y, radius,
        colorR, colorG, colorB,
        opaque, hardness,
        alphaEraser,
        aspectRatio,
        angle,
        lockAlpha,
        colorize );
    }

    static void internalGetColor(
        MyPaintSurface *self,
        float x, float y, float radius,
        float *colorR, float *colorG, float *colorB, float *colorA )
    {
      static_cast<InternalSurface*>(self)->m_owner->getColor(
        x, y,
        radius,
        *colorR, *colorG, *colorB, *colorA);
    }

  public:
    Surface():
      m_surface()
    {
      m_surface.m_owner = this;
      m_surface.draw_dab = internalDrawDab;
      m_surface.get_color = internalGetColor;
    }

    virtual ~Surface() { }

    virtual void getColor(
        float x, float y, float radius,
        float &colorR, float &colorG, float &colorB, float &colorA ) = 0;

    virtual bool drawDab(
        float x, float y, float radius,
        float colorR, float colorG, float colorB,
        float opaque, float hardness,
        float alphaEraser,
        float aspectRatio, float angle,
        float lockAlpha,
        float colorize ) = 0;
  };

  //=======================================================
  //
  // Brush
  //
  //=======================================================

  class Brush {
    MyPaintBrush *m_brush;

  public:
    Brush():
      m_brush(mypaint_brush_new())
    { fromDefaults(); }

    Brush(const Brush &other):
      m_brush(mypaint_brush_new())
    { fromBrush(other); }

    ~Brush()
    { mypaint_brush_unref(m_brush); }

    Brush& operator= (const Brush &other) {
      fromBrush(other);
      return *this;
    }

    void reset()
    { mypaint_brush_reset(m_brush); }

    void newStroke()
    { mypaint_brush_new_stroke(m_brush); }

    int strokeTo(Surface &surface, float x, float y,
                  float pressure, float xtilt, float ytilt, double dtime)
    {
      return mypaint_brush_stroke_to(m_brush, &surface.m_surface,
                                     x, y, pressure, xtilt, ytilt, dtime);
    }

    void setBaseValue(MyPaintBrushSetting id, float value)
    { mypaint_brush_set_base_value(m_brush, id, value); }

    float getBaseValue(MyPaintBrushSetting id) const
    { return mypaint_brush_get_base_value(m_brush, id); }

    bool isConstant(MyPaintBrushSetting id) const
    { return mypaint_brush_is_constant(m_brush, id); }

    int getInputsUsedN(MyPaintBrushSetting id) const
    { return mypaint_brush_get_inputs_used_n(m_brush, id); }

    void setMappingN(MyPaintBrushSetting id, MyPaintBrushInput input, int n)
    { mypaint_brush_set_mapping_n(m_brush, id, input, n); }

    int getMappingN(MyPaintBrushSetting id, MyPaintBrushInput input) const
    { return mypaint_brush_get_mapping_n(m_brush, id, input); }

    void setMappingPoint(MyPaintBrushSetting id, MyPaintBrushInput input, int index, float x, float y)
    { mypaint_brush_set_mapping_point(m_brush, id, input, index, x, y); }

    void getMappingPoint(MyPaintBrushSetting id, MyPaintBrushInput input, int index, float &x, float &y) const
    { mypaint_brush_get_mapping_point(m_brush, id, input, index, &x, &y); }

    float getState(MyPaintBrushState i) const
    { return mypaint_brush_get_state(m_brush, i); }

    void setState(MyPaintBrushState i, float value)
    { return mypaint_brush_set_state(m_brush, i, value); }

    double getTotalStrokePaintingTime() const
    { return mypaint_brush_get_total_stroke_painting_time(m_brush); }

    void setPrintInputs(bool enabled)
    { mypaint_brush_set_print_inputs(m_brush, enabled); }

    void fromDefaults() {
      reset();
      newStroke();
      mypaint_brush_from_defaults(m_brush);
    }

    void fromBrush(const Brush &other) {
      reset();
      newStroke();
      for(int i = 0; i < MYPAINT_BRUSH_SETTINGS_COUNT; ++i) {
        MyPaintBrushSetting id = (MyPaintBrushSetting)i;
        setBaseValue(id, other.getBaseValue(id));
        for(int j = 0; j < MYPAINT_BRUSH_INPUTS_COUNT; ++j) {
          MyPaintBrushInput input = (MyPaintBrushInput)j;
          int n = other.getMappingN(id, input);
          setMappingN(id, input, n);
          for(int index = 0; index < n; ++index) {
            float x = 0.f, y = 0.f;
            other.getMappingPoint(id, input, index, x, y);
            setMappingPoint(id, input, index, x, y);
          }
        }
      }
    }

    bool fromString(const std::string &s) {
      reset();
      newStroke();
      return mypaint_brush_from_string(m_brush, s.c_str());
    }
  };
}

#endif  // MYPAINT_HPP
