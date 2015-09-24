// The main ray tracer.

#pragma warning (disable: 4786)

#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"

#include "parser/Tokenizer.h"
#include "parser/Parser.h"

#include "ui/TraceUI.h"
#include <cmath>
#include <algorithm>

extern TraceUI* traceUI;

#include <iostream>
#include <fstream>

using namespace std;

// Use this variable to decide if you want to print out
// debugging messages.  Gets set in the "trace single ray" mode
// in TraceGLWindow, for example.
bool debugMode = false;

// Trace a top-level ray through pixel(i,j), i.e. normalized window coordinates (x,y),
// through the projection plane, and out into the scene.  All we do is
// enter the main ray-tracing method, getting things started by plugging
// in an initial ray weight of (0.0,0.0,0.0) and an initial recursion depth of 0.

Vec3d RayTracer::trace(double x, double y)
{
  // Clear out the ray cache in the scene for debugging purposes,
  if (TraceUI::m_debug) scene->intersectCache.clear();
  ray r(Vec3d(0,0,0), Vec3d(0,0,0), ray::VISIBILITY);
  scene->getCamera().rayThrough(x,y,r);
  Vec3d ret = traceRay(r, traceUI->getDepth());
  ret.clamp();
  return ret;
}

Vec3d RayTracer::tracePixel(int i, int j)
{
	Vec3d col(0,0,0);

	//UI slider is either 1,2,3,4. Need 1,4,9,16.
	int numSamples = traceUI->getPixels();

	if( ! sceneLoaded() ) return col;

	double x = double(i)/double(buffer_width);
	double y = double(j)/double(buffer_height);

	unsigned char *pixel = buffer + ( i + j * buffer_width ) * 3;

	if(traceUI->doAA())
	{
		for(int k = 0; k < numSamples; k++)
		{
			for(int l = 0; l < numSamples; l++)
			{
				x = double(double(i)+(double(k)/2.0))/(double(buffer_width));
				y = double(double(j)+(double(l)/2.0))/(double(buffer_height));
				col += trace(x, y);
			}
		}
		col /= numSamples * numSamples;
	}
	else
	{
		col = trace(x, y);
	}


	pixel[0] = (int)( 255.0 * col[0]);
	pixel[1] = (int)( 255.0 * col[1]);
	pixel[2] = (int)( 255.0 * col[2]);
	return col;
}


