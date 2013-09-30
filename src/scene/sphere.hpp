/**
 * @file sphere.hpp
 * @brief Class defnition for Sphere.
 *
 * @author Kristin Siu (kasiu)
 * @author Eric Butler (edbutler)
 */

#ifndef _462_SCENE_SPHERE_HPP_
#define _462_SCENE_SPHERE_HPP_

#include "scene/scene.hpp"

namespace _462 {

/**
 * A sphere, centered on its position with a certain radius.
 */
class Sphere : public Geometry
{
public:

    real_t radius;
    const Material* material;

    Sphere();
    virtual ~Sphere();
    virtual void render() const;
    virtual real_t is_intersecting(Vector3 &s, Vector3 &e, real_t *T) const;

    virtual Vector3 transform_vector(const Vector3 &v) const;
    virtual Vector3 transform_point(const Vector3 &p) const;
    virtual Color3 color_at_pixel(const Scene* scene, const Vector3 &surface_pos) const; 
    virtual real_t shadow_intersection(const Vector3 &shadow_dir, const Vector3 &surface_pos) const;
    virtual Color3 get_specular() const; 
    virtual Vector3 normal_of(const Vector3 &surface_pos) const;
    virtual Color3 compute_specular(const Scene* scene, const Vector3 &normal, const Vector3 &incoming_ray, const Vector3 &surface_pos, int depth) const ; 
    virtual real_t get_refractive_index() const;
    virtual real_t compute_refraction(const real_t n, const real_t nt, const Vector3 &incoming_ray,const Vector3 &normal) const;
  
   
    Vector3 compute_refr_ray(const real_t n, const real_t nt,const Vector3 &normal, const Vector3 &dir) const; 
    Color3 compute_texture(const Vector3 &normal) const;  
    Color3 compute_diffuse(const Scene* scene, Vector3 normal,Vector3 surface_position) const ;
    Color3 attenuation(real_t dist, const PointLight light, const Vector3 light_pos, const Vector3 surface_pos)const; 
};

} /* _462 */

#endif /* _462_SCENE_SPHERE_HPP_ */

