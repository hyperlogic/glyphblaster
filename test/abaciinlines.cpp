#ifdef ABACI_H

#include <stdio.h>
#include <stdlib.h>

// Convert from degrees to radians
template <typename Scalar>
inline Scalar DegToRad(Scalar deg)
{
    return deg * (PI / 180.0);
}

// Convert from radians to degrees.
template <typename Scalar>
inline Scalar RadToDeg(Scalar rad)
{
    return rad * (180.0 / PI);
}

// Clamps value between min & max.
template <typename Scalar>
inline Scalar Clamp(Scalar value, Scalar min, Scalar max)
{
    Scalar result = value;
    if (value > max)
        result = max;
    else if (value < min)
        result = min;
    return result;
}

// Limits angle between -PI & PI using modulus arithmetic
template <typename Scalar>
inline Scalar LimitPi(Scalar theta)
{
    return Mod2Pi(theta + PI) - PI;
}

// Limits angle between zero & 2 PI using modulus arithmetic
template <typename Scalar>
inline Scalar Mod2Pi(Scalar theta)
{
    return theta - 2*PI * floor(theta/(2*PI));
}

// Fuzzy comparison between two Scalar values.
template <typename Scalar>
inline bool FuzzyEqual(Scalar rhs, Scalar lhs, Scalar epsilon)
{
    return fabs(rhs - lhs) <= epsilon;
}

// Linear interpolate between two Scalar values.
template <typename Scalar>
inline Scalar Lerp(Scalar a, Scalar b, Scalar t)
{
    return a + (b - a) * t;
}

inline int RandomInt(int min, int max)
{
    return min + (rand() % (max - min + 1));
}

template <typename Scalar>
Scalar RandomScalar(Scalar min, Scalar max)
{
    Scalar t = (Scalar)((rand() % 16127) / 16126.0);
    return Lerp(min, max, t);
}

////////////////////////////////////////////////////////////////////////////////
// Vector2

// returns a random vector on the unit circle.
template <typename Scalar>
Vector2<Scalar> Vector2<Scalar>::RandomUnitVector()
{
    Scalar theta = RandomScalar(0.0f, (Scalar)(2 * PI));
    return Vector2<Scalar>(cos(theta), sin(theta));
}

// Construct from two Scalars
template <typename Scalar>
inline Vector2<Scalar>::Vector2(Scalar xIn, Scalar yIn) : x(xIn), y(yIn) {}

// Construct from Complex
template <typename Scalar>
inline Vector2<Scalar>::Vector2(const Complex<Scalar>& complexIn) :
    x(complexIn.r), y(complexIn.i) {}

// Set from two Scalars
template <typename Scalar>
inline void Vector2<Scalar>::Set(Scalar xIn, Scalar yIn)
{
    x = xIn;
    y = yIn;
}

// Set from Complex
template <typename Scalar>
void Vector2<Scalar>::Set(const Complex<Scalar>& complexIn)
{
    x = complexIn.r;
    y = complexIn.i;
}

// Sets all elements to zero.
template <typename Scalar>
inline void Vector2<Scalar>::SetZero()
{
    x = y = 0;
}

// const array accessor
template <typename Scalar>
Scalar Vector2<Scalar>::operator[](int i) const
{
    return *(&x + i);
}

// array accessor
template <typename Scalar>
Scalar& Vector2<Scalar>::operator[](int i)
{
    return *(&x + i);
}

// Returns a vector with same direction but unit length.
template <typename Scalar>
inline Vector2<Scalar> Vector2<Scalar>::Unit() const
{
    return *this / Len();
}

// Returns vector length.
template <typename Scalar>
inline Scalar Vector2<Scalar>::Len() const
{
    return sqrt(Dot(*this, *this));
}

// Returns length squared.
template <typename Scalar>
inline Scalar Vector2<Scalar>::LenSq() const
{
    return Dot(*this, *this);
}

// Returns a vector with the same direction, but with a Len() <= len.
template <typename Scalar>
inline Vector2<Scalar> Vector2<Scalar>::MinLen(Scalar len) const
{
    Scalar l = Len();
    if (l > len)
        return (*this / l) * len;
    else
        return *this;
}

// Dot product of two vectors.
template <typename Scalar>
inline Scalar Dot(const Vector2<Scalar>& a, const Vector2<Scalar>& b)
{
    return a.x * b.x + a.y * b.y;
}

// Linear interpolation between two vectors
template <typename Scalar>
inline Vector2<Scalar> Lerp(const Vector2<Scalar>& a, const Vector2<Scalar>& b, Scalar t)
{
    return a + (b - a) * t;
}

// Unary minus
template <typename Scalar>
inline Vector2<Scalar> operator-(const Vector2<Scalar>& a)
{
    return Vector2<Scalar>(-a.x, -a.y);
}

// Vector subtraction.
template <typename Scalar>
inline Vector2<Scalar> operator-(const Vector2<Scalar>& a, const Vector2<Scalar>& b)
{
    return Vector2<Scalar>(a.x - b.x, a.y - b.y);
}

// Vector addition.
template <typename Scalar>
inline Vector2<Scalar> operator+(const Vector2<Scalar>& a, const Vector2<Scalar>& b)
{
    return Vector2<Scalar>(a.x + b.x, a.y + b.y);
}

// Vector decrement.
template <typename Scalar>
Vector2<Scalar>& operator-=(Vector2<Scalar>& a, const Vector2<Scalar>& b)
{
    a.x -= b.x;
    a.y -= b.y;
    return a;
}

// Vector increment.
template <typename Scalar>
Vector2<Scalar>& operator+=(Vector2<Scalar>& a, const Vector2<Scalar>& b)
{
    a.x += b.x;
    a.y += b.y;
    return a;
}

// Multplies all elements of a vector by a scalar.
template <typename Scalar>
inline Vector2<Scalar> operator*(const Vector2<Scalar>& v, Scalar scalar)
{
    return Vector2<Scalar>(scalar * v.x, scalar * v.y);
}

// Multplies all elements of a vector by a scalar.
template <typename Scalar>
inline Vector2<Scalar> operator*(Scalar scalar, const Vector2<Scalar>& v)
{
    return Vector2<Scalar>(scalar * v.x, scalar * v.y);
}

// Vector multiplication.
template <typename Scalar>
inline Vector2<Scalar> operator*(const Vector2<Scalar>& a, const Vector2<Scalar>& b)
{
    return Vector2<Scalar>(a.x * b.x, a.y * b.y);
}

// Divides all elements of a vector by a scalar.
template <typename Scalar>
inline Vector2<Scalar> operator/(const Vector2<Scalar>& v, Scalar denominator)
{
    return Vector2<Scalar>(v.x / denominator, v.y / denominator);
}

// Multiplies a scalar to the reciprical of all elements in a vector.
template <typename Scalar>
inline Vector2<Scalar> operator/(Scalar numerator, const Vector2<Scalar>& v)
{
    return Vector2<Scalar>(numerator / v.x, numerator / v.y);
}

// Vector division.
template <typename Scalar>
inline Vector2<Scalar> operator/(const Vector2<Scalar>& a, const Vector2<Scalar>& b)
{
    return Vector2<Scalar>(a.x / b.x, a.y / b.y);
}

