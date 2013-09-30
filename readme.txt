
Describe the required features you have implemented, and which you have not.
if you have missing features, be sure to explain how much you accomplished.
Describe any major design decisions and descriptions of your algorithms.
You should detail anything you think we need to know to understand your code.

General algorithm for raytracer: 

Iterate through each pixel on the screen and for each pixel do the following: 

1.) Computing viewing ray for pixel 
2.) Iterate through geometries to check if ray intersects the geometry 
    If no intersection, then return the background color. 
    If there is an intersection the see algorithm below for color at geometry 

Algorithm for computing color at pixel with intersected geometry: 

color at the pixel is given by the following sum: 
ambient_component + diffuse_component + specular_component 

Computing ambient component: 
ambient light*ambient color of geometry at intersected point 

Computing diffuse component: 
sum up diffuse colors over all light sources within scene
do not add the cases where lightrays intersect another geometry, in which 
case we have a shadow cast. 

Computing specular component:
When viewing ray hits a geometry, we compute and shoot out a reflected ray 
and recursively run the ray tracing algorithm on the new reflected ray
if possible, compute and shoot out a refracted ray and recursively run 
the algorithm on that ray as well. 

Note: computing color components varies based on the type of geometry
thus, virtual functions implemented separately for Spheres, Triangle,
and Models. However, model's are a mesh of triangles, so we simply extend 
the behavior of the triangle code to implement that for Models



