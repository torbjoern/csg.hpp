#pragma once

#include <algorithm> // std::for_each
#include <cassert>
#include <stdio.h>
#include <vector> // for vector
#include <deque> // to remember recursion in non-recursive version
#include <memory> // for unique_ptr
#include <math.h> // for M_PI

#include "torb_vec.h"

// This is a port of CSG.js
// Written 7. Jan 2022 by Torbjoern

namespace csghpp
{
	#define RECURSIVE 0
	static const real PI = 3.14159265358979323846f;

	struct Vertex
	{
		Vertex() : pos(0.f), normal(0.f)
		{}

		Vertex(const Vertex& o)
			: pos(o.pos)
			, normal(o.normal)
			, color(o.color)
		{
		}
		Vertex(vec3 _pos, vec3 _normal, vec3 _color = vec3(1.f, 1.f, 1.f))
			: pos(_pos)
			, normal(_normal)
			, color(_color)
		{
		}
		void flip()
		{
			normal = -normal;
		}
		Vertex interpolate(Vertex b, real t) const
		{
			return Vertex(
				lerp(pos, b.pos, t),
				lerp(normal, b.normal, t),
				lerp(color , b.color, t)
			);
		}
		vec3 pos;
		vec3 normal;
		vec3 color;
	};

	struct Plane
	{
		Plane() : normal(0.f), w(0.f)
		{
		}
		Plane(const Plane& o)
			: normal(o.normal)
			, w(o.w)
		{
		}
		Plane(vec3 _normal, real _w)
			: normal(_normal), w(_w)
		{
		}

		static Plane fromPoints(vec3 a, vec3 b, vec3 c)
		{
			vec3 n = cross(b - a, c - a).unit();
			float w = dot(n, a);
			return Plane(n, w);
		}

		bool ok() const
		{
			return normal.length() > 0.f;
		}

		void flip()
		{
			normal = -normal;
			w = -w;
		}
		vec3 normal;
		real w;
	};

	struct Polygon
	{
		// Represents a convex polygon.
		// Vertices used to initialize must be coplanar and form a convex loop.
		Polygon()
			: vertices()
			, plane(vec3(0.f), 0.f)
			, shared(0)
		{
		}

		Polygon(const std::vector<Vertex>& _vertices, int _shared = 0)
			: vertices(_vertices)
			, plane(Plane::fromPoints(_vertices[0].pos, _vertices[1].pos, _vertices[2].pos))
			, shared(_shared)
		{
		}

