/**
 * @file triangle.hpp
 * @brief Class definition for Triangle.
 *
 * @author Eric Butler (edbutler)
 */

#ifndef _462_SCENE_TRIANGLE_HPP_
#define _462_SCENE_TRIANGLE_HPP_

#include "scene/scene.hpp"

namespace _462 {

/**
 * a triangle geometry.
 * Triangles consist of 3 vertices. Each vertex has its own position, normal,
 * texture coordinate, and material. These should all be interpolated to get
 * values in the middle of the triangle.
 * These values are all in local space, so it must still be transformed by
 * the Geometry's position, orientation, and scale.
 */
class Triangle : public Geometry
{
public:

    struct Vertex
    {
        // note that position and normal are in local space
        Vector3 position;
        Vector3 normal;
        Vector2 tex_coord;
        const Material* material;
    };

    // the triangle's vertices, in CCW order
    Vertex vertices[3];

    Triangle();
    virtual ~Triangle();
    virtual void render() const;
    
    virtual Vector3 transform_vector(const Vector3 &v) const;
    virtual Vector3 transform_point(const Vector3 &p) const;
    virtual Color3 color_at_pixel(const Scene* scene,const Vector3 &surface_pos) const ;
    virtual real_t is_intersecting(Vector3 &s, Vector3 &e, real_t *T) const;
    virtual real_t shadow_intersection(const Vector3 &shadow_dir, const Vector3 &surface_pos) const;      

    virtual Color3 get_specular() const;
    virtual Vector3 normal_of(const Vector3 &surface_pos) const;
    virtual Color3 compute_specular(const Scene* scene, const Vector3 &normal, const Vector3 &incoming_ray, const Vector3 &surface_pos, int depth) const ;
    virtual real_t get_refractive_index() const;
    virtual real_t compute_refraction(const real_t inner_refr, const real_t outer_refr, const Vector3 &incoming_ray,const Vector3 &normal) const;

    Color3 compute_texture_at_vertex(real_t u, real_t v, const Material* material) const; 
    Color3 compute_texture () const;
    Color3 compute_diffuse(const Scene* scene, const Vector3 &normal,const Vector3 &surface_pos) const;
    Color3 attenuation(real_t &dist, const PointLight light, const Vector3 &light_pos, const Vector3 &surface_pos)const; 
};


} /* _462 */

#endif /* _462_SCENE_TRIANGLE_HPP_ */

