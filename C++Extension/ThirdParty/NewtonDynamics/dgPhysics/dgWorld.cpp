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
#include "dgWorld.h"

#include "dgDynamicBody.h"
#include "dgKinematicBody.h"
#include "dgCollisionBox.h"
#include "dgKinematicBody.h"
#include "dgCollisionNull.h"
#include "dgCollisionCone.h"
#include "dgCollisionScene.h"
#include "dgCollisionSphere.h"
#include "dgBroadPhaseMixed.h"
#include "dgCollisionCapsule.h"
#include "dgCollisionInstance.h"
#include "dgCollisionCompound.h"
#include "dgWorldDynamicUpdate.h"
#include "dgCollisionConvexHull.h"
#include "dgBroadPhaseSegregated.h"
#include "dgCollisionChamferCylinder.h"

#include "dgUserConstraint.h"
#include "dgBallConstraint.h"
#include "dgHingeConstraint.h"
#include "dgSkeletonContainer.h"
#include "dgSlidingConstraint.h"
#include "dgUpVectorConstraint.h"
#include "dgUniversalConstraint.h"
#include "dgCorkscrewConstraint.h"

#define DG_DEFAULT_SOLVER_ITERATION_COUNT	4
#define DG_SYNC_THREAD	1
#define DG_ASYNC_THREAD	2

/*
static dgInt32 TestSort(const dgInt32* const  A, const dgInt32* const B, void* const)
{
	if (*A < *B) {
		return -1;
	} else if (*A > *B) {
		return 1;
	}
	return 0;
}

static void TestSort()
{
	int array[] = { 10, 10, 10, 10, 10, 5, 10, 10, 10, 10, 10, 10 };
	dgSort(array, sizeof(array) / sizeof(array[0]), TestSort);
}


static  char *xxx[10] = {"bbbbbbbbbb",
						 "baaaaaaaab",
						 "babbbbbaab",
					 	 "babaaabaab",
						 "baaababaab",
						 "bbbaaabaab",
						 "bbbbaabaab",
						 "babbbbbaab",
						 "baaaaaaaab",
						 "bbbbbbbbbb"};

struct myPath: public dgPathFinder<dgInt32, float> 
{
	dgInt32 goalx;
	dgInt32 goaly;
	myPath ()
		:dgPathFinder<dgInt32, float>(1024, 10)
	{
		for (int i = 0; i < 10; i ++) {
			strcpy (&m_map[i][0], xxx[i]);
		}
	}

	const dgPathNode<dgInt32, float>* CalCulatePath (dgInt32 source, dgInt32 goal)
	{
		goalx = goal % 10;
		goaly = goal / 10;
		return dgPathFinder<dgInt32, float>::CalCulatePath (source, goal) ;
	}


	float GetCostFromParent(const dgPathNode<dgInt32, float>& node) const
	{
		dgInt32 x;
		dgInt32 y;
		dgInt32 x0;
		dgInt32 y0;
		dgInt32 x1;
		dgInt32 y1;
		dgInt32 id;

		id = node.GetId();
		x = id % 10;
		y = id / 10;

		const dgPathNode<dgInt32, float>* parent = node.GetParent();
		id = parent->GetId(); 
		x0 = id % 10;
		y0 = id / 10;

		const dgPathNode<dgInt32, float>* grandParent = parent->GetParent();
		x1 = 2 * x0 - x;
		y1 = 2 * y0 - y;
		if (grandParent) {
			id = grandParent->GetId(); 
			x1 = id % 10;
			y1 = id / 10;
		}

		dgInt32 dx0;
		dgInt32 dy0;
		dgInt32 dx1;
		dgInt32 dy1;
		float penalty;

		dx0 = x0 - x;
		dy0 = y0 - y;
		dx1 = x1 - x0;
		dy1 = y1 - y0;
		penalty = 0.0f;
		if (dx1 * dy0 - dx0 * dy1)	{
			penalty = dgFloat32(1.0f);
		}

		static dgInt32 xxxx;
		if (!xxxx){
			xxxx = 1;
			penalty = 9.1f;
		}

		return (xxx[y][x] == 'a') ? (dgFloat32(1.0f) + penalty): 50.0f;
	}

	float GetEstimatedCostToGoal(dgInt32 id) const
	{
		dgInt32 x;
		dgInt32 y;

		x = id % 10 - goalx;
		y = id / 10 - goaly;
		return dgSqrt ((float)(x * x + y * y));
	}

	dgInt32 EnumerateChildren(dgInt32 parent, dgInt32 array[]) const
	{
		dgInt32 x;
		dgInt32 y;

		x = parent % 10;
		y = parent / 10;

		array[0]	= (y - 1) * 10 + x;
		array[1]	= (y - 0) * 10 + x - 1;
		array[2]	= (y + 1) * 10 + x;
		array[3]	= (y + 0) * 10 + x + 1;
		return 4;
	}

	char m_map[20][20];
};


static void TestAStart()
{
	myPath path; 
	const dgPathNode<dgInt32, float>* firtNode;
	for (firtNode = path.CalCulatePath (5 * 10 + 3, 4 * 10 + 8); firtNode; firtNode= firtNode->GetNext()) {
		dgInt32 id;
		dgInt32 x;
		dgInt32 y;

		id = firtNode->GetId();
		x = id % 10;
		y = id / 10;
		path.m_map[y][x] = '_';
	}
}


class TestGradient: public dgSymmetricConjugateGradientSolver<dgFloat32>
{
	public:
	TestGradient()
		:dgSymmetricConjugateGradientSolver()
	{
		dgFloat32 xxx[4];
		dgFloat32 b[4];
		xxx[0] = 1.0f; xxx[1] = 2.0f; xxx[2] = 3.0f;; xxx[3] = 4.0f;
		dgCovarianceMatrix(4, &A[0][0], xxx, xxx);
		A[0][0] = 10.0f; A[1][1] = 10.0f; A[2][2] = 10.0f;
		dgAssert(dgTestPSDmatrix(4, 4, &A[0][0]));
		dgMatrixTimeVector(4, &A[0][0], xxx, b);

		dgFloat32 x[4];
		x[0] = 0.0f; x[1] = 0.0f; x[2] = 0.0f;; x[3] = 0.0f;
		Solve(4, 1.0e-6f, x, b);
	}

	virtual void MatrixTimeVector(dgFloat32* const out, const dgFloat32* const v) const
	{
		dgMatrixTimeVector(4, &A[0][0], v, out);
	}

	virtual void InversePrecoditionerTimeVector(dgFloat32* const out, const dgFloat32* const v) const
	{
		for (int i = 0; i < 4; i++) {
			out[i] = v[i] / A[i][i];
		}
	}


	dgFloat32 A[4][4];
};
*/

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