// Fuzzy comparison between two Vector2 values.
template <typename Scalar>
bool FuzzyEqual(const Vector2<Scalar>& rhs, const Vector2<Scalar>& lhs, Scalar epsilon)
{
    return (FuzzyEqual(rhs.x, lhs.x, epsilon) &&
            FuzzyEqual(rhs.y, lhs.y, epsilon));
}

////////////////////////////////////////////////////////////////////////////////
// Vector3

// returns a random vector on the unit sphere.
template <typename Scalar>
Vector3<Scalar> Vector3<Scalar>::RandomUnitVector()
{
    Scalar theta = RandomScalar(0.0f, (Scalar)(2 * PI));
    Scalar phi = RandomScalar(0.0f, (Scalar)(2 * PI));
    return Vector3<Scalar>(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi));
}

// Construct from three Scalars
template <typename Scalar>
inline Vector3<Scalar>::Vector3(Scalar xIn, Scalar yIn, Scalar zIn) : x(xIn), y(yIn), z(zIn) {}

// Set from three Scalars
template <typename Scalar>
inline void Vector3<Scalar>::Set(Scalar xIn, Scalar yIn, Scalar zIn)
{
    x = xIn;
    y = yIn;
    z = zIn;
}

// Set all elements to zero
template <typename Scalar>
inline void Vector3<Scalar>::SetZero()
{
    x = y = z = 0;
}

// const array accessor
template <typename Scalar>
Scalar Vector3<Scalar>::operator[](int i) const
{
    return *(&x + i);
}

// array accessor
template <typename Scalar>
Scalar& Vector3<Scalar>::operator[](int i)
{
    return *(&x + i);
}

// Returns a vector with same direction but unit length.
template <typename Scalar>
inline Vector3<Scalar> Vector3<Scalar>::Unit() const
{
    return (*this) / Len();
}

// Returns vector length.
template <typename Scalar>
inline Scalar Vector3<Scalar>::Len() const
{
    return sqrt(Dot(*this, *this));
}

// Returns vector length squared.
template <typename Scalar>
inline Scalar Vector3<Scalar>::LenSq() const
{
    return Dot(*this, *this);
}

// Returns a vector with the same direction, but with a Len() <= len.
template <typename Scalar>
inline Vector3<Scalar> Vector3<Scalar>::MinLen(Scalar len) const
{
    Scalar l = Len();
    if (l > len)
        return (*this / l) * len;
    else
        return *this;
}

// Generate vectors j and k which are orthognal to i and each other.
template <typename Scalar>
inline void Vector3<Scalar>::Basis(Vector3<Scalar>& iOut, Vector3<Scalar>& jOut, Vector3<Scalar>& kOut)
{
    iOut = Unit();
    Scalar xabs = fabs(iOut.x);
    Scalar yabs = fabs(iOut.y);
    Scalar zabs = fabs(iOut.z);

    Vector3<Scalar> j;
    if (xabs <= yabs && xabs <= zabs)
        j.Set(0, -iOut.z, iOut.y);
    else if (yabs <= xabs && yabs <= zabs)
        j.Set(-iOut.z, 0, iOut.x);
    else
        j.Set(-iOut.y, iOut.x, 0);

    jOut = j.Unit();
    kOut = Cross(iOut, jOut);
}

