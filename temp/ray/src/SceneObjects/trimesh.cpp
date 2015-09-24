#include <cmath>
#include <float.h>
#include <algorithm>
#include <assert.h>
#include "trimesh.h"
#include "../ui/TraceUI.h"
extern TraceUI* traceUI;

using namespace std;

Trimesh::~Trimesh()
{
	for( Materials::iterator i = materials.begin(); i != materials.end(); ++i )
		delete *i;
}

// must add vertices, normals, and materials IN ORDER
void Trimesh::addVertex( const Vec3d &v )
{
    vertices.push_back( v );
}

void Trimesh::addMaterial( Material *m )
{
    materials.push_back( m );
}

void Trimesh::addNormal( const Vec3d &n )
{
    normals.push_back( n );
}

// Returns false if the vertices a,b,c don't all exist
bool Trimesh::addFace( int a, int b, int c )
{
    int vcnt = vertices.size();

    if( a >= vcnt || b >= vcnt || c >= vcnt ) return false;

    TrimeshFace *newFace = new TrimeshFace( scene, new Material(*this->material), this, a, b, c );
    newFace->setTransform(this->transform);
    if (!newFace->degen) faces.push_back( newFace );


    // Don't add faces to the scene's object list so we can cull by bounding box
    // scene->add(newFace);
    return true;
}

char* Trimesh::doubleCheck()
// Check to make sure that if we have per-vertex materials or normals
// they are the right number.
{
    if( !materials.empty() && materials.size() != vertices.size() )
        return "Bad Trimesh: Wrong number of materials.";
    if( !normals.empty() && normals.size() != vertices.size() )
        return "Bad Trimesh: Wrong number of normals.";

    return 0;
}

bool Trimesh::intersectLocal(ray& r, isect& i) const
{
	double tmin = 0.0;
	double tmax = 0.0;
	typedef Faces::const_iterator iter;
	bool have_one = false;
	for( iter j = faces.begin(); j != faces.end(); ++j )
	  {
	    isect cur;
	    if( (*j)->intersectLocal( r, cur ) )
	      {
		if( !have_one || (cur.t < i.t) )
		  {
		    i = cur;
		    have_one = true;
		  }
	      }
	  }
	if( !have_one ) i.setT(1000.0);
	return have_one;
}

bool TrimeshFace::intersect(ray& r, isect& i) const {
  return intersectLocal(r, i);
}

// Intersect ray r with the triangle abc.  If it hits returns true,
// and put the parameter in t and the barycentric coordinates of the
// intersection in u (alpha) and v (beta).
bool TrimeshFace::intersectLocal(ray& r, isect& i) const
{
    const Vec3d& a = parent->vertices[ids[0]];
    const Vec3d& b = parent->vertices[ids[1]];
    const Vec3d& c = parent->vertices[ids[2]];

    // YOUR CODE HERE
    Vec3d n = (b-a).cross(c-a);
    n.normalize();

    double planeConst = n.dot(a);

    if( n.dot(r.getDirection()) == 0 )
    {
        //parallel to plane
        return false;
    }

    double t = (planeConst - n.dot(r.getPosition())) / (n.dot(r.getDirection()));

    if(t <= RAY_EPSILON)
        return false;

    Vec3d q = r.at(t);

    double x = ((b-a).cross(q-a)).dot(n);
    double y = ((c-b).cross(q-b)).dot(n);
    double z = ((a-c).cross(q-c)).dot(n);
    
    if(x >= 0 && y >= 0 && z >= 0)
    {
        double l = ((b-a).cross(c-a)).dot(n);
        Vec3d baryCoords(y/l, z/l, x/l);


        if (traceUI->smShadSw() && !parent->normals.empty()) 
        {
            // ... do normals interpolation
            Vec3d nA(parent->normals[ids[0]][0]*baryCoords[0], parent->normals[ids[0]][1]*baryCoords[0], parent->normals[ids[0]][2]*baryCoords[0]);
            Vec3d nB(parent->normals[ids[1]][0]*baryCoords[1], parent->normals[ids[1]][1]*baryCoords[1], parent->normals[ids[1]][2]*baryCoords[1]);
            Vec3d nC(parent->normals[ids[2]][0]*baryCoords[2], parent->normals[ids[2]][1]*baryCoords[2], parent->normals[ids[2]][2]*baryCoords[2]);
            n = nA + nB + nC;
            n.normalize();
        } 

        Material m;
        if(!parent->materials.empty())
        {
            m += (baryCoords[0] * *(parent->materials[ids[0]]));
            m += (baryCoords[1] * *(parent->materials[ids[1]]));
            m += (baryCoords[2] * *(parent->materials[ids[2]]));
        }
        else
        {
            m = getMaterial();
        }

        i.setN(n);
        i.setT(t);
        i.setBary(baryCoords);
        i.setUVCoordinates(Vec2d(baryCoords[0],baryCoords[1]));
        //i.setMaterial(this->getMaterial());
        i.setMaterial(m);
        return true;
    }

    return false;
}


void Trimesh::generateNormals()
// Once you've loaded all the verts and faces, we can generate per
// vertex normals by averaging the normals of the neighboring faces.
{
    int cnt = vertices.size();
    normals.resize( cnt );
    int *numFaces = new int[ cnt ]; // the number of faces assoc. with each vertex
    memset( numFaces, 0, sizeof(int)*cnt );
    
    for( Faces::iterator fi = faces.begin(); fi != faces.end(); ++fi )
    {
		Vec3d faceNormal = (**fi).getNormal();
        
        for( int i = 0; i < 3; ++i )
        {
            normals[(**fi)[i]] += faceNormal;
            ++numFaces[(**fi)[i]];
        }
    }

    for( int i = 0; i < cnt; ++i )
    {
        if( numFaces[i] )
            normals[i]  /= numFaces[i];
    }

    delete [] numFaces;
    vertNorms = true;
}