		void flip()
		{
			std::reverse(vertices.begin(), vertices.end());
			std::for_each(vertices.begin(), vertices.end(), [this](Vertex& v) { v.flip(); });
			plane.flip();
		}
		std::vector<Vertex> vertices;
		Plane plane;
		unsigned shared;
	};

	
void splitPolygon(
	const Plane& plane, // The splitting plane
	const Polygon& polygon,
	std::vector<Polygon>& coplanarFront,
	std::vector<Polygon>& coplanarBack,
	std::vector<Polygon>& front_polys,
	std::vector<Polygon>& back_polys)
{
	enum ePolyType {
		COPLANAR = 0,
		FRONT = 1,
		BACK = 2,
		SPANNING = 3,
	};
	const char* string2type[] = {"COPLANAR", "FRONT", "BACK", "SPAN"};

	// EPSILON is the tolerance used by 'splitPolygon' to decide 
	// if a point is on the plane.
	//static const real EPSILON = 1e-5;
	static const real EPSILON = 1e-4f;

	// Classify each point as well as the entire polygon into one of the above
	// four classes.

	//vec3 n = plane.normal;
	//printf( "Split poly with %d verts along plane{% .2f,% .2f,% .2f}. eps:%f\n", (int)polygon.vertices.size(), n.x, n.y, n.z, EPSILON);

	unsigned polygonType = 0;
	std::vector<ePolyType> types;
	ePolyType type;
	for (const Vertex& v : polygon.vertices)
	{
		real t = dot(plane.normal, v.pos) - plane.w;
		ePolyType type = (t < -EPSILON) ? BACK : ((t > EPSILON) ? FRONT : COPLANAR);
		polygonType |= type;
		types.push_back(type);
		//vec3 pos = polygon.vertices[i].pos;
		//printf("vertex %d is {% .2f,% .2f,% .2f} is %s\n", i, pos.x, pos.y, pos.z, string2type[type] );
	}
	bool isInFront = false;
	// Put the polygon in the correct list, splitting it when necessary.
	switch (polygonType) {
	case COPLANAR:
		isInFront = (plane.normal.dot(polygon.plane.normal) > 0);
		//printf("poly is co-planar because N dot P - w is 0 or almost 0. side of splitting plane: %s \n", isInFront ? "front" : "back");
		(isInFront ? coplanarFront : coplanarBack).push_back(polygon);
		break;
	case FRONT:
		//printf("Poly is in front of plane\n");
		front_polys.push_back(polygon);
		break;
	case BACK:
		//printf("Poly is in behind of plane\n");
		back_polys.push_back(polygon);
		break;
	case SPANNING:
		//printf("Poly spanning the plane\n");
		std::vector<Vertex> fverts; 
		std::vector<Vertex> bverts;
		for (unsigned i = 0; i < polygon.vertices.size(); i++) 
		{
			unsigned j = (i + 1) % polygon.vertices.size();
			ePolyType ti = types[i];
			ePolyType tj = types[j];
			const Vertex& vi = polygon.vertices[i];
			const Vertex& vj = polygon.vertices[j];
			if (ti != BACK) fverts.push_back(vi);
			if (ti != FRONT) bverts.push_back(vi);
			if ((ti | tj) == SPANNING) {
				real t = (plane.w - dot(plane.normal, vi.pos)) / dot(plane.normal, vj.pos - vi.pos);
				Vertex v = vi.interpolate(vj, t);
				fverts.push_back(v);
				bverts.push_back(v);
			}
		}
		if (fverts.size() >= 3) front_polys.push_back( Polygon(fverts));
		if (bverts.size() >= 3) back_polys.push_back( Polygon(bverts));
		break;
	}
}

	struct Node
	{
		// Holds a node in a BSP tree.
		// A BSP tree is built from a collection of polygons
		// by picking a polygon to split along. 
		// That polygon (and all other coplanar polygons) are added directly to that node
		// and the other polygons are added to the front and/or back subtrees.
		// This is not a leafy BPS tree since there is no distinction
		// between internal and leaf nodes.

		Node()
			: plane()
			, front(nullptr)
			, back(nullptr)
		{
		}

		Node(const std::vector<Polygon>& in_polygons)
			: plane()
			, front(nullptr)
			, back(nullptr)
		{
			build(in_polygons);
		}

		// Convert solid space to empty space and empty space to solid space.
		void invert()
		{
#if RECURSIVE == 1
			std::for_each(polygons.begin(), polygons.end(), [this](Polygon& p) { p.flip(); });
			plane.flip();
			if (front) front->invert();
			if (back) back->invert();
			std::swap(front, back);
#else
			std::deque<Node*> nodes;
			nodes.push_back(this);
			while (!nodes.empty())
			{
				Node* n = nodes.front();
				std::for_each(n->polygons.begin(), n->polygons.end(), [this](Polygon& p) { p.flip(); });
				n->plane.flip();
				if (n->front) nodes.push_back(n->front.get());
				if (n->back) nodes.push_back(n->back.get());
				std::swap(n->front, n->back);
				nodes.pop_front();
			}
#endif
		}

		// Put all a copy of all polygons in 'b' into 'a'
		static void concat(std::vector<Polygon>& a, const std::vector<Polygon>& b)
		{
			a.insert(a.end(), b.begin(), b.end());
		}

