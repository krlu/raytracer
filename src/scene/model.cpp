/**
 * @file model.cpp
 * @brief Model class
 *
 * @author Eric Butler (edbutler)
 * @author Zeyang Li (zeyangl)
 */

#include "scene/model.hpp"
#include "scene/material.hpp"
#include <GL/gl.h>
#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>

//arbitrary slop factor
#define EPSILON .000001
#define NOINTERSECTION 0.0
#define UNINITIALIZED -1.0

namespace _462 {

Model::Model() : mesh( 0 ), material( 0 ) { }
Model::~Model() { }

MeshTriangle min_triangle;
real_t ALPH; 
real_t BET; 
real_t GAM;

void Model::render() const
{
    if ( !mesh )
        return;
    if ( material )
        material->set_gl_state();
    mesh->render();
    if ( material )
        material->reset_gl_state();
}

/*helper methods for transform vectors from object space 
 *to world space. One for transforming vector, and one for 
 *transforming point*/
Vector3 Model::transform_vector(const Vector3 &v) const{
	//create a transformation matrix for instancing 
	return inv_trans.transform_vector(v);
}

Vector3 Model::transform_point(const Vector3 &v) const{
	//create a transformation matrix for instancing 
        return inv_trans.transform_point(v);  
}

/* helper function for computing diffuse
 */
real_t Model:: max(const real_t a, const real_t b) const
{
        if(a > b)
                return a;
        else
                return b;
}
 
/* computes the attenuation factor of a light source 
 * based on its distance from the surface point
 *  formula is: color/(const + lin*d + qud*d^2))
 */
Color3 Model::attenuation(real_t &dist, const PointLight light, const Vector3 &light_pos, const Vector3 &surface_pos) const
{
        real_t constant = light.attenuation.constant;
        real_t lin = light.attenuation.linear;
        real_t quad = light.attenuation.quadratic;
        return light.color*(1.0/(constant + lin*dist + quad*pow(dist,2)));
}

/*identically implemented as that of triangle*/
real_t Model::shadow_intersect_triangle(const MeshTriangle &triangle, const Vector3 &shadow_dir, const Vector3 &surface_pos) const{
	 const MeshVertex* vertices = mesh->get_vertices();
         Vector3 d  = transform_vector(shadow_dir);

         Vector3 e1 = transform_point(surface_pos);
 
         Vector3 a = vertices[triangle.vertices[0]].position;
         Vector3 b = vertices[triangle.vertices[1]].position;
         Vector3 c = vertices[triangle.vertices[2]].position;
 
         // we construct each of the entries of the matrix
         // of a linear system 
         real_t A = a.x - b.x;
         real_t B = a.y - b.y;
         real_t C = a.z - b.z;
 
         real_t D = a.x - c.x;
         real_t E = a.y - c.y;
         real_t F = a.z - c.z;
  
         real_t G = d.x;
         real_t H = d.y;
         real_t I = d.z;
 
         real_t J = a.x - e1.x;
         real_t K = a.y - e1.y;
         real_t L = a.z - e1.z;
         // store results of arithemtic to save operations 
         real_t EIHF = E*I - H*F;
         real_t GFDI = G*F - D*I;
         real_t DHEG = D*H - E*G;
 
         real_t AKJB = A*K - J*B;
         real_t JCAL = J*C - A*L;
         real_t BLKC = B*L - K*C;
 
         // use cramers rule to solve 3 by 3 linear system 
         // results are given by the formulas below.     
         real_t  M    =  A*(EIHF) + B*(GFDI) + C*(DHEG);
         real_t beta  = (J*(EIHF) + K*(GFDI) + L*(DHEG))/M;
         real_t gamma = (I*(AKJB) + H*(JCAL) + G*(BLKC))/M;
         real_t  time =-(F*(AKJB) + E*(JCAL) + D*(BLKC))/M;
         // conditions for returning true, note that T must be within the interval [0, infinity) 
         if((time >= 0.0) && (gamma >= 0.0) && (gamma <= 1.0) && (beta >= 0.0) && (beta <= 1.0 - gamma)){
		return time;
	}
	return -1.0;

}
/*similar to shadow intersection for triangles, with extended behavior 
 *in that it iterates through a mesh a triangles within this model*/
real_t Model::shadow_intersection(const Vector3 &shadow_dir, const Vector3 &surface_pos) const{
	real_t min_time = -1.0;
	const MeshTriangle* triangles = mesh->get_triangles();
 	for(unsigned int i=0; i<mesh->num_triangles(); i++){
		real_t time = shadow_intersect_triangle(triangles[i],shadow_dir,surface_pos); 
		if(time != -1.0){
			if(time < min_time || min_time == -1.0){
				min_time = time;
			}
		}
	}		
	return min_time;
}


/*computes the diffuse component of the of the model, exactly like triangle*/
Color3 Model:: compute_diffuse(const Scene* scene, const Vector3 &normal, const Vector3 &surface_pos) const{ 

	Color3 total_diff = Color3::Black;//initialize the color to black, then we start adding to it 
	int num_lights = scene->num_lights();
	int num_shapes = scene->num_geometries();
	Geometry* const* sceneObjects = scene->get_geometries();
	const PointLight* lights = scene->get_lights();

	// iterate through all lights to determine their contributions to the diffuse 
	for(int i=0;i<num_lights;i++){
		Vector3 light_pos = lights[i].position;
		Vector3 light_vector = normalize(light_pos - surface_pos); 
		real_t dist = distance(light_pos, surface_pos);	
		Vector3 slope_pos = surface_pos + EPSILON*light_vector;	
		real_t b_i = 1.0;
		for(int j=0; j<num_shapes; j++){
			//if geometry is a model, we consider the closest model by time
			real_t t = sceneObjects[j]->shadow_intersection(light_vector, slope_pos);	
			if(t != -1.0){
                                 Vector3 geo_surface = slope_pos + light_vector*t;
                                 real_t geo_dis = distance(geo_surface,surface_pos);
	  			 if( geo_dis < dist){
				        b_i = 0.0; //light contributes nothing
                                        break;
                                 }
                         }

		}
		if(b_i == 1.0){
			Color3 atten = attenuation(dist, lights[i], light_pos, surface_pos);
			total_diff+= atten*max(dot(light_vector,normal),0.0); 
		}
	}	

	return total_diff;
}
/* emulates the textures algorithm for the triangle 
 * note: the only discrepancy is we are only considering 
 * the texture of the "minial" (i.e closest) triangle
 */
Color3 Model:: compute_texture_at_vertex(real_t u, real_t v) const{
	int width,height;
        material->get_texture_size(&width,&height);
        if(width == 0 || height == 0){
                return Color3::White;
        }
        real_t mod_U = ((int)(width*u)) % width;
        real_t mod_V = ((int)(height*v)) % height;
 
        Color3 tex_color = material->get_texture_pixel(mod_U,mod_V);
        return tex_color;
}

/*returns the color of the texture of the model
 *by first computing the texutre coordintaes*/
Color3 Model:: compute_texture () const{

	const MeshVertex* vertices = mesh->get_vertices();

	Vector2 tex_A = vertices[min_triangle.vertices[0]].tex_coord;
        Vector2 tex_B = vertices[min_triangle.vertices[1]].tex_coord;
        Vector2 tex_C = vertices[min_triangle.vertices[2]].tex_coord;

	real_t tex_U = ALPH*(tex_A.x) + BET*(tex_B.x) + GAM*(tex_C.x);
        real_t tex_V = ALPH*(tex_A.y) + BET*(tex_B.y) + GAM*(tex_C.y);

	// no need to interpolate in model, because 
	// the material is globalized to the entire object
	// not just each vertex
        Color3 tex_color = compute_texture_at_vertex(tex_U, tex_V);
        return tex_color;

}


real_t Model::compute_refraction(const real_t inner_refr, const real_t outer_refr, const Vector3 &incoming_ray,const Vector3 &normal) const{
        return 0.0;
}

/*returns refractive index for entire mesh*/
real_t Model::get_refractive_index() const{
	return material->refractive_index;
}
/*returns specular constant for entire Model*/
Color3 Model::get_specular() const{
        return material->specular;
}


/*computes the normal with respect to surface position
 *by interpolating the normals of the triangle 
 *there's no need to pass in the surface position for the 
 *model, it is only to obey the virtual function prototype definition*/
Vector3 Model::normal_of(const Vector3 &surface_pos) const{

	const MeshVertex* vertices = mesh->get_vertices();

        Vector3 normalA = vertices[min_triangle.vertices[0]].normal;
        Vector3 normalB = vertices[min_triangle.vertices[1]].normal;
        Vector3 normalC = vertices[min_triangle.vertices[2]].normal;

        Vector3 bary_normal = normalize(norm_matrix*(ALPH*normalA + BET*normalB + GAM*normalC));
        return bary_normal;
}

/*recursively computes the specular component of this triangle
 *identically implement as that of sphere. After ray hits the surface position 
 *it will spawn a reflected ray of the surface, and potentailly a refracted ray
 *we then recursively call the specular function on those new rays*/
Color3 Model::compute_specular(const Scene* scene, const Vector3 &normal, const Vector3 &incoming_ray, const Vector3 &surface_pos, int depth) const{

        Geometry* const* sceneObjects = scene->get_geometries();
        Vector3 refl_ray = normalize(incoming_ray - 2*dot(incoming_ray,normal)*normal);
        Vector3 slop_pos = surface_pos + EPSILON*refl_ray;
        real_t minTime = UNINITIALIZED;
        Geometry* geo = sceneObjects[0];
        Color3 tex_color = compute_texture();
        for(unsigned int i=0;i<scene->num_geometries(); i++){
        Geometry* temp_geo = sceneObjects[i];
                 if(temp_geo->is_intersecting(refl_ray,slop_pos,&minTime) != NOINTERSECTION){
                         geo = temp_geo;
                 }   
 
        }   
        if(minTime != UNINITIALIZED){
                Vector3 new_pos = surface_pos + refl_ray*minTime;
                Color3 returnColor = geo->color_at_pixel(scene,new_pos);
                if(depth == 1)  
                        return returnColor;
                else{
                        int new_depth = depth-1;
                        Color3 specular = geo->get_specular();
                        Vector3 new_norm = geo->normal_of(new_pos);
			// RECURSIVE CALL IS HERE!!!
                        return tex_color*(returnColor + specular*geo->compute_specular(scene,new_norm,refl_ray,new_pos,new_depth));
                     }        
        }                
        return tex_color*scene->background_color;
}

/* outputs the color of the triangle we are currently intersecting 
 * much like that of triangle.cpp
 */
Color3 Model::color_at_pixel(const Scene* scene,const Vector3 &surface_pos) const{

	const Color3 &diffuse =  material->diffuse;
        const Color3 &ambient =  material->ambient;
      
	Vector3 bary_norm = normal_of(surface_pos);
        Color3 tex_color = compute_texture();    
        return tex_color*((scene->ambient_light)*ambient + diffuse*compute_diffuse(scene, bary_norm, surface_pos));
}

/* iterates through every triangle in the model's mesh
 * and performs the triangle intersection test 
 * and returns a non-zero number if the intersection time 
 * is minimal. Otherwise, ignore this intersection 
 */
real_t Model::is_intersecting(Vector3 &s, Vector3 &e, real_t *T) const
{
	real_t has_intersection = 0.0; 
	const MeshTriangle* triangles = mesh->get_triangles();
 	for(unsigned int i=0; i<mesh->num_triangles(); i++){
		real_t time = intersects_triangle(triangles[i],s,e,T); 
		if(time != -1){
			has_intersection = 1.0;
		}
	}		
	return has_intersection;
}

/* standard triangle intersection as seen in triangle.cpp 
 * utilizes cramer's rule
 */
real_t Model::intersects_triangle(const MeshTriangle &triangle, const Vector3 &s, const Vector3 &e, real_t *T) const{

	 const MeshVertex* vertices = mesh->get_vertices();
         Vector3 d  = transform_vector(s);

         Vector3 e1 = transform_point(e);
 
         Vector3 a = vertices[triangle.vertices[0]].position;
         Vector3 b = vertices[triangle.vertices[1]].position;
         Vector3 c = vertices[triangle.vertices[2]].position;
 
         // we construct each of the entries of the matrix
         // of a linear system 
         real_t A = a.x - b.x;
         real_t B = a.y - b.y;
         real_t C = a.z - b.z;
 
         real_t D = a.x - c.x;
         real_t E = a.y - c.y;
         real_t F = a.z - c.z;
  
         real_t G = d.x;
         real_t H = d.y;
         real_t I = d.z;
 
         real_t J = a.x - e1.x;
         real_t K = a.y - e1.y;
         real_t L = a.z - e1.z;
         // store results of arithemtic to save operations 
         real_t EIHF = E*I - H*F;
         real_t GFDI = G*F - D*I;
         real_t DHEG = D*H - E*G;
 
         real_t AKJB = A*K - J*B;
         real_t JCAL = J*C - A*L;
         real_t BLKC = B*L - K*C;
 
         // use cramers rule to solve 3 by 3 linear system 
         // results are given by the formulas below.     
         real_t  M    =  A*(EIHF) + B*(GFDI) + C*(DHEG);
         real_t beta  = (J*(EIHF) + K*(GFDI) + L*(DHEG))/M;
         real_t gamma = (I*(AKJB) + H*(JCAL) + G*(BLKC))/M;
         real_t  time =-(F*(AKJB) + E*(JCAL) + D*(BLKC))/M;
         // conditions for returning true, note that T must be within the interval [0, infinity) 
         if((time >= 0.0) && (gamma >= 0.0) && (gamma <= 1.0) && (beta >= 0.0) && (beta <= 1.0 - gamma)){
		real_t alpha = 1.0 - gamma - beta; 
		if(time < *T || *T == -1) 
		{
			min_triangle = triangle;
			ALPH = alpha;
			BET = beta; 
			GAM = gamma;
			*T = time; 
			return 1.0; 
		}
	}
	return -1.0;
}


} /* _462 */