dgWorld::dgWorld(dgMemoryAllocator* const allocator)
	:dgBodyMasterList(allocator)
	,dgBodyMaterialList(allocator)
	,dgBodyCollisionList(allocator)
	,dgSkeletonList(allocator)
	,dgContactList(allocator) 
	,dgBilateralConstraintList(allocator)
	,dgWorldDynamicUpdate(allocator)
	,dgMutexThread("newtonMainThread", DG_SYNC_THREAD)
	,dgAsyncThread("newtonAsyncThread", DG_ASYNC_THREAD)
	,dgWorldThreadPool(allocator)
	,dgDeadBodies(allocator)
	,dgDeadJoints(allocator)
	,dgWorldPluginList(allocator)
	,m_broadPhase(NULL)
	,m_sentinelBody(NULL)
	,m_pointCollision(NULL)
	,m_userData(NULL)
	,m_allocator (allocator)
	,m_onPostUpdateCallback(NULL)
	,m_listeners(allocator)
	,m_perInstanceData(allocator)
	,m_bodiesMemory (allocator, 64)
	,m_jointsMemory (allocator, 64)
	,m_clusterMemory (allocator, 64)
	,m_solverJacobiansMemory (allocator, 64)
	,m_solverRightHandSideMemory (allocator, 64)
	,m_solverForceAccumulatorMemory (allocator, 64)
{
	//TestAStart();
	//TestSort();
	//TestGradient();

	dgMutexThread* const myThread = this;
	SetParentThread (myThread);

	// avoid small memory fragmentations on initialization
	m_bodiesMemory.Resize(1024);
	m_clusterMemory.Resize(1024);
	m_jointsMemory.Resize(1024 * 2);
	m_solverJacobiansMemory.Resize(1024 * 64);
	m_solverRightHandSideMemory.Resize(1024 * 64);
	m_solverForceAccumulatorMemory.Resize(1024 * 32);

	m_savetimestep = dgFloat32 (0.0f);
	m_allocator = allocator;

	m_onCollisionInstanceDestruction = NULL;
	m_onCollisionInstanceCopyConstrutor = NULL;

	m_onSerializeJointCallback = NULL;	
	m_onDeserializeJointCallback = NULL;	

	m_inUpdate = 0;
	m_clusterLRU = 0;
	m_bodyGroupID = 0;
	m_frameNumber = 0;
	m_dynamicsLru = 0;
	m_genericLRUMark = 0;
	m_bodiesUniqueID = 0;
	m_numberOfSubsteps = 1;
	m_useParallelSolver = 1;
	m_lastExecutionTime = 0;
	m_defualtBodyGroupID = CreateBodyGroupID();
	m_solverIterations = DG_DEFAULT_SOLVER_ITERATION_COUNT;
	
	m_frictiomTheshold = dgFloat32 (0.25f);

	m_userData = NULL;
	m_onClusterUpdate = NULL;
	m_onCreateContact = NULL;
	m_onDestroyContact = NULL;


	m_freezeAccel2 = DG_FREEZE_ACCEL2;
	m_freezeAlpha2 = DG_FREEZE_ACCEL2;
	m_freezeSpeed2 = DG_FREEZE_SPEED2;
	m_freezeOmega2 = DG_FREEZE_SPEED2;

	m_contactTolerance = DG_PRUNE_CONTACT_TOLERANCE;

	dgInt32 steps = 1;
	dgFloat32 freezeAccel2 = m_freezeAccel2;
	dgFloat32 freezeAlpha2 = m_freezeAlpha2;
	dgFloat32 freezeSpeed2 = m_freezeSpeed2;
	dgFloat32 freezeOmega2 = m_freezeOmega2;
	for (dgInt32 i = 0; i < DG_SLEEP_ENTRIES; i ++) {
		m_sleepTable[i].m_maxAccel = freezeAccel2;
		m_sleepTable[i].m_maxAlpha = freezeAlpha2;
		m_sleepTable[i].m_maxVeloc = freezeSpeed2;
		m_sleepTable[i].m_maxOmega = freezeOmega2;
		m_sleepTable[i].m_steps = steps;
		steps += 7;
		freezeAccel2 *= dgFloat32 (1.5f);
		freezeAlpha2 *= dgFloat32 (1.5f);
		freezeSpeed2 *= dgFloat32 (1.5f);
		freezeOmega2 *= dgFloat32 (1.5f);
	}

	m_sleepTable[0].m_maxAccel *= dgFloat32(0.009f);
	m_sleepTable[0].m_maxAlpha *= dgFloat32(0.009f);

	steps += 300;
	m_sleepTable[DG_SLEEP_ENTRIES - 1].m_maxAccel *= dgFloat32 (100.0f);
	m_sleepTable[DG_SLEEP_ENTRIES - 1].m_maxAlpha *= dgFloat32 (100.0f);
	m_sleepTable[DG_SLEEP_ENTRIES - 1].m_maxVeloc = 0.25f;
	m_sleepTable[DG_SLEEP_ENTRIES - 1].m_maxOmega = 0.1f;
	m_sleepTable[DG_SLEEP_ENTRIES - 1].m_steps = steps;

	SetThreadsCount (0);

	m_broadPhase = new (allocator) dgBroadPhaseMixed(this);
	//m_broadPhase = new (allocator) dgBroadPhaseSegregated (this);

	//m_pointCollision = new (m_allocator) dgCollisionPoint(m_allocator);
	dgCollision* const pointCollison = new (m_allocator) dgCollisionPoint(m_allocator);
	m_pointCollision = CreateInstance(pointCollison, 0, dgGetIdentityMatrix());
	pointCollison->Release();

	AddSentinelBody();
}

dgWorld::~dgWorld()
{	
	Sync();
	dgAsyncThread::Terminate();
	dgMutexThread::Terminate();

	UnloadPlugins();
	m_listeners.RemoveAll();

	DestroyAllBodies();
	RemoveAllGroupID();
	m_pointCollision->Release();
	DestroyBody (m_sentinelBody);

	dgDeadBodies& bodyList = *this;
	dgDeadJoints& jointList = *this;

	jointList.DestroyJoints(*this);
	bodyList.DestroyBodies(*this);

	delete m_broadPhase;
}

void dgWorld::DestroyAllBodies()
{
	dgBodyMasterList& me = *this;

	Sync();

	dgSkeletonList& skelList = *this;
	dgSkeletonList::Iterator iter(skelList);
	for (iter.Begin(); iter; iter++) {
		dgSkeletonContainer* const skeleton = iter.GetNode()->GetInfo();
		delete skeleton;
	}
	skelList.RemoveAll();

	while (m_disableBodies.GetRoot()) {
		dgBody* const body = m_disableBodies.GetRoot()->GetKey();
		BodyEnableSimulation(body);
	}

	dgAssert(dgBodyMasterList::GetFirst()->GetInfo().GetBody() == m_sentinelBody);
	for (dgBodyMasterList::dgListNode* node = me.GetFirst()->GetNext(); node;) {
		dgBody* const body = node->GetInfo().GetBody();
		node = node->GetNext();
		DestroyBody(body);
	}

	dgDeadBodies& bodyList = *this;
	dgDeadJoints& jointList = *this;

	jointList.DestroyJoints(*this);
	bodyList.DestroyBodies(*this);

	dgAssert(me.GetFirst()->GetInfo().GetCount() == 0);
	dgAssert(dgBodyCollisionList::GetCount() == 0);
}

void dgWorld::SetThreadsCount (dgInt32 count)
{
	dgThreadHive::SetThreadsCount(count);
}

dgUnsigned32 dgWorld::GetPerformanceCount ()
{
	return 0;
}

void dgWorld::AddSentinelBody()
{
	dgCollision* const collision = new  (m_allocator) dgCollisionNull (m_allocator, 0x4352fe67);
	dgCollisionInstance* const instance = CreateInstance(collision, 0, dgGetIdentityMatrix());
	collision->Release();
	m_sentinelBody = CreateDynamicBody(instance, dgGetIdentityMatrix());
	instance->Release();
}

dgDynamicBody* dgWorld::GetSentinelBody() const
{
	return m_sentinelBody;
}


dgFloat32 dgWorld::GetContactMergeTolerance() const
{
	return m_contactTolerance;
}

void dgWorld::SetContactMergeTolerance(dgFloat32 tolerenace)
{
	m_contactTolerance = dgMax (tolerenace, dgFloat32 (1.e-3f));
}

void dgWorld::EnableParallelSolverOnLargeIsland(dgInt32 mode)
{
	m_useParallelSolver = mode ? 1 : 0;
}

dgInt32 dgWorld::GetParallelSolverOnLargeIsland() const
{
	return m_useParallelSolver ? 1 : 0;
}


void dgWorld::SetFrictionThreshold (dgFloat32 acceleration)
{
	m_frictiomTheshold = dgMax (dgFloat32(1.0e-2f), acceleration);
}


void dgWorld::RemoveAllGroupID()
{
	while (dgBodyMaterialList::GetCount()) {
		dgBodyMaterialList::Remove (dgBodyMaterialList::GetRoot());
	}
	m_bodyGroupID = 0;
	m_defualtBodyGroupID = CreateBodyGroupID();
}

void dgWorld::InitBody (dgBody* const body, dgCollisionInstance* const collision, const dgMatrix& matrix)
{
	dgAssert (collision);

	m_bodiesUniqueID ++;
	body->m_world = this;

	body->m_spawnnedFromCallback = dgUnsigned32 (m_inUpdate ? true : false);
	body->m_uniqueID = dgInt32 (m_bodiesUniqueID);

	dgBodyMasterList::AddBody(body);

	body->SetLinearDamping(dgFloat32(0.1045f));
	body->SetCentreOfMass (dgVector (dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (1.0f))); 
	body->SetAngularDamping (dgVector (dgFloat32 (0.1045f), dgFloat32 (0.1045f), dgFloat32 (0.1045f), dgFloat32 (0.0f)));
	body->AttachCollision(collision);
	body->m_bodyGroupId = dgInt32 (m_defualtBodyGroupID);

	dgMatrix inertia(dgGetIdentityMatrix());
	if (!body->GetCollision()->IsType(dgCollision::dgCollisionLumpedMass_RTTI)) {
		inertia[0][0] = DG_INFINITE_MASS;
		inertia[1][1] = DG_INFINITE_MASS;
		inertia[2][2] = DG_INFINITE_MASS;
		body->SetMassMatrix(DG_INFINITE_MASS * dgFloat32(2.0f), inertia);
	} else {
		dgBodyMasterList::RemoveBody(body);
		body->UpdateLumpedMatrix();
		dgBodyMasterList::AddBody(body);
		inertia[0][0] = body->m_mass.m_x;
		inertia[1][1] = body->m_mass.m_y;
		inertia[2][2] = body->m_mass.m_z;
		body->SetMassMatrix(body->m_mass.m_w, inertia);
	}
	body->SetMatrix(matrix);
	if (!body->GetCollision()->IsType (dgCollision::dgCollisionNull_RTTI)) {
		m_broadPhase->Add (body);
	}
}

