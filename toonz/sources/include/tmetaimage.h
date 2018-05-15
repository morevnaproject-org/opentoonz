#pragma once

#ifndef TMETAIMAGE_INCLUDED
#define TMETAIMAGE_INCLUDED

#include "timage.h"
#include "tthreadmessage.h"
#include "tsmartpointer.h"
#include "tvariant.h"
#include "tconstwrapper.h"

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
typedef TSmartPointerT<TMetaObject> TMetaObjectP;  //!< smart pointer to TMetaObject
typedef TMetaObjectP::Holder        TMetaObjectH;  //!< smart holder of TMetaObject
typedef TMetaObjectP::Const         TMetaObjectPC; //!< smart pointer to constant TMetaObject
typedef std::vector<TMetaObjectP> TMetaObjectList;
typedef TConstArrayWrapperT<TMetaObjectPC, TMetaObjectP> TMetaObjectListCW; // TMetaObjectListConstWrapper

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

  TMetaObject(const TMetaObject &other);

public:
  explicit TMetaObject(const TStringId &typeName = TStringId(), const TVariant &data = TVariant());
  explicit TMetaObject(const std::string &typeName, const TVariant &data = TVariant());
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

  void setDefaults();

  template<typename T>
  const T* getHandler() const
    { return dynamic_cast<const T*>(m_handler); }
  template<typename T>
  T* getHandler()
    { return dynamic_cast<T*>(m_handler); }

  TMetaObjectHandler* handler() { return m_handler; }
  const TMetaObjectHandler* handler() const { return m_handler; }

  void onVariantChanged(const TVariant &value) override;

  virtual TMetaObject* clone() const;

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
protected:
  class LockEvents {
  public:
    TMetaObjectHandler &owner;
    explicit LockEvents(TMetaObjectHandler &owner):
      owner(owner) { ++owner.m_locks; }
    ~LockEvents() { --owner.m_locks; }
  };

private:
  TMetaObject &m_object;
  TAtomicVar m_locks;

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
  virtual void onSetDefaults() { }
  virtual void onDataChanged(const TVariant &value) { }
  virtual void onFixData() { }

public:
  void setDefaults()
    { onSetDefaults(); }
  void dataChanged(const TVariant &value)
    { if (m_locks == 0) onDataChanged(value); }
  void fixData()
    { LockEvents lock(*this); onFixData(); }
};

//-------------------------------------------------------------------

//! An image containing an assistants for painting.

class DVAPI TMetaImage final : public TImage {
public:
  class Reader: public QReadLocker {
  private:
    const TMetaImage &m_image;
    const TMetaObjectListCW m_objects;
  public:
    Reader(const TMetaImage &image):
      QReadLocker(&image.m_rwLock),
      m_image(image),
      m_objects(image.m_objects) { }
    const TMetaImage& image() const
      { return m_image; }
    const TMetaObjectListCW& get() const
      { return m_objects; }
    const TMetaObjectListCW& operator*() const
      { return get(); }
    const TMetaObjectListCW* operator->() const
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
    TMetaObjectList& get() const
      { return m_image.m_objects; }
    TMetaObjectList& operator*() const
      { return get(); }
    TMetaObjectList* operator->() const
      { return &get(); }
  };

private:
  mutable QReadWriteLock m_rwLock;
  TMetaObjectList m_objects;

  TMetaImage(const TMetaImage &other);

  //! not implemented
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
