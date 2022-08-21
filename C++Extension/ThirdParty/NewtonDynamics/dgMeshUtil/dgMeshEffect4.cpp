/* Copyright (c) <2003-2019> <Julio Jerez, Newton Game Dynamics>
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source distribution.
*/

#include "dgPhysicsStdafx.h"
#include "dgBody.h"
#include "dgWorld.h"
#include "dgMeshEffect.h"
#include "dgCollisionConvexHull.h"



#if 0
#define DG_BUILD_HIERACHICAL_HACD

#define DG_CONCAVITY_MAX_THREADS	  8
#define DG_CONCAVITY_SCALE dgFloat64 (100.0f)




class dgHACDEdge
{
	public:
	dgHACDEdge ()
		:m_mark(0)
		,m_proxyListNode(NULL)
		,m_backFaceHandicap(dgFloat64 (1.0))
	{
	}
	~dgHACDEdge ()
	{
	}

	dgInt32 m_mark;
	void* m_proxyListNode;
	dgFloat64 m_backFaceHandicap;
};

class dgHACDClusterFace
{
	public:
	dgHACDClusterFace()
		:m_edge(NULL)
		,m_area(dgFloat64(0.0f))
	{
	}
	~dgHACDClusterFace()
	{
	}

	dgEdge* m_edge;
	dgFloat64 m_area;
	dgBigVector m_normal;
};


class dgHACDCluster: public dgList<dgHACDClusterFace>
{
	public:
	dgHACDCluster ()
		:dgList<dgHACDClusterFace>(NULL)
		,m_color(0)
		,m_hierachicalClusterIndex(0)
		,m_area(dgFloat64 (0.0f))
		,m_concavity(dgFloat64 (0.0f))
	{
	}

	bool IsCoplanar(const dgBigPlane& plane, const dgMeshEffect& mesh, dgFloat64 tolerance) const
	{
		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();
		for (dgListNode* node = GetFirst(); node; node = node->GetNext()) {
			const dgHACDClusterFace& info = node->GetInfo();
			dgEdge* ptr = info.m_edge;
			do {
				const dgBigVector& p = points[ptr->m_incidentVertex];
				dgFloat64 dist = fabs(plane.Evalue(p));
				if (dist > tolerance) {
					return false;
				}
				ptr = ptr->m_next;
			} while (ptr != info.m_edge);
		}
		return true;
	}


	dgInt32 m_color;
	dgInt32 m_hierachicalClusterIndex;
	dgFloat64 m_area;
	dgFloat64 m_concavity;
};