void dgWorld::BodyEnableSimulation (dgBody* const body)
{
	if (!body->m_masterNode) {
		m_disableBodies.Remove(body);
		dgBodyMasterList::AddBody(body);
		body->SetMassMatrix(body->m_mass.m_w, body->CalculateLocalInertiaMatrix());
		m_broadPhase->Add (body);
		dgAssert (body->m_masterNode);
	}
}

void dgWorld::BodyDisableSimulation(dgBody* const body)
{
	if (body->m_masterNode) {
		m_broadPhase->Remove(body);
		dgBodyMasterList::RemoveBody(body);
		m_disableBodies.Insert(0, body);
		dgAssert (!body->m_masterNode);
	}
}

bool dgWorld::GetBodyEnableDisableSimulationState (dgBody* const body) const
{
	return body->m_masterNode ? true : false;
}

dgDynamicBody* dgWorld::CreateDynamicBody(dgCollisionInstance* const collision, const dgMatrix& matrix)
{
	dgDynamicBody* const body = new (m_allocator) dgDynamicBody();
	dgAssert (dgInt32 (sizeof (dgBody) & 0xf) == 0);
	dgAssert ((dgUnsigned64 (body) & 0xf) == 0);

	InitBody (body, collision, matrix);
	return body;
}

dgDynamicBody* dgWorld::CreateDynamicBodyAsymetric(dgCollisionInstance* const collision, const dgMatrix& matrix)
{
	dgDynamicBody* const body = new (m_allocator) dgDynamicBodyAsymetric();
	dgAssert(dgInt32(sizeof(dgBody) & 0xf) == 0);
	dgAssert((dgUnsigned64(body) & 0xf) == 0);

	InitBody(body, collision, matrix);
	return body;
}

dgKinematicBody* dgWorld::CreateKinematicBody (dgCollisionInstance* const collision, const dgMatrix& matrix)
{
	dgKinematicBody* const body = new (m_allocator) dgKinematicBody();
	dgAssert (dgInt32 (sizeof (dgBody) & 0xf) == 0);
	dgAssert ((dgUnsigned64 (body) & 0xf) == 0);

	InitBody (body, collision, matrix);
	return body;
}


void dgWorld::DestroyBody(dgBody* const body)
{
	for (dgListenerList::dgListNode* node = m_listeners.GetLast(); node; node = node->GetPrev()) {
		dgListener& listener = node->GetInfo();
		if (listener.m_onBodyDestroy) {
			listener.m_onBodyDestroy(this, node, body);
		}
	}

	dgDeadBodies& deadBodyList = *this;
	deadBodyList.DestroyBody(body);
}

void dgWorld::DestroyConstraint(dgConstraint* const constraint)
{
	RemoveConstraint (constraint);
	delete constraint;
}

void dgWorld::ExecuteUserJob (dgWorkerThreadTaskCallback userJobKernel, void* const userJobKernelContext, const char* const functionName)
{
	QueueJob (userJobKernel, this, userJobKernelContext, functionName);
}

void dgWorld::SetUserData (void* const userData)
{
	m_userData = userData;
}

void* dgWorld::GetUserData() const
{
	return m_userData;
}

void dgWorld::SetIslandUpdateCallback (OnClusterUpdate callback)
{
	m_onClusterUpdate = callback;
}

void dgWorld::SetCreateDestroyContactCallback(OnCreateContact createContactCallback, OnDestroyContact destroyContactCallback)
{
	m_onCreateContact = createContactCallback;
	m_onDestroyContact = destroyContactCallback;
}

void* dgWorld::AddListener (const char* const nameid, void* const userData)
{
	dgListenerList::dgListNode* const node = m_listeners.Append();
	dgListener& listener = node->GetInfo();
	strncpy (listener.m_name, nameid, sizeof (listener.m_name));
	listener.m_world = this;
	listener.m_userData = userData;
	return node;
}

void dgWorld::ListenerSetDestroyCallback(void* const listenerNode, OnListenerDestroyCallback destroyCallback)
{
	dgListener& listener = ((dgListenerList::dgListNode*) listenerNode)->GetInfo();
	listener.m_onListenerDestroy = destroyCallback;
}

void dgWorld::ListenerSetPreUpdate(void* const listenerNode, OnListenerUpdateCallback updateCallback)
{
	dgListener& listener = ((dgListenerList::dgListNode*) listenerNode)->GetInfo();
	listener.m_onPreUpdate = updateCallback;
}

void dgWorld::ListenerSetPostUpdate(void* const listenerNode, OnListenerUpdateCallback updateCallback)
{
	dgListener& listener = ((dgListenerList::dgListNode*) listenerNode)->GetInfo();
	listener.m_onPostUpdate = updateCallback;
}

void dgWorld::ListenerSetPostStep(void* const listenerNode, OnListenerUpdateCallback updateCallback)
{
	dgListener& listener = ((dgListenerList::dgListNode*) listenerNode)->GetInfo();
	listener.m_onPostStep = updateCallback;
}

void* dgWorld::GetListenerUserData (void* const listenerNode) const
{
	dgListener& listener = ((dgListenerList::dgListNode*) listenerNode)->GetInfo();
	return listener.m_userData;
}

void dgWorld::SetListenerBodyDestroyCallback (void* const listenerNode, OnListenerBodyDestroyCallback callback)
{
	dgListener& listener = ((dgListenerList::dgListNode*) listenerNode)->GetInfo();
	listener.m_onBodyDestroy = callback;
}

void dgWorld::SetListenerBodyDebugCallback (void* const listenerNode, OnListenerDebugCallback callback)
{
	dgListener& listener = ((dgListenerList::dgListNode*) listenerNode)->GetInfo();
	listener.m_onDebugCallback = callback;
}

dgWorld::OnListenerBodyDestroyCallback dgWorld::GetListenerBodyDestroyCallback (void* const listenerNode) const
{
	dgListener& listener = ((dgListenerList::dgListNode*) listenerNode)->GetInfo();
	return listener.m_onBodyDestroy;
}

void dgWorld::ListenersDebug(void* const debugContext)
{
	for (dgListenerList::dgListNode* node = m_listeners.GetFirst(); node; node = node->GetNext()) {
		dgListener& listener = node->GetInfo();
		if (listener.m_onDebugCallback) {
			listener.m_onDebugCallback (this, listener.m_userData, debugContext);
		}
	}
}

void* dgWorld::FindListener (const char* const nameid) const
{
	for (dgListenerList::dgListNode* node = m_listeners.GetFirst(); node; node = node->GetNext()) {
		dgListener& listener = node->GetInfo();
		if (!strcmp (nameid, listener.m_name)) {
			return node;
		}
	}
	return NULL;
}


dgBallConstraint* dgWorld::CreateBallConstraint (
	const dgVector& pivot, 
	dgBody* const body0, 
	dgBody* const body1)
{

	dgAssert (body0);
	dgAssert (body0 != body1);
	dgBallConstraint* const constraint = new (m_allocator) dgBallConstraint;

	AttachConstraint (constraint, body0, body1);
	constraint->SetPivotPoint (pivot);
	return constraint;
}


dgHingeConstraint* dgWorld::CreateHingeConstraint (
	const dgVector& pivot, 
	const dgVector& pinDir, 
	dgBody* const body0, 
	dgBody* const body1)
{
	dgAssert (body0);
	dgAssert (body0 != body1);
	dgHingeConstraint* const constraint = new (m_allocator) dgHingeConstraint;

	AttachConstraint (constraint, body0, body1);
	constraint->SetPivotAndPinDir (pivot, pinDir, constraint->m_localMatrix0, constraint->m_localMatrix1);
	return constraint;
}


dgUpVectorConstraint* dgWorld::CreateUpVectorConstraint (const dgVector& pin, dgBody *body)
{
	dgAssert (body);
	dgUpVectorConstraint* const constraint = new (m_allocator) dgUpVectorConstraint;

	AttachConstraint (constraint, body, NULL);
	constraint->InitPinDir (pin);
	return constraint;
}