// Do recursive ray tracing!  You'll want to insert a lot of code here
// (or places called from here) to handle reflection, refraction, etc etc.
Vec3d RayTracer::traceRay(ray& r, int depth)
{
	isect i;
	Vec3d colorC;
	if(scene->intersect(r, i)) 
	{
		const Material& m = i.getMaterial();
		colorC = m.shade(scene, r, i);

		if(depth <= 0)
			return colorC;

		Vec3d q = r.at(i.t);
		
		Vec3d rayDir = -1 * r.getDirection();
		rayDir.normalize();

		//REFLECTION
		Vec3d reflectionIntensity = m.kr(i);

		Vec3d reflectDir = 2 * (rayDir * i.N) * i.N - rayDir;
		reflectDir.normalize();

		ray reflected(q, reflectDir, ray::REFLECTION);

		reflectionIntensity %= traceRay(reflected, depth - 1);
		colorC += reflectionIntensity;
		//END REFLECTION

		//REFRACTION
		Vec3d refractionIntensity = m.kt(i);
		double indexRatio, flip;

		if(refractionIntensity[0] <= 0.0 && refractionIntensity[1] <= 0.0 && refractionIntensity[2] <= 0.0)
		{
			return colorC;
		}

		if(i.N * r.getDirection() < 0.0)
		{
			indexRatio = 1.0 / m.index(i);
			flip = -1;
		}
		else
		{
			indexRatio = m.index(i) / 1.0;
			flip = 1;
		}

		//Check for TIR
		if(i.N * r.getDirection() > 0.0f)
    	{
	        double iAngle = i.N * r.getDirection();
	        if((1 - (indexRatio*indexRatio*(1.0f - iAngle * iAngle))) < 0.0)
	            return colorC;
    	}

		Vec3d rayNormalPart = (rayDir * i.N) * i.N;
		Vec3d rayTangentialPart = rayNormalPart - rayDir;

		Vec3d refractTangential = (indexRatio) * rayTangentialPart;

		double temp = refractTangential * refractTangential;
		if(temp > 1) temp = 0;

		Vec3d refractNormal = (flip * i.N) * sqrt( 1.0 - temp);

		Vec3d refractDir = refractNormal + refractTangential;
		reflectDir.normalize();

		ray refracted(q, refractDir, ray::REFRACTION);
		refractionIntensity %= traceRay(refracted, depth - 1);

		colorC += refractionIntensity;
		//END REFRACTION
	}
	else 
	{
		//do cube mapping?
		if(!traceUI->doCubeMap())
			return Vec3d(0,0,0);


		TextureMap* texture;
		double u, v;
		Vec3d direction = r.getDirection();
		double x = direction.n[0];
		double y = direction.n[1];
		double z = direction.n[2];

		double max = std::abs(x);
		if(std::abs(y) > max) max = std::abs(y);
		if(std::abs(z) > max) max = std::abs(z);


		if(max == std::abs(x))
		{
			if(x < 0)
			{
				texture = cubeMap->getXnegMap();
				u = ((z/std::abs(x)) + 1)/2;
				v = ((y/std::abs(x)) + 1)/2;
			}
			else
			{
				texture = cubeMap->getXposMap();
				u = ((-z/std::abs(x)) + 1)/2;
				v = ((y/std::abs(x)) + 1)/2;
			}
		}
		else if(max == std::abs(y))
		{
			if(y < 0)
			{
				texture = cubeMap->getYnegMap();
				u = ((x/std::abs(y)) + 1)/2;
				v = ((z/std::abs(y)) + 1)/2;
			}
			else
			{
				texture = cubeMap->getYposMap();
				u = ((x/std::abs(y)) + 1)/2;
				v = ((-z/std::abs(y)) + 1)/2;
			}
		}
		else
		{
			if(z < 0)
			{
				texture = cubeMap->getZnegMap();
				u = ((-x/std::abs(z)) + 1)/2;
				v = ((y/std::abs(z)) + 1)/2;
			}
			else
			{
				texture = cubeMap->getZposMap();
				u = ((x/std::abs(z)) + 1)/2;
				v = ((y/std::abs(z)) + 1)/2;
			}
		}

		colorC = texture->getMappedValue(Vec2d(u,v));

	}
	return colorC;
}

RayTracer::RayTracer()
	: scene(0), buffer(0), buffer_width(256), buffer_height(256), m_bBufferReady(false)
{}

RayTracer::~RayTracer()
{
	delete scene;
	delete [] buffer;
}

void RayTracer::getBuffer( unsigned char *&buf, int &w, int &h )
{
	buf = buffer;
	w = buffer_width;
	h = buffer_height;
}

double RayTracer::aspectRatio()
{
	return sceneLoaded() ? scene->getCamera().getAspectRatio() : 1;
}

bool RayTracer::loadScene( char* fn ) {
	ifstream ifs( fn );
	if( !ifs ) {
		string msg( "Error: couldn't read scene file " );
		msg.append( fn );
		traceUI->alert( msg );
		return false;
	}
	
	// Strip off filename, leaving only the path:
	string path( fn );
	if( path.find_last_of( "\\/" ) == string::npos ) path = ".";
	else path = path.substr(0, path.find_last_of( "\\/" ));

	// Call this with 'true' for debug output from the tokenizer
	Tokenizer tokenizer( ifs, false );
    Parser parser( tokenizer, path );
	try {
		delete scene;
		scene = 0;
		scene = parser.parseScene();

		if(traceUI->useAccelerator())
		{
			accelerator->build(scene->beginObjects(), scene->endObjects());
		}

	} 
	catch( SyntaxErrorException& pe ) {
		traceUI->alert( pe.formattedMessage() );
		return false;
	}
	catch( ParserException& pe ) {
		string msg( "Parser: fatal exception " );
		msg.append( pe.message() );
		traceUI->alert( msg );
		return false;
	}
	catch( TextureMapException e ) {
		string msg( "Texture mapping exception: " );
		msg.append( e.message() );
		traceUI->alert( msg );
		return false;
	}

	if( !sceneLoaded() ) return false;

	return true;
}

void RayTracer::traceSetup(int w, int h)
{
	if (buffer_width != w || buffer_height != h)
	{
		buffer_width = w;
		buffer_height = h;
		bufferSize = buffer_width * buffer_height * 3;
		delete[] buffer;
		buffer = new unsigned char[bufferSize];
	}
	memset(buffer, 0, w*h*3);
	m_bBufferReady = true;
}

