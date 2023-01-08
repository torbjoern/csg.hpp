# csg.hpp
 A port of the csg.js library to C++11

 original csg.js from evanw
 https://github.com/evanw/csg.js/

 arcball camera from nlguillemot
 https://github.com/nlguillemot/arcball_camera

 demo uses freeglut

 Example use:

	#include "csg.cpp"

	auto a = CSG::cube(vec3(-.25, -.25, -.25));
	auto b = CSG::sphere(vec3(.25, .25, .25), 1.3);

	a.setColor(1, 1, 0);
	b.setColor(0, 0.5, 1);
	auto c = a.subOp(b);

	// Do something with polygons in C.

For example render:

cube_sub_sphere.png