class dgHACDClusterGraph
	:public dgGraph<dgHACDCluster, dgHACDEdge> 
	,public dgAABBPolygonSoup 
{
	public:

	class dgHACDConveHull: public dgConvexHull3d
	{
		class dgConvexHullRayCastData
		{
			public:
			dgFloat64 m_normalProjection;
			dgConvexHull3DFace* m_face;
		};

		public: 
		dgHACDConveHull (const dgHACDConveHull& hull)
			:dgConvexHull3d(hull)
			,m_mark(1)
		{
		}

		dgHACDConveHull (dgMemoryAllocator* const allocator, const dgBigVector* const points, dgInt32 count)
			:dgConvexHull3d(allocator, &points[0].m_x, sizeof (dgBigVector),count, dgFloat64 (0.0f))
			,m_mark(1)
		{

		}

		dgFloat64 CalculateTriangleConcavity(const dgBigVector& normal, dgInt32 i0, dgInt32 i1, dgInt32 i2, const dgBigVector* const points)
		{
			dgUnsigned32 head = 1;
			dgUnsigned32 tail = 0;
			dgBigVector pool[1<<8][3];

			pool[0][0] = points[i0];
			pool[0][1] = points[i1];
			pool[0][2] = points[i2];

			const dgBigVector step(normal.Scale(dgFloat64(4.0f) * GetDiagonal()));

			dgFloat64 concavity = dgFloat32(0.0f);
			dgFloat64 minArea = dgFloat32(0.125f);
			dgFloat64 minArea2 = minArea * minArea * 0.5f;

			dgInt32 maxCount = 4;
			dgUnsigned32 mask = (sizeof (pool) / (3 * sizeof (pool[0][0]))) - 1;

			dgConvexHull3DFace* firstGuess = NULL;
			while ((tail != head) && (maxCount >= 0)) {
				maxCount --;
				dgBigVector p0(pool[tail][0]);
				dgBigVector p1(pool[tail][1]);
				dgBigVector p2(pool[tail][2]);
				tail = (tail + 1) & mask;

				dgBigVector q1((p0 + p1 + p2).Scale(dgFloat64(1.0f / 3.0f)));
				dgBigVector q0(q1 + step);

				//dgFloat64 param = convexHull.RayCast(q0, q1, &firstGuess);
				dgFloat64 param = FastRayCast(q0, q1, &firstGuess);
				if (param > dgFloat64(1.0f)) {
					param = dgFloat64(1.0f);
				}
				dgBigVector dq(step.Scale(dgFloat32(1.0f) - param));
				dgFloat64 lenght2 = sqrt (dq % dq);
				//dgAssert (lenght2 < GetDiagonal());
				if (lenght2 > concavity) {
					concavity = lenght2;
				}

				if (((head + 1) & mask) != tail) {
					dgBigVector edge10(p1 - p0);
					dgBigVector edge20(p2 - p0);
					dgBigVector n(edge10 * edge20);
					dgFloat64 area2 = n % n;
					if (area2 > minArea2) {
						dgBigVector p01((p0 + p1).Scale(dgFloat64(0.5f)));
						dgBigVector p12((p1 + p2).Scale(dgFloat64(0.5f)));
						dgBigVector p20((p2 + p0).Scale(dgFloat64(0.5f)));

						pool[head][0] = p0;
						pool[head][1] = p01;
						pool[head][2] = p20;
						head = (head + 1) & mask;

						if (((head + 1) & mask) != tail) {
							pool[head][0] = p1;
							pool[head][1] = p12;
							pool[head][2] = p01;
							head = (head + 1) & mask;

							if (((head + 1) & mask) != tail)	{
								pool[head][0] = p2;
								pool[head][1] = p20;
								pool[head][2] = p12;
								head = (head + 1) & mask;
							}
						}
					}
				}
			}
			return concavity;
		}



		dgFloat64 FaceRayCast (const dgConvexHull3DFace* const face, const dgBigVector& origin, const dgBigVector& dist, dgFloat64& normalProjection) const
		{
			dgInt32 i0 = face->m_index[0];
			dgInt32 i1 = face->m_index[1];
			dgInt32 i2 = face->m_index[2];

			const dgBigVector& p0 = m_points[i0];
			dgBigVector normal ((m_points[i1] - p0) * (m_points[i2] - p0));

			dgFloat64 N = (origin - p0) % normal;
			dgFloat64 D = dist % normal;

			if (fabs(D) < dgFloat64 (1.0e-16f)) { // 
				normalProjection = dgFloat32 (0.0);
				if (N > dgFloat64 (0.0f)) {
					return dgFloat32 (-1.0e30);
				} else {

					return dgFloat32 (1.0e30);
				}
			}
			normalProjection = D;
			return - N / D;
		}

		dgConvexHull3DFace* ClosestFaceVertexToPoint (const dgBigVector& point)
		{
			// note, for this function to be effective point should be an already close point to the Hull.
			// for example casting the point to the OBB or the AABB of the full is a good first guess. 
			dgConvexHull3DFace* closestFace = &GetFirst()->GetInfo();	
			dgInt8 pool[256 * (sizeof (dgConvexHull3DFace*) + sizeof (dgFloat64))];
			dgUpHeap<dgConvexHull3DFace*,dgFloat64> heap (pool, sizeof (pool));

			for (dgInt32 i = 0; i < 3; i ++) {
				dgBigVector dist (m_points[closestFace->m_index[i]] - point);
				heap.Push(closestFace, dist % dist);
			}

			m_mark ++;	
			dgFloat64 minDist = heap.Value();
			while (heap.GetCount()) {
				dgConvexHull3DFace* const face = heap[0];	
				if (heap.Value() < minDist) {
					minDist = heap.Value();
					closestFace = face;
				}
				heap.Pop();
				//face->m_mark = m_mark;
				face->SetMark(m_mark);
				for (dgInt32 i = 0; i < 3; i ++) {
					//const dgConvexHull3DFace* twin = &face->m_twin[i]->GetInfo();	
					dgConvexHull3DFace* twin = &face->GetTwin(i)->GetInfo();	
					//if (twin->m_mark != m_mark) {
					if (twin->GetMark() != m_mark) {
						dgBigVector dist (m_points[twin->m_index[i]] - point);
						// use hysteresis to prevent stops at a local minimal, but at the same time fast descend
						dgFloat64 dist2 = dist % dist;
						if (dist2 < (minDist * dgFloat64 (1.001f))) {
							heap.Push(twin, dist2);
						}
					}
				}
			}

			return closestFace;
		}


		// this version have input sensitive complexity (approximately  log2)
		// when casting parallel rays and using the last face as initial guess this version has const time complexity 
		dgFloat64 RayCast (const dgBigVector& localP0, const dgBigVector& localP1, dgConvexHull3DFace** firstFaceGuess)
		{
			dgConvexHull3DFace* face = &GetFirst()->GetInfo();
			if (firstFaceGuess && *firstFaceGuess) {
				face = *firstFaceGuess;
			} else {
				if (GetCount() > 32) {
					dgVector q0 (localP0);
					dgVector q1 (localP1);
					if (dgRayBoxClip (q0, q1, m_aabbP0, m_aabbP1)) {
						face = ClosestFaceVertexToPoint (q0);
					}
				}
			}

			m_mark ++;	
			//face->m_mark = m_mark;
			face->SetMark (m_mark);
			dgInt8 pool[256 * (sizeof (dgConvexHullRayCastData) + sizeof (dgFloat64))];
			dgDownHeap<dgConvexHullRayCastData,dgFloat64> heap (pool, sizeof (pool));

			dgFloat64 t0 = dgFloat64 (-1.0e20);			//for the maximum entering segment parameter;
			dgFloat64 t1 = dgFloat64 ( 1.0e20);			//for the minimum leaving segment parameter;
			dgBigVector dS (localP1 - localP0);		// is the segment direction vector;
			dgConvexHullRayCastData data;
			data.m_face = face;
			dgFloat64 t = FaceRayCast (face, localP0, dS, data.m_normalProjection);
			if (data.m_normalProjection >= dgFloat32 (0.0)) {
				t = dgFloat64 (-1.0e30);
			}

			heap.Push (data, t);
			while (heap.GetCount()) {
				dgConvexHullRayCastData data (heap[0]);
				dgFloat64 t = heap.Value();
				dgConvexHull3DFace* face = data.m_face;
				dgFloat64 normalDistProjection = data.m_normalProjection;
				heap.Pop();
				bool foundThisBestFace = true;
				if (normalDistProjection < dgFloat64 (0.0f)) {
					if (t > t0) {
						t0 = t;
					}
					if (t0 > t1) {
						return dgFloat64 (1.2f);
					}
				} else {
					foundThisBestFace = false;
				}

				for (dgInt32 i = 0; i < 3; i ++) {
					//dgConvexHull3DFace* const face1 = &face->m_twin[i]->GetInfo();
					dgConvexHull3DFace* const face1 = &face->GetTwin(i)->GetInfo();

					//if (face1->m_mark != m_mark) {
					if (face1->GetMark() != m_mark) {
						//face1->m_mark = m_mark;
						face1->SetMark (m_mark);
						dgConvexHullRayCastData data;
						data.m_face = face1;
						dgFloat64 t = FaceRayCast (face1, localP0, dS, data.m_normalProjection);
						if (data.m_normalProjection >= dgFloat32 (0.0)) {
							t = dgFloat64 (-1.0e30);
						} else if (t > t0) {
							foundThisBestFace = false;
						} else if (fabs (t - t0) < dgFloat64 (1.0e-10f)) {
							return dgConvexHull3d::RayCast (localP0, localP1);
						}
						if ((heap.GetCount() + 2)>= heap.GetMaxCount()) {
							// remove t values that are old and way outside interval [0.0, 1.0]  
							for (dgInt32 i = heap.GetCount() - 1; i >= 0; i--) {
								dgFloat64 val = heap.Value(i);
								if ((val < dgFloat64 (-100.0f)) || (val > dgFloat64 (100.0f))) {
									heap.Remove(i);
								}
							}
						}
						heap.Push (data, t);
					}
				}
				if (foundThisBestFace) {
					if ((t0 >= dgFloat64 (0.0f)) && (t0 <= dgFloat64 (1.0f))) {
						if (firstFaceGuess) {
							*firstFaceGuess = face;
						}
						return t0;
					}
					break;
				}
			}

			return dgFloat64 (1.2f);

		}

		dgFloat64 FastRayCast (const dgBigVector& localP0, const dgBigVector& localP1, dgConvexHull3DFace** guess)
		{
#if 0
	#ifdef _DEBUG
			dgFloat64 t0 = dgConvexHull3d::RayCast (localP0, localP1);
			dgFloat64 t1 = RayCast (localP0, localP1, guess);
			dgAssert (fabs(t0 - t1) < dgFloat64 (1.0e-5f));
	#endif
#endif

			//return dgConvexHull3d::RayCast (localP0, localP1);
			return RayCast (localP0, localP1, guess);
		}

		dgInt32 m_mark;
	};

	class dgHACDConvacityLookAheadTree
	{
		public:
		DG_CLASS_ALLOCATOR(allocator)

			dgHACDConvacityLookAheadTree (dgMemoryAllocator* const allocator, dgEdge* const face, dgFloat64 concavity)
			:m_concavity(concavity)	
			,m_faceList (allocator)
			,m_left (NULL)
			,m_right (NULL)
		{
			m_faceList.Append(face);
		}


		dgHACDConvacityLookAheadTree (dgMemoryAllocator* const allocator, dgHACDConvacityLookAheadTree* const leftChild, dgHACDConvacityLookAheadTree* const rightChild, dgFloat64 concavity)
			:m_concavity(concavity)	
			,m_faceList (allocator)
			,m_left (leftChild)
			,m_right (rightChild)
		{
			dgAssert (leftChild);
			dgAssert (rightChild);

			dgFloat64 concavityTest = m_concavity - dgFloat64 (1.0e-5f);
			//if ((m_left->m_faceList.GetCount() == 1) || (m_right->m_faceList.GetCount() == 1)) {
			if ((((m_left->m_faceList.GetCount() == 1) || (m_right->m_faceList.GetCount() == 1))) ||
				((concavityTest <= m_left->m_concavity) && (concavityTest <= m_right->m_concavity))) {
					//The the parent has lower concavity this mean that the two do no add more detail, 
					//the can be deleted and replaced the parent node
					// for example the two children can be two convex strips that are part of a larger convex piece
					// but each part has a non zero concavity, while the convex part has a lower concavity 
					m_faceList.Merge (m_left->m_faceList);
					m_faceList.Merge (m_right->m_faceList);

					delete m_left;
					delete m_right;
					m_left = NULL;
					m_right = NULL;
			} else {
				for (dgList<dgEdge*>::dgListNode* node = m_left->m_faceList.GetFirst(); node; node = node->GetNext()) {
					m_faceList.Append(node->GetInfo());
				}
				for (dgList<dgEdge*>::dgListNode* node = m_right->m_faceList.GetFirst(); node; node = node->GetNext()) {
					m_faceList.Append(node->GetInfo());
				}
			}
		}

		~dgHACDConvacityLookAheadTree ()
		{
			if (m_left) {
				dgAssert (m_right);
				delete m_left;
				delete m_right;
			}
		}

		dgInt32 GetNodesCount () const
		{
			dgInt32 count = 0;
			dgInt32 stack = 1;
			const dgHACDConvacityLookAheadTree* pool[1024];
			pool[0] = this;
			while (stack) {
				stack --;
				count ++;
				const dgHACDConvacityLookAheadTree* const root = pool[stack];
				if (root->m_left) {
					dgAssert (root->m_right);
					pool[stack] = root->m_left;
					stack ++;
					dgAssert (stack < sizeof (pool)/sizeof (pool[0]));
					pool[stack] = root->m_right;
					stack ++;
					dgAssert (stack < sizeof (pool)/sizeof (pool[0]));
				}
			}
			return count;
		}

		void ReduceByCount (dgInt32 count, dgDownHeap<dgHACDConvacityLookAheadTree*, dgFloat64>& approximation)
		{
			if (count < 1) {
				count = 1;
			}
//			dgInt32 nodesCount = GetNodesCount();

			approximation.Flush();
			dgHACDConvacityLookAheadTree* tmp = this;
			approximation.Push(tmp, m_concavity);
//			nodesCount --;
			//while (nodesCount && (approximation.GetCount() < count) && (approximation.Value() >= dgFloat32 (0.0f))) {
			while ((approximation.GetCount() < count) && (approximation.Value() >= dgFloat32 (0.0f))) {
				dgHACDConvacityLookAheadTree* worseCluster = approximation[0];
				if (!worseCluster->m_left && approximation.Value() >= dgFloat32 (0.0f)) {
					approximation.Pop();
					approximation.Push(worseCluster, dgFloat32 (-1.0f));
				} else {
					dgAssert (worseCluster->m_left);
					dgAssert (worseCluster->m_right);
					approximation.Pop();
					approximation.Push(worseCluster->m_left, worseCluster->m_left->m_concavity);
					approximation.Push(worseCluster->m_right, worseCluster->m_right->m_concavity);
//					nodesCount -= 2;
				}
			}
		}


		void ReduceByConcavity (dgFloat64 concavity, dgDownHeap<dgHACDConvacityLookAheadTree*, dgFloat64>& approximation)
		{
			approximation.Flush();
			dgHACDConvacityLookAheadTree* tmp = this;

			approximation.Push(tmp, m_concavity);
			while (approximation.Value() > concavity) {
				dgHACDConvacityLookAheadTree* worseCluster = approximation[0];
				if (!worseCluster->m_left && approximation.Value() >= dgFloat32 (0.0f)) {
					approximation.Pop();
					approximation.Push(worseCluster, dgFloat32 (-1.0f));
				} else {
					dgAssert (worseCluster->m_left);
					dgAssert (worseCluster->m_right);
					approximation.Pop();
					approximation.Push(worseCluster->m_left, worseCluster->m_left->m_concavity);
					approximation.Push(worseCluster->m_right, worseCluster->m_right->m_concavity);
				}
			}
		}

		dgFloat64 m_concavity; 
		dgList<dgEdge*> m_faceList;
		dgHACDConvacityLookAheadTree* m_left;
		dgHACDConvacityLookAheadTree* m_right;
	};

	class dgPairProxy
	{
		public:
		dgPairProxy()
			:m_nodeA(NULL)
			,m_nodeB(NULL)
			,m_hierachicalClusterIndexA(0)
			,m_hierachicalClusterIndexB(0)
			,m_area(dgFloat64(0.0f))
		{
		}

		~dgPairProxy()
		{
		}

		dgListNode* m_nodeA;
		dgListNode* m_nodeB;
		dgInt32 m_hierachicalClusterIndexA;
		dgInt32 m_hierachicalClusterIndexB;
		dgFloat64 m_area;
		dgFloat64 m_distanceConcavity;
	};

	class dgHACDRayCasterContext: public dgFastRayTest
	{
		public:
		dgHACDRayCasterContext (const dgVector& l0, const dgVector& l1, dgHACDClusterGraph* const me, dgInt32 mycolor)
			:dgFastRayTest (l0, l1)
			,m_myColor(mycolor)
			,m_colorHit(-1)
			,m_param (1.0f) 
			,m_me (me) 
		{
		}

		dgInt32 m_myColor;
		dgInt32 m_colorHit;
		dgFloat32 m_param;
		dgHACDClusterGraph* m_me;
	};


	dgHACDClusterGraph(dgMeshEffect& mesh, dgFloat32 backFaceDistanceFactor, dgReportProgress reportProgressCallback)
		:dgGraph<dgHACDCluster, dgHACDEdge> (mesh.GetAllocator())
		,dgAABBPolygonSoup()
		,m_mark(0)
		,m_faceCount(0)
		,m_vertexMark(0)
		,m_progress(0)
		,m_cancavityTreeIndex(0)
		,m_invFaceCount(dgFloat32 (1.0f))
		,m_vertexMarks(NULL)
		,m_diagonal(dgFloat64(1.0f))
		,m_vertexPool(NULL)
		,m_proxyList(mesh.GetAllocator())
		,m_concavityTreeArray(NULL)
		,m_convexProximation(mesh.GetAllocator())
		,m_priorityHeap (mesh.GetCount() + 2048, mesh.GetAllocator())
		,m_reportProgressCallback (reportProgressCallback)
		,m_parallerConcavityCalculator(mesh.GetAllocator())
	{
		
		m_parallerConcavityCalculator.SetThreadsCount(DG_CONCAVITY_MAX_THREADS);

		// precondition the mesh for better approximation
		mesh.ConvertToPolygons();

		m_faceCount = mesh.GetTotalFaceCount();

		dgMemoryAllocator* const allocator = mesh.GetAllocator();
		m_invFaceCount = dgFloat32 (1.0f) / (m_faceCount);

		// init some auxiliary structures
		dgInt32 vertexCount = mesh.GetVertexCount();
		m_vertexMarks =  (dgInt32*) dgMallocStack(vertexCount * sizeof(dgInt32));
		m_vertexPool =  (dgBigVector*) dgMallocStack(vertexCount * sizeof(dgBigVector));
		memset(m_vertexMarks, 0, vertexCount * sizeof(dgInt32));

		m_cancavityTreeIndex = m_faceCount + 1;
		m_concavityTreeArray = (dgHACDConvacityLookAheadTree**) dgMallocStack(2 * m_cancavityTreeIndex * sizeof(dgHACDConvacityLookAheadTree*));
		memset(m_concavityTreeArray, 0, 2 * m_cancavityTreeIndex * sizeof(dgHACDConvacityLookAheadTree*));

		// scan the mesh and and add a node for each face
		dgInt32 color = 1;
		dgMeshEffect::Iterator iter(mesh);

		dgInt32 meshMask = mesh.IncLRU();
		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();
		for (iter.Begin(); iter; iter++) {
			dgEdge* const edge = &(*iter);
			if ((edge->m_mark != meshMask) && (edge->m_incidentFace > 0)) {

				// call the progress callback
				//ReportProgress();

				dgListNode* const clusterNode = AddNode ();
				dgHACDCluster& cluster = clusterNode->GetInfo().m_nodeData;
				cluster.SetAllocator(mesh.GetAllocator());

				dgFloat64 perimeter = dgFloat64(0.0f);
				dgEdge* ptr = edge;
				do {
					dgBigVector p1p0(points[ptr->m_incidentVertex] - points[ptr->m_prev->m_incidentVertex]);
					perimeter += sqrt(p1p0 % p1p0);
					ptr->m_incidentFace = color;
					ptr->m_userData = dgUnsigned64 (clusterNode);
					ptr->m_mark = meshMask;
					ptr = ptr->m_next;
				} while (ptr != edge);

				dgBigVector normal = mesh.FaceNormal(edge, &points[0][0], sizeof(dgBigVector));
				dgFloat64 mag = sqrt(normal % normal);

				cluster.m_color = color;
				cluster.m_hierachicalClusterIndex = color;
				cluster.m_area = dgFloat64(0.5f) * mag;
				cluster.m_concavity = CalculateConcavityMetric (dgFloat64 (0.0f), cluster.m_area, perimeter, 1, 0);

				dgHACDClusterFace& face = cluster.Append()->GetInfo();
				face.m_edge = edge;
				face.m_area = dgFloat64(0.5f) * mag;
				face.m_normal = normal.Scale(dgFloat64(1.0f) / mag);

				//m_concavityTreeArray[color] = new (allocator) dgHACDConvacityLookAheadTree (allocator, edge, cluster.m_concavity);
				m_concavityTreeArray[color] = new (allocator) dgHACDConvacityLookAheadTree (allocator, edge, dgFloat64 (0.0f));

				color ++;
			}
		}

		// add all link adjacent faces links
		for (dgListNode* clusterNode = GetFirst(); clusterNode; clusterNode = clusterNode->GetNext()) {

			// call the progress callback
			//ReportProgress();

			dgHACDCluster& cluster = clusterNode->GetInfo().m_nodeData;
			dgHACDClusterFace& face = cluster.GetFirst()->GetInfo();
			dgEdge* const edge = face.m_edge;
			dgEdge* ptr = edge; 
			do {
				if (ptr->m_twin->m_incidentFace > 0) {
					dgAssert (ptr->m_twin->m_userData);
					dgListNode* const twinClusterNode = (dgListNode*) ptr->m_twin->m_userData;
					dgAssert (twinClusterNode);

					bool doubleEdge = false;
					for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNode = clusterNode->GetInfo().GetFirst(); edgeNode; edgeNode = edgeNode->GetNext()) {
						if (edgeNode->GetInfo().m_node == twinClusterNode) {
							doubleEdge = true;
							break;
						}
					}
					if (!doubleEdge) {
						clusterNode->GetInfo().AddEdge (twinClusterNode);
					}
				}
				ptr = ptr->m_next;
			} while (ptr != edge);
		}

		Trace();

		// add links to back faces
		dgPolygonSoupDatabaseBuilder builder (mesh.GetAllocator());
		dgVector polygon[64];
		dgInt32 indexList[64];

		dgMatrix matrix (dgGetIdentityMatrix());
		for (dgInt32 i = 0; i < sizeof (polygon) / sizeof (polygon[0]); i ++) {
			indexList[i] = i;
		}

		dgBigVector minAABB;
		dgBigVector maxAABB;
		mesh.CalculateAABB (minAABB, maxAABB);
		maxAABB -= minAABB;
		dgFloat32 rayDiagonalLength = dgFloat32 (sqrt (maxAABB % maxAABB));
		m_diagonal = rayDiagonalLength;

		builder.Begin();
		dgTree<dgListNode*,dgInt32> clusterMap (GetAllocator());
		for (dgListNode* clusterNode = GetFirst(); clusterNode; clusterNode = clusterNode->GetNext()) {

			// call the progress callback
			//ReportProgress();

			dgHACDCluster& cluster = clusterNode->GetInfo().m_nodeData;
			clusterMap.Insert(clusterNode, cluster.m_color);
			dgHACDClusterFace& face = cluster.GetFirst()->GetInfo();
			dgEdge* const edge = face.m_edge;
			dgInt32 count = 0;
			dgEdge* ptr = edge;
			do {
				polygon[count] = points[ptr->m_incidentVertex];
				count ++;
				ptr = ptr->m_prev;
			} while (ptr != edge);

			builder.AddMesh(&polygon[0].m_x, count, sizeof (dgVector), 1, &count, indexList, &cluster.m_color, matrix);
		}
		builder.End(false);
		Create (builder, false);


		dgFloat32 distanceThreshold = rayDiagonalLength * backFaceDistanceFactor;
		for (dgListNode* clusterNodeA = GetFirst(); clusterNodeA; clusterNodeA = clusterNodeA->GetNext()) {

			// call the progress callback
			//ReportProgress();
			dgHACDCluster& clusterA = clusterNodeA->GetInfo().m_nodeData;
			dgHACDClusterFace& faceA = clusterA.GetFirst()->GetInfo();
			dgEdge* const edgeA = faceA.m_edge;
			dgEdge* ptr = edgeA;

			dgVector p0 (points[ptr->m_incidentVertex]);
			dgVector p1 (points[ptr->m_next->m_incidentVertex]);
			ptr = ptr->m_next->m_next;
			do {
				dgVector p2 (points[ptr->m_incidentVertex]);
				dgVector p01 ((p0 + p1).Scale (dgFloat32 (0.5f)));
				dgVector p12 ((p1 + p2).Scale (dgFloat32 (0.5f)));
				dgVector p20 ((p2 + p0).Scale (dgFloat32 (0.5f)));

				CastBackFace (clusterNodeA, p0, p01, p20, distanceThreshold, clusterMap);
				CastBackFace (clusterNodeA, p1, p12, p01, distanceThreshold, clusterMap);
				CastBackFace (clusterNodeA, p2, p20, p12, distanceThreshold, clusterMap);
				CastBackFace (clusterNodeA, p01, p12, p20, distanceThreshold, clusterMap);

				p1 = p2;
				ptr = ptr->m_next;
			} while (ptr != edgeA);
		}

		Trace();
	}

	~dgHACDClusterGraph ()
	{
		for (dgInt32 i = 0; i < m_faceCount * 2; i ++) {
			if (m_concavityTreeArray[i]) {
				delete m_concavityTreeArray[i];
			}
		}

		dgFreeStack(m_concavityTreeArray);
		dgFreeStack(m_vertexPool);
		dgFreeStack(m_vertexMarks);
	}


	void CastBackFace (
		dgListNode* const clusterNodeA,
		const dgVector& p0, 
		const dgVector& p1, 
		const dgVector& p2,
		dgFloat32 distanceThreshold,
		dgTree<dgListNode*,dgInt32>& clusterMap)
	{
		dgVector origin ((p0 + p1 + p2).Scale (dgFloat32 (1.0f/3.0f)));

		dgFloat32 rayDistance = distanceThreshold * dgFloat32 (2.0f);

		dgHACDCluster& clusterA = clusterNodeA->GetInfo().m_nodeData;
		dgHACDClusterFace& faceA = clusterA.GetFirst()->GetInfo();
		dgVector end (origin - dgVector (faceA.m_normal).Scale (rayDistance));

		dgHACDRayCasterContext ray (origin, end, this, clusterA.m_color);
		ForAllSectorsRayHit(ray, RayHit, &ray);

		if (ray.m_colorHit != -1) {
			dgAssert (ray.m_colorHit != ray.m_myColor);
			dgFloat32 distance = rayDistance * ray.m_param;

			if (distance < distanceThreshold) {

				dgAssert (ray.m_colorHit != clusterA.m_color);
				dgAssert (clusterMap.Find(ray.m_colorHit));
				dgListNode* const clusterNodeB = clusterMap.Find(ray.m_colorHit)->GetInfo();
				dgHACDCluster& clusterB = clusterNodeB->GetInfo().m_nodeData;

				dgHACDClusterFace& faceB = clusterB.GetFirst()->GetInfo();
				dgEdge* const edgeB = faceB.m_edge;

				bool isAdjacent = false;
				dgEdge* ptrA = faceA.m_edge;
				do {
					dgEdge* ptrB = edgeB;
					do {
						if (ptrB->m_twin == ptrA) {
							ptrA = faceA.m_edge->m_prev;
							isAdjacent = true;
							break;
						}
						ptrB = ptrB->m_next;
					} while (ptrB != edgeB);

					ptrA = ptrA->m_next;
				} while (ptrA != faceA.m_edge);

				if (!isAdjacent) {

					isAdjacent = false;
					for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNode = clusterNodeA->GetInfo().GetFirst(); edgeNode; edgeNode = edgeNode->GetNext()) {
						if (edgeNode->GetInfo().m_node == clusterNodeB) {
							isAdjacent = true;
							break;
						}
					}

					if (!isAdjacent) {

						dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* const edgeNodeAB = clusterNodeA->GetInfo().AddEdge (clusterNodeB);
						dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* const edgeNodeBA = clusterNodeB->GetInfo().AddEdge (clusterNodeA);

						dgHACDEdge& edgeAB = edgeNodeAB->GetInfo().m_edgeData;
						dgHACDEdge& edgeBA = edgeNodeBA->GetInfo().m_edgeData;
						edgeAB.m_backFaceHandicap = dgFloat64 (0.5f);
						edgeBA.m_backFaceHandicap = dgFloat64 (0.5f);
					}
				}
			}
		}
	}


	void Trace() const
	{
		/*
		for (dgListNode* clusterNodeA = GetFirst(); clusterNodeA; clusterNodeA = clusterNodeA->GetNext()) {
		dgHACDCluster& clusterA = clusterNodeA->GetInfo().m_nodeData;
		//dgHACDClusterFace& faceA = clusterA.GetFirst()->GetInfo();
		//dgEdge* const edgeA = faceA.m_edge;

		dgTrace (("cluster node: %d\n", clusterA.m_color));
		dgTrace (("            links: "));
		for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeA = clusterNodeA->GetInfo().GetFirst(); edgeNodeA; edgeNodeA = edgeNodeA->GetNext()) {
		dgListNode* const clusterNodeB = edgeNodeA->GetInfo().m_node;
		dgHACDCluster& clusterB = clusterNodeB->GetInfo().m_nodeData;
		dgTrace (("%d ", clusterB.m_color));
		}
		dgTrace (("\n"));
		}
		dgTrace (("\n"));
		*/
	}


	// you can insert cal callback here  to print the progress as it collapse clusters
	void ReportProgress ()
	{
		m_progress ++;
		if (m_reportProgressCallback) {
			dgFloat32 progress = dgFloat32(m_progress) * m_invFaceCount;
			m_reportProgressCallback (progress);
		}
	}

	dgMeshEffect* CreatePatitionMesh (dgMeshEffect& mesh, dgInt32 maxVertexPerHull)
	{
		dgMemoryAllocator* const allocator = mesh.GetAllocator();
		dgMeshEffect* const convexPartionMesh = new (allocator) dgMeshEffect(allocator, true);

		dgMeshEffect::dgVertexAtribute polygon[256];
		memset(polygon, 0, sizeof(polygon));
		dgArray<dgBigVector> convexVertexBuffer(mesh.GetCount(), GetAllocator());
		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();

		convexPartionMesh->BeginBuild();
		dgFloat64 layer = dgFloat64 (0.0f);
		for (dgList<dgHACDConvacityLookAheadTree*>::dgListNode* clusterNode = m_convexProximation.GetFirst(); clusterNode; clusterNode = clusterNode->GetNext()) {
			dgHACDConvacityLookAheadTree* const cluster = clusterNode->GetInfo();

			dgInt32 vertexCount = 0;
			for (dgList<dgEdge*>::dgListNode* faceNode = cluster->m_faceList.GetFirst(); faceNode; faceNode = faceNode->GetNext()) {
				dgEdge* const edge = faceNode->GetInfo();
				dgEdge* ptr = edge;
				do {
					dgInt32 index = ptr->m_incidentVertex;
					convexVertexBuffer[vertexCount] = points[index];
					vertexCount++;
					ptr = ptr->m_next;
				} while (ptr != edge);
			}
			dgConvexHull3d convexHull(allocator, &convexVertexBuffer[0].m_x, sizeof(dgBigVector), vertexCount, 0.0, maxVertexPerHull);
			if (convexHull.GetCount()) {
				const dgBigVector* const vertex = convexHull.GetVertexPool();
				for (dgConvexHull3d::dgListNode* node = convexHull.GetFirst(); node; node = node->GetNext()) {
					const dgConvexHull3DFace* const face = &node->GetInfo();

					dgInt32 i0 = face->m_index[0];
					dgInt32 i1 = face->m_index[1];
					dgInt32 i2 = face->m_index[2];

					polygon[0].m_vertex = vertex[i0];
					polygon[0].m_vertex.m_w = layer;

					polygon[1].m_vertex = vertex[i1];
					polygon[1].m_vertex.m_w = layer;

					polygon[2].m_vertex = vertex[i2];
					polygon[2].m_vertex.m_w = layer;

					convexPartionMesh->AddPolygon(3, &polygon[0].m_vertex.m_x, sizeof(dgMeshEffect::dgVertexAtribute), 0);
				}
				layer += dgFloat64 (1.0f);
			}
		}
		convexPartionMesh->EndBuild(1.0e-5f);

		m_progress = m_faceCount - 1;
		ReportProgress();

		return convexPartionMesh;
	}



	static dgFloat32 RayHit (void* const context, const dgFloat32* const polygon, dgInt32 strideInBytes, const dgInt32* const indexArray, dgInt32 indexCount)
	{
		dgHACDRayCasterContext& me = *((dgHACDRayCasterContext*) context);
		dgVector normal (&polygon[indexArray[indexCount] * (strideInBytes / sizeof (dgFloat32))]);
		dgFloat32 t = me.PolygonIntersect (normal, polygon, strideInBytes, indexArray, indexCount);
		if (t < me.m_param) {
			dgInt32 faceColor = me.m_me->GetTagId(indexArray);
			if (faceColor != me.m_myColor) {
				me.m_param = t;
				me.m_colorHit = faceColor;
			}
		}
		return t;
	}


	dgFloat64 ConcavityByFaceMedian (dgInt32 faceCountA, dgInt32 faceCountB) const
	{
		dgFloat64 faceCountCost = DG_CONCAVITY_SCALE * dgFloat64 (0.1f) * (faceCountA + faceCountB) * m_invFaceCount;
		//faceCountCost *= 0;
		return faceCountCost;
	}

	dgFloat64 CalculateConcavityMetric (dgFloat64 convexConcavity, dgFloat64 area, dgFloat64 perimeter, dgInt32 faceCountA, dgInt32 faceCountB) const 
	{
		dgFloat64 edgeCost = perimeter * perimeter / (dgFloat64(4.0f * dgPi) * area);
		return convexConcavity * DG_CONCAVITY_SCALE + edgeCost + ConcavityByFaceMedian (faceCountA, faceCountB);
	}

	void SubmitInitialEdgeCosts (dgMeshEffect& mesh) 
	{
		m_mark ++;
		for (dgListNode* clusterNodeA = GetFirst(); clusterNodeA; clusterNodeA = clusterNodeA->GetNext()) {
			// call the progress callback
			//ReportProgress();

			for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeAB = clusterNodeA->GetInfo().GetFirst(); edgeNodeAB; edgeNodeAB = edgeNodeAB->GetNext()) {
				dgHACDEdge& edgeAB = edgeNodeAB->GetInfo().m_edgeData;
				dgFloat64 weight = edgeAB.m_backFaceHandicap; 
				if (edgeAB.m_mark != m_mark) {
					edgeAB.m_mark = m_mark;
					dgListNode* const clusterNodeB = edgeNodeAB->GetInfo().m_node;
					for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeBA = clusterNodeB->GetInfo().GetFirst(); edgeNodeBA; edgeNodeBA = edgeNodeBA->GetNext()) {
						dgListNode* const clusterNode = edgeNodeBA->GetInfo().m_node;
						if (clusterNode == clusterNodeA) {
							dgHACDEdge& edgeBA = edgeNodeBA->GetInfo().m_edgeData;
							edgeBA.m_mark = m_mark;
							dgAssert (!edgeAB.m_proxyListNode);
							dgAssert (!edgeBA.m_proxyListNode);

							dgList<dgPairProxy>::dgListNode* const proxyNode = SubmitEdgeCost (mesh, clusterNodeA, clusterNodeB, weight * edgeBA.m_backFaceHandicap);
							edgeAB.m_proxyListNode = proxyNode;
							edgeBA.m_proxyListNode = proxyNode;
							break;
						}
					}
				}
			}
		}
	}

	dgInt32 CopyVertexToPool(const dgMeshEffect& mesh, const dgHACDCluster& cluster, dgInt32 start)
	{
		dgInt32 count = start;

		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();
		for (dgList<dgHACDClusterFace>::dgListNode* node = cluster.GetFirst(); node; node = node->GetNext()) {
			const dgHACDClusterFace& clusterFace = node->GetInfo();
			dgEdge* edge = clusterFace.m_edge;
			do {
				dgInt32 index = edge->m_incidentVertex;
				if (m_vertexMarks[index] != m_vertexMark) {
					m_vertexMarks[index] = m_vertexMark;
					m_vertexPool[count] = points[index];
					count++;
				}
				edge = edge->m_next;
			} while (edge != clusterFace.m_edge);
		}
		return count;
	}


	void MarkInteriorClusterEdges (dgMeshEffect& mesh, dgInt32 mark, const dgHACDCluster& cluster, dgInt32 colorA, dgInt32 colorB) const
	{
		dgAssert (colorA != colorB);
		for (dgList<dgHACDClusterFace>::dgListNode* node = cluster.GetFirst(); node; node = node->GetNext()) {
			dgHACDClusterFace& clusterFace = node->GetInfo();
			dgEdge* edge = clusterFace.m_edge;
			do {
				if ((edge->m_twin->m_incidentFace == colorA) || (edge->m_twin->m_incidentFace == colorB)) {
					edge->m_mark = mark;
					edge->m_twin->m_mark = mark;
				}
				edge = edge->m_next;
			} while (edge != clusterFace.m_edge);
		}
	}

	dgFloat64 CalculateClusterPerimeter (dgMeshEffect& mesh, dgInt32 mark, const dgHACDCluster& cluster, dgInt32 colorA, dgInt32 colorB) const
	{
		dgAssert (colorA != colorB);
		dgFloat64 perimeter = dgFloat64 (0.0f);
		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();
		for (dgList<dgHACDClusterFace>::dgListNode* node = cluster.GetFirst(); node; node = node->GetNext()) {
			dgHACDClusterFace& clusterFace = node->GetInfo();
			dgEdge* edge = clusterFace.m_edge;
			do {
				if (!((edge->m_twin->m_incidentFace == colorA) || (edge->m_twin->m_incidentFace == colorB))) {
					dgBigVector p1p0(points[edge->m_twin->m_incidentVertex] - points[edge->m_incidentVertex]);
					perimeter += sqrt(p1p0 % p1p0);
				}
				edge = edge->m_next;
			} while (edge != clusterFace.m_edge);
		}

		return perimeter;
	}

	void HeapCollectGarbage () 
	{
		if ((m_priorityHeap.GetCount() + 20) > m_priorityHeap.GetMaxCount()) {
			for (dgInt32 i = m_priorityHeap.GetCount() - 1; i >= 0; i--) {
				dgList<dgPairProxy>::dgListNode* const emptyNode = m_priorityHeap[i];
				dgPairProxy& emptyPair = emptyNode->GetInfo();
				if ((emptyPair.m_nodeA == NULL) && (emptyPair.m_nodeB == NULL)) {
					m_priorityHeap.Remove(i);
				}
			}
		}
	}


	dgFloat64 CalculateConcavity(dgHACDConveHull& hull, const dgMeshEffect& mesh, const dgHACDCluster& cluster)
	{
		dgFloat64 concavity = dgFloat32(0.0f);

		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();
		for (dgList<dgHACDClusterFace>::dgListNode* node = cluster.GetFirst(); node; node = node->GetNext()) {
			dgHACDClusterFace& clusterFace = node->GetInfo();
			dgEdge* edge = clusterFace.m_edge;
			dgInt32 i0 = edge->m_incidentVertex;
			dgInt32 i1 = edge->m_next->m_incidentVertex;
			for (dgEdge* ptr = edge->m_next->m_next; ptr != edge; ptr = ptr->m_next) {
				dgInt32 i2 = ptr->m_incidentVertex;
				dgFloat64 val = hull.CalculateTriangleConcavity(clusterFace.m_normal, i0, i1, i2, points);
				if (val > concavity) {
					concavity = val;
				}
				i1 = i2;
			}
		}

		return concavity;
	}

	dgFloat64 CalculateConcavitySingleThread (dgHACDConveHull& hull, dgMeshEffect& mesh, dgHACDCluster& clusterA, dgHACDCluster& clusterB)
	{
		return dgMax(CalculateConcavity(hull, mesh, clusterA), CalculateConcavity(hull, mesh, clusterB));
	}


	class dgConvexHullRayCastContext
	{
		public: 
		dgConvexHullRayCastContext (dgHACDConveHull& hull, dgMeshEffect& mesh, dgThreadHive* const manager)
			:m_atomicLock(0)
			,m_mesh(&mesh)
			,m_cluster(NULL)
			,m_threadManager(manager)
			,m_faceNode(NULL)
		{
			for(dgInt32 i = 0; i < DG_CONCAVITY_MAX_THREADS; i ++) {
				hullArray[i] = new (mesh.GetAllocator()) dgHACDConveHull (hull);
			}
		}

		~dgConvexHullRayCastContext ()
		{
			for(dgInt32 i = 0; i < DG_CONCAVITY_MAX_THREADS; i ++) {
				delete hullArray[i];
			}
		}

		void SetCluster (dgHACDCluster& cluster)
		{
			m_cluster = &cluster;
			m_node = m_cluster->GetFirst();
			memset (m_concavity, 0, sizeof (m_concavity));
		}

		dgFloat64 GetConcavity() const 
		{
			dgFloat64 concavity = dgFloat32(0.0f);
			for (dgInt32 i = 0; i < DG_CONCAVITY_MAX_THREADS; i ++) {	
				if (concavity < m_concavity[i]) {
					concavity = m_concavity[i];
				}
			}
			return concavity;
		}


		static void RayCastKernel (void* const context, dgInt32 threadID)
		{
			dgConvexHullRayCastContext* const data = (dgConvexHullRayCastContext*) context;
			const dgBigVector* const points = (dgBigVector*) data->m_mesh->GetVertexPool();
			
			data->m_threadManager->GetIndirectLock(&data->m_atomicLock, threadID);
			dgList<dgHACDClusterFace>::dgListNode* node = data->m_node;
			if (node) {
				data->m_node = node->GetNext();
			}
			data->m_threadManager->ReleaseIndirectLock (&data->m_atomicLock);
			for (; node;) {

				dgHACDClusterFace& clusterFace = node->GetInfo();
				dgEdge* edge = clusterFace.m_edge;
				dgInt32 i0 = edge->m_incidentVertex;
				dgInt32 i1 = edge->m_next->m_incidentVertex;
				for (dgEdge* ptr = edge->m_next->m_next; ptr != edge; ptr = ptr->m_next) {
					dgInt32 i2 = ptr->m_incidentVertex;
					dgFloat64 val = data->hullArray[threadID]->CalculateTriangleConcavity(clusterFace.m_normal, i0, i1, i2, points);
					if (val > data->m_concavity[threadID]) {
						data->m_concavity[threadID] = val;
					}
					i1 = i2;
				}

				data->m_threadManager->GetIndirectLock(&data->m_atomicLock, threadID);
				node = data->m_node;
				if (node) {
					data->m_node = node->GetNext();;
				}
				data->m_threadManager->ReleaseIndirectLock (&data->m_atomicLock);
			}
		}


		dgInt32 m_atomicLock;
		dgMeshEffect* m_mesh;
		dgHACDCluster* m_cluster;
		dgThreadHive* m_threadManager;
		dgList<dgHACDClusterFace>::dgListNode* m_node;

		dgList<dgHACDClusterFace>::dgListNode* m_faceNode;
		dgFloat64 m_concavity[DG_CONCAVITY_MAX_THREADS];
		dgHACDConveHull* hullArray[DG_CONCAVITY_MAX_THREADS];		
	};


	dgFloat64 CalculateConcavityMultiThread (dgHACDConveHull& hull, dgMeshEffect& mesh, dgHACDCluster& clusterA, dgHACDCluster& clusterB)
	{
		dgConvexHullRayCastContext data (hull, mesh, &m_parallerConcavityCalculator);

		dgInt32 threadsCount = m_parallerConcavityCalculator.GetThreadCount();	
		data.SetCluster (clusterA);
		for (dgInt32 i = 0; i < threadsCount; i ++) {		
			m_parallerConcavityCalculator.QueueJob(dgConvexHullRayCastContext::RayCastKernel, &data);
		}
		m_parallerConcavityCalculator.SynchronizationBarrier();
		dgFloat64 concavity = data.GetConcavity();

		data.SetCluster (clusterB);
		for (dgInt32 i = 0; i < threadsCount; i ++) {		
			m_parallerConcavityCalculator.QueueJob(dgConvexHullRayCastContext::RayCastKernel, &data);
		}
		m_parallerConcavityCalculator.SynchronizationBarrier();
		
		concavity = dgMax(concavity, data.GetConcavity());
		//dgFloat64 xxx = CalculateConcavitySingleThread (hull, mesh, clusterA, clusterB);
		//dgAssert (fabs(concavity - xxx) < dgFloat64 (1.0e-5f));
		return concavity;
	}

	dgList<dgPairProxy>::dgListNode* SubmitEdgeCost (dgMeshEffect& mesh, dgListNode* const clusterNodeA, dgListNode* const clusterNodeB, dgFloat64 perimeterHandicap)
	{
		dgHACDCluster& clusterA = clusterNodeA->GetInfo().m_nodeData;
		dgHACDCluster& clusterB = clusterNodeB->GetInfo().m_nodeData;
		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();

		bool flatStrip = true;
		dgFloat64 tol = dgFloat64 (1.0e-5f) * m_diagonal;
		dgHACDClusterFace& clusterFaceA = clusterA.GetFirst()->GetInfo();
		dgBigPlane plane(clusterFaceA.m_normal, -(points[clusterFaceA.m_edge->m_incidentVertex] % clusterFaceA.m_normal));

		if (clusterA.GetCount() > 1) {
			flatStrip = clusterA.IsCoplanar(plane, mesh, tol);
		}

		if (flatStrip) {
			flatStrip = clusterB.IsCoplanar(plane, mesh, tol);
		}

		dgList<dgPairProxy>::dgListNode* pairNode = NULL;
		if (!flatStrip) {
			m_vertexMark ++;
			dgInt32 vertexCount = CopyVertexToPool(mesh, clusterA, 0);
			vertexCount = CopyVertexToPool(mesh, clusterB, vertexCount);

			dgHACDConveHull convexHull(mesh.GetAllocator(), m_vertexPool, vertexCount);

			if (convexHull.GetVertexCount()) {
				dgInt32 mark = mesh.IncLRU();
				MarkInteriorClusterEdges (mesh, mark, clusterA, clusterA.m_color, clusterB.m_color);
				MarkInteriorClusterEdges (mesh, mark, clusterB, clusterA.m_color, clusterB.m_color);

				dgFloat64 area = clusterA.m_area + clusterB.m_area;
				dgFloat64 perimeter = CalculateClusterPerimeter (mesh, mark, clusterA, clusterA.m_color, clusterB.m_color) +
									  CalculateClusterPerimeter (mesh, mark, clusterB, clusterA.m_color, clusterB.m_color);

	
				dgFloat64 concavity = dgFloat64 (0.0f);
				if ((convexHull.GetCount() > 128) && ((clusterA.GetCount() > 256) || (clusterB.GetCount() > 256))) { 
					concavity = CalculateConcavityMultiThread (convexHull, mesh, clusterA, clusterB);
				} else {
					concavity = CalculateConcavitySingleThread (convexHull, mesh, clusterA, clusterB);
				}

				if (concavity < dgFloat64(1.0e-3f)) {
					concavity = dgFloat64(0.0f);
				}

				// see if the heap will overflow
				HeapCollectGarbage ();

				// add a new pair to the heap
				dgList<dgPairProxy>::dgListNode* pairNode = m_proxyList.Append();
				dgPairProxy& pair = pairNode->GetInfo();
				pair.m_nodeA = clusterNodeA;
				pair.m_nodeB = clusterNodeB;
				pair.m_distanceConcavity = concavity;
				pair.m_hierachicalClusterIndexA = clusterA.m_hierachicalClusterIndex;
				pair.m_hierachicalClusterIndexB = clusterB.m_hierachicalClusterIndex;

				pair.m_area = area;
				dgFloat64 cost = CalculateConcavityMetric (concavity, area, perimeter * perimeterHandicap, clusterA.GetCount(), clusterB.GetCount());
				m_priorityHeap.Push(pairNode, cost);

				return pairNode;
			}
		}
		return pairNode;
	}


	void CollapseEdge (dgList<dgPairProxy>::dgListNode* const pairNode, dgMeshEffect& mesh, dgFloat64 concavity)
	{
		dgListNode* adjacentNodes[1024];
		dgPairProxy& pair = pairNode->GetInfo();

		dgMemoryAllocator* const allocator = mesh.GetAllocator();


		dgAssert((pair.m_nodeA && pair.m_nodeB) || (!pair.m_nodeA && !pair.m_nodeB));
		if (pair.m_nodeA && pair.m_nodeB) {
			// call the progress callback
			ReportProgress();

			dgListNode* const clusterNodeA = pair.m_nodeA;
			dgListNode* const clusterNodeB = pair.m_nodeB;
			dgAssert (clusterNodeA != clusterNodeB);

			dgHACDCluster& clusterA = clusterNodeA->GetInfo().m_nodeData;
			dgHACDCluster& clusterB = clusterNodeB->GetInfo().m_nodeData;

			dgAssert (&clusterA != &clusterB);
			dgAssert(clusterA.m_color != clusterB.m_color);

			dgHACDConvacityLookAheadTree* const leftTree = m_concavityTreeArray[pair.m_hierachicalClusterIndexA];
			dgHACDConvacityLookAheadTree* const rightTree = m_concavityTreeArray[pair.m_hierachicalClusterIndexB];
			dgAssert (leftTree);
			dgAssert (rightTree);
			m_concavityTreeArray[pair.m_hierachicalClusterIndexA] = NULL;
			m_concavityTreeArray[pair.m_hierachicalClusterIndexB] = NULL;
			dgAssert (m_cancavityTreeIndex < (2 * (m_faceCount + 1)));

			dgFloat64 treeConcavity = pair.m_distanceConcavity;
//			 dgAssert (treeConcavity < 0.1);
			m_concavityTreeArray[m_cancavityTreeIndex] = new (allocator) dgHACDConvacityLookAheadTree (allocator, leftTree, rightTree, treeConcavity);
			clusterA.m_hierachicalClusterIndex = m_cancavityTreeIndex;
			clusterB.m_hierachicalClusterIndex = m_cancavityTreeIndex;
			m_cancavityTreeIndex ++;

			// merge two clusters
			while (clusterB.GetCount()) {

				dgHACDCluster::dgListNode* const nodeB = clusterB.GetFirst();
				clusterB.Unlink(nodeB);
	
				// now color code all faces of the merged cluster
				dgHACDClusterFace& faceB = nodeB->GetInfo();
				dgEdge* ptr = faceB.m_edge;
				do {
					ptr->m_incidentFace = clusterA.m_color;
					ptr = ptr->m_next;
				} while (ptr != faceB.m_edge);
				clusterA.Append(nodeB);
			}
			clusterA.m_area = pair.m_area;
			clusterA.m_concavity = concavity;

			// invalidate all proxies that are still in the heap
			dgInt32 adjacentCount = 1;
			adjacentNodes[0] = clusterNodeA;
			for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeAB = clusterNodeA->GetInfo().GetFirst(); edgeNodeAB; edgeNodeAB = edgeNodeAB->GetNext()) {
				dgHACDEdge& edgeAB = edgeNodeAB->GetInfo().m_edgeData;
				dgList<dgPairProxy>::dgListNode* const proxyNode = (dgList<dgPairProxy>::dgListNode*) edgeAB.m_proxyListNode;
				if (proxyNode) {
					dgPairProxy& pairProxy = proxyNode->GetInfo();
					dgAssert ((edgeNodeAB->GetInfo().m_node == pairProxy.m_nodeA) || (edgeNodeAB->GetInfo().m_node == pairProxy.m_nodeB));
					pairProxy.m_nodeA = NULL;
					pairProxy.m_nodeB = NULL;
					edgeAB.m_proxyListNode = NULL;
				}

				adjacentNodes[adjacentCount] = edgeNodeAB->GetInfo().m_node;
				adjacentCount ++;
				dgAssert (adjacentCount < sizeof (adjacentNodes)/ sizeof (adjacentNodes[0]));
			}

			for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeBA = clusterNodeB->GetInfo().GetFirst(); edgeNodeBA; edgeNodeBA = edgeNodeBA->GetNext()) {
				dgHACDEdge& edgeBA = edgeNodeBA->GetInfo().m_edgeData;
				dgList<dgPairProxy>::dgListNode* const proxyNode = (dgList<dgPairProxy>::dgListNode*) edgeBA.m_proxyListNode;
				if (proxyNode) {
					dgPairProxy& pairProxy = proxyNode->GetInfo();
					pairProxy.m_nodeA = NULL;
					pairProxy.m_nodeB = NULL;
					edgeBA.m_proxyListNode = NULL;
				}

				bool alreadyLinked = false;
				dgListNode* const node = edgeNodeBA->GetInfo().m_node;
				for (dgInt32 i = 0; i < adjacentCount; i ++) {
					if (node == adjacentNodes[i]) {
						alreadyLinked = true;
						break;
					}
				}
				if (!alreadyLinked) {
					clusterNodeA->GetInfo().AddEdge (node);
					node->GetInfo().AddEdge (clusterNodeA);
				}
			}
			DeleteNode (clusterNodeB);

			// submit all new costs for each edge connecting this new node to any other node 
			for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeAB = clusterNodeA->GetInfo().GetFirst(); edgeNodeAB; edgeNodeAB = edgeNodeAB->GetNext()) {
				dgHACDEdge& edgeAB = edgeNodeAB->GetInfo().m_edgeData;
				dgListNode* const clusterNodeB = edgeNodeAB->GetInfo().m_node;
				dgFloat64 weigh = edgeAB.m_backFaceHandicap;
				for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeBA = clusterNodeB->GetInfo().GetFirst(); edgeNodeBA; edgeNodeBA = edgeNodeBA->GetNext()) {
					dgListNode* const clusterNode = edgeNodeBA->GetInfo().m_node;
					if (clusterNode == clusterNodeA) {
						dgHACDEdge& edgeBA = edgeNodeBA->GetInfo().m_edgeData;
						dgList<dgPairProxy>::dgListNode* const proxyNode = SubmitEdgeCost (mesh, clusterNodeA, clusterNodeB, weigh * edgeBA.m_backFaceHandicap);
						if (proxyNode) {
							edgeBA.m_proxyListNode = proxyNode;
							edgeAB.m_proxyListNode = proxyNode;
						}
						break;
					}
				}
			}
		}
		m_proxyList.Remove(pairNode);
	}

