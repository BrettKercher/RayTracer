

#pragma once
#include <vector>

template<typename T>

class kdTree
{
	public:
		typedef typename std::vector<T*>::const_iterator ObjectIterator;


		kdTree() {}

		void build(ObjectIterator b, ObjectIterator e)
		{

		}

		void choosePlane()
		{

		}
	protected:
		int maxDepth;
		int leaves;

};