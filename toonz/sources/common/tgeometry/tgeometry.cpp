

#include "tgeometry.h"
#ifdef LINUX
#include <values.h>
#endif

using namespace std;

const T3DPointD TConsts::nap3d((numeric_limits<double>::max)(),
                               (numeric_limits<double>::max)(),
                               (numeric_limits<double>::max)());

const TThickPoint TConsts::natp((numeric_limits<double>::max)(),
                                (numeric_limits<double>::max)(),
                                (numeric_limits<double>::max)());

const TPointD TConsts::napd((numeric_limits<double>::max)(),
                            (numeric_limits<double>::max)());

const TPointI TConsts::nap((numeric_limits<int>::max)(),
                           (numeric_limits<int>::max)());

const TRectD TConsts::infiniteRectD(-(numeric_limits<double>::max)(),
                                    -(numeric_limits<double>::max)(),
                                    (numeric_limits<double>::max)(),
                                    (numeric_limits<double>::max)());

const TRectI TConsts::infiniteRectI(-(numeric_limits<int>::max)(),
                                    -(numeric_limits<int>::max)(),
                                    (numeric_limits<int>::max)(),
                                    (numeric_limits<int>::max)());

//==================================================================================================

// operazioni fra affini
TAffine &TAffine::operator=(const TAffine &a) {
  a11 = a.a11;
  a12 = a.a12;
  a13 = a.a13;
  a21 = a.a21;
  a22 = a.a22;
  a23 = a.a23;
  return *this;
}
//--------------------------------------------------------------------------------------------------
TAffine TAffine::operator*(const TAffine &b) const {
  return TAffine(a11 * b.a11 + a12 * b.a21, a11 * b.a12 + a12 * b.a22,
                 a11 * b.a13 + a12 * b.a23 + a13,

                 a21 * b.a11 + a22 * b.a21, a21 * b.a12 + a22 * b.a22,
                 a21 * b.a13 + a22 * b.a23 + a23);
}
//--------------------------------------------------------------------------------------------------
TAffine TAffine::operator*=(const TAffine &b) { return *this = *this * b; }
//--------------------------------------------------------------------------------------------------
TAffine TAffine::inv() const {
  if (a12 == 0.0 && a21 == 0.0) {
    assert(a11 != 0.0);
    assert(a22 != 0.0);
    double inv_a11 = 1.0 / a11;
    double inv_a22 = 1.0 / a22;
    return TAffine(inv_a11, 0, -a13 * inv_a11, 0, inv_a22, -a23 * inv_a22);
  } else if (a11 == 0.0 && a22 == 0.0) {
    assert(a12 != 0.0);
    assert(a21 != 0.0);
    double inv_a21 = 1.0 / a21;
    double inv_a12 = 1.0 / a12;
    return TAffine(0, inv_a21, -a23 * inv_a21, inv_a12, 0, -a13 * inv_a12);
  } else {
    double d = 1. / det();
    return TAffine(a22 * d, -a12 * d, (a12 * a23 - a22 * a13) * d, -a21 * d,
                   a11 * d, (a21 * a13 - a11 * a23) * d);
  }
}
//--------------------------------------------------------------------------------------------------
double TAffine::det() const { return a11 * a22 - a12 * a21; }

//--------------------------------------------------------------------------------------------------

// Confronti tra affini
bool TAffine::operator==(const TAffine &a) const {
  return a11 == a.a11 && a12 == a.a12 && a13 == a.a13 && a21 == a.a21 &&
         a22 == a.a22 && a23 == a.a23;
}
//--------------------------------------------------------------------------------------------------
bool TAffine::operator!=(const TAffine &a) const {
  return a11 != a.a11 || a12 != a.a12 || a13 != a.a13 || a21 != a.a21 ||
         a22 != a.a22 || a23 != a.a23;
}
//--------------------------------------------------------------------------------------------------
bool TAffine::isIdentity(double err) const {
  return ((a11 - 1.0) * (a11 - 1.0) + (a22 - 1.0) * (a22 - 1.0) + a12 * a12 +
          a13 * a13 + a21 * a21 + a23 * a23) < err;
}
//--------------------------------------------------------------------------------------------------
bool TAffine::isTranslation(double err) const {
  return ((a11 - 1.0) * (a11 - 1.0) + (a22 - 1.0) * (a22 - 1.0) + a12 * a12 +
          a21 * a21) < err;
}
//--------------------------------------------------------------------------------------------------
bool TAffine::isIsotropic(double err) const {
  return areAlmostEqual(a11, a22, err) && areAlmostEqual(a12, -a21, err);
}

//--------------------------------------------------------------------------------------------------

// applicazione
TPointD TAffine::operator*(const TPointD &p) const {
  return TPointD(p.x * a11 + p.y * a12 + a13, p.x * a21 + p.y * a22 + a23);
}

