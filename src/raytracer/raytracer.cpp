/**
 * @file raytacer.cpp
 * @brief Raytracer class
 *
 * Implement these functions for project 2.
 *
 * @author H. Q. Bovik (hqbovik)
 * @bug Unimplemented
 */

#include "raytracer.hpp"
#include "scene/scene.hpp"
#include "scene/sphere.hpp"
#include "scene/triangle.hpp" 
#include <SDL/SDL_timer.h>
#include <iostream>

#define SPHERE  1.0 
#define TRIANGLE -1.0 
#define NOSHAPE  0.0
#define NOINTERSECTION 0.0
#define UNINITIALIZED -1.0

#define RED Color3(1,0,0)
#define GREEN Color3(0,1,0)
#define BLUE Color3(0,0,1)

namespace _462 {

Raytracer::Raytracer()
    : scene( 0 ), width( 0 ), height( 0 ) { }

Raytracer::~Raytracer() { }

/**
 * Initializes the raytracer for the given scene. Overrides any previous
 * initializations. May be invoked before a previous raytrace completes.
 * @param scene The scene to raytrace.
 * @param width The width of the image being raytraced.
 * @param height The height of the image being raytraced.
 * @return true on success, false on error. The raytrace will abort if
 *  false is returned.
 */
bool Raytracer::initialize( Scene* scene, size_t width, size_t height )
{
    this->scene = scene;
    this->width = width;
    this->height = height;
    this->camera = scene->camera; 
    
    // Compute basis vectors with information from camera 
    Vector3 g = camera.get_direction();
    Vector3 t = camera.get_up();  
    w = normalize(g); 
    u = normalize(cross(t,w)); 
    v = cross(w,u);
    u = -u;
    e = camera.get_position();
    //retrieve addition data for viewing frame 
    fov = camera.get_fov_radians();
    nearClip = camera.get_near_clip();
    current_row = 0;
    
    // compute bounds for the viewing frame 
    top = tan(fov/2.0)*fabs(nearClip); 
    right = ((1.0)*width/height)*top; 
    left = -right;
    bottom = -top; 

    Geometry* const* sceneObjects = scene->get_geometries();
    for(unsigned int i=0; i<scene->num_geometries(); i++){
	 Geometry* shape = sceneObjects[i]; 
	 make_inverse_transformation_matrix(&shape->inv_trans, shape->position, shape->orientation, shape->scale);
	 Matrix4 trans;
	 make_transformation_matrix(&trans, shape->position, shape->orientation, shape->scale);
         make_normal_matrix(&shape->norm_matrix,trans);
    }
    return true;
}	


/**
 * Performs a raytrace on the given pixel on the current scene.
 * The pixel is relative to the bottom-left corner of the image.
 * @param scene The scene to trace.
 * @param x The x-coordinate of the pixel to trace.
 * @param y The y-coordinate of the pixel to trace.
 * @param width The width of the screen in pixels.
 * @param height The height of the screen in pixels.
 * @return The color of that pixel in the final image.
 */


// TODO make NON STATIC trace_pixel function. 
Color3 Raytracer::trace_pixel( const Scene* scene, size_t x, size_t y, size_t width, size_t height )
{
    assert( 0 <= x && x < width );
    assert( 0 <= y && y < height );
 
    // compute s for viewing ray  
    real_t u_s = left + (right - left)*(x + 0.5)/width; 
    real_t v_s = bottom + (top - bottom) *(y + 0.5)/height; 
    Vector3 ray_dir = (u_s*u) + (v_s*v) + (nearClip*w);
    // direction of the viewing ray, normalized for unit length
    Vector3 dir_norm = normalize(ray_dir); 
    Geometry* const* sceneObjects = scene->get_geometries();
    
    // intialize values to be updated 
    // throughout the loop. 
    // in particular, we want to find a minimal time and 
    // then return the color of the pixel for the intersection
    // at that time 
    real_t minTime = UNINITIALIZED; 

    Color3 returnColor = RED; 
    Geometry* geo = sceneObjects[0];  
    for(unsigned int i=0;i<scene->num_geometries(); i++){
	Geometry* temp_geo = sceneObjects[i];
	if(temp_geo->is_intersecting(dir_norm,e,&minTime) != NOINTERSECTION){
			geo = temp_geo;
	}
	
    } 
    if(minTime != UNINITIALIZED){
	Vector3 surface_pos = e + dir_norm*minTime;
	Color3 specular = geo->get_specular();
	Vector3 normal = geo->normal_of(surface_pos);
	if(geo->get_refractive_index() != 0){
		return geo->compute_specular(scene,normal,dir_norm,surface_pos,3); 
	}
	returnColor = geo->color_at_pixel(scene,surface_pos) + specular*(geo->compute_specular(scene,normal,dir_norm,surface_pos,3));
	return returnColor;
    }
    else
	return scene->background_color; 
}

/**
 * Raytraces some portion of the scene. Should raytrace for about
 * max_time duration and then return, even if the raytrace is not copmlete.
 * The results should be placed in the given buffer.
 * @param buffer The buffer into which to place the color data. It is
 *  32-bit RGBA (4 bytes per pixel), in row-major order.
 * @param max_time, If non-null, the maximum suggested time this
 *  function raytrace before returning, in seconds. If null, the raytrace
 *  should run to completion. BECAUSE NULL == 0 [krlu]
 * @return true if the raytrace is complete, false if there is more
 *  work to be done.
 */
bool Raytracer::raytrace( unsigned char *buffer, real_t* max_time )
{
    // TODO Add any modifications to this algorithm, if needed.

    static const size_t PRINT_INTERVAL = 64;

    // the time in milliseconds that we should stop
    unsigned int end_time = 0;
    bool is_done;

    if ( max_time ) {
        // convert duration to milliseconds
        unsigned int duration = (unsigned int) ( *max_time * 1000 );
        end_time = SDL_GetTicks() + duration;
    }

    // until time is up, run the raytrace. we render an entire row at once
    // for simplicity and efficiency.
    for ( ; !max_time || end_time > SDL_GetTicks(); ++current_row ) {

        if ( current_row % PRINT_INTERVAL == 0 ) {
            printf( "Raytracing (row %lu)...\n", current_row );
        }

        // we're done if we finish the last row
        is_done = current_row == height;
        // break if we finish
        if ( is_done )
            break;

        for ( size_t x = 0; x < width; ++x ) {
            // trace a pixel
            Color3 color = trace_pixel( scene, x, current_row, width, height );
            // write the result to the buffer, always use 1.0 as the alpha
            color.to_array( &buffer[4 * ( current_row * width + x )] );
        }
    }

    if ( is_done ) {
        printf( "Done raytracing!\n" );
    }

    return is_done;
}

} /* _462 */
	
