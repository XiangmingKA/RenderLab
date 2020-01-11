template<typename V, typename _0, typename _1, typename _2>
template<typename ...Args>
const Ptr<V> HEMesh<V, _0, _1, _2>::AddVertex(Args&& ... args) {
	auto v = Basic::New<V>(std::forward<Args>(args)...);
	vertices.insert(v);
	return v;
}

template<typename V, typename _0, typename _1, typename _2>
template<typename ...Args>
const Ptr<typename V::E_t> HEMesh<V, _0, _1, _2>::_AddEdge(Ptr<V> v0, Ptr<V> v1, Ptr<E> e, Args && ... args) {
	if (v0 == v1) {
		printf("ERROR::HEMesh::AddEdge\n"
			"\t""v0 == v1\n");
		return nullptr;
	}
	if (V::IsConnected(v0, v1)) {
		printf("ERROR::HEMesh::AddEdge\n"
			"\t""v0 is already connected with v1\n");
		return nullptr;
	}

	bool newEdge = false;
	if (!e) {
		e = Basic::New<E>(std::forward<Args>(args)...);
		edges.insert(e);
		newEdge = true;
	}
	
	auto he0 = Basic::New<HE>();
	auto he1 = Basic::New<HE>();
	halfEdges.insert(he0);
	halfEdges.insert(he1);
	// [init]
	e->SetHalfEdge(he0);

	he0->SetNext(he1);
	he0->SetPair(he1);
	he0->SetOrigin(v0);
	he0->SetEdge(e);

	he1->SetNext(he0);
	he1->SetPair(he0);
	he1->SetOrigin(v1);
	he1->SetEdge(e);

	// [link he0]
	if (!v0->IsIsolated()) {
		auto inV0 = v0->FindFreeIncident();
		assert(inV0 != nullptr);
		auto outV0 = inV0->Next();

		inV0->SetNext(he0);
		he1->SetNext(outV0);
	}
	else
		v0->SetHalfEdge(he0);

	// [link he1]
	if (!v1->IsIsolated()) {
		auto inV1 = v1->FindFreeIncident();
		assert(inV1 != nullptr);
		auto outV1 = inV1->Next();

		inV1->SetNext(he1);
		he0->SetNext(outV1);
	}
	else
		v1->SetHalfEdge(he1);

	return e;
}

template<typename V, typename _0, typename _1, typename _2>
template<typename ...Args>
const Ptr<typename V::P_t> HEMesh<V, _0, _1, _2>::AddPolygon(const std::vector<Ptr<HE>> heLoop, Args && ... args) {
	if (heLoop.size() == 0) {
		printf("ERROR::HEMesh::AddPolygon:\n"
			"\t""heLoop is empty\n");
		return nullptr;
	}
	for (int i = 0; i < heLoop.size(); i++) {
		if (!heLoop[i]->IsFree()) {
			printf("ERROR::HEMesh::AddPolygon:\n"
				"\t""heLoop[%d] isn't free\n", i);
			return nullptr;
		}
		int next = (i + 1) % heLoop.size();
		if (heLoop[i]->End() != heLoop[next]->Origin()) {
			printf("ERROR::HEMesh::AddPolygon:\n"
				"\t""heLoop[%d]'s end isn't heLoop[%d]'s origin\n", i, next);
			return nullptr;
		}
	}

	// reorder link
	for (int i = 0; i < heLoop.size(); i++) {
		int next = (i + 1) % heLoop.size();
		if (!HE::MakeAdjacent(heLoop[i], heLoop[next])) {
			printf("ERROR::HEMesh::AddPolygon:\n"
				"\t""the polygon would introduce a non-monifold condition\n");
			return nullptr;
		}
	}

	// link polygon and heLoop
	auto polygon = Basic::New<P>(std::forward<Args>(args)...);
	polygons.insert(polygon);

	polygon->SetHalfEdge(heLoop[0]);
	for (auto he : heLoop)
		he->SetPolygon(polygon);

	return polygon;
}

template<typename V, typename _0, typename _1, typename _2>
void HEMesh<V, _0, _1, _2>::RemovePolygon(Ptr<P> polygon) {
	assert(polygon != nullptr);
	for (auto he : polygon->BoundaryHEs())
		he->SetPolygon(nullptr);
	polygons.erase(polygon);
}

