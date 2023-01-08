#pragma once

typedef double real ;

struct ivec2 {
  ivec2() : x(0), y(0) {}
  ivec2(int v) : x(v), y(v) {}
  ivec2(int x, int y) : x(x), y(y) {}
  int x,y;
};

struct vec2 {
  vec2() : x(0.f), y(0.f) {}
  vec2(real v) : x(v), y(v) {}
  vec2(real x, real y) : x(x), y(y) {}
  real x,y;
};

real dot(const vec2 a, const vec2 b)
{
  return a.x*b.x + a.y*b.y;
}

vec2 operator-(vec2 a, vec2 b)
{
  return vec2(a.x-b.x, a.y-b.y);
}

struct vec3 {
  vec3() : x(0.f), y(0.f), z(0.f){}
  vec3(real v) : x(v), y(v), z(v) {}
  vec3(real x, real y) : x(x), y(y), z(0.f) {}
  vec3(real x, real y, real z) : x(x), y(y), z(z) {}

  real dot(const vec3 b) const
  {
      return x * b.x + y * b.y + z * b.z;
  }

  real length() const
  {
      return sqrtf( this->dot(*this) );
  }

  vec3 unit() const
  {
      real rlen = 1.0f / length();
      return vec3(x * rlen, y * rlen, z*rlen);
  }

  vec3 cross(vec3 b) const
  {
      vec3 a = *this;
      return vec3(
          a.y * b.z - a.z * b.y,
          a.z * b.x - a.x * b.z,
          a.x * b.y - a.y * b.x);
  }

  real x,y,z;
};

real dot(const vec3 a, const vec3 b)
{
    return a.dot(b);
}

vec3 operator/(vec3 a, real v)
{
  return vec3(a.x/v, a.y/v, a.z/v);
}

vec3 operator-(vec3 a)
{
    return vec3(-a.x, -a.y, -a.z);
}
vec3 operator+(vec3 a, vec3 b)
{
    return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}
vec3 operator-(vec3 a, vec3 b)
{
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}
vec3 operator*(vec3 a, real s)
{
    return vec3(a.x * s, a.y * s, a.z * s);
}
vec3 lerp(vec3 a, vec3 b, real t)
{
    return a + (b - a) * t;
}
vec3 cross(vec3 a, vec3 b)
{
    return vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x);
}