		// Recursively remove all polys in 'polygons' that are inside this Node
		std::vector<Polygon> clipPolygons(const std::vector<Polygon>& input)
		{
#if RECURSIVE == 1
			if (!plane.ok()) return input; // return a copy
			std::vector<Polygon> pfront;
			std::vector<Polygon> pback;
			for (const Polygon& p : input)
			{
				splitPolygon(plane, p, pfront, pback, pfront, pback);
			}
			if (front) pfront = front->clipPolygons(pfront);
			if (back) pback = back->clipPolygons(pback);
			else pback.resize(0);
			
			
			concat(pfront, pback);
			return pfront;
#else
			std::vector<Polygon> sum;
			std::deque<const Node*> nodes;
			std::deque< std::vector<Polygon> > polylists;
			nodes.push_back(this);
			polylists.push_back(input);

			while (nodes.empty() == false)
			{
				const Node* n = nodes.front();
				const std::vector<Polygon>& polys = polylists.front();
				if ( !n->plane.ok() ) {
					concat(sum, polys);
					nodes.pop_front();
					polylists.pop_front();
					continue;
				}

				std::vector<Polygon> pfront, pback;
				for (const Polygon& p : polys)
				{
					splitPolygon(n->plane, p, pfront, pback, pfront, pback);
				}
				if (n->front)
				{
					nodes.push_back(n->front.get());
					polylists.push_back(pfront);
				}
				else {
					concat(sum, pfront);
				}

				if (n->back)
				{
					nodes.push_back(n->back.get());
					polylists.push_back(pback);
				}
				nodes.pop_front();
				polylists.pop_front();
			}
			return sum;
#endif
		}

		// Remove all polygons in this BSP tree that are inside the other BSP tree 'bsp'
		void clipTo(Node& bsp)
		{
#if RECURSIVE == 1
			this->polygons = bsp.clipPolygons(this->polygons);
			if (front) front->clipTo(bsp);
			if (back) back->clipTo(bsp);
#else
		std::deque<Node*> nodes;
		nodes.push_back(this);
		while (nodes.empty() == false)
		{
			Node* n = nodes.front();
			nodes.pop_front();
			n->polygons = bsp.clipPolygons(n->polygons);
			if (n->front) nodes.push_back(n->front.get());
			if (n->back) nodes.push_back(n->back.get());
		}
#endif	
		}

		// Return a list of all polys in this BSP tree.
		std::vector<Polygon> allPolygons() const
		{
#if RECURSIVE == 1
			// Recursive
			std::vector<Polygon> sum = polygons;
			if (front) concat(sum, front->allPolygons());
			if (back) concat(sum, back->allPolygons());
			return sum;
#else
			std::vector<Polygon> sum;
			std::deque<const Node*> nodes;
			nodes.push_back(this);
			while (nodes.empty() == false)
			{
				const Node* n = nodes.front();
				nodes.pop_front();

				//sum.insert(sum.end(), n->polygons.begin(), n->polygons.end());
				concat(sum, n->polygons);
				if (n->front) nodes.push_back(n->front.get());
				if (n->back) nodes.push_back(n->back.get());
			}
			return sum;
#endif			
		}

		// Build a BSP tree out of 'polygons' polygons
		void build(std::vector<Polygon> input)
		{
#if RECURSIVE == 1
			if (input.empty()) return;
			if (!plane.ok()) plane = input[0].plane;

			std::vector<Polygon> pfront, pback;
			//printf("node %p begin.\n");

			for (Polygon& p: input)
			{
				splitPolygon(plane, p, this->polygons, this->polygons, pfront, pback);
			}
			if (pfront.empty() == false)
			{
				if (!front) front = std::unique_ptr<Node>(new Node);
				front->build(pfront);
			}
			if (pback.empty() == false)
			{
				if (!back) back = std::unique_ptr<Node>(new Node);
				back->build(pback);
			}
#else
			if (input.empty()) return;

			std::deque<Node*> nodes;
			std::deque< std::vector<Polygon> > polylists;
			nodes.push_back(this);
			polylists.push_back(input);
			while (nodes.empty() == false)
			{
				Node* n = nodes.front();
				const std::vector<Polygon>& list = polylists.front();
				assert(!list.empty() && "list of polys empty");
				if (!n->plane.ok()) n->plane = list[0].plane;
				std::vector<Polygon> pfront, pback;
				for (const Polygon& p : list)
				{
					splitPolygon( n->plane, p, n->polygons, n->polygons, pfront, pback);
				}
				if (pfront.empty() == false)
				{
					if (!n->front) {
						n->front = std::unique_ptr<Node>(new Node);
					}
					nodes.push_back(n->front.get());
					polylists.push_back(pfront);
				}
				if (pback.empty() == false)
				{
					if (!n->back) {
						n->back = std::unique_ptr<Node>(new Node);
					}
					nodes.push_back(n->back.get());
					polylists.push_back(pback);
				}
				nodes.pop_front();
				polylists.pop_front();
			}
#endif
			//printf("node %p complete\n",(void*)this);
		}

