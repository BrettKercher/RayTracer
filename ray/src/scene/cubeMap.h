
#pragma once

#include "../scene/material.h"

class CubeMap
{
public:
	void setXnegMap(TextureMap* nX) { negX = nX; }
	void setXposMap(TextureMap* pX) { posX = pX; }
	void setYnegMap(TextureMap* nY) { negY = nY; }
	void setYposMap(TextureMap* pY) { posY = pY; }
	void setZnegMap(TextureMap* nZ) { negZ = nZ; }
	void setZposMap(TextureMap* pZ) { posZ = pZ; }

	TextureMap* getXnegMap() { return negX; }
	TextureMap* getXposMap() { return posX; }
	TextureMap* getYnegMap() { return negY; }
	TextureMap* getYposMap() { return posY; }
	TextureMap* getZnegMap() { return negZ; }
	TextureMap* getZposMap() { return posZ; }

protected:
	TextureMap* negX;
	TextureMap* posX;
	TextureMap* negY;
	TextureMap* posY;
	TextureMap* negZ;
	TextureMap* posZ;
};