template<typename V, typename _0, typename _1, typename _2>
void HEMesh<V, _0, _1, _2>::RemoveEdge(Ptr<E> e, bool needErase) {
	assert(e != nullptr);
	auto he0 = e->HalfEdge();
	auto he1 = he0->Pair();

	if (!he0->IsFree())
		RemovePolygon(he0->Polygon());
	if (!he1->IsFree())
		RemovePolygon(he1->Polygon());

	// init
	auto v0 = he0->Origin();
	auto v1 = he1->Origin();
	auto inV0 = he0->Pre();
	auto inV1 = he1->Pre();
	auto outV0 = he0->RotateNext();
	auto outV1 = he1->RotateNext();

	// [link off he0]
	if (v0->HalfEdge() == he0)
		v0->SetHalfEdge(outV0 == he0 ? nullptr : outV0);

	inV0->SetNext(outV0);

	// [link off he1]
	if (v1->HalfEdge() == he1)
		v1->SetHalfEdge(outV1 == he1 ? nullptr : outV1);

	inV1->SetNext(outV1);

	// delete
	halfEdges.erase(he0);
	halfEdges.erase(he1);
	if(needErase)
		edges.erase(e);
}

template<typename V, typename _0, typename _1, typename _2>
void HEMesh<V, _0, _1, _2>::RemoveVertex(Ptr<V> v) {
	for (auto e : v->AdjEdges())
		RemoveEdge(e);
	vertices.erase(v);
}

template<typename V, typename _0, typename _1, typename _2>
Ptr<V> HEMesh<V, _0, _1, _2>::SpiltEdge(Ptr<E> e) {
	auto he01 = e->HalfEdge();
	auto he10 = he01->Pair();

	if (he01->IsBoundary() && he10->IsBoundary()) {
		printf("ERROR::HEMesh::SpiltEdge:\n"
			"\t""two side of edge are boundaries\n");
		return nullptr;
	}

	if (he01->IsBoundary() || he10->IsBoundary()) {
		if (he01->IsBoundary())
			std::swap(he01, he10);

		// he0 isn't boundary, he1 is boundary

		if (he01->Polygon()->Degree() != 3) {
			printf("ERROR::HEMesh::SpiltEdge:\n"
				"\t""polygon's degree %zd is not 3\n", he01->Polygon()->Degree());
			return nullptr;
		}
		
		auto he12 = he01->Next();
		auto he20 = he12->Next();

		auto v0 = he01->Origin();
		auto v1 = he10->Origin();
		auto v2 = he12->End();

		RemoveEdge(e, false); // don't erase
		
		auto v3 = AddVertex();

		auto e30 = AddEdge(v3, v0);
		auto e31 = _AddEdge(v3, v1, e); // use old edge
		auto e32 = AddEdge(v3, v2);

		auto he31 = e31->HalfEdge();
		auto he32 = e32->HalfEdge();
		auto he23 = he32->Pair();
		auto he03 = e30->HalfEdge()->Pair();

		AddPolygon({ he31, he12, he23 });
		AddPolygon({ he32, he20, he03 });

		return v3;
	}

	if (he01->Polygon()->Degree() != 3 || he10->Polygon()->Degree() != 3)
	{
		printf("ERROR::HEMesh::SpiltEdge:\n"
			"\t""polygon's degree (%zd, %zd) is not 3\n",
			he01->Polygon()->Degree(), he10->Polygon()->Degree());
		return nullptr;
	}

	auto he12 = he01->Next();
	auto he20 = he12->Next();
	auto he03 = he10->Next();
	auto he31 = he03->Next();

	auto v0 = he01->Origin();
	auto v1 = he12->Origin();
	auto v2 = he20->Origin();
	auto v3 = he31->Origin();

	RemoveEdge(e, false); // don't erase

	auto v4 = AddVertex();

	auto e40 = AddEdge(v4, v0);
	auto e41 = _AddEdge(v4, v1, e); // use old edge
	auto e42 = AddEdge(v4, v2);
	auto e43 = AddEdge(v4, v3);

	auto he40 = e40->HalfEdge();
	auto he41 = e41->HalfEdge();
	auto he42 = e42->HalfEdge();
	auto he43 = e43->HalfEdge();

	auto he04 = he40->Pair();
	auto he14 = he41->Pair();
	auto he24 = he42->Pair();
	auto he34 = he43->Pair();

	AddPolygon({ he40,he03,he34 });
	AddPolygon({ he41,he12,he24 });
	AddPolygon({ he42,he20,he04 });
	AddPolygon({ he43,he31,he14 });

	return v4;
}

