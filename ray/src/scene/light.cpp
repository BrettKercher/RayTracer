#include <cmath>

#include "light.h"

using namespace std;

double DirectionalLight::distanceAttenuation(const Vec3d& P) const
{
  // distance to light is infinite, so f(di) goes to 0.  Return 1.
  return 1.0;
}

Vec3d DirectionalLight::shadowAttenuation(const Vec3d& p) const
{
  // YOUR CODE HERE:
  // You should implement shadow-handling code here.
  isect i;

  ray shadowRay(p, getDirection(p), ray::SHADOW);

  if(scene->intersect(shadowRay, i))
  {
    if(i.t <= RAY_EPSILON)
      return Vec3d(1,1,1);
    else
      return Vec3d(0,0,0);
  }
  return Vec3d(1,1,1);
}

Vec3d DirectionalLight::getColor() const
{
  return color;
}

Vec3d DirectionalLight::getDirection(const Vec3d& P) const
{
  // for directional light, direction doesn't depend on P
  return -orientation;
}

double PointLight::distanceAttenuation(const Vec3d& P) const
{

  // YOUR CODE HERE

  // You'll need to modify this method to attenuate the intensity 
  // of the light based on the distance between the source and the 
  // point P.  For now, we assume no attenuation and just return 1.0

  Vec3d lightVec = position - P;
  double d = lightVec.length();

  return std::min(1.0, (1.0 / (constantTerm + (linearTerm * d) + (quadraticTerm * d * d) ) ) );
}

Vec3d PointLight::getColor() const
{
  return color;
}

Vec3d PointLight::getDirection(const Vec3d& P) const
{
  Vec3d ret = position - P;
  ret.normalize();
  return ret;
}


Vec3d PointLight::shadowAttenuation(const Vec3d& p) const
{
  // YOUR CODE HERE:
  // You should implement shadow-handling code here.
  isect i;

  Vec3d d = (position - p);
  double distanceToLight = d.length();
  d.normalize();

  ray shadowRay(p, d, ray::SHADOW);

  if(scene->intersect(shadowRay, i))
  {
    if(i.t <= RAY_EPSILON)
      return Vec3d(1,1,1);
    else if( distanceToLight > i.t )
      return Vec3d(0,0,0);
    else
      return Vec3d(1,1,1);
  }
  return Vec3d(1,1,1);
}