		Plane plane;
		std::unique_ptr<Node> front;
		std::unique_ptr<Node> back;
		std::vector< Polygon > polygons;
	};
	
	struct CSG {
		CSG() { }

		CSG(const std::vector<Polygon>& _polygons)
			: polygons(_polygons)
		{
		}

		CSG unionOp(const CSG& other)
		{
			Node a(polygons);
			Node b(other.polygons);
			a.clipTo(b);
			b.clipTo(a);
			b.invert();
			b.clipTo(a);
			b.invert();
			a.build(b.allPolygons());
			return CSG(a.allPolygons());
		}
		CSG subOp(const CSG& other)
		{
			Node a(polygons);
			Node b(other.polygons);
			a.invert();
			a.clipTo(b);
			b.clipTo(a);
			b.invert();
			b.clipTo(a);
			b.invert();
			a.build(b.allPolygons());
			a.invert();
			return CSG(a.allPolygons());
		}
		CSG intersectOp(const CSG& other)
		{
			Node a(polygons);
			Node b(other.polygons);
			a.invert();
			b.clipTo(a);
			b.invert();
			a.clipTo(b);
			b.clipTo(a);
			a.build(b.allPolygons());
			a.invert();
			return CSG(a.allPolygons());
		}

		static CSG cube(vec3 c = vec3(0.f), vec3 radius = 1.0f)
		{
			struct IndicesNormal
			{
				int indices[4];
				vec3 normal;
			};
			IndicesNormal data[] = {
		    { {0, 4, 6, 2}, vec3(-1, 0, 0) },
		    { {1, 3, 7, 5}, vec3(+1, 0, 0) },
		    { {0, 1, 5, 4}, vec3(0, -1, 0) },
		    { {2, 6, 7, 3}, vec3(0, +1, 0) },
		    { {0, 2, 3, 1}, vec3(0, 0, -1) },
		    { {4, 5, 7, 6}, vec3(0, 0, +1) },
			};
			
			std::vector<Polygon> polys;
			for (int face=0; face<6; face++)
			{
				vec3 normal = data[face].normal;
				std::vector<Vertex> verts;
				for (int i : data[face].indices)
				{
					vec3 pos = vec3(
						c.x + radius.x * real(2 * !!(i & 1) - 1),
						c.y + radius.y * real(2 * !!(i & 2) - 1),
						c.z + radius.z * real(2 * !!(i & 4) - 1)
					);
					verts.push_back(Vertex(pos, normal));
				}
				polys.push_back(Polygon(verts, 0));
			}

			return CSG( polys );
		}
	
	static CSG sphere(vec3 center = vec3(0.f), 
		real radius = 1.0f, int stacks = 8, int slices = 16)
	{
		auto pointOnSphere = [&](real theta, real phi)
		{
			theta *= 2.f * PI;
			phi *= 1.f * PI;
			auto dir = vec3(
				(real)cosf(theta) * sinf(phi),
				(real)cosf(phi),
				(real)sinf(theta) * sinf(phi)
			);
			return Vertex(center + (dir * radius), dir);
		};
		CSG csg;
		std::vector<Polygon>& polygons = csg.polygons;
		std::vector<Vertex> vertices;
		for (real i = 0; i < slices; i++) {
			for (real j = 0; j < stacks; j++) {
				vertices.push_back(pointOnSphere(i / slices, j / stacks));
				if (j > 0) vertices.push_back(pointOnSphere((i + 1) / slices, j / stacks));
				if (j < stacks - 1) vertices.push_back(pointOnSphere((i + 1) / slices, (j + 1) / stacks));
				vertices.push_back(pointOnSphere(i / slices, (j + 1) / stacks));
				polygons.push_back(Polygon(vertices));
				vertices.clear();
			}
		}
		return csg;

	}