//--------------------------------------------------------------------------------------------------

TPointD TAffine::transformDirection(const TPointD &p) const {
  return TPointD(p.x * a11 + p.y * a12, p.x * a21 + p.y * a22);
}

//--------------------------------------------------------------------------------------------------

TRectD TAffine::operator*(const TRectD &rect) const {
  if (rect != TConsts::infiniteRectD) {
    TPointD p1 = *this * rect.getP00(), p2 = *this * rect.getP01(),
            p3 = *this * rect.getP10(), p4 = *this * rect.getP11();
    return TRectD(
        std::min({p1.x, p2.x, p3.x, p4.x}), std::min({p1.y, p2.y, p3.y, p4.y}),
        std::max({p1.x, p2.x, p3.x, p4.x}), std::max({p1.y, p2.y, p3.y, p4.y}));
  } else
    return TConsts::infiniteRectD;
}

//--------------------------------------------------------------------------------------------------

TAffine TAffine::place(double u, double v, double x, double y) const {
  return TAffine(a11, a12, x - (a11 * u + a12 * v), a21, a22,
                 y - (a21 * u + a22 * v));
}

//--------------------------------------------------------------------------------------------------

TAffine TAffine::place(const TPointD &pIn, const TPointD &pOut) const {
  return TAffine(a11, a12, pOut.x - (a11 * pIn.x + a12 * pIn.y), a21, a22,
                 pOut.y - (a21 * pIn.x + a22 * pIn.y));
}

//==================================================================================================

TRotation::TRotation(double degrees) {
  double rad, sn, cs;
  int idegrees = (int)degrees;
  if ((double)idegrees == degrees && idegrees % 90 == 0) {
    switch ((idegrees / 90) & 3) {
    case 0:
      sn = 0;
      cs = 1;
      break;
    case 1:
      sn = 1;
      cs = 0;
      break;
    case 2:
      sn = 0;
      cs = -1;
      break;
    case 3:
      sn = -1;
      cs = 0;
      break;
    default:
      sn = 0;
      cs = 0;
      break;
    }
  } else {
    rad                         = degrees * M_PI_180;
    sn                          = sin(rad);
    cs                          = cos(rad);
    if (sn == 1 || sn == -1) cs = 0;
    if (cs == 1 || cs == -1) sn = 0;
  }
  a11 = cs;
  a12 = -sn;
  a21 = -a12;
  a22 = a11;
}
//--------------------------------------------------------------------------------------------------
TRotation::TRotation(const TPointD &center, double degrees) {
  TAffine a = TTranslation(center) * TRotation(degrees) * TTranslation(-center);
  a11       = a.a11;
  a12       = a.a12;
  a13       = a.a13;
  a21       = a.a21;
  a22       = a.a22;
  a23       = a.a23;
}

//==================================================================================================

TScale::TScale(const TPointD &center, double sx, double sy) {
  TAffine a = TTranslation(center) * TScale(sx, sy) * TTranslation(-center);
  a11       = a.a11;
  a12       = a.a12;
  a13       = a.a13;
  a21       = a.a21;
  a22       = a.a22;
  a23       = a.a23;
}
//--------------------------------------------------------------------------------------------------
TScale::TScale(const TPointD &center, double s) {
  TAffine a = TTranslation(center) * TScale(s) * TTranslation(-center);
  a11       = a.a11;
  a12       = a.a12;
  a13       = a.a13;
  a21       = a.a21;
  a22       = a.a22;
  a23       = a.a23;
}

//==================================================================================================

TPoint4D TAffine4::operator*(const TPoint4D &b) const {
  return TPoint4D(
    b.x*a11 + b.y*a21 + b.z*a31 + b.w*a41,
    b.x*a12 + b.y*a22 + b.z*a32 + b.w*a42,
    b.x*a13 + b.y*a23 + b.z*a33 + b.w*a43,
    b.x*a14 + b.y*a24 + b.z*a34 + b.w*a44 );
}

TAffine4 TAffine4::operator*(const TAffine4 &b) const {
  return TAffine4(
    *this * b.rowX(),
    *this * b.rowY(),
    *this * b.rowZ(),
    *this * b.rowW() );
}

TAffine4 TAffine4::operator*=(const TAffine4 &b)
  { return *this = *this * b; }

