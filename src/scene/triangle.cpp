/**
 * @file triangle.cpp
 * @brief Function definitions for the Triangle class.
 *
 * @author Eric Butler (edbutler)
 */

#include "scene/triangle.hpp"
#include "application/opengl.hpp"
#include "stdio.h"
#include "scene/scene.hpp"

// arbitrary slop factor
#define EPSILON .000001
#define NOINTERSECTION 0.0
#define UNINITIALIZED -1.0

namespace _462 {
real_t ALPHA;
real_t BETA;
real_t GAMMA; 

Vector3 camera_eye;
real_t  ray_time; 
Vector3 ray_direction;


Triangle::Triangle()
{
    vertices[0].material = 0;
    vertices[1].material = 0;
    vertices[2].material = 0;
}

Triangle::~Triangle() { }

void Triangle::render() const
{
    bool materials_nonnull = true;
    for ( int i = 0; i < 3; ++i )
        materials_nonnull = materials_nonnull && vertices[i].material;

    // this doesn't interpolate materials. Ah well.
    if ( materials_nonnull )
        vertices[0].material->set_gl_state();

    glBegin(GL_TRIANGLES);

    glNormal3dv( &vertices[0].normal.x );
    glTexCoord2dv( &vertices[0].tex_coord.x );
    glVertex3dv( &vertices[0].position.x );

    glNormal3dv( &vertices[1].normal.x );
    glTexCoord2dv( &vertices[1].tex_coord.x );
    glVertex3dv( &vertices[1].position.x);

    glNormal3dv( &vertices[2].normal.x );
    glTexCoord2dv( &vertices[2].tex_coord.x );
    glVertex3dv( &vertices[2].position.x);

    glEnd();

    if ( materials_nonnull )
        vertices[0].material->reset_gl_state();
}

/*transform vectors from object space to world space 
 *one method for point, one method for vectors*/
Vector3 Triangle::transform_vector(const Vector3 &v) const {
        return inv_trans.transform_vector(v);
}
 
Vector3 Triangle::transform_point(const Vector3 &p) const {
        return inv_trans.transform_point(p);
}

 
/* static helper function for computing diffuse
 */
real_t max(real_t a, real_t b)
{
        if(a > b)
                return a;
        else
                return b;
}
 
/* computes the attenuation factor of a light source 
 * based on its distance from the surface point
 * formula is: color/(const + lin*d + qud*d^2)) 
 * */
Color3 Triangle::attenuation(real_t &dist, const PointLight light, const Vector3 &light_pos, const Vector3 &surface_pos) const
{
        real_t constant = light.attenuation.constant;
        real_t lin = light.attenuation.linear;
        real_t quad = light.attenuation.quadratic;
        return light.color*(1.0/(constant + lin*dist + quad*pow(dist,2)));
}

/* returns the time of intersection of a shadow ray if an intersection occurs
 * otherwise return -1, implying that the surface point is illuminated by light
 */
real_t Triangle::shadow_intersection(const Vector3 &shadow_dir, const Vector3 &surface_pos) const{

	Vector3 d  = transform_vector(shadow_dir);
	Vector3 e1 = transform_point(surface_pos);  
	Vector3 a = vertices[0].position; 
	Vector3 b = vertices[1].position;
	Vector3 c = vertices[2].position;

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

	// use cramers to solve 3 by 3 linear system 
	// results are given by the formulas below. 	
	real_t  M   =  A*(EIHF) + B*(GFDI) + C*(DHEG); 
	real_t beta = (J*(EIHF) + K*(GFDI) + L*(DHEG))/M;
	real_t gamma = (I*(AKJB) + H*(JCAL) + G*(BLKC))/M; 
	real_t	 t =-(F*(AKJB) + E*(JCAL) + D*(BLKC))/M; 
 	// conditions for returning true, note that T must be within the interval [0, infinity) 
	if((t >= 0.0) && (gamma >= 0.0) && (gamma <= 1.0) && (beta >= 0.0) && (beta <= 1.0 - gamma)){	
//		printf("shadow here!!\n");	
		return t;	
	}
	return -1.0;
}

/* computes the diffuse component by iterating through all lights within the scene
 * much the like sphere. 
 * note: only difference is the barycentric coordinates are an
 * additional weighing factor in the final output
 * */
Color3 Triangle:: compute_diffuse(const Scene* scene, const Vector3 &normal,const Vector3 &surface_pos)const{
 	Color3 total_diff = Color3::Black;
        int num_lights = scene->num_lights();
	int num_shapes = scene->num_geometries(); 
	Geometry* const* sceneObjects = scene->get_geometries();
        const PointLight* lights = scene->get_lights();

        // iterate throught all lights in the scene
        for(int i=0;i<num_lights;i++){
	
		 real_t b_i = 1.0;
                 Vector3 light_pos = lights[i].position;
                 Vector3 light_vector = normalize(light_pos - surface_pos);
                 real_t dist = distance(light_pos,surface_pos);
 		 // apply slope factor of some arbitrarily small epsilon
 		 // to the surface position, we will shoot out shadow ray from here
                 Vector3 slope_pos = surface_pos + EPSILON*light_vector;
 		   
		 // check if a geometry casts a shadow, in which case 
 	         // this light contributes nothing to the diffuse color
                 for(int j=0; j<num_shapes; j++){ 
                         real_t t = sceneObjects[j]->shadow_intersection(light_vector, slope_pos);
			  // check if the intersection point is in front of the light 
                         if(t != -1.0){
                                 Vector3 geo_surface = slope_pos + light_vector*t;
                                 real_t geo_dis = distance(geo_surface,surface_pos);
                                 if(geo_dis < dist){
                                         b_i = 0.0; //light contributes nothing
                                         break;
                                 }
                         }
                 }
                 if(b_i == 1.0){
                         Color3 atten = attenuation(dist,lights[i], light_pos,surface_pos);
                         total_diff = total_diff + atten*max(dot(light_vector,normal),0);
                 }
        }     
	return total_diff;
}

/* first generates textures by interpolating the texture coordinates
 * and calling the get_texture function on each vertex
 * then interpolates the resulting textures for each vertex
 */
Color3 Triangle::compute_texture_at_vertex(real_t u, real_t v, const Material* material) const{
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

/*computes texture for entire triangle by compute texture 
 *at each vertex and then interpolating them */
Color3 Triangle::compute_texture () const{

	// each texture coordinate is of form (x,y)
	Vector2 tex_A = vertices[0].tex_coord;
	Vector2 tex_B = vertices[1].tex_coord;
	Vector2 tex_C = vertices[2].tex_coord;

	real_t tex_U = ALPHA*(tex_A.x) + BETA*(tex_B.x) + GAMMA*(tex_C.x);
	real_t tex_V = ALPHA*(tex_A.y) + BETA*(tex_B.y) + GAMMA*(tex_C.y);
	// compute the texture at each vertex
	Color3 color_A = compute_texture_at_vertex(tex_U,tex_V,vertices[0].material);	
	Color3 color_B = compute_texture_at_vertex(tex_U,tex_V,vertices[1].material);	
	Color3 color_C = compute_texture_at_vertex(tex_U,tex_V,vertices[2].material);	

	Color3 tex_color = ALPHA*color_A + BETA*color_B + GAMMA*color_C;
	return tex_color;
}

/*retruns refractive index for this triangle
 *by interpolating the ref-indices of each vertex*/
real_t Triangle::get_refractive_index() const{
	real_t refA = vertices[0].material->refractive_index;
	real_t refB = vertices[1].material->refractive_index;
	real_t refC = vertices[2].material->refractive_index;
	
	real_t bary_ref = ALPHA*refA + BETA*refB + GAMMA*refC;
	return bary_ref;  
}


/* helper functions for computing specular component of triangles
 */
Color3 Triangle::get_specular() const{
	Color3 specA = vertices[0].material->specular;
	Color3 specB = vertices[1].material->specular;
	Color3 specC = vertices[2].material->specular;
	
	Color3 bary_spec = ALPHA*specA + BETA*specB + GAMMA*specC;
        return bary_spec;
}

/*helper function for retrieving the normal at the given position
 *of this triangle*/
Vector3 Triangle::normal_of(const Vector3 &surface_pos) const{
	
	Vector3 normalA = vertices[0].normal;
	Vector3 normalB = vertices[1].normal;
	Vector3 normalC = vertices[2].normal;

	Vector3 bary_normal = normalize(norm_matrix*(ALPHA*normalA + BETA*normalB + GAMMA*normalC));
        return bary_normal;
}

real_t Triangle::compute_refraction(const real_t inner_refr, const real_t outer_refr, const Vector3 &incoming_ray,const Vector3 &normal) const{
        return 0.0;
}

/*recursively computes the specular component of this triangle
 *identically implement as that of sphere. After ray hits the surface position 
 *it will spawn a reflected ray of the surface, and potentailly a refracted ray
 *we then recursively call the specular function on those new rays*/
Color3 Triangle::compute_specular(const Scene* scene, const Vector3 &normal, 
const Vector3 &incoming_ray, const Vector3 &surface_pos, int depth) const{

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
                        // RECURSIVE CALL IS HERE!!!!
                	return tex_color*(returnColor + specular*geo->compute_specular(scene,new_norm,refl_ray,new_pos,new_depth));
         	     }    
        }    
	return tex_color*scene->background_color;
}



