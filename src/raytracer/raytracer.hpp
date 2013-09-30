/**
 * @file raytacer.hpp
 * @brief Raytracer class
 *
 * Implement these functions for project 2.
 *
 * @author H. Q. Bovik (hqbovik)
 * @bug Unimplemented
 */

#ifndef _462_RAYTRACER_HPP_
#define _462_RAYTRACER_HPP_

#include "math/color.hpp"
#include "math/vector.hpp"
#include "math/camera.hpp"

namespace _462 {

class Scene;

class Raytracer
{
public:

    Raytracer();

    ~Raytracer();

    bool initialize( Scene* scene, size_t width, size_t height ); 
    Color3 trace_pixel(const Scene* scene, size_t x, size_t y,size_t width, size_t height);
    bool raytrace( unsigned char* buffer, real_t* max_time );

private:

    // the scene to trace
    Scene* scene;
   
    // u,v,w the basis vectors. e the camera eye 
    Vector3 u,v,w,e; 
    real_t fov, nearClip; 
    real_t top, right, bottom, left;
    Camera camera; 
    
    // the dimensions of the image to trace
    size_t width, height;

    // the next row to raytrace
    size_t current_row;
};

} /* _462 */

#endif /* _462_RAYTRACER_HPP_ */