	static CSG cylinder(real radius = 1.f, vec3 start = vec3(0.f-1.f,0.f), vec3 end = vec3(0.f, 1.f, 0.f) )
	{
		CSG csg;
		std::vector<Polygon>& polygons = csg.polygons;
		
		vec3 ray = end - start;
		int slices = 16;
		vec3 axisZ = ray.unit();
		bool isY = fabs(axisZ.y > 0.5f);
		vec3 axisX = cross(vec3(isY, !isY, 0), axisZ).unit();
		vec3 axisY = cross(axisX, axisZ).unit();
		Vertex vstart = Vertex(start, -axisZ);
		Vertex vend = Vertex(end, axisZ.unit());
		
		auto point = [&](int stack, real slice, real normalBlend)
		{
			real angle = slice * PI * 2.f;
			vec3 out = axisX * cosf(angle) + (axisY * sinf(angle));
			vec3 pos = start + (ray * stack) + (out * radius);
			vec3 normal = out * (1.f - fabs(normalBlend)) + (axisZ * normalBlend);
			return Vertex(pos, normal);
		};
		
		for (int i = 0; i < slices; i++)
		{
			real t0 = i / real(slices);
			real t1 = (i + 1) / real(slices);
			std::vector<Vertex> verts_a = { vstart, point(0, t0, -1.f), point(0, t1, -1.f) };
			std::vector<Vertex> verts_b = { point(0, t1, 0.f), point(0, t0, 0.f), point(1, t0, 0.f), point(1, t1, 0.f) };
			std::vector<Vertex> verts_c = { vend, point(1, t1, 1.f), point(1, t0, 1.f) };
			polygons.push_back(Polygon(verts_a));
			polygons.push_back(Polygon(verts_b));
			polygons.push_back(Polygon(verts_c));
		}
		return csg;
	}

	void setColor(float r, float g, float b)
	{
		vec3 color = vec3(r, g, b);
		for (Polygon& p : polygons)
		{
			for (Vertex& v : p.vertices)
			{
				v.color = color;
			}
		}
	}

	std::vector<Polygon> polygons;
	};

	bool operator==(const Vertex& a, const Vertex& b)
	{
		return
			a.pos.x == b.pos.x &&
			a.pos.y == b.pos.y &&
			a.pos.z == b.pos.z &&
			a.normal.x == b.normal.x &&
			a.normal.y == b.normal.y &&
			a.normal.z == b.normal.z;
	}

	struct Model
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned> index;

		unsigned addVertex(const Vertex& nv)
		{
			unsigned i = 0;
			for (const Vertex& v : vertices)
			{
				if (v == nv)
				{
					return i;
				}
				++i;
			}
			vertices.push_back(nv);
			return i;
		}
	};

	Model fromPolygons(std::vector<Polygon> polys)
	{
		Model m;
		for (const Polygon& poly : polys)
		{
			if (poly.vertices.empty()) continue;

			unsigned a = m.addVertex(poly.vertices[0]);
			for (size_t i = 2; i < poly.vertices.size(); i++)
			{
				auto b = m.addVertex( poly.vertices[i-1] );
				auto c = m.addVertex(poly.vertices[i]);
				if (a != b && b != c && c != a)
				{
					m.index.push_back(a);
					m.index.push_back(b);
					m.index.push_back(c);
				}
			}
		}
		return m;
	}

	void stats(CSG &o)
	{
		int vertex_count  = 0;
		for (Polygon& p : o.polygons)
		{
			vertex_count  += (int)p.vertices.size();
			//vec3 n = p.plane.normal;
			//printf("p plane:%.2f, %.2f, %.2f, num verts:%d\n", n.x, n.y, n.z, p.vertices.size());
		}

		printf("CSG object polys: %d, vertices:%d \n", (int)o.polygons.size(), vertex_count);
	}

}; // CSG namespace