#ifdef DG_BUILD_HIERACHICAL_HACD
	void CollapseClusters (dgMeshEffect& mesh, dgFloat64 maxConcavity, dgInt32 maxClustesCount)
	{

		maxConcavity *= (m_diagonal * DG_CONCAVITY_SCALE);
		while (m_priorityHeap.GetCount()) {
			dgFloat64 concavity =  m_priorityHeap.Value();
			dgList<dgPairProxy>::dgListNode* const pairNode = m_priorityHeap[0];
			m_priorityHeap.Pop();
			CollapseEdge (pairNode, mesh, concavity);

//if (m_progress == 24)
//break;

		}



		dgInt32 treeCounts = 0;
		for (dgInt32 i = 0; i < m_cancavityTreeIndex; i ++) {
			if (m_concavityTreeArray[i]) {
				m_concavityTreeArray[treeCounts] = m_concavityTreeArray[i];
				m_concavityTreeArray[i] = NULL;
				treeCounts ++;
			}
		}

		if (treeCounts > 1) {

			for (dgInt32 i = 0; i < treeCounts; i ++) {
				if (m_concavityTreeArray[i]->m_faceList.GetCount()==1) {
					delete m_concavityTreeArray[i];
					m_concavityTreeArray[i] = m_concavityTreeArray[treeCounts-1];
					m_concavityTreeArray[treeCounts-1]= NULL;
					treeCounts --;
					i--;
				}
			}


			dgFloat32 C = 10000;
			while (treeCounts > 1)	 {
				dgHACDConvacityLookAheadTree* const leftTree = m_concavityTreeArray[treeCounts-1];
				dgHACDConvacityLookAheadTree* const rightTree = m_concavityTreeArray[treeCounts-2];
				m_concavityTreeArray[treeCounts-1] = NULL;
				m_concavityTreeArray[treeCounts-2] = new (mesh.GetAllocator()) dgHACDConvacityLookAheadTree (mesh.GetAllocator(), leftTree, rightTree, C);
				C *= 2;
				treeCounts --;
			}

		}

		dgHACDConvacityLookAheadTree* const tree = m_concavityTreeArray[0];
		dgDownHeap<dgHACDConvacityLookAheadTree*, dgFloat64> approximation(maxClustesCount * 2, mesh.GetAllocator());

		tree->ReduceByCount (maxClustesCount, approximation);
		//		tree->ReduceByConcavity (maxConcavity, approximation);

		while (approximation.GetCount()) {
			m_convexProximation.Append(approximation[0]);
			approximation.Pop();
		}
	}