dgSlidingConstraint* dgWorld::CreateSlidingConstraint (
	const dgVector& pivot, 
	const dgVector& pinDir, 
	dgBody* const body0, 
	dgBody* const body1)
{
	dgAssert (body0);
	dgAssert (body0 != body1);
	dgSlidingConstraint* const constraint = new (m_allocator) dgSlidingConstraint;

	AttachConstraint (constraint, body0, body1);
	constraint->SetPivotAndPinDir (pivot, pinDir, constraint->m_localMatrix0, constraint->m_localMatrix1);
	return constraint;
}


dgCorkscrewConstraint* dgWorld::CreateCorkscrewConstraint (
	const dgVector& pivot, 
	const dgVector& pinDir, 
	dgBody* const body0, 
	dgBody* const body1)
{
	dgAssert (body0);
	dgAssert (body0 != body1);
	dgCorkscrewConstraint* const constraint = new (m_allocator) dgCorkscrewConstraint;

	AttachConstraint (constraint, body0, body1);
	constraint->SetPivotAndPinDir (pivot, pinDir, constraint->m_localMatrix0, constraint->m_localMatrix1);
	return constraint;
}


dgUniversalConstraint* dgWorld::CreateUniversalConstraint (
	const dgVector& pivot, 
	const dgVector& pin0, 
	const dgVector& pin1, 
	dgBody* const body0, 
	dgBody* const body1)
{
	dgAssert (body0);
	dgAssert (body0 != body1);
	dgUniversalConstraint* const constraint = new (m_allocator) dgUniversalConstraint;

	AttachConstraint (constraint, body0, body1);
	constraint->SetPivotAndPinDir(pivot, pin0, pin1, constraint->m_localMatrix0, constraint->m_localMatrix1);
	return constraint;
}

dgInt32 dgWorld::GetBodiesCount() const
{
	const dgBodyMasterList& list = *this;
	return list.GetCount() - 1;
}

dgInt32 dgWorld::GetConstraintsCount() const
{
	const dgBodyMasterList& list = *this;
	return dgInt32 (list.m_constraintCount);
}


void dgWorld::BodySetMatrix (dgBody* const body, const dgMatrix& matrix)
{
	#define DG_RECURSIVE_SIZE	1024

	dgBody* queue[DG_RECURSIVE_SIZE];

	dgInt32 index = 1;
	queue[0] = body;
	m_genericLRUMark ++;
	body->m_genericLRUMark = m_genericLRUMark;
	dgMatrix relMatrix (body->GetMatrix().Inverse() * matrix);
	while (index) {
		index --;
		dgBody* body1 = queue[index];
		dgAssert (body1 != m_sentinelBody);

		// why should I do this? I do no remember the reason
		//m_broadPhase->Remove (body);
		//m_broadPhase->Add (body);

		dgMatrix matrix1 (body1->GetMatrix() * relMatrix);
		//body1->SetOmega (dgVector (dgFloat32 (0.0f)));
		//body1->SetVelocity (dgVector (dgFloat32 (0.0f)));
		body1->SetOmega (matrix1.RotateVector (body1->GetOmega()));
		body1->SetVelocity (matrix1.RotateVector (body1->GetVelocity()));
		
		body1->SetMatrix (matrix1);
		body1->UpdateCollisionMatrix (dgFloat32 (0.0f), 0);
		body1->SetSleepState(false);

		for (dgBodyMasterListRow::dgListNode* jointNode = body1->m_masterNode->GetInfo().GetFirst(); jointNode; jointNode = jointNode->GetNext()) {
			dgBodyMasterListCell& cell = jointNode->GetInfo();
			body1 = cell.m_bodyNode;
			if (body1 != m_sentinelBody) {
				if (body1->m_genericLRUMark != m_genericLRUMark) {
					dgConstraint* constraint;
					constraint = cell.m_joint;
					if (constraint->GetId() != dgConstraint::m_contactConstraint) {
						body1->m_genericLRUMark = m_genericLRUMark;
						queue[index] = body1;
						index ++;
						dgAssert (index < DG_RECURSIVE_SIZE);
					}
				}
			}
		}
	}
}


bool dgWorld::AreBodyConnectedByJoints (dgBody* const originSrc, dgBody* const targetSrc)
{
	#define DG_QEUEU_SIZE	1024
	dgBody* queue[DG_QEUEU_SIZE];

	m_genericLRUMark ++;

	dgBody* origin1 = originSrc;
	dgBody* target1 = targetSrc;
	if (origin1->GetInvMass().m_w == dgFloat32 (0.0f)) {
		dgSwap (origin1, target1);
	}

	dgAssert (origin1->GetInvMass().m_w != dgFloat32 (0.0f));
	dgBody* const origin = origin1;
	dgBody* const target = target1;

	dgInt32 end = 1;
	dgInt32 start = 0;
	queue[0] = origin;
	origin->m_genericLRUMark = m_genericLRUMark;

	while (start != end) {
		dgBody* const originVar = queue[start];
		start ++;
		start &= (DG_QEUEU_SIZE - 1);
	
		for (dgBodyMasterListRow::dgListNode* jointNode = originVar->m_masterNode->GetInfo().GetFirst(); jointNode; jointNode = jointNode->GetNext()) {
			dgBodyMasterListCell& cell = jointNode->GetInfo();

			dgBody* const body = cell.m_bodyNode;
			if (body->m_genericLRUMark != m_genericLRUMark) {
				dgConstraint* const constraint = cell.m_joint;
				if (constraint->GetId() != dgConstraint::m_contactConstraint) {
					if (body == target) {
						return true;
					}
					body->m_genericLRUMark = m_genericLRUMark;
					queue[end] = body;
					end ++;
					end &= (DG_QEUEU_SIZE - 1);
				}
			}
		}
	}
	return false;
}

void dgWorld::FlushCache()
{
	// delete all contacts
	dgContactList& contactList = *this;
	for (dgInt32 i = contactList.m_contactCount - 1; i >= 0; i--) {
		dgContact* const contact = contactList[i];
		contact->m_killContact = 1;
	}
	m_broadPhase->DeleteDeadContact(0.0f);

	// clean up memory in bradPhase
	m_broadPhase->InvalidateCache ();

	// sort body list
	SortMasterList();
}

void dgWorld::StepDynamics (dgFloat32 timestep)
{
	//SerializeToFile ("xxx.bin");

	dgAssert (m_inUpdate == 0);
	dgAssert (GetThreadCount() >= 1);

	m_inUpdate ++;
	m_frameNumber ++;

	D_TRACKTIME();
	UpdateSkeletons();
	UpdateBroadphase(timestep);
	UpdateDynamics (timestep);

	if (m_listeners.GetCount()) {
		for (dgListenerList::dgListNode* node = m_listeners.GetFirst(); node; node = node->GetNext()) {
			dgListener& listener = node->GetInfo();
			if (listener.m_onPostUpdate) {
				listener.m_onPostUpdate(this, listener.m_userData, timestep);
			}
		}
	}

	m_inUpdate --;
}

void dgWorldThreadPool::OnBeginWorkerThread (dgInt32 threadId)
{
}

void dgWorldThreadPool::OnEndWorkerThread (dgInt32 threadId)
{
}

void dgWorld::Execute (dgInt32 threadID)
{
	dgMutexThread::Execute (threadID);
}

void dgWorld::UpdateTransforms(dgBodyMasterList::dgListNode* node, dgInt32 threadID)
{
	const dgInt32 threadsCount = GetThreadCount();
	while (node) {
		dgBody* const body = node->GetInfo().GetBody();
		if (body->m_transformIsDirty && body->m_matrixUpdate) {
			body->m_matrixUpdate (*body, body->m_matrix, threadID);
		}
		body->m_transformIsDirty = false;

		for (dgInt32 i = 0; i < threadsCount; i++) {
			node = node ? node->GetNext() : NULL;
		}
	}
}

void dgWorld::UpdateTransforms(void* const context, void* const nodePtr, dgInt32 threadID)
{
	dgWorld* const world = (dgWorld*)context;
	dgBodyMasterList::dgListNode* node = (dgBodyMasterList::dgListNode*) nodePtr;
	world->UpdateTransforms(node, threadID);
}

