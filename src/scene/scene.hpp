/**
 * @file scene.hpp
 * @brief Class definitions for scenes.
 *
 * @author Eric Butler (edbutler)
 * @author Kristin Siu (kasiu)
 */

#ifndef _462_SCENE_SCENE_HPP_
#define _462_SCENE_SCENE_HPP_

#include "math/vector.hpp"
#include "math/quaternion.hpp"
#include "math/matrix.hpp"
#include "math/camera.hpp"
#include "scene/material.hpp"
#include "scene/mesh.hpp"
#include <string>
#include <vector>

namespace _462 {

class Scene;
struct PointLight;

class Geometry
{
public:
    Geometry();
    virtual ~Geometry();

    /*
       World transformation are applied in the following order:
       1. Scale
       2. Orientation
       3. Position
    */

    // The world position of the object.
    Vector3 position;

    // The world orientation of the object.
    // Use Quaternion::to_matrix to get the rotation matrix.
    Quaternion orientation;

    // The world scale of the object.
    Vector3 scale;

    Matrix4 inv_trans;
    Matrix3 norm_matrix;
    /**
     * Renders this geometry using OpenGL in the local coordinate space.
     */
    virtual void render() const = 0;
	
    /*	virtual function for determining if the viewing ray intersects 
     *	a given geometry, rules for return values are as follows: 
     * 	-1 indicates intersection a sphere
     * 	 1 indicates intersection with a triangle
     * 	 0 indicates no intersection */
    virtual real_t is_intersecting(Vector3 &s, Vector3 &e, real_t *T) const = 0;

    /*  virtual function for evaluating color at specified pixel
     *  utilizes several helper methods with in each class  
     */
    virtual Color3 color_at_pixel(const Scene* scene,const Vector3 &surface_pos) const = 0;

    virtual Color3 compute_specular(const Scene* scene, const Vector3 &normal, const Vector3 &incoming_ray, const Vector3 &surface_pos, int depth) const = 0;    
    /* transformation helper functions based on the inverse transform matrix*/
    virtual Vector3 transform_vector(const Vector3 &v) const = 0;
    virtual Vector3 transform_point(const Vector3 &v) const = 0;

    virtual real_t get_refractive_index() const = 0; 
    virtual real_t compute_refraction(const real_t inner_refr, const real_t outer_refr, const Vector3 &incoming_ray,const Vector3 &normal) const = 0; 
    virtual Color3 get_specular() const = 0; 
    virtual Vector3 normal_of(const Vector3 &surface_pos) const = 0;     

    /* determines whether a given position is in shadow*/	
    virtual real_t shadow_intersection(const Vector3 &shadow_dir, const Vector3 &surface_pos) const = 0;      
};


struct PointLight
{
    struct Attenuation
    {
        real_t constant;
        real_t linear;
        real_t quadratic;
    };

    PointLight();

    // The position of the light, relative to world origin.
    Vector3 position;
    // The color of the light (both diffuse and specular)
    Color3 color;
    // attenuation
    Attenuation attenuation;
};

/**
 * The container class for information used to render a scene composed of
 * Geometries.
 */
class Scene
{
public:

    /// the camera
    Camera camera;
    /// the background color
    Color3 background_color;
    /// the amibient light of the scene
    Color3 ambient_light;
    /// the refraction index of air
    real_t refractive_index;

    /// Creates a new empty scene.
    Scene();

    /// Destroys this scene. Invokes delete on everything in geometries.
    ~Scene();

    // accessor functions
    Geometry* const* get_geometries() const;
    size_t num_geometries() const;
    const PointLight* get_lights() const;
    size_t num_lights() const;
    Material* const* get_materials() const;
    size_t num_materials() const;
    Mesh* const* get_meshes() const;
    size_t num_meshes() const;

    /// Clears the scene, and invokes delete on everything in geometries.
    void reset();

    // functions to add things to the scene
    // all pointers are deleted by the scene upon scene deconstruction.
    void add_geometry( Geometry* g );
    void add_material( Material* m );
    void add_mesh( Mesh* m );
    void add_light( const PointLight& l );

private:

    typedef std::vector< PointLight > PointLightList;
    typedef std::vector< Material* > MaterialList;
    typedef std::vector< Mesh* > MeshList;
    typedef std::vector< Geometry* > GeometryList;

    // list of all lights in the scene
    PointLightList point_lights;
    // all materials used by geometries
    MaterialList materials;
    // all meshes used by models
    MeshList meshes;
    // list of all geometries. deleted in dctor, so should be allocated on heap.
    GeometryList geometries;

private:

    // no meaningful assignment or copy
    Scene(const Scene&);
    Scene& operator=(const Scene&);

};

} /* _462 */

#endif /* _462_SCENE_SCENE_HPP_ */