#else 
	void CollapseClusters (dgMeshEffect& mesh, dgFloat64 maxConcavity, dgInt32 maxClustesCount)
	{
		maxConcavity *= (m_diagonal * DG_CONCAVITY_SCALE);

		bool terminate = false;
		while (m_priorityHeap.GetCount() && !terminate) {
			dgFloat64 concavity =  m_priorityHeap.Value();
			dgList<dgPairProxy>::dgListNode* const pairNode = m_priorityHeap[0];
			if ((concavity < maxConcavity) && (GetCount() < maxClustesCount)) {
				terminate  = true;
			} else {
				m_priorityHeap.Pop();
				CollapseEdge (pairNode, mesh, concavity);
			}
		}
	}
#endif

	dgInt32 m_mark;
	dgInt32 m_faceCount;
	dgInt32 m_vertexMark;
	dgInt32 m_progress;
	dgInt32 m_cancavityTreeIndex;
	dgInt32* m_vertexMarks;
	dgFloat32 m_invFaceCount;
	dgFloat64 m_diagonal;
	dgBigVector* m_vertexPool;
	dgList<dgPairProxy> m_proxyList;
	dgHACDConvacityLookAheadTree** m_concavityTreeArray;	
	dgList<dgHACDConvacityLookAheadTree*> m_convexProximation;
	dgUpHeap<dgList<dgPairProxy>::dgListNode*, dgFloat64> m_priorityHeap;
	dgReportProgress m_reportProgressCallback;
	dgThreadHive m_parallerConcavityCalculator;
};