void dgWorld::RunStep ()
{
	D_TRACKTIME();
	
	BeginSection();
	dgUnsigned64 timeAcc = dgGetTimeInMicrosenconds();

	dgFloat32 step = m_savetimestep / m_numberOfSubsteps;
	for (dgUnsigned32 i = 0; i < m_numberOfSubsteps; i ++) {
		StepDynamics (step);

		dgDeadBodies& bodyList = *this;
		dgDeadJoints& jointList = *this;

		jointList.DestroyJoints (*this);
		bodyList.DestroyBodies (*this);
	}

	const dgBodyMasterList* const masterList = this;
	dgBodyMasterList::dgListNode* threadNode = masterList->GetFirst();
	const dgInt32 threadsCount = GetThreadCount();
	for (dgInt32 i = 0; i < threadsCount; i++) {
		QueueJob(UpdateTransforms, this, threadNode, "dgWorld::UpdateTransforms");
		threadNode = threadNode ? threadNode->GetNext() : NULL;
	}
	SynchronizationBarrier();

	if (m_listeners.GetCount()) {
		for (dgListenerList::dgListNode* node = m_listeners.GetFirst(); node; node = node->GetNext()) {
			dgListener& listener = node->GetInfo();
			if (listener.m_onPostStep) {
				listener.m_onPostStep(this, listener.m_userData, m_savetimestep);
			}
		}
	}

	if (m_onPostUpdateCallback) {
		m_onPostUpdateCallback (this, m_savetimestep);
	}

	m_lastExecutionTime = (dgGetTimeInMicrosenconds() - timeAcc) * dgFloat32 (1.0e-6f);
	EndSection();
}

void dgWorld::TickCallback(dgInt32 threadID)
{
	RunStep();
}

void dgWorld::Update (dgFloat32 timestep)
{
	m_savetimestep = timestep;
	#ifdef DG_USE_THREAD_EMULATION
		dgFloatExceptions exception;
		dgSetPrecisionDouble precision;
		RunStep ();
	#else 
		// runs the update in a separate thread and wait until the update is completed before it returns.
		// this will run well on single core systems, since the two thread are mutually exclusive 
		dgMutexThread::Tick();
	#endif
}

void dgWorld::UpdateAsync (dgFloat32 timestep)
{
	
	#ifdef DG_USE_THREAD_EMULATION
		Update(timestep);
	#else
		m_savetimestep = timestep;
		dgAsyncThread::Tick();
		//Update(timestep);
	#endif
}

void dgWorld::SetCollisionInstanceConstructorDestructor (OnCollisionInstanceDuplicate constructor, OnCollisionInstanceDestroy destructor)
{
	m_onCollisionInstanceDestruction = destructor;
	m_onCollisionInstanceCopyConstrutor = constructor;
}

dgInt32 dgWorld::GetBroadPhaseType() const
{
	return dgInt32 (m_broadPhase->GetType());
}

void dgWorld::SetBroadPhaseType(dgInt32 type)
{
	if (type != GetBroadPhaseType()) {
		dgBroadPhase* newBroadPhase = NULL;
		
		switch (type)
		{
			case m_broadphaseSegregated:
				newBroadPhase = new (m_allocator) dgBroadPhaseSegregated (this);
				break;

			case m_broadphaseMixed:
			default:
				newBroadPhase = new (m_allocator) dgBroadPhaseMixed(this);
				break;
		}

		m_broadPhase->MoveNodes(newBroadPhase);
		delete m_broadPhase;
		m_broadPhase = newBroadPhase;
	}
}

void dgWorld::ResetBroadPhase()
{
	dgBroadPhase* newBroadPhase = NULL;

	switch (GetBroadPhaseType())
	{
		case m_broadphaseSegregated:
			newBroadPhase = new (m_allocator) dgBroadPhaseSegregated (this);
			break;

		case m_broadphaseMixed:
		default:
			newBroadPhase = new (m_allocator) dgBroadPhaseMixed(this);
			break;
	}

	m_broadPhase->MoveNodes(newBroadPhase);
	delete m_broadPhase;
	m_broadPhase = newBroadPhase;
}

dgContact* dgWorld::FindContactJoint (const dgBody* body0, const dgBody* body1) const
{
	dgAssert (m_broadPhase);
	return m_broadPhase->m_contactCache.FindContactJoint(body0, body1);
}

dgSkeletonContainer* dgWorld::CreateNewtonSkeletonContainer (dgBody* const rootBone)
{
	dgAssert (rootBone);
	dgSkeletonList* const list = this;
	dgAssert (rootBone->GetType() == dgBody::m_dynamicBody);
	dgSkeletonContainer* const container = new (m_allocator) dgSkeletonContainer(this, (dgDynamicBody*)rootBone);

	container->m_listNode = list->Append(container);
	return container;
}

void dgWorld::DestroySkeletonContainer (dgSkeletonContainer* const container)
{
	dgSkeletonList* const list = this;
	dgAssert(container->m_listNode);
	list->Remove(container->m_listNode);
	delete container;
}

dgBroadPhaseAggregate* dgWorld::CreateAggreGate() const
{
	return m_broadPhase->CreateAggregate();
}

void dgWorld::DestroyAggregate(dgBroadPhaseAggregate* const aggregate) const
{
	m_broadPhase->DestroyAggregate((dgBroadPhaseAggregate*) aggregate);
}

dgDeadJoints::dgDeadJoints(dgMemoryAllocator* const allocator)
	:dgTree<dgConstraint*, void* >(allocator)
	,m_lock(0)
{
}

void dgDeadJoints::DestroyJoint(dgConstraint* const joint)
{
	dgScopeSpinLock lock(&m_lock);
	dgWorld& me = *((dgWorld*)this);
	me.DestroyConstraint(joint);
}

void dgDeadJoints::DestroyJoints(dgWorld& world)
{
	dgScopeSpinLock lock(&m_lock);
	Iterator iter (*this);
	for (iter.Begin(); iter; iter++) {
		dgTreeNode* const node = iter.GetNode();
		dgConstraint* const joint = node->GetInfo();
		world.DestroyConstraint (joint);
	}
	RemoveAll ();
}

dgDeadBodies::dgDeadBodies(dgMemoryAllocator* const allocator)
	:dgTree<dgBody*, void*>(allocator)
	,m_lock(0)
{
}

void dgDeadBodies::DestroyBody(dgBody* const body)
{
	if (body->m_destructor) {
		body->m_destructor(*body);
	}
	body->m_isdead = 1;
	body->SetDestructorCallback (NULL);
	body->SetMatrixUpdateCallback (NULL);
	body->SetExtForceAndTorqueCallback (NULL);

	for (dgBodyMasterListRow::dgListNode* node = body->GetMasterList()->GetInfo().GetLast(); node; node = node->GetPrev()) {
		dgConstraint* const joint = node->GetInfo().m_joint;
		if (joint && (joint->GetId() == dgConstraint::m_contactConstraint)) {
			dgContact* const contactJoint = (dgContact*)joint;
			contactJoint->m_killContact = 1;
		}
	}

	dgScopeSpinLock lock(&m_lock);
	Insert (body, body);
}

void dgDeadBodies::DestroyBodies(dgWorld& world)
{
//	dgScopeSpinLock lock(&m_lock);
	if (GetCount()) {
		world.m_broadPhase->DeleteDeadContact(0.0f);
		Iterator iter(*this);
		for (iter.Begin(); iter; iter++) {
			dgTreeNode* const bodyNode = iter.GetNode();
			dgBody* const body = bodyNode->GetInfo();

			if (world.m_disableBodies.Find(body)) {
				world.m_disableBodies.Remove(body);
			} else {
				world.m_broadPhase->Remove(body);
				world.dgBodyMasterList::RemoveBody(body);
			}

			dgAssert(body->m_collision);
			body->m_collision->Release();
			delete body;
		}
		RemoveAll();
	}
}

void dgWorld::UpdateBroadphase(dgFloat32 timestep)
{
	m_broadPhase->UpdateContacts (timestep);
}

#if 1
dgInt32 dgWorld::CompareJointByInvMass(const dgBilateralConstraint* const jointA, const dgBilateralConstraint* const jointB, void* notUsed)
{
	dgInt32 modeA = jointA->m_solverModel;
	dgInt32 modeB = jointB->m_solverModel;

	if (modeA < modeB) {
		return -1;
	} else if (modeA > modeB) {
		return 1;
	} else {
		dgFloat32 invMassA = dgMin(jointA->GetBody0()->m_invMass.m_w, jointA->GetBody1()->m_invMass.m_w);
		dgFloat32 invMassB = dgMin(jointB->GetBody0()->m_invMass.m_w, jointB->GetBody1()->m_invMass.m_w);
		if (invMassA < invMassB) {
			return -1;
		} else if (invMassA > invMassB) {
			return 1;
		}
	}
	return 0;
}

