#ifndef _RTX_HITABLE_TRI_MESH_H_
#define _RTX_HITABLE_TRI_MESH_H_

#include <CppUtil/RTX/BVH_Node.h>

namespace RTX {
	class TriMesh : public BVH_Node{
		HITABLE_SETUP(TriMesh)
	public:
		TriMesh(const std::vector<Vertex> & vertexs, CppUtil::Basic::CPtr<Material> material = NULL);

		inline bool IsValid() const { return isValid; };
	protected:
		bool isValid;
	};
}

#endif // !_RTX_HITABLE_TRI_MESH_H_