#endif

dgMeshEffect* dgMeshEffect::CreateSimplification(dgInt32 maxVertexCount, dgReportProgress reportProgressCallback, void* const reportPrgressUserData) const
{
	if (GetVertexCount() <= maxVertexCount) {
		return new (GetAllocator()) dgMeshEffect(*this); 
	}
dgAssert (0);
return new (GetAllocator()) dgMeshEffect(*this); 
/*
	//	dgMeshEffect triangleMesh(*this);
	if (maxHullsCount <= 1) {
		maxHullsCount = 1;
	}
	if (maxConcavity <= dgFloat32 (1.0e-5f)) {
		maxConcavity = dgFloat32 (1.0e-5f);
	}

	if (maxVertexPerHull < 4) {
		maxVertexPerHull = 4;
	}
	ClampValue(backFaceDistanceFactor, dgFloat32 (0.01f), dgFloat32 (1.0f));

	if (reportProgressCallback) {
		reportProgressCallback (0.0f);
	}


	// make a copy of the mesh
	dgMeshEffect mesh(*this);
	mesh.ClearAttributeArray();


	dgInt32 faceCount = mesh.GetTotalFaceCount();
	if (faceCount  > meshSimplicationMaxFaceCount) {
		mesh.Triangulate();

		dgPolyhedra polygon(GetAllocator());
		dgInt32 mark = mesh.IncLRU();
		polygon.BeginFace();
		dgPolyhedra::Iterator iter (mesh);
		for (iter.Begin(); iter; iter ++){
			dgEdge* const face = &(*iter);

			if ((face->m_mark != mark) && (face->m_incidentFace > 0)) {
				dgInt32	index[DG_MESH_EFFECT_POINT_SPLITED];

				dgEdge* ptr = face;
				dgInt32 indexCount = 0;
				do {
					index[indexCount] = ptr->m_incidentVertex;
					ptr->m_mark = mark;
					indexCount ++;
					ptr = ptr->m_next;
				} while (ptr != face);
				polygon.AddFace(indexCount, index);
			}
		}
		polygon.EndFace();

		polygon.Optimize(&mesh.m_points[0].m_x, sizeof (dgFloat64), 1000.0f, meshSimplicationMaxFaceCount);

		mesh.RemoveAll();
		
		mark = polygon.IncLRU();
		mesh.BeginFace();
		dgPolyhedra::Iterator iter1 (polygon);
		for (iter1.Begin(); iter1; iter1 ++){
			dgEdge* const face = &(*iter1);
			if ((face->m_mark != mark) && (face->m_incidentFace > 0)) {
				dgInt32	index[DG_MESH_EFFECT_POINT_SPLITED];
				dgEdge* ptr = face;
				dgInt32 indexCount = 0;
				do {
					ptr->m_mark = mark;
					index[indexCount] = dgInt32 (ptr->m_incidentVertex);
					indexCount ++;
					ptr = ptr->m_next;
				} while (ptr != face);
				mesh.AddFace(indexCount, index);
			}
		}
		mesh.EndFace();

		faceCount = mesh.GetTotalFaceCount();
		mesh.ClearAttributeArray();
	}

	// create a general connectivity graph    
	dgHACDClusterGraph graph (mesh, backFaceDistanceFactor, reportProgressCallback);

	// calculate initial edge costs
	graph.SubmitInitialEdgeCosts (mesh);

	// collapse the graph
	graph.CollapseClusters (mesh, maxConcavity, maxHullsCount);

	// Create Partition Mesh
	return graph.CreatePatitionMesh (mesh, maxVertexPerHull);
*/
}