void dgWorld::UpdateSkeletons()
{
	D_TRACKTIME();
	dgSkeletonList& skelManager = *this;

	if (skelManager.m_skelListIsDirty) {
		skelManager.m_skelListIsDirty = false;
		dgSkeletonList::Iterator iter(skelManager);
		for (iter.Begin(); iter; iter++) {
			dgSkeletonContainer* const skeleton = iter.GetNode()->GetInfo();
			delete skeleton;
		}
		skelManager.RemoveAll();

		m_dynamicsLru = m_dynamicsLru + 1;
		dgUnsigned32 lru = m_dynamicsLru;

		dgBodyMasterList& masterList = *this;
		m_solverJacobiansMemory.ResizeIfNecessary((2 * (masterList.m_constraintCount + 1024)) * sizeof (dgBilateralConstraint*));
		dgBilateralConstraint** const jointList = (dgBilateralConstraint**)&m_solverJacobiansMemory[0];

		dgInt32 jointCount = 0;
		for (dgBodyMasterList::dgListNode* node = masterList.GetFirst(); node; node = node->GetNext()) {
			const dgBodyMasterListRow& graphNode = node->GetInfo();
			dgBody* const srcBody = graphNode.GetBody();

			for (dgBodyMasterListRow::dgListNode* jointNode = srcBody->m_masterNode->GetInfo().GetLast(); jointNode; jointNode = jointNode->GetPrev()) {
				dgBodyMasterListCell* const cell = &jointNode->GetInfo();
				dgConstraint* const constraint = cell->m_joint;
				dgAssert(constraint);
				dgAssert((constraint->m_body0 == srcBody) || (constraint->m_body1 == srcBody));
				dgAssert((constraint->m_body0 == cell->m_bodyNode) || (constraint->m_body1 == cell->m_bodyNode));
				bool test = constraint->IsBilateral();
				test = test && (constraint->m_solverModel < 2);
				test = test && (constraint->m_dynamicsLru != lru);
				test = test && (constraint->GetMassScaleBody0() == dgFloat32 (1.0f));
				test = test && (constraint->GetMassScaleBody1() == dgFloat32 (1.0f));
				//if (constraint->IsBilateral() && (constraint->m_solverModel < 2) && (constraint->m_dynamicsLru != lru)) {
				if (test) {
					constraint->m_dynamicsLru = lru;
					jointList[jointCount] = (dgBilateralConstraint*)constraint;
					jointCount++;
				}
			}
		}

		dgSortIndirect(jointList, jointCount, CompareJointByInvMass);

		const dgInt32 poolSize = 1024 * 4;
		dgBilateralConstraint* loopJoints[64];
		dgSkeletonContainer::dgNode* queuePool[poolSize];

		m_dynamicsLru = m_dynamicsLru + 1;
		lru = m_dynamicsLru;
		for (dgInt32 i = 0; i < jointCount; i++) {
			dgBilateralConstraint* const constraint = jointList[i];
			if (constraint->m_dynamicsLru != lru) {
				dgQueue<dgSkeletonContainer::dgNode*> queue(queuePool, poolSize);

				dgInt32 loopCount = 0;
				dgDynamicBody* const rootBody = (dgDynamicBody*)((constraint->GetBody0()->GetInvMass().m_w < constraint->GetBody1()->GetInvMass().m_w) ? constraint->GetBody0() : constraint->GetBody1());
				dgSkeletonContainer* const skeleton = CreateNewtonSkeletonContainer(rootBody);
				dgSkeletonContainer::dgNode* const rootNode = skeleton->GetRoot();
				if (rootBody->GetInvMass().m_w == dgFloat32 (0.0f)) {
					if (constraint->IsBilateral() && (constraint->m_dynamicsLru != lru)) {
						constraint->m_dynamicsLru = lru;
						dgDynamicBody* const childBody = (dgDynamicBody*)((constraint->GetBody0() == rootBody) ? constraint->GetBody1() : constraint->GetBody0());
						if (!constraint->m_solverModel) {
							if ((childBody->m_dynamicsLru != lru) && (childBody->GetInvMass().m_w != dgFloat32(0.0f))) {
								childBody->m_dynamicsLru = lru;
								dgSkeletonContainer::dgNode* const node = skeleton->AddChild((dgBilateralConstraint*)constraint, rootNode);
								queue.Insert(node);
							}
						}
					}
				} else {
					queue.Insert(rootNode);
					rootBody->m_dynamicsLru = lru;
				}

				while (!queue.IsEmpty()) {
					dgInt32 count = queue.m_firstIndex - queue.m_lastIndex;
					if (count < 0) {
						count += queue.m_mod;
					}

					dgInt32 index = queue.m_lastIndex;
					queue.Reset();

					for (dgInt32 j = 0; j < count; j++) {
						dgSkeletonContainer::dgNode* const parentNode = queue.m_pool[index];
						dgDynamicBody* const parentBody = skeleton->GetBody(parentNode);

						for (dgBodyMasterListRow::dgListNode* jointNode1 = parentBody->m_masterNode->GetInfo().GetFirst(); jointNode1; jointNode1 = jointNode1->GetNext()) {
							dgBodyMasterListCell* const cell1 = &jointNode1->GetInfo();
							dgConstraint* const constraint1 = cell1->m_joint;
							if (constraint1->IsBilateral() && (constraint1->m_dynamicsLru != lru)) {
								constraint1->m_dynamicsLru = lru;

								dgDynamicBody* const childBody = (dgDynamicBody*)((constraint1->GetBody0() == parentBody) ? constraint1->GetBody1() : constraint1->GetBody0());
								if (!constraint1->m_solverModel) {
									if ((childBody->m_dynamicsLru != lru) && (childBody->GetInvMass().m_w != dgFloat32(0.0f))) {
										childBody->m_dynamicsLru = lru;
										dgSkeletonContainer::dgNode* const childNode = skeleton->AddChild((dgBilateralConstraint*)constraint1, parentNode);
										queue.Insert(childNode);
									} else if (loopCount < (sizeof (loopJoints) / sizeof(loopJoints[0]))) {
										loopJoints[loopCount] = (dgBilateralConstraint*)constraint1;
										loopCount++;
									}

								//} else if ((constraint1->m_solverModel != 2) && loopCount < (sizeof (loopJoints) / sizeof(loopJoints[0]))) {
								  } else if ((constraint1->m_solverModel == 1) && (loopCount < (sizeof (loopJoints) / sizeof(loopJoints[0])))) {
									dgAssert (constraint1->m_solverModel != 0);
									loopJoints[loopCount] = (dgBilateralConstraint*)constraint1;
									loopCount++;
								}
							}
						}
						index++;
						if (index >= queue.m_mod) {
							index = 0;
						}
					}
				}

				skeleton->Finalize(loopCount, loopJoints);
			}
		}
	}

	dgSkeletonList::Iterator iter(skelManager);
	for (iter.Begin(); iter; iter++) {
		dgSkeletonContainer* const skeleton = iter.GetNode()->GetInfo();
		skeleton->ClearSelfCollision();
	}
}

#else

dgInt32 dgWorld::CompareJointByInvMass(const dgBilateralConstraint* const jointA, const dgBilateralConstraint* const jointB, void* notUsed)
{
	dgAssert (jointA->m_solverModel < 2);
	dgAssert (jointB->m_solverModel < 2);

	dgWorld* const world = jointA->GetBody0()->GetWorld();
	dgBody* const rootA = world->FindRoot (jointA->GetBody0());
	dgBody* const rootB = world->FindRoot (jointB->GetBody0());

	if (rootA->m_uniqueID < rootB->m_uniqueID) {
		return -1;
	} else if (rootA->m_uniqueID > rootB->m_uniqueID) {
		return 1;
	}

	dgFloat32 invMassA[2];
	dgFloat32 invMassB[2];
	invMassA[0] = jointA->GetBody0()->m_invMass.m_w;
	invMassA[1] = jointA->GetBody1()->m_invMass.m_w;

	invMassB[0] = jointB->GetBody0()->m_invMass.m_w;
	invMassB[1] = jointB->GetBody1()->m_invMass.m_w;

	if (invMassA[0] < invMassA[1]) {
		dgSwap(invMassA[0], invMassA[1]);
	}
	if (invMassB[0] < invMassB[1]) {
		dgSwap(invMassA[0], invMassA[1]);
	}

	if (invMassA[1] < invMassB[1]) {
		return -1;
	} else if (invMassA[1] > invMassB[1]) {
		return 1;
	}

	if (invMassA[0] < invMassB[0]) {
		return -1;
	} else if (invMassA[0] > invMassB[0]) {
		return 1;
	}

	return 0;
}

