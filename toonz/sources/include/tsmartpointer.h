#pragma once

#ifndef TSMARTPOINTER_INCLUDED
#define TSMARTPOINTER_INCLUDED

#include "tutil.h"
#include "tatomicvar.h"

#undef DVAPI
#undef DVVAR
#ifdef TNZCORE_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif

//=========================================================

#ifndef NDEBUG
#define INSTANCE_COUNT_ENABLED
#endif

//=========================================================

class DVAPI TSmartObject {
  TAtomicVar m_refCount;

#ifdef INSTANCE_COUNT_ENABLED
  const TINT32 m_classCodeRef;
  static const TINT32 m_unknownClassCode;
#endif

public:
  typedef short ClassCode;

  TSmartObject(ClassCode
#ifdef INSTANCE_COUNT_ENABLED
                   classCode
#endif
               )
      : m_refCount()
#ifdef INSTANCE_COUNT_ENABLED
      , m_classCodeRef(classCode)
#endif
  {
#ifdef INSTANCE_COUNT_ENABLED
    incrementInstanceCount();
#endif
  }

  TSmartObject()
      : m_refCount()
#ifdef INSTANCE_COUNT_ENABLED
      , m_classCodeRef(m_unknownClassCode)
#endif
  {
#ifdef INSTANCE_COUNT_ENABLED
    incrementInstanceCount();
#endif
  }

  virtual ~TSmartObject() {
    assert(m_refCount == 0);
#ifdef INSTANCE_COUNT_ENABLED
    decrementInstanceCount();
#endif
  }

  inline void addRef() { ++m_refCount; }
  inline void release() {
    if ((--m_refCount) <= 0) delete this;
  };
  inline TINT32 getRefCount() const { return m_refCount; }

  static TINT32 getInstanceCount(ClassCode code);

private:
  void incrementInstanceCount();
  void decrementInstanceCount();

private:
  // not implemented
  TSmartObject(const TSmartObject &);
  TSmartObject &operator=(const TSmartObject &);
};

#define DECLARE_CLASS_CODE                                                     \
  \
private:                                                                       \
  static const TSmartObject::ClassCode m_classCode;                            \
  \
public:                                                                        \
  inline static TINT32 getInstanceCount() {                                    \
    return TSmartObject::getInstanceCount(m_classCode);                        \
  }

#define DEFINE_CLASS_CODE(T, ID)                                               \
  const TSmartObject::ClassCode T::m_classCode = ID;

//=========================================================

template <class T>
class TSmartPointerBaseT {
public:
  typedef T Type;
protected:
  T *m_pointer;
  T& reference() const
    { assert(m_pointer); return *m_pointer; }
public:
  TSmartPointerBaseT() : m_pointer(0) {}
  TSmartPointerBaseT(const TSmartPointerBaseT &src) : m_pointer(src.m_pointer)
    { if (m_pointer) m_pointer->addRef(); }
  explicit TSmartPointerBaseT(T *pointer) : m_pointer(pointer)
    { if (m_pointer) m_pointer->addRef(); }
  virtual ~TSmartPointerBaseT()
    { if (m_pointer) { m_pointer->release(); m_pointer = 0; } }
  TSmartPointerBaseT& operator=(const TSmartPointerBaseT &src)
    { set(src); return *this; }

  void set(T *pointer) {
    if (m_pointer != pointer) {
      // call 'addRef' before 'release'
      if (pointer) pointer->addRef();
      if (m_pointer) m_pointer->release();
      m_pointer = pointer;
    }
  }
  void set(const TSmartPointerBaseT &src)
    { set(src.m_pointer); }
  void reset()
    { set(0); }
  bool operator!() const
    { return m_pointer == 0; }
  operator bool() const
    { return m_pointer != 0; }

  bool operator==(const TSmartPointerBaseT &p) const
    { return m_pointer == p.m_pointer; }
  bool operator!=(const TSmartPointerBaseT &p) const
    { return m_pointer != p.m_pointer; }
  bool operator<(const TSmartPointerBaseT &p) const
    { return m_pointer < p.m_pointer; }
  bool operator>(const TSmartPointerBaseT &p) const
    { return m_pointer > p.m_pointer; }

  bool operator==(const T *p) const
    { return m_pointer == p; }
  bool operator!=(const T *p) const
    { return m_pointer != p; }
  bool operator<(const T *p) const
    { return m_pointer < p; }
  bool operator>(const T *p) const
    { return m_pointer > p; }
};

//=========================================================

template <class T>
class TSmartPointerT: public TSmartPointerBaseT<T> {
public:
  typedef TSmartPointerBaseT<T> Base;
  TSmartPointerT() {}
  TSmartPointerT(const TSmartPointerT &src): Base(src) {}
  TSmartPointerT(T *pointer): Base(pointer) {}
  TSmartPointerT& operator=(const TSmartPointerT &src) { Base::set(src); return *this; }
  T* operator->() const { return &Base::reference(); }
  T& operator*() const { return Base::reference(); }
  T* getPointer() const { return Base::m_pointer; }
};

//=========================================================

//! smart reference returns constant pointer when reference is constant
//! and returns non-constant pointer when reference is non-constant
template <class T>
class TSmartRefT: public TSmartPointerBaseT<T> {
public:
  typedef TSmartPointerBaseT<T> Base;
  TSmartRefT() {}
  TSmartRefT(const TSmartRefT &src): Base(src) {}
  explicit TSmartRefT(T *pointer): Base(pointer) {}
  TSmartRefT& operator=(const TSmartRefT &src) { Base::set(src); return *this; }
  const T* operator->() const { return &Base::reference(); }
  const T& operator*() const { return Base::reference(); }
  const T* getPointer() const { return Base::m_pointer; }
  T* operator->() { return &Base::reference(); }
  T& operator*() { return Base::reference(); }
  T* getPointer() { return Base::m_pointer; }
};

//=========================================================

template <class DERIVED, class BASE>
class TDerivedSmartPointerT : public TSmartPointerT<DERIVED> {
public:
  typedef TDerivedSmartPointerT<DERIVED, BASE> DerivedSmartPointer;

  TDerivedSmartPointerT(){};
  TDerivedSmartPointerT(DERIVED *pointer) : TSmartPointerT<DERIVED>(pointer) {}

  TDerivedSmartPointerT(const TSmartPointerT<BASE> &p) {
    TSmartPointerT<DERIVED>::m_pointer =
        dynamic_cast<DERIVED *>(p.getPointer());
    if (TSmartPointerT<DERIVED>::m_pointer)
      TSmartPointerT<DERIVED>::m_pointer->addRef();
  }
};

//=========================================================

typedef TSmartPointerT<TSmartObject> TSmartObjectP;

#endif
