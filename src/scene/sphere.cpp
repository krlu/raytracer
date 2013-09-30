/**
 * @file sphere.cpp
 * @brief Function defnitions for the Sphere class.
 *
 * @author Kristin Siu (kasiu)
 * @author Eric Butler (edbutler)
 */

#include "scene/sphere.hpp"
#include "application/opengl.hpp"
#include "scene/scene.hpp"
#include "stdio.h"
namespace _462 {

#define SPHERE_NUM_LAT 80
#define SPHERE_NUM_LON 100

#define SPHERE_NUM_VERTICES ( ( SPHERE_NUM_LAT + 1 ) * ( SPHERE_NUM_LON + 1 ) )
#define SPHERE_NUM_INDICES ( 6 * SPHERE_NUM_LAT * SPHERE_NUM_LON )
// index of the x,y sphere where x is lat and y is lon
#define SINDEX(x,y) ((x) * (SPHERE_NUM_LON + 1) + (y))
#define VERTEX_SIZE 8
#define TCOORD_OFFSET 0
#define NORMAL_OFFSET 2
#define VERTEX_OFFSET 5

// arbitrary slope factor
#define EPSILON 0.000001

#define NOINTERSECTION 0.0
#define UNINITIALIZED -1.0

static unsigned int Indices[SPHERE_NUM_INDICES];
static float Vertices[VERTEX_SIZE * SPHERE_NUM_VERTICES];

static void init_sphere()
{
    static bool initialized = false;
    if ( initialized )
        return;

    for ( int i = 0; i <= SPHERE_NUM_LAT; i++ ) {
        for ( int j = 0; j <= SPHERE_NUM_LON; j++ ) {
            real_t lat = real_t( i ) / SPHERE_NUM_LAT;
            real_t lon = real_t( j ) / SPHERE_NUM_LON;
            float* vptr = &Vertices[VERTEX_SIZE * SINDEX(i,j)];

            vptr[TCOORD_OFFSET + 0] = lon;
            vptr[TCOORD_OFFSET + 1] = 1-lat;

            lat *= PI;
            lon *= 2 * PI;
            real_t sinlat = sin( lat );

            vptr[NORMAL_OFFSET + 0] = vptr[VERTEX_OFFSET + 0] = sinlat * sin( lon );
            vptr[NORMAL_OFFSET + 1] = vptr[VERTEX_OFFSET + 1] = cos( lat ),
            vptr[NORMAL_OFFSET + 2] = vptr[VERTEX_OFFSET + 2] = sinlat * cos( lon );
        }
    }

    for ( int i = 0; i < SPHERE_NUM_LAT; i++ ) {
        for ( int j = 0; j < SPHERE_NUM_LON; j++ ) {
            unsigned int* iptr = &Indices[6 * ( SPHERE_NUM_LON * i + j )];

            unsigned int i00 = SINDEX(i,  j  );
            unsigned int i10 = SINDEX(i+1,j  );
            unsigned int i11 = SINDEX(i+1,j+1);
            unsigned int i01 = SINDEX(i,  j+1);

            iptr[0] = i00;
            iptr[1] = i10;
            iptr[2] = i11;
            iptr[3] = i11;
            iptr[4] = i01;
            iptr[5] = i00;
        }
    }

    initialized = true;
}

Sphere::Sphere()
    : radius(0), material(0) {}

Sphere::~Sphere() {}

void Sphere::render() const
{
    // create geometry if we haven't already
    init_sphere();

    if ( material )
        material->set_gl_state();

    // just scale by radius and draw unit sphere
    glPushMatrix();
    glScaled( radius, radius, radius );
    glInterleavedArrays( GL_T2F_N3F_V3F, VERTEX_SIZE * sizeof Vertices[0], Vertices );
    glDrawElements( GL_TRIANGLES, SPHERE_NUM_INDICES, GL_UNSIGNED_INT, Indices );
    glPopMatrix();

    if ( material )
        material->reset_gl_state();
}


/* transformation helper functions 
 * creates a transformation matrix 
 * and invokes different transformation 
 * functions for points and vectors
 * */
Vector3 Sphere::transform_vector(const Vector3 &v) const {
	return inv_trans.transform_vector(v);
}

Vector3 Sphere::transform_point(const Vector3 &p) const {
	return inv_trans.transform_point(p);
}


/* static helper function for computing diffuse
 */
real_t max_of(real_t a, real_t b)
{
	if(a > b)
		return a; 
	else
		return b;
}

/* generates mapped 2D coordinates and mods by the the
 * width and height of the texture. then passes results
 * into the texture function for the sphere.
 */
Color3 Sphere::compute_texture(const Vector3 &normal) const{
	real_t THETA = acos(normal.y);
	real_t PHI   = atan2(normal.x, normal.z); 
	real_t u = PHI/(2.0*PI);
	real_t v = (PI-THETA)/PI;

	int width,height;
	material->get_texture_size(&width,&height);
	if(width == 0 || height == 0){
		return Color3::White;
	}
	int mod_U = ((int)(width*u)) % width;
        int mod_V = ((int)(height*v)) % height;
 
 	Color3 texture_color = material->get_texture_pixel(mod_U,mod_V);	
	return texture_color;
}

/* computes the attenuation factor of a light source 
 * based on its distance from the surface point 
 * */
Color3 Sphere::attenuation(real_t dist, const PointLight light, const Vector3 light_pos, const Vector3 surface_pos)const {
	real_t constant = light.attenuation.constant;
	real_t lin = light.attenuation.linear;
	real_t quad = light.attenuation.quadratic;
	return light.color*(1.0/(constant + lin*dist + quad*pow(dist,2)));
}


/* returns the time of intersection of a shadow ray if an intersection occurs
 * otherwise return -1, implying that the surface point is illuminated by light
 */
real_t Sphere::shadow_intersection(const Vector3 &shadow_dir, const Vector3 &surface_pos) const
{
	const Vector3 &E = transform_point(surface_pos);
	const Vector3 &d = transform_vector(shadow_dir);
	const Vector3 &c = transform_point(position);
	Vector3 ec = E-c;
	real_t product_ec = dot(ec,ec);
	real_t product_dd = dot(d,d); 
	real_t discriminant = dot(d,ec)*dot(d,ec) - product_dd*(product_ec - radius*radius);
       
	if (discriminant >= 0){
		// using a variant of the quadratic formula to find values for T 
		// we take the minimum of T1,T2 or just T1 when T2 is negative
		real_t T1 = (dot(-d, ec) + sqrt(discriminant))/product_dd; 	
		real_t T2 = (dot(-d, ec) - sqrt(discriminant))/product_dd;
		
		if(T1 <= 0.0) 
			return -1.0;  	
		else if(T2 <= 0.0){ 
			return T1;
		}
		else {
			return T2;
		}
	}	
	return -1.0;		
}

/* utilizes the summation formula for computing the diffuse color
 * */ 
Color3 Sphere::compute_diffuse(const Scene* scene, Vector3 normal, Vector3 surface_pos) const {

	Color3 total_diff = Color3::Black;
	int num_lights = scene->num_lights();
	const PointLight* lights = scene->get_lights();
	Geometry* const* sceneObjects = scene->get_geometries();
	// iterate throught all lights in the scene
	for(int i=0;i<num_lights;i++){
		real_t b_i = 1.0;
		Vector3 light_pos = lights[i].position;
		Vector3 light_vector = normalize(light_pos - surface_pos);
		real_t dist = distance(light_pos,surface_pos);

		Vector3 slope_pos = surface_pos + EPSILON*light_vector;

		for(unsigned int j=0;j<scene->num_geometries(); j++){	

			real_t t = sceneObjects[j]->shadow_intersection(light_vector, slope_pos);
			
			if(t != -1.0){
				// check if the intersection point is in front of the light
				Vector3 geo_surface = slope_pos + light_vector*t; 
				real_t geo_dis = distance(geo_surface,surface_pos);  
				if(geo_dis < dist){
					//if intersection with geometry is in front of light
					b_i = 0.0; 
					break; 	
				}
			}
		}
		if(b_i == 1.0){
			Color3 atten = attenuation(dist,lights[i], light_pos,surface_pos);	
			total_diff = total_diff + atten*max_of(dot(light_vector,normal),0); 		  	 
		}
	}
	return total_diff;	
}

real_t Sphere::compute_refraction(const real_t n, const real_t nt, const Vector3 &ray, const Vector3 &normal) const{
	//assumes vectors are normalized
	real_t c = dot(ray,normal);
	real_t R_0 = pow((nt-1)/(nt+1),2);
	real_t R = R_0 + (1-R_0)*(pow(1.0+c,5));
	return R;	
}

Vector3 Sphere::compute_refr_ray(const real_t n, const real_t nt,const Vector3 &normal, const Vector3 &dir) const{
	real_t nsq = pow(n,2); 
	real_t dn = 1 - pow(dot(dir,normal),2); 
	real_t discriminant = 1 - (nsq*dn)/pow(nt,2);
	if(discriminant < 0){
		return Vector3(0,0,0); 
	}
	else{
		Vector3 first_term = n*(dir - normal*dot(dir,normal))/nt;
		return normalize(first_term - normal*sqrt(discriminant));
	} 
}

/* recursively computes the specular component for spheres
 * After ray hits the surface position 
 * it will spawn a reflected ray of the surface, and potentailly a refracted ray
 * we then recursively call the specular function on those new rays*/
Color3 Sphere::compute_specular(const Scene* scene, const Vector3 &normal, const Vector3 &incoming_ray,
						    const Vector3 &surface_pos, int depth) const{	

	Geometry* const* sceneObjects = scene->get_geometries();
	real_t product = dot(incoming_ray,normal);
	Vector3 refl_ray;
	if(product < 0)
		refl_ray = normalize(incoming_ray - 2*dot(incoming_ray,normal)*normal);
	else
		refl_ray = normalize(incoming_ray + 2*dot(incoming_ray,-normal)*normal);

	Vector3 slop_pos = surface_pos + EPSILON*refl_ray;
	real_t minTime = UNINITIALIZED;
	Geometry* geo1 = sceneObjects[0];
	Geometry* geo2 = sceneObjects[0];

	Color3 tex_color = compute_texture(normal);
	for(unsigned int i=0;i<scene->num_geometries(); i++){
	        Geometry* temp_geo = sceneObjects[i];
	        if(temp_geo->is_intersecting(refl_ray,slop_pos,&minTime) != NOINTERSECTION){
	                geo1 = temp_geo;
	        }
     	}
	real_t R = 1; 
	int new_depth = depth - 1;
	Vector3 refr_ray;
	
	real_t curr_refr = get_refractive_index(); 
	real_t min_ref_time = UNINITIALIZED;
	Color3 refr_color;
	Color3 refl_color;	
	if(curr_refr != 0){
		if(product < 0){
			// computes the fresnel coefficient
			refr_ray = compute_refr_ray(1,curr_refr,normal,incoming_ray);	
			R = compute_refraction(1,curr_refr,refr_ray,normal); 
		}
		else{
			refr_ray = compute_refr_ray(curr_refr,1,normal, incoming_ray);
			R = compute_refraction(curr_refr,1,-incoming_ray,normal); 
		}
		/*check for total internal reflection*/
		if(length(refr_ray) != 0){
			Vector3 refr_slop_pos = surface_pos + EPSILON*refr_ray;
			for(unsigned int i=0;i<scene->num_geometries(); i++){
        			Geometry* temp_geo = sceneObjects[i];
			       	if(temp_geo->is_intersecting(refr_ray,refr_slop_pos,&min_ref_time) != NOINTERSECTION){
	               			geo2 = temp_geo;
	       		 	}	
			}
			if(min_ref_time != UNINITIALIZED){	
				Vector3 refr_surf_pos = surface_pos + refr_ray*min_ref_time;
				Vector3 new_refr_norm = geo2->normal_of(refr_surf_pos);
				Color3 temp_color = geo2->color_at_pixel(scene,refr_surf_pos);
				if(depth > 1){	
					Color3 refr_spec = geo2->get_specular();
					// RECURSION HERE
					refr_color = tex_color*(temp_color + refr_spec*geo2
					->compute_specular(scene,new_refr_norm,refr_ray,refr_surf_pos,new_depth));
				}
				else
					refl_color = Color3::Black;
			}
			else{
				refr_color = tex_color*scene->background_color;
			}	
		}
		else{
			R = 1;
		}
     	}
									
	if(minTime != UNINITIALIZED){
        	Vector3 new_pos = surface_pos + refl_ray*minTime;
		Vector3 new_norm = geo1->normal_of(new_pos);
        	 Color3 temp_color = geo1->color_at_pixel(scene,new_pos);
		if(depth > 1){ 	
			Color3 specular = geo1->get_specular();	
			// RECURSION HERE
			refl_color = tex_color*(temp_color + specular*geo1->compute_specular(scene,new_norm,refl_ray,new_pos,new_depth));
	  	}
		else
			refl_color = tex_color*temp_color;		
     	}
	else{	
  		refl_color = tex_color*scene->background_color;
	}
	if(R == 1)
		return refl_color;

	return R*refl_color + (1 - R)*refr_color;
}


/*returns refractive index of this sphere*/
real_t Sphere::get_refractive_index() const{
	return material->refractive_index;
}

/*helper functions for computing specular*/
Color3 Sphere::get_specular() const{
	return material->specular;
}
/*helper function for computing normal
 * with respect to surface position
 */
Vector3 Sphere::normal_of(const Vector3 &surface_pos) const{

	Vector3 trans_s_pos = transform_point(surface_pos);  
	const Vector3 &center =  transform_point(position); 

	// compute the normal vector with respect to the surface position
	Vector3 normal = normalize(norm_matrix*((trans_s_pos - center)/radius));         
	return normal;
}
 
/* Returns the color at the given pixel 
 * by computing the sum of all diffuse lights and ambient light 
 */
Color3 Sphere::color_at_pixel(const Scene *scene, const Vector3 &surface_pos) const 
{
	const Color3 &diffuse =  material->diffuse;
	const Color3 &ambient =  material->ambient;

	// compute the normal vector with respect to the surface position
	Vector3 normal = normal_of(surface_pos);         
	
	// compute appropriate map for textures 
	Color3 texture_color = compute_texture(normal);	
	
	return texture_color*((scene->ambient_light)*ambient + diffuse*compute_diffuse(scene,normal,surface_pos));
}


/* Checks for solutions to the following equation:
 * (e + td - c)*(e + td - c) - R^2 = 0 
 * which is a quadratic for t and thus we only need check
 * if the discriminant: 
 * (d*(e-c)^2 - (d*d)((e-c)*(e-c) - R^2) >= 0 
 * NOTE: * notation indicates dot product in the context of 
 * this documentation, we call dot(x,y)  to represent the 
 * dot product of vectors x and y 
 *  s is the directional vector, e is the camera eye starting point 
 *  */
real_t Sphere::is_intersecting(Vector3 &s, Vector3 &e, real_t *T) const
{
	Vector3 D = s; // the viewing ray 

	// transform the viewing ray, the eye, and the center of the sphere 
	// center of sphere is the position of geometry here
	const Vector3 &E = transform_point(e);
	const Vector3 &d = transform_vector(D);
	const Vector3 &c = transform_point(position);

	Vector3 ec = E-c;
	real_t product_ec = dot(ec,ec);
	real_t product_dd = dot(d,d); 
	real_t discriminant = dot(d,ec)*dot(d,ec) - product_dd*(product_ec - radius*radius);
       
        real_t local_min;	
	if (discriminant >= 0){
		// using a variant of the quadratic formula to find values for T 
		// we take the minimum of T1,T2 or just T1 when T2 is negative
		real_t T1 = (dot(-d, ec) + sqrt(discriminant))/product_dd; 	
		real_t T2 = (dot(-d, ec) - sqrt(discriminant))/product_dd;
		if(T1 <= 0.0) 
			return 0.0;  	
		else if(T2 <= 0.0) 
			local_min = T1;
		else 
			local_min = T2;
		// return 1 and update globals when time is minimal
		// otherwise we ignore this intersection with the sphere	
		if(local_min < *T || *T == -1){
			*T = local_min;
			return 1.0;
		}
	}
	return 0.0;	 
}

} /* _462 */

