void dgWorld::UpdateSkeletons()
{
	D_TRACKTIME();
	dgSkeletonList& skelManager = *this;
skelManager.m_skelListIsDirty = true;

	if (skelManager.m_skelListIsDirty) {
		skelManager.m_skelListIsDirty = false;
		dgSkeletonList::Iterator iter(skelManager);
		for (iter.Begin(); iter; iter++) {
			dgSkeletonContainer* const skeleton = iter.GetNode()->GetInfo();
			delete skeleton;
		}
		skelManager.RemoveAll();

		const dgBilateralConstraintList& jointList = *this;
		m_solverJacobiansMemory.ResizeIfNecessary((jointList.GetCount() + 1024) * sizeof (dgBilateralConstraint*));
		dgBilateralConstraint** const jointArray = (dgBilateralConstraint**)&m_solverJacobiansMemory[0];
		
		dgInt32 jointCount = 0;
		for (dgBilateralConstraintList::dgListNode* node = jointList.GetFirst(); node; node = node->GetNext()) {
			dgBilateralConstraint* const joint = node->GetInfo();
			if (joint->m_solverModel < 2) {
				joint->m_body0->InitJointSet();
				joint->m_body1->InitJointSet();
				jointArray[jointCount] = joint;
				jointCount++;
			}
		}

		for (dgInt32 i = 0; i < jointCount; i++) {
			dgBilateralConstraint* const joint = jointArray[i];
			dgAssert(joint->GetBody0()->m_invMass.m_w > dgFloat32(0.0f));
			if (joint->GetBody1()->m_invMass.m_w > dgFloat32(0.0f)) {
				UnionSet(joint);
			} else {
				dgBody* const root = FindRootAndSplit(joint->GetBody0());
				root->m_disjointInfo.m_jointCount += 1;
			}
		}

		dgSortIndirect(jointArray, jointCount, CompareJointByInvMass);
		
		for (dgInt32 i = 0; i < jointCount; i++) {
			dgBilateralConstraint* const joint = jointArray[i];
			dgBody* const root = FindRoot (joint->GetBody0());
			root->m_disjointInfo.m_rank = -1;
		}

		int skelCount = 0;
		for (dgInt32 i = 0; i < jointCount; i++) {
			dgBilateralConstraint* const joint = jointArray[i];
			dgBody* const root = FindRoot(joint->GetBody0());
			if (root->m_disjointInfo.m_rank == -1) {
				root->m_disjointInfo.m_rank = skelCount;
				skelCount ++;
			}
		}

		dgInt32* batchStart = dgAlloca (dgInt32, skelCount + 1);
		memset (batchStart, 0, sizeof (dgInt32) *(skelCount + 1));
		for (dgInt32 i = 0; i < jointCount; i++) {
			dgBilateralConstraint* const joint = jointArray[i];
			dgBody* const root = FindRoot(joint->GetBody0());
			batchStart[root->m_disjointInfo.m_rank] += 1;
		}

		dgInt32 acc = 0;
		for (dgInt32 i = 0; i <= skelCount ; i++) {
			dgInt32 count = batchStart[i];
			batchStart[i] = acc;
			acc += count;
		}

		for (dgInt32 i = 0; i < skelCount ; i++) {
			dgInt32 index = batchStart[i];
			dgInt32 count = batchStart[i + 1] - i;
			dgBilateralConstraint** const constraint = &jointArray[index];

			for (dgInt32 j = 0; j < count; j ++) {
				dgBilateralConstraint* const joint = constraint[j];
				joint->m_body0->InitJointSet();
				joint->m_body1->InitJointSet();
			}

			dgInt32 loopCount = 0;
			dgInt32 spanningCount = 0;
			dgBilateralConstraint** const loopJoints = dgAlloca (dgBilateralConstraint*, count);
			dgBilateralConstraint** const spanningTree = dgAlloca (dgBilateralConstraint*, count);
			for (dgInt32 j = 0; j < count; j ++) {
				dgBilateralConstraint* const joint = constraint[j];
				dgBody* const root0 = FindRoot(joint->GetBody0());
				dgBody* const root1 = FindRoot(joint->GetBody1());
				if (root0 != root1) {
					UnionSet(joint);
					spanningTree[spanningCount] = joint;
					spanningCount ++;
				} else {
					loopJoints[loopCount] = joint;
					loopCount ++;
				}
			}
			//skeleton->Finalize(loopCount, loopJoints);

		}
	}

	dgSkeletonList::Iterator iter(skelManager);
	for (iter.Begin(); iter; iter++) {
		dgSkeletonContainer* const skeleton = iter.GetNode()->GetInfo();
		skeleton->ClearSelfCollision();
	}
}
#endif

void dgWorld::OnSerializeToFile(void* const fileHandle, const void* const buffer, dgInt32 size)
{
	dgAssert((size & 0x03) == 0);
	size_t bytes = fwrite(buffer, size, 1, (FILE*)fileHandle);
	bytes=0;
}

void dgWorld::OnDeserializeFromFile(void* const fileHandle, void* const buffer, dgInt32 size)
{
	dgAssert((size & 0x03) == 0);
	FILE* const file = (FILE*)fileHandle;
	size_t bytes = fread(buffer, size, 1, file);
	bytes=0;
}

void dgWorld::OnBodySerializeToFile(dgBody& body, void* const userData, dgSerialize serializeCallback, void* const serializeHandle)
{
	const char* const bodyIndentification = "NewtonGravityBody\0\0\0\0";
	int size = (dgInt32(strlen(bodyIndentification)) + 3) & -4;
	serializeCallback(serializeHandle, &size, sizeof (size));
	serializeCallback(serializeHandle, bodyIndentification, size);
}

void dgWorld::SetJointSerializationCallbacks(OnJointSerializationCallback serializeJoint, OnJointDeserializationCallback deserializeJoint)
{
	m_onSerializeJointCallback = serializeJoint;
	m_onDeserializeJointCallback = deserializeJoint;
}

void dgWorld::GetJointSerializationCallbacks(OnJointSerializationCallback* const serializeJoint, OnJointDeserializationCallback* const deserializeJoint) const
{
	*serializeJoint = m_onSerializeJointCallback;
	*deserializeJoint = m_onDeserializeJointCallback;
}

dgBody* dgWorld::FindBodyFromSerializedID(dgInt32 serializedID) const
{
	const dgBodyMasterList& me = *this;
	for (dgBodyMasterList::dgListNode* node = me.GetFirst()->GetNext(); node; node = node->GetNext()) {
		const dgBodyMasterListRow& graphNode = node->GetInfo();
		if (graphNode.GetBody()->m_serializedEnum == serializedID) {
			return graphNode.GetBody();
		}
	}
	return NULL;
}

dgInt32 dgWorld::SerializeToFileSort(const dgBody* const body0, const dgBody* const body1, void* const context)
{
	if (body0->m_uniqueID < body1->m_uniqueID) {
		return -1;
	} else if (body0->m_uniqueID > body1->m_uniqueID) {
		return 1;
	}
	return 0;
}

void dgWorld::SerializeScene(void* const userData, OnBodySerialize bodyCallback, dgSerialize serializeCallback, void* const serializeHandle) const
{
	dgBody** const array = new dgBody*[GetBodiesCount()];

	dgInt32 count = 0;
	const dgBodyMasterList& me = *this;
	for (dgBodyMasterList::dgListNode* node = me.GetFirst()->GetNext(); node; node = node->GetNext()) {
		const dgBodyMasterListRow& graphNode = node->GetInfo();
		array[count] = graphNode.GetBody();
		array[count]->m_serializedEnum = count;
		count++;
		dgAssert(count <= GetBodiesCount());
	}

	dgSortIndirect(array, count, SerializeToFileSort);
	SerializeBodyArray(userData, bodyCallback ? bodyCallback : OnBodySerializeToFile, array, count, serializeCallback, serializeHandle);
	SerializeJointArray(count, OnSerializeToFile, serializeHandle);

	for (dgBodyMasterList::dgListNode* node = me.GetFirst()->GetNext(); node; node = node->GetNext()) {
		const dgBodyMasterListRow& graphNode = node->GetInfo();
		graphNode.GetBody()->m_serializedEnum = -1;
	}

	delete[] array;
}

void dgWorld::DeserializeScene(void* const userData, OnBodyDeserialize bodyCallback, dgDeserialize deserializeCallback, void* const serializeHandle)
{
	dgTree<dgBody*, dgInt32> bodyMap(GetAllocator());
	DeserializeBodyArray(userData, bodyCallback ? bodyCallback : OnBodyDeserializeFromFile, bodyMap, deserializeCallback, serializeHandle);
	DeserializeJointArray(bodyMap, deserializeCallback, serializeHandle);

	const dgBodyMasterList& me = *this;
	for (dgBodyMasterList::dgListNode* node = me.GetFirst()->GetNext(); node; node = node->GetNext()) {
		const dgBodyMasterListRow& graphNode = node->GetInfo();
		graphNode.GetBody()->m_serializedEnum = -1;
	}
}

void dgWorld::OnBodyDeserializeFromFile(dgBody& body, void* const userData, dgDeserialize deserializeCallback, void* const fileHandle)
{
}