// Dot product of two vectors.
template <typename Scalar>
inline Scalar Dot(const Vector3<Scalar>& a, const Vector3<Scalar>& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Cross product of two vectors.
template <typename Scalar>
inline Vector3<Scalar> Cross(const Vector3<Scalar>& a, const Vector3<Scalar>& b)
{
    return Vector3<Scalar>(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

// Linear interpolation between two vectors
template <typename Scalar>
inline Vector3<Scalar> Lerp(const Vector3<Scalar>& a, const Vector3<Scalar>& b, Scalar t)
{
    return a + (b - a) * t;
}

// Unary minus
template <typename Scalar>
inline Vector3<Scalar> operator-(const Vector3<Scalar>& a)
{
    return Vector3<Scalar>(-a.x, -a.y, -a.z);
}

// Vector subtraction.
template <typename Scalar>
inline Vector3<Scalar> operator-(const Vector3<Scalar>& a, const Vector3<Scalar>& b)
{
    return Vector3<Scalar>(a.x - b.x, a.y - b.y, a.z - b.z);
}

// Vector addition.
template <typename Scalar>
inline Vector3<Scalar> operator+(const Vector3<Scalar>& a, const Vector3<Scalar>& b)
{
    return Vector3<Scalar>(a.x + b.x, a.y + b.y, a.z + b.z);
}

// Vector decrement.
template <typename Scalar>
Vector3<Scalar>& operator-=(Vector3<Scalar>& a, const Vector3<Scalar>& b)
{
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return a;
}

// Vector increment.
template <typename Scalar>
Vector3<Scalar>& operator+=(Vector3<Scalar>& a, const Vector3<Scalar>& b)
{
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
}

// Multplies all elements of a vector by a scalar.
template <typename Scalar>
inline Vector3<Scalar> operator*(const Vector3<Scalar>& v, Scalar factor)
{
    return Vector3<Scalar>(v.x * factor, v.y * factor, v.z * factor);
}

// Multplies all elements of a vector by a scalar.
template <typename Scalar>
inline Vector3<Scalar> operator*(Scalar factor, const Vector3<Scalar>& v)
{
    return Vector3<Scalar>(v.x * factor, v.y * factor, v.z * factor);
}

// Vector multiplication
template <typename Scalar>
inline Vector3<Scalar> operator*(const Vector3<Scalar>& a, const Vector3<Scalar>& b)
{
    return Vector3<Scalar>(a.x * b.x, a.y * b.y, a.z * b.z);
}

// Divides all elements of a vector by a scalar.
template <typename Scalar>
inline Vector3<Scalar> operator/(const Vector3<Scalar>& v, Scalar denominator)
{
    return Vector3<Scalar>(v.x / denominator, v.y / denominator, v.z / denominator);
}

// Multiplies a scalar to the reciprical of all elements in a vector.
template <typename Scalar>
inline Vector3<Scalar> operator/(Scalar numerator, const Vector3<Scalar>& v)
{
    return Vector3<Scalar>(numerator / v.x, numerator / v.y, numerator / v.z);
}

// Vector division.
template <typename Scalar>
inline Vector3<Scalar> operator/(const Vector3<Scalar>& a, const Vector3<Scalar>& b)
{
    return Vector3<Scalar>(a.x / b.x, a.y / b.y, a.z / b.z);
}

// Fuzzy comparison between two Vector3 values.
template <typename Scalar>
bool FuzzyEqual(const Vector3<Scalar>& rhs, const Vector3<Scalar>& lhs, Scalar epsilon)
{
    return (FuzzyEqual(rhs.x, lhs.x, epsilon) &&
            FuzzyEqual(rhs.y, lhs.y, epsilon) &&
            FuzzyEqual(rhs.z, lhs.z, epsilon));
}

////////////////////////////////////////////////////////////////////////////////
// Vector4

// Construct from four Scalars.
template <typename Scalar>
inline Vector4<Scalar>::Vector4(Scalar xIn, Scalar yIn, Scalar zIn, Scalar wIn) : x(xIn), y(yIn), z(zIn), w(wIn) {}

// Set from four Scalars.
template <typename Scalar>
inline void Vector4<Scalar>::Set(Scalar xIn, Scalar yIn, Scalar zIn, Scalar wIn)
{
    x = xIn;
    y = yIn;
    z = zIn;
    w = wIn;
}

// Set all elements to zero.
template <typename Scalar>
inline void Vector4<Scalar>::SetZero()
{
    x = y = z = w = 0;
}

// Returns a vector with same direction but unit length.
template <typename Scalar>
inline Vector4<Scalar> Vector4<Scalar>::Unit() const
{
    return (*this) / Len();
}

// Returns vector length.
template <typename Scalar>
inline Scalar Vector4<Scalar>::Len() const
{
    return sqrt(Dot(*this, *this));
}

// Returns length squared.
template <typename Scalar>
inline Scalar Vector4<Scalar>::LenSq() const
{
    return Dot(*this, *this);
}

// Returns a vector with the same direction, but with a Len() <= len.
template <typename Scalar>
inline Vector4<Scalar> Vector4<Scalar>::MinLen(Scalar len) const
{
    Scalar l = Len();
    if (l > len)
        return (*this / l) * len;
    else
        return *this;
}

// const array accessor
template <typename Scalar>
inline Scalar Vector4<Scalar>::operator[](int i) const
{
    return *(&x + i);
}

// array accessor
template <typename Scalar>
inline Scalar& Vector4<Scalar>::operator[](int i)
{
    return *(&x + i);
}

// Dot product of two vectors.
template <typename Scalar>
inline Scalar Dot(const Vector4<Scalar>& a, const Vector4<Scalar>& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

// Linear interpolation between two vectors
template <typename Scalar>
inline Vector4<Scalar> Lerp(const Vector4<Scalar>& a, const Vector4<Scalar>& b, Scalar t)
{
    return a + (b - a) * t;
}

// Unary minus.
template <typename Scalar>
inline Vector4<Scalar> operator-(const Vector4<Scalar>& v)
{
    return Vector4<Scalar>(-v.x, -v.y, -v.z, -v.w);
}

// Vector subtraction.
template <typename Scalar>
inline Vector4<Scalar> operator-(const Vector4<Scalar>& a, const Vector4<Scalar>& b)
{
    return Vector4<Scalar>(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

// Vector addition.
template <typename Scalar>
inline Vector4<Scalar> operator+(const Vector4<Scalar>& a, const Vector4<Scalar>& b)
{
    return Vector4<Scalar>(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

// Vector decrement.
template <typename Scalar>
Vector4<Scalar>& operator-=(Vector4<Scalar>& a, const Vector4<Scalar>& b)
{
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    a.w -= b.w;
    return a;
}

// Vector increment.
template <typename Scalar>
Vector4<Scalar>& operator+=(Vector4<Scalar>& a, const Vector4<Scalar>& b)
{
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    a.w += b.w;
    return a;
}

// Multplies all elements of a vector by a scalar.
template <typename Scalar>
inline Vector4<Scalar> operator*(const Vector4<Scalar>& v, Scalar factor)
{
    return Vector4<Scalar>(factor * v.x, factor * v.y, factor * v.z, factor * v.w);
}

// Multplies all elements of a vector by a scalar.
template <typename Scalar>
inline Vector4<Scalar> operator*(Scalar factor, const Vector4<Scalar>& v)
{
    return Vector4<Scalar>(factor * v.x, factor * v.y, factor * v.z, factor * v.w);
}

// Vector multiplication.
template <typename Scalar>
inline Vector4<Scalar> operator*(const Vector4<Scalar>& a, const Vector4<Scalar>& b)
{
    return Vector4<Scalar>(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

// Divides all elements of a vector by a scalar.
template <typename Scalar>
inline Vector4<Scalar> operator/(const Vector4<Scalar>& v, Scalar denominator)
{
    return Vector4<Scalar>(v.x / denominator, v.y / denominator, v.z / denominator, v.w / denominator);
}

// Multiplies a scalar to the reciprical of all elements in a vector.
template <typename Scalar>
inline Vector4<Scalar> operator/(Scalar numerator, const Vector4<Scalar>& v)
{
    return Vector4<Scalar>(numerator / v.x, numerator / v.y, numerator / v.z, numerator / v.w);
}

// Vector division.
template <typename Scalar>
inline Vector4<Scalar> operator/(const Vector4<Scalar>& a, const Vector4<Scalar>& b)
{
    return Vector4<Scalar>(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
}

// Fuzzy comparison between two Vector4 values.
template <typename Scalar>
bool FuzzyEqual(const Vector4<Scalar>& rhs, const Vector4<Scalar>& lhs, Scalar epsilon)
{
    return (FuzzyEqual(rhs.x, lhs.x, epsilon) &&
            FuzzyEqual(rhs.y, lhs.y, epsilon) &&
            FuzzyEqual(rhs.z, lhs.z, epsilon) &&
            FuzzyEqual(rhs.w, lhs.w, epsilon));
}

////////////////////////////////////////////////////////////////////////////////
// Quat

// Create from axis and angle
template <typename Scalar>
inline Quat<Scalar> Quat<Scalar>::AxisAngle(const Vector3<Scalar>& axis, Scalar angle)
{
    Vector3<Scalar> n = axis.Unit() * static_cast<Scalar>(sin(angle/2.0f));
    Scalar w = cos(angle/2.0f);
    return Quat<Scalar>(n.x, n.y, n.z, w);
}

// Create identity Quat
template <typename Scalar>
inline Quat<Scalar> Quat<Scalar>::Identity()
{
    return Quat<Scalar>(0, 0, 0, 1);
}

// Construct from four Scalars.
template <typename Scalar>
inline Quat<Scalar>::Quat(Scalar iIn, Scalar jIn, Scalar kIn, Scalar rIn) : i(iIn), j(jIn), k(kIn), r(rIn) {}

// Set from four Scalars.
template <typename Scalar>
inline void Quat<Scalar>::Set(Scalar iIn, Scalar jIn, Scalar kIn, Scalar rIn)
{
    i = iIn; j = jIn; k = kIn; r = rIn;
}

// const array accessor
template <typename Scalar>
Scalar Quat<Scalar>::operator[](int index) const
{
    return *(&i + index);
}

// array accessor
template <typename Scalar>
Scalar& Quat<Scalar>::operator[](int index)
{
    return *(&i + index);
}

// Set all elements to zero.
template <typename Scalar>
inline void Quat<Scalar>::SetZero()
{
    i = j = k = r = 0;
}

// Returns this quaternion normalized.
template <typename Scalar>
inline Quat<Scalar> Quat<Scalar>::Unit() const
{
    Scalar len = Len();
    return Quat<Scalar>(i / len, j / len, k / len, r / len);
}

// Returns quat length.
template <typename Scalar>
inline Scalar Quat<Scalar>::Len() const
{
    return sqrt(Dot(*this, *this));
}

// Returns quat length squared.
template <typename Scalar>
inline Scalar Quat<Scalar>::LenSq() const
{
    return Dot(*this, *this);
}

// Rotate a vector.
template <typename Scalar>
inline Vector3<Scalar> Quat<Scalar>::Rotate(const Vector3<Scalar>& v) const
{
    Quat<Scalar> r = (*this) * Quat<Scalar>(v.x, v.y, v.z, 0.0f) * ~(*this);
    return Vector3<Scalar>(r.i, r.j, r.k);
}

// Dot product
template <typename Scalar>
inline Scalar Dot(const Quat<Scalar>& a, const Quat<Scalar>& b)
{
    return a.i * b.i + a.j * b.j + a.k * b.k + a.r * b.r;
}

// Linear interpolation between two quaternions
template <typename Scalar>
inline Quat<Scalar> Lerp(const Quat<Scalar>& a, const Quat<Scalar>& b, Scalar t)
{
    Quatf diff(b - a);
    diff.Set(diff.i * t, diff.j * t, diff.k * t, diff.r * t);
    return a + diff;
}

// Quaternion conjugate
template <typename Scalar>
inline Quat<Scalar> operator~(const Quat<Scalar>& v)
{
    return Quat<Scalar>(-v.i, -v.j, -v.k, v.r);
}

// Unary minus.
template <typename Scalar>
inline Quat<Scalar> operator-(const Quat<Scalar>& v)
{
    return Quat<Scalar>(-v.i, -v.j, -v.k, -v.r);
}

// Quaternion subtraction.
template <typename Scalar>
inline Quat<Scalar> operator-(const Quat<Scalar>& a, const Quat<Scalar>& b)
{
    return Quat<Scalar>(a.i - b.i, a.j - b.j, a.k - b.k, a.r - b.r);
}

// Quaternion addition.
template <typename Scalar>
inline Quat<Scalar> operator+(const Quat<Scalar>& a, const Quat<Scalar>& b)
{
    return Quat<Scalar>(a.i + b.i, a.j + b.j, a.k + b.k, a.r + b.r);
}

// Quaternion multplication.
template <typename Scalar>
inline Quat<Scalar> operator*(const Quat<Scalar>& q1, const Quat<Scalar>& q2)
{
    return Quat<Scalar>( q1.i * q2.r + q1.j * q2.k - q1.k * q2.j + q1.r * q2.i,
                        -q1.i * q2.k + q1.j * q2.r + q1.k * q2.i + q1.r * q2.j,
                         q1.i * q2.j - q1.j * q2.i + q1.k * q2.r + q1.r * q2.k,
                        -q1.i * q2.i - q1.j * q2.j - q1.k * q2.k + q1.r * q2.r);
}

// Quaternian exponential, e ^ x
template <typename Scalar>
Quat<Scalar> Exp(const Quat<Scalar>& x)
{
    Scalar angle = Vector3<Scalar>(x.i, x.j, x.k).Len();
    Vector3<Scalar> n;
    if (angle > 0.0001f)
        n = Vector3<Scalar>(x.i, x.j, x.k).Unit() * static_cast<Scalar>(sin(angle / 2));
    else
        n.Set(0,0,0);

    return Quat<Scalar>(n.x, n.y, n.z, cos(angle / 2));
}

// Quaternian logarithm, ln(x)
template <typename Scalar>
Quat<Scalar> Log(const Quat<Scalar>& x)
{
    Scalar cos_a = x.r;
    if (cos_a > 1) cos_a = 1;
    if (cos_a < -1) cos_a = -1;

    Scalar sin_a = sqrt(1 - cos_a * cos_a);

    if (fabs(sin_a) < 0.0005)
        sin_a = 1;
    else
        sin_a = 1/sin_a;

    Scalar angle = 2 * acos(cos_a);
    Quat<Scalar> log;
    log.i = x.i * sin_a * angle;
    log.j = x.j * sin_a * angle;
    log.k = x.k * sin_a * angle;
    log.r = 0;
    return log;
}

// Fuzzy comparison between two Quat values.
template <typename Scalar>
bool FuzzyEqual(const Quat<Scalar>& rhs, const Quat<Scalar>& lhs, Scalar epsilon)
{
    return (FuzzyEqual(rhs.i, lhs.i, epsilon) &&
            FuzzyEqual(rhs.j, lhs.j, epsilon) &&
            FuzzyEqual(rhs.k, lhs.k, epsilon) &&
            FuzzyEqual(rhs.r, lhs.r, epsilon));
}

////////////////////////////////////////////////////////////////////////////////
// Matrix helpers

template <typename Scalar>
inline Scalar Dot3(const Vector3<Scalar>& a, const Vector3<Scalar>& b)
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

template <typename Scalar>
inline Scalar Dot3(const Vector3<Scalar>& a, const Vector4<Scalar>& b)
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

template <typename Scalar>
inline Scalar Dot3(const Vector4<Scalar>& a, const Vector3<Scalar>& b)
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

template <typename Scalar>
inline Scalar Dot3(const Vector4<Scalar>& a, const Vector4<Scalar>& b)
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

////////////////////////////////////////////////////////////////////////////////
// Matrix

// Create Matrix from three principle axes and a translation.
template <typename Scalar>
inline Matrix<Scalar> Matrix<Scalar>::Axes(const Vector3<Scalar>& xAxis, const Vector3<Scalar>& yAxis, const Vector3<Scalar>& zAxis, const Vector3<Scalar>& trans)
{
    Matrix<Scalar> m;
    m.SetXAxis(xAxis);
    m.SetYAxis(yAxis);
    m.SetZAxis(zAxis);
    m.SetTrans(trans);
    return m;
}

// Create a Matrix from four row vectors.
template <typename Scalar>
inline Matrix<Scalar> Matrix<Scalar>::Rows(const Vector4<Scalar>& row0In, const Vector4<Scalar>& row1In, const Vector4<Scalar>& row2In, const Vector4<Scalar>& row3In)
{
    Matrix<Scalar> m;
    m.col0.x = row0In.x; m.col1.x = row0In.y; m.col2.x = row0In.z; m.col3.x = row0In.w;
    m.col0.y = row1In.x; m.col1.y = row1In.y; m.col2.y = row1In.z; m.col3.y = row1In.w;
    m.col0.z = row2In.x; m.col1.z = row2In.y; m.col2.z = row2In.z; m.col3.z = row2In.w;
    m.col0.w = row3In.x; m.col1.w = row3In.y; m.col2.w = row3In.z; m.col3.w = row3In.w;
    return m;
}


// Create a Matrix from a translation.
template <typename Scalar>
inline Matrix<Scalar> Matrix<Scalar>::Trans(const Vector3<Scalar>& trans)
{
    Matrix<Scalar> m;
    m.SetXAxis(Vector3<Scalar>(1, 0, 0));
    m.SetYAxis(Vector3<Scalar>(0, 1, 0));
    m.SetZAxis(Vector3<Scalar>(0, 0, 1));
    m.SetTrans(trans);
    return m;
}


// Create a scale Matrix.
template <typename Scalar>
inline Matrix<Scalar> Matrix<Scalar>::Scale(const Vector3<Scalar>& scale)
{
    Matrix<Scalar> m;
    m.SetXAxis(Vector3<Scalar>(scale.x, 0, 0));
    m.SetYAxis(Vector3<Scalar>(0, scale.y, 0));
    m.SetZAxis(Vector3<Scalar>(0, 0, scale.z));
    m.SetTrans(Vector3<Scalar>(0, 0, 0));
    return m;
}

// Create a uniform scale Matrix.
template <typename Scalar>
inline Matrix<Scalar> Matrix<Scalar>::Scale(Scalar uniformScale)
{
    Matrix<Scalar> m;
    m.SetXAxis(Vector3<Scalar>(uniformScale, 0, 0));
    m.SetYAxis(Vector3<Scalar>(0, uniformScale, 0));
    m.SetZAxis(Vector3<Scalar>(0, 0, uniformScale));
    m.SetTrans(Vector3<Scalar>(0, 0, 0));
    return m;
}

// Create a Matrix from a quaternion.
template <typename Scalar>
inline Matrix<Scalar> Matrix<Scalar>::FromQuat(const Quat<Scalar>& q)
{
    Matrix<Scalar> m;
    m.SetXAxis(q.Rotate(Vector3<Scalar>(1, 0, 0)));
    m.SetYAxis(q.Rotate(Vector3<Scalar>(0, 1, 0)));
    m.SetZAxis(q.Rotate(Vector3<Scalar>(0, 0, 1)));
    m.SetTrans(Vector3<Scalar>(0, 0, 0));
    return m;
}

// Create a Matrix from a Quat and a translation.
template <typename Scalar>
Matrix<Scalar> Matrix<Scalar>::QuatTrans(const Quat<Scalar>& q, const Vector3<Scalar>& trans)
{
    Matrix<Scalar> m;
    m.SetXAxis(q.Rotate(Vector3<Scalar>(1, 0, 0)));
    m.SetYAxis(q.Rotate(Vector3<Scalar>(0, 1, 0)));
    m.SetZAxis(q.Rotate(Vector3<Scalar>(0, 0, 1)));
    m.SetTrans(trans);
    return m;
}

// Create a Matrix from a Scale vector, a Quat and a translation.
template <typename Scalar>
Matrix<Scalar> Matrix<Scalar>::ScaleQuatTrans(const Vector3<Scalar>& scale, const Quat<Scalar>& q, const Vector3<Scalar>& trans)
{
    Matrix<Scalar> m;
    m.SetXAxis(q.Rotate(Vector3<Scalar>(scale.x, 0, 0)));
    m.SetYAxis(q.Rotate(Vector3<Scalar>(0, scale.y, 0)));
    m.SetZAxis(q.Rotate(Vector3<Scalar>(0, 0, scale.z)));
    m.SetTrans(trans);
    return m;
}

// Create a Matrix from a rotation represented by an axis and an angle.
template <typename Scalar>
Matrix<Scalar> Matrix<Scalar>::AxisAngle(const Vector3<Scalar>& axis, Scalar angle)
{
    Matrix<Scalar> m;
    Quat<Scalar> q = Quat<Scalar>::AxisAngle(axis, angle);
    m.SetXAxis(q.Rotate(Vector3<Scalar>(1, 0, 0)));
    m.SetYAxis(q.Rotate(Vector3<Scalar>(0, 1, 0)));
    m.SetZAxis(q.Rotate(Vector3<Scalar>(0, 0, 1)));
    m.SetTrans(Vector3<Scalar>(0, 0, 0));
    return m;
}

// Create an identity Matrix.
template <typename Scalar>
inline Matrix<Scalar> Matrix<Scalar>::Identity()
{
    Matrix<Scalar> m;
    m.col0.Set(1, 0, 0, 0);
    m.col1.Set(0, 1, 0, 0);
    m.col2.Set(0, 0, 1, 0);
    m.col3.Set(0, 0, 0, 1);
    return m;
}

// Create a persective transformation Matrix.
template <typename Scalar>
inline Matrix<Scalar> Matrix<Scalar>::Frustum(Scalar fovy, Scalar aspect, Scalar nearVal, Scalar farVal)
{
    Matrix<Scalar> m;
    Scalar f = 1.0f / tan(fovy / 2);
    m.col0.Set(f / aspect, 0, 0, 0);
    m.col1.Set(0, f, 0, 0);
    m.col2.Set(0, 0, (farVal + nearVal) / (nearVal - farVal), -1);
    m.col3.Set(0, 0, (2 * farVal * nearVal) / (nearVal - farVal), 0);
    return m;
}

// Create an orthograpic projection Matrix.
template <typename Scalar>
inline Matrix<Scalar> Matrix<Scalar>::Ortho(Scalar left, Scalar right, Scalar bottom, Scalar top, Scalar nearVal, Scalar farVal)
{
    Matrix<Scalar> m;
    Scalar tx = -(right + left) / (right - left);
    Scalar ty = -(top + bottom) / (top - bottom);
    Scalar tz = -(farVal + nearVal) / (farVal - nearVal);
    m.col0.Set(2.0f / (right - left), 0, 0, 0);
    m.col1.Set(0, 2.0f / (top - bottom), 0, 0);
    m.col2.Set(0, 0, -2.0f / (farVal - nearVal), 0);
    m.col3.Set(tx, ty, tz, 1);
    return m;
}

// Create a look at matrix. (x is forward)
template <typename Scalar>
inline Matrix<Scalar> Matrix<Scalar>::LookAt(const Vector3<Scalar>& eye, const Vector3<Scalar>& target, const Vector3<Scalar>& up)
{
    const Scalar Epsilon = 0.001;
    const Scalar EpsilonSq = Epsilon * Epsilon;

    Vector3<Scalar> x;
    if ((target - eye).LenSq() < EpsilonSq)
        x = Vector3<Scalar>(1, 0, 0);  // target == eye, so pick (1,0,0) for the x-axis
    else
        x = (target - eye).Unit();

    Vector3<Scalar> u;
    if (up.LenSq() < EpsilonSq)
        u = Vector3<Scalar>(0, 1, 0);  // up is zero, so pick (0,1,0) for the y-axis.
    else
        u = up;

    if (fabs(Dot(u, x)) > (u.LenSq() - EpsilonSq)) // are u & x parallel?
        u = (u + Vector3<Scalar>(0.2,0.2,0.2)).Unit();  // nudge u so it's no longer parallel.

    Vector3<Scalar> z = Cross(x, u).Unit();
    Vector3<Scalar> y = Cross(z, x).Unit();

    Matrix<Scalar> m;
    m.SetXAxis(x);
    m.SetYAxis(y);
    m.SetZAxis(z);
    m.SetTrans(eye);
    return m;
}

// Axes accessors
template <typename Scalar>
inline Vector3<Scalar> Matrix<Scalar>::GetXAxis() const
{
    return Vector3<Scalar>(col0.x, col0.y, col0.z);
}

template <typename Scalar>
inline Vector3<Scalar> Matrix<Scalar>::GetYAxis() const
{
    return Vector3<Scalar>(col1.x, col1.y, col1.z);
}

template <typename Scalar>
inline Vector3<Scalar> Matrix<Scalar>::GetZAxis() const
{
    return Vector3<Scalar>(col2.x, col2.y, col2.z);
}

template <typename Scalar>
inline Vector3<Scalar> Matrix<Scalar>::GetTrans() const
{
    return Vector3<Scalar>(col3.x, col3.y, col3.z);
}

template <typename Scalar>
inline void Matrix<Scalar>::SetXAxis(const Vector3<Scalar>& xAxis)
{
    col0.Set(xAxis.x, xAxis.y, xAxis.z, 0);
}

template <typename Scalar>
inline void Matrix<Scalar>::SetYAxis(const Vector3<Scalar>& yAxis)
{
    col1.Set(yAxis.x, yAxis.y, yAxis.z, 0);
}

template <typename Scalar>
inline void Matrix<Scalar>::SetZAxis(const Vector3<Scalar>& zAxis)
{
    col2.Set(zAxis.x, zAxis.y, zAxis.z, 0);
}

template <typename Scalar>
inline void Matrix<Scalar>::SetTrans(const Vector3<Scalar>& trans)
{
    col3.Set(trans.x, trans.y, trans.z, 1);
}

// Row accessors
template <typename Scalar>
inline Vector4<Scalar> Matrix<Scalar>::GetRow(int index) const
{
    Scalar* p = (Scalar*)&col0;
    return Vector4<Scalar>(p[0 + index], p[4 + index], p[8 + index], p[12 + index]);
}

template <typename Scalar>
inline void Matrix<Scalar>::SetRow(int index, const Vector4<Scalar>& row)
{
    Scalar* p = (Scalar*)&col0;
    p[0 + index] = row.x;
    p[4 + index] = row.y;
    p[8 + index] = row.z;
    p[12 + index] = row.w;
}

// Column accessors
template <typename Scalar>
inline Vector4<Scalar> Matrix<Scalar>::GetCol(int index) const
{
    Vector4<Scalar>* v = (Vector4<Scalar>*)&col0;
    return v[index];
}

template <typename Scalar>
inline void Matrix<Scalar>::SetCol(int index, const Vector4<Scalar>& col)
{
    Vector4<Scalar>* v = (Vector4<Scalar>*)&col0;
    v[index] = col;
}

// Element accessors
template <typename Scalar>
inline const Scalar& Matrix<Scalar>::Elem(int r, int c) const
{
    return ((Scalar*)&col0)[c * 4 + r];
}

template <typename Scalar>
inline Scalar& Matrix<Scalar>::Elem(int r, int c)
{
    return ((Scalar*)&col0)[c * 4 + r];
}

// Multiplies by uniform scale.
template <typename Scalar>
inline void Matrix<Scalar>::SetScale(Scalar scale)
{
    SetXAxis(GetXAxis() * scale);
    SetYAxis(GetYAxis() * scale);
    SetZAxis(GetZAxis() * scale);
}

// Multiplies by non-uniform scale.
template <typename Scalar>
inline void Matrix<Scalar>::SetScale(const Vector3<Scalar>& scale)
{
    SetXAxis(GetXAxis() * scale.x);
    SetYAxis(GetYAxis() * scale.y);
    SetZAxis(GetZAxis() * scale.z);
}

// Returns the rotation component of this Matrix.
template <typename Scalar>
Quat<Scalar> Matrix<Scalar>::GetQuat() const
{
    // TODO: add to unit test!

    int x = 0, y = 1, z = 2;
    // create a matrix with no scale
    Matrix<Scalar> m = Axes(GetXAxis().Unit(), GetYAxis().Unit(), GetZAxis().Unit(), Vector3<Scalar>(0,0,0));
    Scalar trace = m.Elem(0,0) + m.Elem(1,1) + m.Elem(2,2);
    Vector4<Scalar> q;
    if (trace < -1.0)
    {
        int i = x, j = y, k = z;
        if (m.Elem(y,y) > m.Elem(x,x)) { i = y; j = z; k = x; }
        if (m.Elem(z,z) > m.Elem(i,i)) { i = z; j = x; k = y; }
        Scalar r = sqrt(m.Elem(i,i) - m.Elem(j,j) - m.Elem(k,k) + 1);
        Scalar s = 0.5 / r;
        q[i] = 0.5 * r;
        q[j] = (m.Elem(i,j) + m.Elem(j,i)) * s;
        q[k] = (m.Elem(k,i) + m.Elem(i,k)) * s;
        q[3] = (m.Elem(k,j) + m.Elem(j,k)) * s;
    }
    else
    {
        Scalar r = sqrt(trace + 1.0);
        Scalar s = 0.5 / r;
        q[0] = (m.Elem(z,y) - m.Elem(y,z)) * s;
        q[1] = (m.Elem(x,z) - m.Elem(z,x)) * s;
        q[2] = (m.Elem(y,x) - m.Elem(x,y)) * s;
        q[3] = 0.5 * r;
    }
    return Quat<Scalar>(q.x, q.y, q.z, q.w);
}

// Multiply the 3x3 component of this Matrix with a column vector.
template <typename Scalar>
inline Vector3<Scalar> Matrix<Scalar>::Mul3x3(const Vector3<Scalar>& v) const
{
    Vector4<Scalar> r = col0 * v.x;
    r += col1 * v.y;
    r += col2 * v.z;
    return Vector3<Scalar>(r.x, r.y, r.z);
}

// Multiply the 3x4 component of this Matrix with a column vector. (w component of vector is 1.0)
template <typename Scalar>
inline Vector3<Scalar> Matrix<Scalar>::Mul3x4(const Vector3<Scalar>& v) const
{
    Vector4<Scalar> r = col0 * v.x;
    r += col1 * v.y;
    r += col2 * v.z;
    r += col3;
    return Vector3<Scalar>(r.x, r.y, r.z);
}

// Multiply this Matrix with a column vector.
template <typename Scalar>
inline Vector4<Scalar> Matrix<Scalar>::Mul4x4(const Vector4<Scalar>& v) const
{
    Vector4<Scalar> r = col0 * v.x;
    r += col1 * v.y;
    r += col2 * v.z;
    r += col3 * v.w;
    return r;
}

// Returns the transpose of this Matrix
template <typename Scalar>
inline Matrix<Scalar> Matrix<Scalar>::Transpose() const
{
    return Rows(col0, col1, col2, col3);
}

// If the 3x3 portion of this Matrix is Orthogonal (i.e. columns are orthogonal unit vectors)
// this will return the Inverse of that matrix.
template <typename Scalar>
Matrix<Scalar> Matrix<Scalar>::OrthoInverse() const
{
    Matrix<Scalar> r(*this);
    r.SetTrans(Vector3<Scalar>(0, 0, 0));
    r = r.Transpose();
    r.SetTrans(-r.Mul3x4(GetTrans()));
    return r;
}

// Full 4x4 Matrix Inverse, returns Identity if matrix has no inverse.
template <typename Scalar>
Matrix<Scalar> Matrix<Scalar>::FullInverse() const
{
    Matrix<Scalar> m;
    if (::FullInverse((*this), m))
        return m;
    else
        return Identity();
}

// Matrix addition
template <typename Scalar>
Matrix<Scalar> operator+(const Matrix<Scalar>& a, const Matrix<Scalar>& b)
{
    Matrix<Scalar> m;
    m.col0 = a.col0 + b.col0;
    m.col1 = a.col1 + b.col1;
    m.col2 = a.col2 + b.col2;
    m.col3 = a.col3 + b.col3;
    return m;
}

// Matrix subtraction
template <typename Scalar>
Matrix<Scalar> operator-(const Matrix<Scalar>& a, const Matrix<Scalar>& b)
{
    Matrix<Scalar> m;
    m.col0 = a.col0 - b.col0;
    m.col1 = a.col1 - b.col1;
    m.col2 = a.col2 - b.col2;
    m.col3 = a.col3 - b.col3;
    return m;
}

// Matrix multiplication
template <typename Scalar>
Matrix<Scalar> operator*(const Matrix<Scalar>& a, const Matrix<Scalar>& b)
{
    Vector4<Scalar> a_row0 = a.GetRow(0);
    Vector4<Scalar> a_row1 = a.GetRow(1);
    Vector4<Scalar> a_row2 = a.GetRow(2);
    Vector4<Scalar> a_row3 = a.GetRow(3);

    return Matrix<Scalar>::Rows(Vector4<Scalar>(Dot(a_row0, b.col0), Dot(a_row0, b.col1),
                                                Dot(a_row0, b.col2), Dot(a_row0, b.col3)),
                                Vector4<Scalar>(Dot(a_row1, b.col0), Dot(a_row1, b.col1),
                                                Dot(a_row1, b.col2), Dot(a_row1, b.col3)),
                                Vector4<Scalar>(Dot(a_row2, b.col0), Dot(a_row2, b.col1),
                                                Dot(a_row2, b.col2), Dot(a_row2, b.col3)),
                                Vector4<Scalar>(Dot(a_row3, b.col0), Dot(a_row3, b.col1),
                                                Dot(a_row3, b.col2), Dot(a_row3, b.col3)));
}

template <typename T>
void AbaciSwap(T& a, T& b)
{
    T temp = a;
    a = b;
    b = temp;
}

// Full 4x4 Matrix inverse, returns false if Matrix has no inverse.
template <typename Scalar>
bool FullInverse(const Matrix<Scalar>& m, Matrix<Scalar>& result)
{
    // Gaussian-Jordan Elimination, TODO: There are more numerically stable ways to do this...

    Scalar temp[4][8];
    Scalar* row[4];

    // initialize the r pointers
    for (int r = 0; r < 4; ++r)
        row[r] = temp[r];

    // initialize the augmented temp matrix.
    // the first four columns are from m
    temp[0][0] = m.col0.x; temp[0][1] = m.col1.x; temp[0][2] = m.col2.x; temp[0][3] = m.col3.x;
    temp[1][0] = m.col0.y; temp[1][1] = m.col1.y; temp[1][2] = m.col2.y; temp[1][3] = m.col3.y;
    temp[2][0] = m.col0.z; temp[2][1] = m.col1.z; temp[2][2] = m.col2.z; temp[2][3] = m.col3.z;
    temp[3][0] = m.col0.w; temp[3][1] = m.col1.w; temp[3][2] = m.col2.w; temp[3][3] = m.col3.w;

    // the second four are identity
    temp[0][4] = 1; temp[0][5] = 0; temp[0][6] = 0; temp[0][7] = 0;
    temp[1][4] = 0; temp[1][5] = 1; temp[1][6] = 0; temp[1][7] = 0;
    temp[2][4] = 0; temp[2][5] = 0; temp[2][6] = 1; temp[2][7] = 0;
    temp[3][4] = 0; temp[3][5] = 0; temp[3][6] = 0; temp[3][7] = 1;

    // bubble up row with largest leading number (partial pivot)
    if (fabs(row[0][0]) < fabs(row[1][0]))
        AbaciSwap(row[0], row[1]);
    if (fabs(row[0][0]) < fabs(row[2][0]))
        AbaciSwap(row[0], row[2]);
    if (fabs(row[0][0]) < fabs(row[3][0]))
        AbaciSwap(row[0], row[3]);

    if (fabs(row[0][0]) < 0.00001)  // column is all zeros, there is no inverse.
    {
        return false;
    }

    // mult row[0] by 1/row[0][0].  To introduce a leading 1.
    Scalar s = 1 / row[0][0];
    for (int c = 0; c < 8; ++c) row[0][c] *= s;

    // add multiples of top row to lower rows so that all entries below leading 1 become zeros.
    for (int r = 1; r < 4; ++r)
    {
        Scalar s = row[r][0];
        for (int c = 0; c < 8; ++c) row[r][c] -= s * row[0][c];
    }

    // move row with largest leading number
    if (fabs(row[1][1]) < fabs(row[2][1]))
        AbaciSwap(row[1], row[2]);
    if (fabs(row[1][1]) < fabs(row[3][1]))
        AbaciSwap(row[1], row[3]);

    if (fabs(row[1][1]) < 0.00001)  // column is all zeros, there is no inverse.
    {
        return false;
    }

    // mult row[1] by 1/row[1][1].  To introduce a leading 1.
    s = 1 / row[1][1];
    for (int c = 0; c < 8; ++c) row[1][c] *= s;

    // add multiples of top row to lower rows so that all entries below leading 1 become zeros.
    for (int r = 2; r < 4; ++r)
    {
        Scalar s = row[r][1];
        for (int c = 0; c < 8; ++c) row[r][c] -= s * row[1][c];
    }

    // move row with largest leading number
    if (fabs(row[2][2]) < fabs(row[3][2]))
        AbaciSwap(row[2], row[3]);

    if (fabs(row[2][2]) < 0.00001)  // column is all zeros, there is no inverse.
    {
        return false;
    }

    // mult row[2] by 1/row[2][2].  To introduce a leading 1.
    s = 1.0f / row[2][2];
    for (int c = 0; c < 8; ++c) row[2][c] *= s;

    // add multiples of top row to lower rows so that all entries below leading 1 become zeros.
    for (int r = 3; r < 4; ++r)
    {
        Scalar s = row[r][2];
        for (int c = 0; c < 8; ++c) row[r][c] -= s * row[2][c];
    }

    // mult row[3] by 1/row[3][3]. To introduce a leading 1.
    s = 1.0f / row[3][3];
    for (int c = 0; c < 8; ++c) row[3][c] *= s;

    // at this point row matrix should be in row-echelon form.

    // add multiples of row[3] to above rows, to zero out that column
    for (int r = 0; r < 3; ++r)
    {
        Scalar s = row[r][3];
        for (int c = 0; c < 8; ++c) row[r][c] -= s * row[3][c];
    }

    // add multiples of row[2] to above rows, to zero out that column
    for (int r = 0; r < 2; ++r)
    {
        Scalar s = row[r][2];
        for (int c = 0; c < 8; ++c) row[r][c] -= s * row[2][c];
    }

    // add multiples of row[1] to above row, to zero out that column
    for (int r = 0; r < 1; ++r)
    {
        Scalar s = row[r][1];
        for (int c = 0; c < 8; ++c) row[r][c] -= s * row[1][c];
    }

    // init result
    result.col0.Set(row[0][4], row[1][4], row[2][4], row[3][4]);
    result.col1.Set(row[0][5], row[1][5], row[2][5], row[3][5]);
    result.col2.Set(row[0][6], row[1][6], row[2][6], row[3][6]);
    result.col3.Set(row[0][7], row[1][7], row[2][7], row[3][7]);

    return true;
}


// Print to stdout.
template <typename Scalar>
void PrintMatrix(const Matrix<Scalar>& m)
{
    printf("| %15.5f, %15.5f, %15.5f, %15.5f |\n", m.col0.x, m.col1.x, m.col2.x, m.col3.x);
    printf("| %15.5f, %15.5f, %15.5f, %15.5f |\n", m.col0.y, m.col1.y, m.col2.y, m.col3.y);
    printf("| %15.5f, %15.5f, %15.5f, %15.5f |\n", m.col0.z, m.col1.z, m.col2.z, m.col3.z);
    printf("| %15.5f, %15.5f, %15.5f, %15.5f |\n", m.col0.w, m.col1.w, m.col2.w, m.col3.w);
}

////////////////////////////////////////////////////////////////////////////////
// Complex

// Construct from two floats
template <typename Scalar>
inline Complex<Scalar>::Complex(Scalar rIn, Scalar iIn) : r(rIn), i(iIn) {}

// Construct from a Vector2
template <typename Scalar>
inline Complex<Scalar>::Complex(const Vector2<Scalar>& vector2In) :
    r(vector2In.x), i(vector2In.y) {}

// Set from two Scalars
template <typename Scalar>
inline void Complex<Scalar>::Set(Scalar rIn, Scalar iIn)
{
    r = rIn;
    i = iIn;
}

// Length
template <typename Scalar>
inline Scalar Complex<Scalar>::Len() const
{
    return sqrt(Dot(*this, *this));
}

// Square of length
template <typename Scalar>
inline Scalar Complex<Scalar>::LenSq() const
{
    return Dot(*this, *this);
}

// Unit
template <typename Scalar>
inline Complex<Scalar> Complex<Scalar>::Unit() const
{
    return *this / Complex<Scalar>(Len(), 0);
}

// Returns a vector with the same direction, but with a Len() <= len.
template <typename Scalar>
inline Complex<Scalar> Complex<Scalar>::MinLen(Scalar len) const
{
    Scalar l = Len();
    if (l > len)
        return (*this / Complex<Scalar>(l, 0)) * Complex<Scalar>(len, 0);
    else
        return *this;
}

// Dot product
template <typename Scalar>
inline Scalar Dot(const Complex<Scalar>& a, const Complex<Scalar>& b)
{
    return a.r * b.r + a.i * b.i;
}

// Complex conjugate
template <typename Scalar>
inline Complex<Scalar> operator~(const Complex<Scalar>& a)
{
    return Complex<Scalar>(a.r, -a.i);
}

// Unary minus.
template <typename Scalar>
inline Complex<Scalar> operator-(const Complex<Scalar>& a)
{
    return Complex<Scalar>(-a.r, -a.i);
}

// Unary plus.
template <typename Scalar>
inline Complex<Scalar> operator+(const Complex<Scalar>& a)
{
    return Complex<Scalar>(a.r, a.i);
}

// Complex addition.
template <typename Scalar>
inline Complex<Scalar> operator+(const Complex<Scalar>& a, const Complex<Scalar>& b)
{
    return Complex<Scalar>(a.r + b.r, a.i + b.i);
}

// Complex subtraction.
template <typename Scalar>
inline Complex<Scalar> operator-(const Complex<Scalar>& a, const Complex<Scalar>& b)
{
    return Complex<Scalar>(a.r - b.r, a.i - b.i);
}

// Complex multiplication.
template <typename Scalar>
inline Complex<Scalar> operator*(const Complex<Scalar>& a, const Complex<Scalar>& b)
{
    Scalar aa = a.r;
    Scalar bb = a.i;
    Scalar cc = b.r;
    Scalar dd = b.i;

    // four muls & two adds
    // return Complex<Scalar>(aa * cc - (bb * dd), aa * dd + bb * cc);

    Scalar a_c = aa * cc;
    Scalar b_d = bb * dd;

    // three muls & five adds (faster?)
    return Complex<Scalar>(a_c - b_d, (aa + bb) * (cc + dd) - a_c - b_d);
}

// Multiplication by a real number.
template <typename Scalar>
inline Complex<Scalar> operator*(Scalar scalar, const Complex<Scalar>& c)
{
    return Complex<Scalar>(scalar, 0) * c;
}

// Multiplication by a real number.
template <typename Scalar>
inline Complex<Scalar> operator*(const Complex<Scalar>& c, Scalar scalar)
{
    return c * Complex<Scalar>(scalar, 0);
}

// Complex division.
template <typename Scalar>
inline Complex<Scalar> operator/(const Complex<Scalar>& a, const Complex<Scalar>& b)
{
    Scalar aa = a.r;
    Scalar bb = a.i;
    Scalar cc = b.r;
    Scalar dd = b.i;
    Scalar denom = cc * cc + dd * dd;

    return Complex<Scalar>((aa * cc + bb * dd) / denom, (bb * cc - aa * dd) / denom);
}

// Complex division by a scalar denominator (x + 0i)
template <typename Scalar>
Complex<Scalar> operator/(const Complex<Scalar>& c, Scalar denominator)
{
    return c / Complex<Scalar>(denominator, 0);
}

// Complex division with a scalar numerator (x + 0i)
template <typename Scalar>
Complex<Scalar> operator/(Scalar numerator, const Complex<Scalar>& c)
{
    return Complex<Scalar>(numerator, 0) / c;
}

// Square root
template <typename Scalar>
Complex<Scalar> sqrt(const Complex<Scalar>& z)
{
    Scalar x = z.r;
    Scalar y = z.i;

    if (x == Scalar())
    {
        Scalar t = sqrt(fabs(y) / 2);
        return Complex<Scalar>(t, y < Scalar() ? -t : t);
    }
    else
    {
        Scalar t = sqrt(2 * (z.Len() + fabs(x)));
        Scalar u = t / 2;
        return x > Scalar() ? Complex<Scalar>(u, y / t) :
                              Complex<Scalar>(fabs(y) / t, y < Scalar() ? -u : u);
    }
}

// Exponent e ^ z
template <typename Scalar>
Complex<Scalar> Exp(const Complex<Scalar>& z)
{
    Scalar e = exp(z.r);
    return Complex<Scalar>(e * cos(z.i), e * sin(z.i));
}

// e ^ (0 + xi)
template <typename Scalar>
inline Complex<Scalar> ExpI(Scalar x)
{
    return Complex<Scalar>(cos(x), sin(x));
}

// Natural Logarithm, ln(z)
template <typename Scalar>
Complex<Scalar> Log(const Complex<Scalar>& z)
{
    return Complex<Scalar>(log(z.Len()), atan2(z.i, z.r));
}

// Fuzzy comparison between two Complex values.
template <typename Scalar>
bool FuzzyEqual(const Complex<Scalar>& rhs, const Complex<Scalar>& lhs, Scalar epsilon)
{
    return (FuzzyEqual(rhs.r, lhs.r, epsilon) &&
            FuzzyEqual(rhs.i, lhs.i, epsilon));
}

#endif