/* based on the ambient, and diffuse components of color 
 * and the barycentric coordinates of the point on the triangle
 * we compute the color at the given pixel coordinates
 * and then interpolate with alpha beta gamma
 * */
Color3 Triangle::color_at_pixel(const Scene* scene, const Vector3 &surface_pos) const {

	Color3 ambA = vertices[0].material->ambient;
	Color3 ambB = vertices[1].material->ambient;
	Color3 ambC = vertices[2].material->ambient;

	Color3 diffA = vertices[0].material->diffuse;
	Color3 diffB = vertices[1].material->diffuse;
	Color3 diffC = vertices[2].material->diffuse;

	Color3 bary_amb  = ALPHA*ambA + BETA*ambB + GAMMA*ambC;
	Color3 bary_diff = ALPHA*diffA + BETA*diffB + GAMMA*diffC;
	Vector3 bary_normal = normal_of(surface_pos);
	Color3 tex_color = compute_texture();
	return tex_color*(scene->ambient_light*bary_amb + bary_diff*compute_diffuse(scene,bary_normal, surface_pos));
}
 
// determines if our viewing ray intersects this given object
// abstractly speaking we construct the following system: 
// e + T(d) = a + BETA(b-a) + GAMMA(c-a)
// And solve for T, BETA, and GAMMA using cramer's rule  
real_t Triangle::is_intersecting(Vector3 &s, Vector3 &e, real_t* T) const
{
	Vector3 d  = transform_vector(s);
	Vector3 e1 = transform_point(e);  
	Vector3 a = vertices[0].position; 
	Vector3 b = vertices[1].position;
	Vector3 c = vertices[2].position;
        camera_eye = e1; 
	ray_direction = d; 	

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
	real_t  M   =  A*(EIHF) + B*(GFDI) + C*(DHEG); 
	real_t beta = (J*(EIHF) + K*(GFDI) + L*(DHEG))/M;
	real_t gamma = (I*(AKJB) + H*(JCAL) + G*(BLKC))/M; 
	real_t	 TIME =-(F*(AKJB) + E*(JCAL) + D*(BLKC))/M; 
 	// conditions for returning true, note that T must be within the interval [0, infinity) 
	if((TIME >= 0.0) && (gamma >= 0.0) && (gamma <= 1.0) && (beta >= 0.0) && (beta <= 1.0 - gamma)){		
		ray_time = TIME;
		real_t alpha = 1.0 - beta - gamma;
		// the intersection returns -1 if the time of intersection is 
		// in fact a minimal time 
		if(TIME < *T || *T == -1.0){
			ALPHA = alpha;
			BETA = beta; 
			GAMMA = gamma;
			*T = TIME;					
			return -1.0; 
		}	
	}
	return 0.0; 
}

} /* _462 */

