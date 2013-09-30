/**
 * @file model.hpp
 * @brief Model class
 *
 * @author Eric Butler (edbutler)
 */

#ifndef _462_SCENE_MODEL_HPP_
#define _462_SCENE_MODEL_HPP_

#include "scene/scene.hpp"
#include "scene/mesh.hpp"

namespace _462 {

/**
 * A mesh of triangles.
 */
class Model : public Geometry
{
public:

    const Mesh* mesh;
    const Material* material;

    Model();
    virtual ~Model();
    virtual Vector3 transform_vector(const Vector3 &v) const;
    virtual Vector3 transform_point(const Vector3 &v) const;
	
    virtual void render() const;
    virtual Color3 color_at_pixel(const Scene* scene, const Vector3 &surface_pos) const;
    virtual real_t is_intersecting(Vector3 &s, Vector3 &e, real_t *T) const;
    virtual real_t shadow_intersection(const Vector3 &shadow_dir, const Vector3 &surface_pos) const;

    virtual Color3 get_specular() const;
    virtual Vector3 normal_of(const Vector3 &surface_pos) const;
    virtual Color3 compute_specular(const Scene* scene, const Vector3 &normal, const Vector3 &incoming_ray, const Vector3 &surface_pos, int depth) const ;
    virtual real_t get_refractive_index() const;
    virtual real_t compute_refraction(const real_t inner_refr, const real_t outer_refr, const Vector3 &incoming_ray,const Vector3 &normal) const;

    Color3 compute_texture_at_vertex(real_t u, real_t v) const;
    Color3 compute_texture () const;
  
    Color3 attenuation(real_t &dist, const PointLight light, const Vector3 &light_pos, const Vector3 &surface_pos) const;
    Color3 compute_diffuse(const Scene* scene, const Vector3 &normal, const Vector3 &surface_pos) const;
    real_t intersects_triangle(const MeshTriangle &triangle, const Vector3 &s, const Vector3 &e, real_t *T) const;
    real_t max(const real_t a, const real_t b) const; 
    real_t shadow_intersect_triangle(const MeshTriangle &triangle, const Vector3 &ray, const Vector3 &surface_pos) const;
};


} /* _462 */

#endif /* _462_SCENE_MODEL_HPP_ */