TAffine4 TAffine4::inv() const {
  TAffine4 r;
  r.a11 = a22*(a33*a44 - a34*a43) + a23*(a34*a42 - a32*a44) + a24*(a32*a43 - a33*a42);
  r.a12 = a21*(a34*a43 - a33*a44) + a23*(a31*a44 - a34*a41) + a24*(a33*a41 - a31*a43);
  r.a13 = a21*(a32*a44 - a34*a42) + a22*(a34*a41 - a31*a44) + a24*(a31*a42 - a32*a41);
  r.a14 = a21*(a33*a42 - a32*a43) + a22*(a31*a43 - a33*a41) + a23*(a32*a41 - a31*a42);

  double det = a11*r.a11 + a12*r.a21 + a13*r.a31 + a14*r.a41;
  if (fabs(det) > TConsts::epsilon) det = 1.0/det;
  r.a11 *= det;
  r.a12 *= det;
  r.a13 *= det;
  r.a14 *= det;

  r.a21 = det*( a12*(a34*a43 - a33*a44) + a13*(a32*a44 - a34*a42) + a14*(a33*a42 - a32*a43) );
  r.a22 = det*( a11*(a33*a44 - a34*a43) + a13*(a34*a41 - a31*a44) + a14*(a31*a43 - a33*a41) );
  r.a23 = det*( a11*(a34*a42 - a32*a44) + a12*(a31*a44 - a34*a41) + a14*(a32*a41 - a31*a42) );
  r.a24 = det*( a11*(a32*a43 - a33*a42) + a12*(a33*a41 - a31*a43) + a13*(a31*a42 - a32*a41) );
  r.a31 = det*( a12*(a23*a44 - a24*a43) + a13*(a24*a42 - a22*a44) + a14*(a22*a43 - a23*a42) );
  r.a32 = det*( a11*(a24*a43 - a23*a44) + a13*(a21*a44 - a24*a41) + a14*(a23*a41 - a21*a43) );
  r.a33 = det*( a11*(a22*a44 - a24*a42) + a12*(a24*a41 - a21*a44) + a14*(a21*a42 - a22*a41) );
  r.a34 = det*( a11*(a23*a42 - a22*a43) + a12*(a21*a43 - a23*a41) + a13*(a22*a41 - a21*a42) );
  r.a41 = det*( a12*(a24*a33 - a23*a34) + a13*(a22*a34 - a24*a32) + a14*(a23*a32 - a22*a33) );
  r.a42 = det*( a11*(a23*a34 - a24*a33) + a13*(a24*a31 - a21*a34) + a14*(a21*a33 - a23*a31) );
  r.a43 = det*( a11*(a24*a32 - a22*a34) + a12*(a21*a34 - a24*a31) + a14*(a22*a31 - a21*a32) );
  r.a44 = det*( a11*(a22*a33 - a23*a32) + a12*(a23*a31 - a21*a33) + a13*(a21*a32 - a22*a31) );

  return r;
}

TAffine TAffine4::get2d(double z) const {
  return TAffine(
    a11, a21, z*a31 + a41,
    a12, a22, z*a32 + a42 );
}

TAffine4 TAffine4::translation(double x, double y, double z) {
  TAffine4 r;
  r.rowW().x = x;
  r.rowW().y = y;
  r.rowW().z = z;
  return r;
}

TAffine4 TAffine4::scale(double x, double y, double z) {
  TAffine4 r;
  r.a11 = x;
  r.a22 = y;
  r.a33 = z;
  return r;
}

TAffine4 TAffine4::rotation(double x, double y, double z, double angle) {
  TAffine4 r;
  double k = x*x + y*y + z*z;
  if (k > TConsts::epsilon*TConsts::epsilon) {
	k = 1.0 / sqrt(k);
	double s = sin(angle);
    double c = cos(angle);
    double ic = 1.0 - c;
    x *= k;
    y *= k;
    z *= k;

    r.a11 = ic*x*x + c;
    r.a12 = ic*x*y + s*z;
    r.a13 = ic*z*x - s*y;

    r.a21 = ic*x*y - s*z;
    r.a22 = ic*y*y + c;
    r.a23 = ic*y*z + s*x;

    r.a31 = ic*z*x + s*y;
    r.a32 = ic*y*z - s*x;
    r.a33 = ic*z*z + c;
  }
  return r;
}

TAffine4 TAffine4::rotationX(double angle) {
  TAffine4 r;
  double s = sin(angle);
  double c = cos(angle);
  r.a22 =  c;
  r.a23 =  s;
  r.a32 = -s;
  r.a33 =  c;
  return r;
}

TAffine4 TAffine4::rotationY(double angle) {
  TAffine4 r;
  double s = sin(angle);
  double c = cos(angle);
  r.a11 =  c;
  r.a13 = -s;
  r.a31 =  s;
  r.a33 =  c;
  return r;
}

TAffine4 TAffine4::rotationZ(double angle) {
  TAffine4 r;
  double s = sin(angle);
  double c = cos(angle);
  r.a11 =  c;
  r.a12 =  s;
  r.a21 = -s;
  r.a22 =  c;
  return r;
}
