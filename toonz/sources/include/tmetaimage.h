#pragma once

#ifndef TMETAIMAGE_INCLUDED
#define TMETAIMAGE_INCLUDED

#include "timage.h"
#include "tthreadmessage.h"
#include "tsmartpointer.h"
#include "tvariant.h"

#include <QReadLocker>
#include <QWriteLocker>
#include <QReadWriteLock>

#include <string>

#undef DVAPI
#undef DVVAR
#ifdef TMETAIMAGE_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif

//-------------------------------------------------------------------

class TMetaObject;
class TMetaObjectHandler;
typedef TSmartPointerT<TMetaObject> TMetaObjectP;
typedef TSmartRefT<TMetaObject> TMetaObjectR;
typedef std::vector<TMetaObjectR> TMetaObjectRefList;

//-------------------------------------------------------------------

class DVAPI TMetaObject: public TSmartObject, public TVariantOwner {
public:
  typedef TMetaObjectHandler* (*Fabric)(TMetaObject&);
  typedef std::map<TStringId, Fabric> Registry;

  template<typename T>
  class Registrator {
  public:
    typedef T Type;
    static TMetaObjectHandler* fabric(TMetaObject &obj)
      { return new Type(obj); }
    Registrator(const std::string &typeName)
      { registerType(typeName, fabric); }
    Registrator(const TStringId &typeName)
      { registerType(typeName, fabric); }
  };

private:
  TStringId m_type;
  TMetaObjectHandler *m_handler;
  TVariant m_data;

public:
  explicit TMetaObject(const TStringId &typeName = TStringId());
  explicit TMetaObject(const std::string &typeName);
  ~TMetaObject();

  void setType(const TStringId &name);
  inline void setType(const std::string &name)
    { setType(TStringId(name)); }
  inline void resetType()
    { setType(TStringId()); }
  inline const TStringId& getType() const
    { return m_type; }
  inline const std::string& getTypeName() const
    { return m_type.str(); }
  inline const TVariant& data() const
    { return m_data; }
  inline TVariant& data()
    { return m_data; }

  template<typename T>
  const T* getHandler() const
    { return dynamic_cast<const T*>(m_handler); }
  template<typename T>
  T* getHandler()
    { return dynamic_cast<T*>(m_handler); }

  void onVariantChanged(const TVariant &value) override;

  static Registry& registry();
  static void registerType(const TStringId &name, Fabric fabric);
  static void unregisterType(const TStringId &name);
  inline static void registerType(const std::string &name, Fabric fabric)
    { registerType(TStringId(name), fabric); }
  inline static void unregisterType(const std::string &name)
    { unregisterType(TStringId::find(name)); }
};

//-------------------------------------------------------------------

class DVAPI TMetaObjectHandler {
private:
  TMetaObject &m_object;
  TAtomicVar m_fixingData;

public:
  TMetaObjectHandler(TMetaObject &object):
    m_object(object) { }
  virtual ~TMetaObjectHandler() { }

  inline const TMetaObject& object() const
    { return m_object; }
  inline TMetaObject& object()
    { return m_object; }
  inline const TVariant& data() const
    { return object().data(); }
  inline TVariant& data()
    { return object().data(); }

protected:
  virtual void onDataChanged(const TVariant &value) { }
  virtual void onFixData() { }

public:
  void dataChanged(const TVariant &value)
    { if (m_fixingData == 0) onDataChanged(value); }

  void fixData() {
    ++m_fixingData;
    if (m_fixingData == 1) onFixData();
    --m_fixingData;
  }
};

//-------------------------------------------------------------------

//! An image containing an assistants for painting.

class DVAPI TMetaImage final : public TImage {
public:
  class Reader: public QReadLocker {
  private:
    const TMetaImage &m_image;
  public:
    Reader(const TMetaImage &image):
      QReadLocker(&image.m_rwLock), m_image(image) { }
    const TMetaImage& image() const
      { return m_image; }
    const TMetaObjectRefList& get() const
      { return m_image.m_objects; }
    const TMetaObjectRefList& operator*() const
      { return get(); }
    const TMetaObjectRefList* operator->() const
      { return &get(); }
  };

  class Writer: public QWriteLocker {
  private:
    TMetaImage &m_image;
  public:
    Writer(TMetaImage &image):
      QWriteLocker(&image.m_rwLock), m_image(image) { }
    TMetaImage& image() const
      { return m_image; }
    TMetaObjectRefList& get() const
      { return m_image.m_objects; }
    TMetaObjectRefList& operator*() const
      { return get(); }
    TMetaObjectRefList* operator->() const
      { return &get(); }
  };

private:
  mutable QReadWriteLock m_rwLock;
  TMetaObjectRefList m_objects;

  //! not implemented
  TMetaImage(const TMetaImage &other);
  TMetaImage &operator=(const TMetaImage &) { return *this; }

public:
  TMetaImage();
  ~TMetaImage();

  //! Return the image type
  TImage::Type getType() const override { return TImage::META; }
  //! Return a clone of image
  TImage* cloneImage() const override;
  //! Return the bbox of the image
  TRectD getBBox() const override;
};

#endif