template<typename V, typename _0, typename _1, typename _2>
bool HEMesh<V, _0, _1, _2>::RotateEdge(Ptr<E> e) {
	auto he01 = e->HalfEdge();
	auto he10 = he01->Pair();

	auto poly0 = he01->Polygon();
	auto poly1 = he10->Polygon();
	
	if (poly0 == poly1) {
		printf("ERROR::HEMesh::SpiltEdge:\n"
			"\t""two side of edge are same\n");
		return false;
	}

	auto v0 = he01->Origin();
	auto v1 = he10->Origin();

	auto he02 = he10->Next();
	auto he13 = he01->Next();
	
	auto v2 = he02->End();
	auto v3 = he13->End();

	auto heLoop0 = poly0->BoundaryHEs(he13->Next(), he01);
	auto heLoop1 = poly1->BoundaryHEs(he02->Next(), he10);

	RemoveEdge(e, false); // don't erase
	_AddEdge(v2, v3, e); // use old edge

	auto he23 = e->HalfEdge();
	auto he32 = he23->Pair();
	
	heLoop0.push_back(he02);
	heLoop0.push_back(he23);
	heLoop1.push_back(he13);
	heLoop1.push_back(he32);

	AddPolygon(heLoop0);
	AddPolygon(heLoop1);

	return true;
}

template<typename V, typename _0, typename _1, typename _2>
void HEMesh<V, _0, _1, _2>::Clear() {
	vertices.clear();
	halfEdges.clear();
	edges.clear();
	polygons.clear();
}

template<typename V, typename _0, typename _1, typename _2>
void HEMesh<V, _0, _1, _2>::Reserve(size_t n) {
	vertices.reserve(n);
	halfEdges.reserve(6*n);
	edges.reserve(3*n);
	polygons.reserve(2*n);
}

template<typename V, typename _0, typename _1, typename _2>
bool HEMesh<V, _0, _1, _2>::HaveBoundary() const {
	for (auto he : halfEdges){
		if (he->IsBoundary())
			return true;
	}
	return false;
}

template<typename V, typename _0, typename _1, typename _2>
bool HEMesh<V, _0, _1, _2>::IsTriMesh() const {
	for (auto poly : polygons) {
		if (poly->Degree() != 3)
			return false;
	}
	return true;
}

template<typename V, typename _0, typename _1, typename _2>
const std::vector<std::vector<Ptr<typename V::HE>>> HEMesh<V, _0, _1, _2>::Boundaries() {
	std::vector<std::vector<Ptr<HE>>> boundaries;
	std::set<Ptr<HE>> found;
	for (auto he : halfEdges) {
		if (he->IsBoundary() && found.find(he) == found.end()) {
			boundaries.push_back(std::vector<Ptr<HE>>());
			auto cur = he;
			do {
				boundaries.back().push_back(cur);
				found.insert(cur);
				cur = cur->Next();
			} while (cur != he);
		}
	}
	return boundaries;
}

template<typename V, typename _0, typename _1, typename _2>
const Ptr<typename V::P_t> HEMesh<V, _0, _1, _2>::EraseVertex(Ptr<V> v) {
	bool isBoundary = v->IsBoundary();
	auto pHE = v->HalfEdge()->Next();
	RemoveVertex(v);
	if (isBoundary)
		return nullptr;
	return AddPolygon(pHE->Loop());
}

template<typename V, typename _0, typename _1, typename _2>
template<typename ...Args>
const Ptr<V> HEMesh<V, _0, _1, _2>::AddPolygonVertex(Ptr<HE> he, Args && ... args) {
	auto p = he->Polygon();

	auto pre = he->Pre();
	auto v0 = he->Origin();
	auto v1 = AddVertex(std::forward<Args>(args)...);
	
	auto he01 = Basic::New<HE>();
	auto he10 = Basic::New<HE>();
	halfEdges.insert(he01);
	halfEdges.insert(he10);
	auto e01 = Basic::New<E>();
	edges.insert(e01);

	he01->SetNext(he10);
	he01->SetPair(he10);
	he01->SetOrigin(v0);
	he01->SetEdge(e01);
	he01->SetPolygon(p);

	he10->SetNext(he);
	he10->SetPair(he01);
	he10->SetOrigin(v1);
	he10->SetEdge(e01);
	he10->SetPolygon(p);

	e01->SetHalfEdge(he01);
	v1->SetHalfEdge(he10);

	pre->SetNext(he01);

	return v1;
}

template<typename V, typename _0, typename _1, typename _2>
template<typename ...Args>
const Ptr<V> HEMesh<V, _0, _1, _2>::AddPolygonVertex(Ptr<P> p, Ptr<V> v, Args&& ... args) {
	auto he = p->HalfEdge();
	while (he->Origin() != v)
		he = he->Next();
	return AddPolygonVertex(he, std::forward<Args>(args)...);
}

template<typename V, typename _0, typename _1, typename _2>
const Ptr<V> HEMesh<V, _0, _1, _2>::CollapseEdge(Ptr<E> e) {
	return nullptr;
}