void dgWorld::DeserializeBodyArray (void* const userData, OnBodyDeserialize bodyCallback, dgTree<dgBody*, dgInt32>&bodyMap, dgDeserialize deserializeCallback, void* const serializeHandle)
{
	dgInt32 revision = dgDeserializeMarker(deserializeCallback, serializeHandle);

	dgTree<const dgCollision*, dgInt32> shapeMap(GetAllocator());

	dgInt32 uniqueShapes;
	deserializeCallback(serializeHandle, &uniqueShapes, sizeof (uniqueShapes));
	for (dgInt32 i = 0; i < uniqueShapes; i++) {
		dgInt32 id;

		deserializeCallback(serializeHandle, &id, sizeof (id));
		dgCollisionInstance instance(this, deserializeCallback, serializeHandle, revision);
		dgDeserializeMarker(deserializeCallback, serializeHandle);

		const dgCollision* const shape = instance.GetChildShape();
		shapeMap.Insert(shape, id);
		shape->AddRef();
	}

	dgInt32 bodyCount;
	deserializeCallback(serializeHandle, &bodyCount, sizeof (bodyCount));
	for (dgInt32 i = 0; i < bodyCount; i++) {
		dgInt32 bodyType;
		deserializeCallback(serializeHandle, &bodyType, sizeof (bodyType));
		dgBody* body = NULL;

		switch (bodyType) 
		{
			case dgBody::m_dynamicBody:
			{
				body = new (m_allocator)dgDynamicBody(this, &shapeMap, deserializeCallback, serializeHandle, revision);
				break;
			}
			case dgBody::m_kinematicBody:
			{
				body = new (m_allocator)dgKinematicBody(this, &shapeMap, deserializeCallback, serializeHandle, revision);
				break;
			}

			case dgBody::m_dynamicBodyAsymatric:
			{
				body = new (m_allocator)dgDynamicBodyAsymetric(this, &shapeMap, deserializeCallback, serializeHandle, revision);
				break;
			}

		}

		dgAssert(body);
		m_bodiesUniqueID++;
		body->m_freeze = false;
		body->m_sleeping = false;
		body->m_equilibrium = false;
		body->m_spawnnedFromCallback = false;
		body->m_uniqueID = dgInt32(m_bodiesUniqueID);

		dgBodyMasterList::AddBody(body);
		body->SetMatrix(body->GetMatrix());
		m_broadPhase->Add(body);
		if (body->IsRTTIType(dgBody::m_dynamicBodyRTTI)) {
			dgDynamicBody* const dynBody = (dgDynamicBody*)body;
			dynBody->SetMassMatrix(dynBody->m_mass.m_w, dynBody->CalculateLocalInertiaMatrix());
		}

		// load user related data 
		bodyCallback(*body, userData, deserializeCallback, serializeHandle);

		bodyMap.Insert(body, body->m_serializedEnum);

		// sync to next body
		dgDeserializeMarker(deserializeCallback, serializeHandle);
	}

	dgTree<const dgCollision*, dgInt32>::Iterator iter(shapeMap);
	for (iter.Begin(); iter; iter++) {
		const dgCollision* const collision = iter.GetNode()->GetInfo();
		collision->Release();
	}
}

void dgWorld::DeserializeJointArray (const dgTree<dgBody*, dgInt32>&bodyMap, dgDeserialize serializeCallback, void* const userData)
{
	dgInt32 count = 0;

	dgDeserializeMarker (serializeCallback, userData);
	serializeCallback(userData, &count, sizeof (count));	

	for (dgInt32 i = 0; i < count; i ++) {
		if (m_onDeserializeJointCallback) {
			dgInt32 bodyIndex0; 
			dgInt32 bodyIndex1; 

			serializeCallback(userData, &bodyIndex0, sizeof (bodyIndex0));
			serializeCallback(userData, &bodyIndex1, sizeof (bodyIndex1));

			dgBody* const body0 = (bodyIndex0 != -1) ? bodyMap.Find (bodyIndex0)->GetInfo() : NULL;
			dgBody* const body1 = (bodyIndex1 != -1) ? bodyMap.Find (bodyIndex1)->GetInfo() : NULL;
			m_onDeserializeJointCallback (body0, body1, serializeCallback, userData);
		}
		dgDeserializeMarker(serializeCallback, userData);
	}

	dgDeserializeMarker(serializeCallback, userData);
}

void dgWorld::SerializeBodyArray(void* const userData, OnBodySerialize bodyCallback, dgBody** const array, dgInt32 count, dgSerialize serializeCallback, void* const fileHandle) const
{
	dgSerializeMarker(serializeCallback, fileHandle);

	// serialize all collisions
	dgInt32 uniqueShapes = 0;
	dgTree<dgInt32, const dgCollision*> shapeMap(GetAllocator());
	for (dgInt32 i = 0; i < count; i++) {
		dgBody* const body = array[i];
		dgAssert(body->m_world == this);
		dgCollisionInstance* const instance = body->GetCollision();
		const dgCollision* const collision = instance->GetChildShape();
		dgTree<dgInt32, const dgCollision*>::dgTreeNode* const shapeNode = shapeMap.Insert(uniqueShapes, collision);
		if (shapeNode) {
			uniqueShapes++;
		}
	}

	serializeCallback(fileHandle, &uniqueShapes, sizeof (uniqueShapes));
	dgTree<dgInt32, const dgCollision*>::Iterator iter(shapeMap);
	for (iter.Begin(); iter; iter++) {
		dgInt32 id = iter.GetNode()->GetInfo();
		const dgCollision* const collision = iter.GetKey();
		dgCollisionInstance instance(this, collision, 0, dgMatrix(dgGetIdentityMatrix()));
		serializeCallback(fileHandle, &id, sizeof (id));
		instance.Serialize(serializeCallback, fileHandle);
		dgSerializeMarker(serializeCallback, fileHandle);
	}

	serializeCallback(fileHandle, &count, sizeof (count));
	for (dgInt32 i = 0; i < count; i++) {
		dgBody* const body = array[i];

		dgInt32 bodyType = body->GetType();
		serializeCallback(fileHandle, &bodyType, sizeof (bodyType));

		// serialize the body
		body->Serialize(shapeMap, serializeCallback, fileHandle);

		// serialize body custom data
		bodyCallback(*body, userData, serializeCallback, fileHandle);

		dgSerializeMarker(serializeCallback, fileHandle);
	}
}

void dgWorld::SerializeJointArray(dgInt32 bodyCount, dgSerialize serializeCallback, void* const userData) const
{
	dgInt32 count = 0;
	const dgBodyMasterList* me = this;
	for (dgBodyMasterList::dgListNode* node = me->GetFirst(); node; node = node->GetNext()) {
		const dgBodyMasterListRow& info = node->GetInfo();
		for (dgBodyMasterListRow::dgListNode *jointNode = info.GetFirst(); jointNode; jointNode = jointNode->GetNext()) {
			const dgBodyMasterListCell& cell = jointNode->GetInfo();

			dgConstraint* const joint = cell.m_joint;
			count += joint->IsBilateral() ? 1 : 0;
		}
	}

	//	dgTree<int, dgBody*> bodyMap (GetAllocator());
	//	for (dgInt32 i = 0; i < bodyCount; i ++) {
	//		bodyMap.Insert (i, bodyArray[i]);
	//	}

	count /= 2;
	dgSerializeMarker(serializeCallback, userData);
	serializeCallback(userData, &count, sizeof (count));

	dgTree<int, dgConstraint*> map(GetAllocator());
	for (dgBodyMasterList::dgListNode* node = me->GetFirst(); node; node = node->GetNext()) {
		dgBodyMasterListRow& info = node->GetInfo();
		for (dgBodyMasterListRow::dgListNode *jointNode = info.GetFirst(); jointNode; jointNode = jointNode->GetNext()) {
			const dgBodyMasterListCell& cell = jointNode->GetInfo();
			dgConstraint* const joint = cell.m_joint;
			if (joint->IsBilateral()) {
				if (!map.Find(joint)) {
					map.Insert(0, joint);
					dgAssert(joint->GetBody0());
					dgAssert(joint->GetBody1());
					const dgInt32 body0 = (joint->GetBody0() != m_sentinelBody) ? joint->GetBody0()->m_serializedEnum : -1;
					const dgInt32 body1 = (joint->GetBody1() != m_sentinelBody) ? joint->GetBody1()->m_serializedEnum : -1;

					serializeCallback(userData, &body0, sizeof (dgInt32));
					serializeCallback(userData, &body1, sizeof (dgInt32));

					dgBilateralConstraint* const bilateralJoint = (dgBilateralConstraint*)joint;
					bilateralJoint->Serialize(serializeCallback, userData);

					dgSerializeMarker(serializeCallback, userData);
				}
			}
		}
	}

	dgSerializeMarker(serializeCallback, userData);
}

