/* Copyright (c) <2003-2016> <Julio Jerez, Newton Game Dynamics>
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
#include "dgBroadPhase.h"
#include "dgConstraint.h"
#include "dgDynamicBody.h"
#include "dgCollisionInstance.h"
#include "dgSkeletonContainer.h"
#include "dgWorldDynamicUpdate.h"
#include "dgBilateralConstraint.h"
#include "dgCollisionDeformableMesh.h"

#define DG_CCD_EXTRA_CONTACT_COUNT			(8 * 3)
//#define DG_PARALLEL_JOINT_COUNT_CUT_OFF	(256)
#define DG_PARALLEL_JOINT_COUNT_CUT_OFF	(128)
//#define DG_PARALLEL_JOINT_COUNT_CUT_OFF	(1)

dgVector dgWorldDynamicUpdate::m_velocTol (dgFloat32 (1.0e-8f));


class dgWorldDynamicUpdateSyncDescriptor
{
	public:
	dgWorldDynamicUpdateSyncDescriptor()
	{
		memset (this, 0, sizeof (dgWorldDynamicUpdateSyncDescriptor));
	}

	dgFloat32 m_timestep;
	dgInt32 m_atomicCounter;
	
	dgInt32 m_clusterCount;
	dgInt32 m_firstCluster;
};


void dgJacobianMemory::Init(dgWorld* const world, dgInt32 rowsCount, dgInt32 bodyCount)
{
	world->m_solverJacobiansMemory.ResizeIfNecessary((rowsCount + 1) * sizeof(dgLeftHandSide));
	m_leftHandSizeBuffer = (dgLeftHandSide*)&world->m_solverJacobiansMemory[0];

	world->m_solverRightHandSideMemory.ResizeIfNecessary((rowsCount + 1) * sizeof(dgRightHandSide));
	m_righHandSizeBuffer = (dgRightHandSide*)&world->m_solverRightHandSideMemory[0];

	world->m_solverForceAccumulatorMemory.ResizeIfNecessary((bodyCount + 8) * sizeof(dgJacobian));
	m_internalForcesBuffer = (dgJacobian*)&world->m_solverForceAccumulatorMemory[0];
	dgAssert(bodyCount <= (((world->m_solverForceAccumulatorMemory.GetBytesCapacity() - 16) / dgInt32(sizeof(dgJacobian))) & (-8)));

	dgAssert((dgUnsigned64(m_leftHandSizeBuffer) & 0x01f) == 0);
	dgAssert((dgUnsigned64(m_internalForcesBuffer) & 0x01f) == 0);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

dgWorldDynamicUpdate::dgWorldDynamicUpdate(dgMemoryAllocator* const allocator)
	:m_bodies(0)
	,m_joints(0)
	,m_clusters(0)
	,m_markLru(0)
	,m_softBodyCriticalSectionLock(0)
	,m_clusterData(NULL)
	,m_parallelSolver(allocator)
{
	m_parallelSolver.m_world = (dgWorld*) this;
}

void dgWorldDynamicUpdate::UpdateDynamics(dgFloat32 timestep)
{
	DG_TRACKTIME(__FUNCTION__);

	m_bodies = 0;
	m_joints = 0;
	m_clusters = 0;
	m_softBodiesCount = 0;
	dgWorld* const world = (dgWorld*) this;
	world->m_dynamicsLru = world->m_dynamicsLru + DG_BODY_LRU_STEP;
	m_markLru = world->m_dynamicsLru;

	dgDynamicBody* const sentinelBody = world->m_sentinelBody;
	sentinelBody->m_index = 0; 
	sentinelBody->m_resting = 1;
	sentinelBody->m_sleeping = 1;
	sentinelBody->m_autoSleep = 1;
	sentinelBody->m_equilibrium = 1;
	sentinelBody->m_dynamicsLru = m_markLru;

	BuildClusters(timestep);
//	BuildClustersOld(timestep);

	const dgInt32 threadCount = world->GetThreadCount();	

	dgWorldDynamicUpdateSyncDescriptor descriptor;
	descriptor.m_timestep = timestep;

	dgInt32 index = m_softBodiesCount;
	descriptor.m_atomicCounter = 0;
	descriptor.m_firstCluster = index;
	descriptor.m_clusterCount = m_clusters - index;

	dgInt32 useParallelSolver = world->m_useParallelSolver;
//useParallelSolver = 0;
	if (useParallelSolver) {
		dgInt32 count = 0;
		for (dgInt32 i = 0; (i < m_clusters) && (m_clusterData[index + i].m_jointCount >= DG_PARALLEL_JOINT_COUNT_CUT_OFF); i++) {
			count++;
		}
		if (count) {
			CalculateReactionForcesParallel(&m_clusterData[index], count, timestep);
			index += count;
		}
	}

	if (index < m_clusters) {
		descriptor.m_atomicCounter = 0;
		descriptor.m_firstCluster = index;
		descriptor.m_clusterCount = m_clusters - index;
		for (dgInt32 i = 0; i < threadCount; i ++) {
			world->QueueJob (CalculateClusterReactionForcesKernel, &descriptor, world, "dgWorldDynamicUpdate::CalculateClusterReactionForces");
		}
		world->SynchronizationBarrier();
	}

	dgBodyInfo* const bodyArrayPtr = &world->m_bodiesMemory[0];
	for (dgInt32 i = 0; i < m_softBodiesCount; i++) {
		dgBodyCluster* const cluster = &m_clusterData[i];
//		IntegrateExternalForce(cluster, timestep, 0);
		dgBodyInfo* const bodyArray = &bodyArrayPtr[cluster->m_bodyStart];
		dgAssert (cluster->m_bodyCount == 2);
		dgDynamicBody* const body = (dgDynamicBody*)bodyArray[1].m_body;
		dgAssert (body->m_collision->IsType(dgCollision::dgCollisionLumpedMass_RTTI));
		body->IntegrateOpenLoopExternalForce(timestep);
		IntegrateVelocity(cluster, DG_SOLVER_MAX_ERROR, timestep, 0);
	}

	m_clusterData = NULL;
}

dgInt32 dgWorldDynamicUpdate::CompareKey(dgInt32 highA, dgInt32 lowA, dgInt32 highB, dgInt32 lowB)
{
	if (highA < highB) {
		return 1;
	} else if (highA > highB) {
		return -1;
	}
	if (lowA < lowB) {
		return 1;
	} else if (lowA > lowB) {
		return -1;
	}
	return 0;
}

dgInt32 dgWorldDynamicUpdate::CompareJointInfos(const dgJointInfo* const infoA, const dgJointInfo* const infoB, void*)
{
	return CompareKey(infoA->m_jointCount, infoA->m_setId, infoB->m_jointCount, infoB->m_setId);
	}

dgInt32 dgWorldDynamicUpdate::CompareClusterInfos(const dgBodyCluster* const clusterA, const dgBodyCluster* const clusterB, void* notUsed)
{
	return CompareKey(clusterA->m_jointCount, clusterA->m_bodyStart, clusterB->m_jointCount, clusterB->m_bodyStart);
}

DG_INLINE dgBody* dgWorldDynamicUpdate::FindRoot(dgBody* const body) const
{
	dgBody* node = body;
	for (; node->m_disjointInfo.m_parent != node; node = node->m_disjointInfo.m_parent);
	return node;
}

DG_INLINE dgBody* dgWorldDynamicUpdate::FindRootAndSplit(dgBody* const body) const
{
	dgBody* node = body;
	while (node->m_disjointInfo.m_parent != node) {
		dgBody* const prev = node;
		node = node->m_disjointInfo.m_parent;
		prev->m_disjointInfo.m_parent = node->m_disjointInfo.m_parent;
	}
	return node;
}

DG_INLINE void dgWorldDynamicUpdate::UnionSet(const dgConstraint* const joint) const
{
	dgBody* const body0 = joint->GetBody0();
	dgBody* const body1 = joint->GetBody1();
	dgBody* root0 = FindRootAndSplit(body0);
	dgBody* root1 = FindRootAndSplit(body1);
	if (root0 != root1) {
		if (root0->m_disjointInfo.m_rank < root1->m_disjointInfo.m_rank) {
			dgSwap(root0, root1);
		}
		root1->m_disjointInfo.m_parent = root0;
		if (root0->m_disjointInfo.m_rank == root1->m_disjointInfo.m_rank) {
			root0->m_disjointInfo.m_rank += 1;
			dgAssert (root0->m_disjointInfo.m_rank <= 6);
		}
		root0->m_disjointInfo.m_rowCount += root1->m_disjointInfo.m_rowCount;
		root0->m_disjointInfo.m_bodyCount += root1->m_disjointInfo.m_bodyCount;
		root0->m_disjointInfo.m_jointCount += root1->m_disjointInfo.m_jointCount;
	}
	root0->m_disjointInfo.m_jointCount ++;
	root0->m_disjointInfo.m_rowCount += joint->m_maxDOF;
}

void dgWorldDynamicUpdate::BuildClusters(dgFloat32 timestep)
{
	DG_TRACKTIME(__FUNCTION__);
	dgWorld* const world = (dgWorld*) this;
	dgContactList& contactList = *world;
	dgBodyMasterList& masterList = *world;
	const dgBilateralConstraintList& jointList = *world;
	dgInt32 jointCount = contactList.m_activeContactsCount;

	dgArray<dgJointInfo>& jointArray = world->m_jointsMemory;
	jointArray.ResizeIfNecessary(jointCount + jointList.GetCount());
	dgJointInfo* const baseJointArray = &jointArray[0];

#ifdef _DEBUG
	for (dgBodyMasterList::dgListNode* node = masterList.GetLast(); node; node = node->GetPrev()) {
		const dgBodyMasterListRow& graphNode = node->GetInfo();
		dgBody* const body = graphNode.GetBody();
		if (body->GetInvMass().m_w == dgFloat32(0.0f)) {
			for (; node; node = node->GetPrev()) {
				//dgAssert ((body->GetType() == dgBody::m_kinamticBody) ||(node->GetInfo().GetBody()->GetInvMass().m_w == dgFloat32(0.0f)));
				dgAssert(node->GetInfo().GetBody()->GetInvMass().m_w == dgFloat32(0.0f));
			}
			break;
		}
	}
#endif

	// add bilateral joints to the joint array
	for (dgBilateralConstraintList::dgListNode* node = jointList.GetFirst(); node; node = node->GetNext()) {
		dgConstraint* const contact = node->GetInfo();
		baseJointArray[jointCount].m_joint = contact;
		jointCount++;
	}

	// form all disjoints sets
	for (dgInt32 i = 0; i < jointCount; i ++) {
		dgConstraint* const contact = baseJointArray[i].m_joint;
		dgAssert(contact->GetBody0()->m_invMass.m_w > dgFloat32(0.0f));
		if (contact->GetBody1()->m_invMass.m_w > dgFloat32 (0.0f)) {
			UnionSet(contact);
		} else {
			dgBody* const root = FindRootAndSplit(contact->GetBody0());
			root->m_disjointInfo.m_jointCount += 1;
			root->m_disjointInfo.m_rowCount += contact->m_maxDOF;
		}
	}

	// find and tag all sleeping disjoint sets, 
	// and add single bodies as a set of zero joints and one body
	dgInt32 bodyInfoCount = 0;
	dgInt32 clustersCount = 0;
	dgInt32 augmentedJointCount = jointCount;

	dgArray<dgBodyCluster>& clusterMemory = world->m_clusterMemory;
	for (dgBodyMasterList::dgListNode* node = masterList.GetLast(); node && (node->GetInfo().GetBody()->GetInvMass().m_w != dgFloat32(0.0f)); node = node->GetPrev()) {
		dgBody* const body = node->GetInfo().GetBody();
		if (body->IsRTTIType(dgBody::m_dynamicBodyRTTI)) {
			dgBody* root = body;
			bool state = true;
			do {
				state &= root->m_equilibrium;
				root = root->m_disjointInfo.m_parent;
			} while (root->m_disjointInfo.m_parent != root);
			root->m_resting &= state;

			if (!root->m_resting && !root->m_disjointInfo.m_jointCount) {
				dgJointInfo& jointInfo = jointArray[augmentedJointCount];

				dgAssert (root == body);
				dgAssert (root->m_index == -1);
				dgAssert (root->m_disjointInfo.m_bodyCount == 1);

				root->m_index = clustersCount;
				jointInfo.m_body = body;
				jointInfo.m_jointCount = 0;
				jointInfo.m_setId = root->m_index;
				jointInfo.m_bodyCount = root->m_disjointInfo.m_bodyCount;
				jointInfo.m_pairCount = 0;

				dgBodyCluster& cluster = clusterMemory[clustersCount];
				cluster.m_bodyCount = 2;
				cluster.m_jointCount = 0;
				cluster.m_rowCount = 0;
				cluster.m_hasSoftBodies = 0;
				cluster.m_isContinueCollision = 0;
				cluster.m_bodyStart = root->m_index;

				clustersCount ++;
				bodyInfoCount += 2;
				augmentedJointCount ++;
			}
		}
	}

	// remove all sleeping joints sets
	dgJointInfo* const augmentedJointArray = &jointArray[0];
	for (dgInt32 i = jointCount - 1; i >= 0; i --) {
		dgJointInfo* const jointInfo = &augmentedJointArray[i];
		dgConstraint* const constraint = jointInfo->m_joint;
		dgBody* const root = FindRoot (constraint->GetBody0());
		if (root->m_resting) {
			augmentedJointCount --;
			augmentedJointArray[i] = augmentedJointArray[augmentedJointCount];
		} else {
			if (root->m_index == -1) {
				root->m_index = clustersCount;

				dgBodyCluster& cluster = clusterMemory[clustersCount];
				cluster.m_bodyCount = root->m_disjointInfo.m_bodyCount + 1;
				cluster.m_jointCount = root->m_disjointInfo.m_jointCount;
				cluster.m_rowCount = root->m_disjointInfo.m_rowCount;
				cluster.m_hasSoftBodies = 0;
				cluster.m_bodyStart = root->m_index;
				cluster.m_isContinueCollision = 0;

				clustersCount++;
				bodyInfoCount += root->m_disjointInfo.m_bodyCount + 1;
			}
			jointInfo->m_setId = root->m_index;
			jointInfo->m_pairCount = constraint->m_maxDOF;
			jointInfo->m_bodyCount = root->m_disjointInfo.m_bodyCount;
			jointInfo->m_jointCount = root->m_disjointInfo.m_jointCount;
		}
	}

	m_clusterData = &world->m_clusterMemory[0];
	dgSort(augmentedJointArray, augmentedJointCount, CompareJointInfos);
	dgSort(m_clusterData, clustersCount, CompareClusterInfos);

	dgInt32 rowStart = 0;
	dgInt32 bodyStart = 0;
	dgInt32 jointStart = 0;
	dgInt32 softBodiesCount = 0;
	for (dgInt32 i = 0; i < clustersCount; i++) {
		dgBodyCluster& cluster = m_clusterData[i];
		cluster.m_rowStart = rowStart;
		cluster.m_bodyStart = bodyStart;
		cluster.m_jointStart = jointStart;

		rowStart += cluster.m_rowCount;
		bodyStart += cluster.m_bodyCount;
		softBodiesCount += cluster.m_hasSoftBodies;
		jointStart += cluster.m_jointCount ? cluster.m_jointCount : 1;
	}
	m_solverMemory.Init(world, rowStart, bodyStart);
	world->m_bodiesMemory.ResizeIfNecessary(bodyStart);

	rowStart = 0;
	for (dgInt32 i = 0; i < clustersCount; i++) {
		const dgBodyCluster& cluster = m_clusterData[i];
		dgBodyInfo* const bodyArray = &world->m_bodiesMemory[cluster.m_bodyStart];
		dgJointInfo* const jointSetArray = &augmentedJointArray[cluster.m_jointStart];
		bodyArray[0].m_body = world->GetSentinelBody();

		if (cluster.m_jointCount) {
			dgInt32 bodyIndex = 1;
			for (dgInt32 j = 0; j < cluster.m_jointCount; j++) {
				dgJointInfo* const jointInfo = &jointSetArray[j];
				dgConstraint* const joint = jointInfo->m_joint;
				dgBody* const body0 = joint->m_body0;
				dgBody* const body1 = joint->m_body1;

				dgAssert(body0->GetInvMass().m_w != dgFloat32(0.0f));
				if (body0->m_disjointInfo.m_rank >= 0) {
					body0->m_disjointInfo.m_rank = -1;
					body0->m_index = bodyIndex;
					bodyArray[bodyIndex].m_body = body0;
					bodyIndex++;
					dgAssert(bodyIndex <= cluster.m_bodyCount);
				}
				dgInt32 m0 = body0->m_index;

				dgInt32 m1 = 0;
				if (body1->GetInvMass().m_w != dgFloat32(0.0f)) {
					if (body1->m_disjointInfo.m_rank >= 0) {
						body1->m_disjointInfo.m_rank = -1;
						body1->m_index = bodyIndex;
						bodyArray[bodyIndex].m_body = body1;
						bodyIndex++;
						dgAssert(bodyIndex <= cluster.m_bodyCount);
					}
					m1 = body1->m_index;
				}
				
				jointInfo->m_m0 = m0;
				jointInfo->m_m1 = m1;
				jointInfo->m_pairStart = rowStart;
				rowStart += jointInfo->m_pairCount; 
			}
		} else {
			dgAssert(cluster.m_bodyCount == 2);
			bodyArray[1].m_body = jointSetArray[0].m_body;
		}
	}
	
	m_bodies = bodyStart;
	m_joints = jointStart;
	m_clusters = clustersCount;
	m_softBodiesCount = softBodiesCount;
}

void dgWorldDynamicUpdate::BuildClustersOld(dgFloat32 timestep)
{
	DG_TRACKTIME(__FUNCTION__);
	dgWorld* const world = (dgWorld*) this;
	dgUnsigned32 lru = m_markLru - 1;

	dgBodyMasterList& masterList = *world;

	m_bodies = 0;
	m_joints = 0;
	m_clusters = 0;
	dgAssert (masterList.GetFirst()->GetInfo().GetBody() == world->m_sentinelBody);
	world->m_solverJacobiansMemory.ResizeIfNecessary ((2 * (masterList.m_constraintCount + 1024)) * sizeof (dgDynamicBody*));
	dgDynamicBody** const stackPoolBuffer = (dgDynamicBody**)&world->m_solverJacobiansMemory[0];

	for (dgBodyMasterList::dgListNode* node = masterList.GetLast(); node; node = node->GetPrev()) {
		const dgBodyMasterListRow& graphNode = node->GetInfo();
		dgBody* const body = graphNode.GetBody();
		
		if (body->GetInvMass().m_w == dgFloat32(0.0f)) {
#ifdef _DEBUG
			for (; node; node = node->GetPrev()) {
				//dgAssert ((body->GetType() == dgBody::m_kinamticBody) ||(node->GetInfo().GetBody()->GetInvMass().m_w == dgFloat32(0.0f)));
				dgAssert(node->GetInfo().GetBody()->GetInvMass().m_w == dgFloat32(0.0f));
			}
#endif
			break;
		}

		if (body->IsRTTIType(dgBody::m_dynamicBodyRTTI)) {
			dgDynamicBody* const dynamicBody = (dgDynamicBody*)body;
			if (dynamicBody->m_dynamicsLru < lru) {
				if (!(dynamicBody->m_freeze | dynamicBody->m_spawnnedFromCallback | dynamicBody->m_sleeping)) {
					SpanningTree(dynamicBody, stackPoolBuffer, timestep);
				}
			}
			dynamicBody->m_spawnnedFromCallback = false;
		}
	}

	dgSort(m_clusterData, m_clusters, CompareClusters);

	dgInt32 maxRowCount = 0;
	dgInt32 softBodiesCount = 0;
	for (dgInt32 i = 0; i < m_clusters; i++) {
		dgBodyCluster& cluster = m_clusterData[i];
		cluster.m_rowStart = maxRowCount;
		maxRowCount += cluster.m_rowCount;
		softBodiesCount += cluster.m_hasSoftBodies;
	}
	m_solverMemory.Init(world, maxRowCount, m_bodies);
	m_softBodiesCount = softBodiesCount; 
}

void dgWorldDynamicUpdate::SpanningTree (dgDynamicBody* const body, dgDynamicBody** const queueBuffer, dgFloat32 timestep)
{
	dgInt32 stack = 1;
	dgInt32 bodyCount = 1;
	dgInt32 jointCount = 0;
	dgInt32 hasSoftBodies = 0;
	dgInt32 isInEquilibrium = 1;

	dgWorld* const world = (dgWorld*) this;
	const dgInt32 clusterLRU = world->m_clusterLRU;
	const dgUnsigned32 lruMark = m_markLru - 1;

	world->m_clusterLRU ++;

	queueBuffer[0] = body;
	dgBodyInfo* const bodyArray0 = &world->m_bodiesMemory[0];

	bodyArray0[m_bodies].m_body = world->m_sentinelBody;
	dgAssert(world->m_sentinelBody->m_index == 0);
	dgAssert(dgInt32(world->m_sentinelBody->m_dynamicsLru) == m_markLru);

	bool globalAutoSleep = true;
	while (stack) {
		stack --;
		dgDynamicBody* const srcBody = queueBuffer[stack];

		if (srcBody->m_dynamicsLru < lruMark) {
//hack
//srcBody->m_equilibrium = false;

			dgAssert(srcBody->GetInvMass().m_w > dgFloat32(0.0f));
			dgAssert(srcBody->m_masterNode);

			dgInt32 bodyIndex = m_bodies + bodyCount;
			world->m_bodiesMemory.ResizeIfNecessary (bodyIndex + 1);
			dgBodyInfo* const bodyArray1 = &world->m_bodiesMemory[0];
			bodyArray1[bodyIndex].m_body = srcBody;
			isInEquilibrium &= srcBody->m_equilibrium;
			globalAutoSleep &= (srcBody->m_autoSleep & srcBody->m_equilibrium); 
			
			srcBody->m_index = bodyCount;
			srcBody->m_dynamicsLru = lruMark;
			srcBody->m_resting = srcBody->m_equilibrium;

			hasSoftBodies |= (srcBody->m_collision->IsType(dgCollision::dgCollisionDeformableMesh_RTTI) ? 1 : 0);

			srcBody->m_sleeping = false;

			bodyCount++;
			for (dgBodyMasterListRow::dgListNode* jointNode = srcBody->m_masterNode->GetInfo().GetFirst(); jointNode; jointNode = jointNode->GetNext()) {

				dgBodyMasterListCell* const cell = &jointNode->GetInfo();
				dgConstraint* const constraint = cell->m_joint;
				dgAssert(constraint);
				if (constraint->m_clusterLRU != clusterLRU) {
					dgBody* const linkBody = cell->m_bodyNode;
					dgAssert((constraint->m_body0 == srcBody) || (constraint->m_body1 == srcBody));
					dgAssert((constraint->m_body0 == linkBody) || (constraint->m_body1 == linkBody));
					const dgContact* const contact = (constraint->GetId() == dgConstraint::m_contactConstraint) ? (dgContact*)constraint : NULL;

					bool check0 = linkBody->IsCollidable();
					check0 = check0 && (!contact || (contact->m_contactActive && contact->m_maxDOF) || (srcBody->m_continueCollisionMode | linkBody->m_continueCollisionMode));
					if (check0) {
						bool check1 = constraint->m_dynamicsLru != lruMark;
						if (check1) {
							const dgInt32 jointIndex = m_joints + jointCount;
							dgJointInfo* const jointInfo = &world->m_jointsMemory[jointIndex];

							constraint->m_index = jointCount;
							constraint->m_clusterLRU = clusterLRU;
							constraint->m_dynamicsLru = lruMark;

							jointInfo->m_joint = constraint;
							const dgInt32 rows = constraint->m_maxDOF;
							jointInfo->m_pairStart = 0;
							jointInfo->m_pairCount = rows;
							jointCount++;

							dgAssert(constraint->m_body0);
							dgAssert(constraint->m_body1);
						}

						dgDynamicBody* const adjacentBody = (dgDynamicBody*)linkBody;
						if ((adjacentBody->m_dynamicsLru != lruMark) && (adjacentBody->GetInvMass().m_w > dgFloat32(0.0f))) {
							queueBuffer[stack] = adjacentBody;
							stack++;
						}
					}
				}
			}
		}
	}

	dgBodyInfo* const bodyArray = &world->m_bodiesMemory[0]; 
	if (globalAutoSleep) {
		for (dgInt32 i = 1; i < bodyCount; i++) {
			dgBody* const body1 = bodyArray[m_bodies + i].m_body;
			body1->m_dynamicsLru = m_markLru;
			body1->m_sleeping = globalAutoSleep;
		}
	} else {
		if (world->m_clusterUpdate) {
			dgClusterCallbackStruct record;
			record.m_world = world;
			record.m_count = bodyCount;
			record.m_strideInByte = sizeof (dgBodyInfo);
			record.m_bodyArray = &bodyArray[m_bodies].m_body;
			if (!world->m_clusterUpdate(world, &record, bodyCount)) {
				for (dgInt32 i = 0; i < bodyCount; i++) {
					dgBody* const body1 = bodyArray[m_bodies + i].m_body;
					body1->m_dynamicsLru = m_markLru;
				}
				return;
			}
		}

		//world->m_clusterMemory.ResizeIfNecessary  (m_clusters + 1);
		//m_clusterData = &world->m_clusterMemory[0];
		//dgBodyCluster& cluster = m_clusterData[m_clusters];
		dgBodyCluster& cluster = world->m_clusterMemory[m_clusters];
		m_clusterData = &world->m_clusterMemory[0];

		cluster.m_bodyStart = m_bodies;
		cluster.m_jointStart = m_joints;
		cluster.m_bodyCount = bodyCount;
		cluster.m_jointCount = jointCount;
		
		cluster.m_rowStart = 0;
		cluster.m_isContinueCollision = 0;
		cluster.m_hasSoftBodies = dgInt16 (hasSoftBodies);

		dgJointInfo* const constraintArrayPtr = &world->m_jointsMemory[0];
		dgJointInfo* const constraintArray = &constraintArrayPtr[m_joints];

		dgInt32 rowCount = 0;
		dgInt32 isContinueCollisionCluster = 0;
		for (dgInt32 i = 0; i < jointCount; i++) {
			dgJointInfo* const jointInfo = &constraintArray[i];
			dgConstraint* const joint = jointInfo->m_joint;

			dgBody* const body0 = joint->m_body0;
			dgBody* const body1 = joint->m_body1;

			dgInt32 m0 = (body0->GetInvMass().m_w != dgFloat32(0.0f)) ? body0->m_index : 0;
			dgInt32 m1 = (body1->GetInvMass().m_w != dgFloat32(0.0f)) ? body1->m_index : 0;

			jointInfo->m_m0 = m0;
			jointInfo->m_m1 = m1;

			body0->m_dynamicsLru = m_markLru;
			body1->m_dynamicsLru = m_markLru;

			dgAssert (constraintArray[i].m_pairCount >= 0);
			dgAssert (constraintArray[i].m_pairCount < 64);
			rowCount += constraintArray[i].m_pairCount;
			if (joint->GetId() == dgConstraint::m_contactConstraint) {
				if (body0->m_continueCollisionMode | body1->m_continueCollisionMode) {
					dgInt32 ccdJoint = false;
					const dgVector& veloc0 = body0->m_veloc;
					const dgVector& veloc1 = body1->m_veloc;

					const dgVector& omega0 = body0->m_omega;
					const dgVector& omega1 = body1->m_omega;

					const dgVector& com0 = body0->m_globalCentreOfMass;
					const dgVector& com1 = body1->m_globalCentreOfMass;

					const dgCollisionInstance* const collision0 = body0->m_collision;
					const dgCollisionInstance* const collision1 = body1->m_collision;
					dgFloat32 dist = dgMax(body0->m_collision->GetBoxMinRadius(), body1->m_collision->GetBoxMinRadius()) * dgFloat32(0.25f);

					dgVector relVeloc(veloc1 - veloc0);
					dgVector relOmega(omega1 - omega0);
					dgVector relVelocMag2(relVeloc.DotProduct(relVeloc));
					dgVector relOmegaMag2(relOmega.DotProduct(relOmega));

					if ((relOmegaMag2.m_w > dgFloat32(1.0f)) || ((relVelocMag2.m_w * timestep * timestep) > (dist * dist))) {
						dgTriplex normals[16];
						dgTriplex points[16];
						dgInt64 attrib0[16];
						dgInt64 attrib1[16];
						dgFloat32 penetrations[16];
						dgFloat32 timeToImpact = timestep;
						const dgInt32 ccdContactCount = world->CollideContinue(collision0, body0->m_matrix, veloc0, omega0, collision1, body1->m_matrix, veloc1, omega1,
																			   timeToImpact, points, normals, penetrations, attrib0, attrib1, 6, 0);

						for (dgInt32 j = 0; j < ccdContactCount; j++) {
							dgVector point(&points[j].m_x);
							dgVector normal(&normals[j].m_x);
							dgVector vel0(veloc0 + omega0.CrossProduct(point - com0));
							dgVector vel1(veloc1 + omega1.CrossProduct(point - com1));
							dgVector vRel(vel1 - vel0);
							dgFloat32 contactDistTravel = vRel.DotProduct(normal).m_w * timestep;
							ccdJoint |= (contactDistTravel > dist);
						}
					}
					//ccdJoint = body0->m_continueCollisionMode | body1->m_continueCollisionMode;
					isContinueCollisionCluster |= ccdJoint;
					rowCount += DG_CCD_EXTRA_CONTACT_COUNT;
				}
			}
		}

		if (isContinueCollisionCluster) {
			rowCount = dgMax(rowCount, 64);
		}

		cluster.m_rowCount = rowCount;
		cluster.m_isContinueCollision = dgInt16 (isContinueCollisionCluster);

		m_clusters++;
		m_bodies += bodyCount;
		m_joints += jointCount;
	}
}

dgInt32 dgWorldDynamicUpdate::CompareBodyJacobianPair(const dgBodyJacobianPair* const infoA, const dgBodyJacobianPair* const infoB, void* notUsed)
{
	if (infoA->m_bodyIndex < infoB->m_bodyIndex) {
		return -1;
	} else if (infoA->m_bodyIndex > infoB->m_bodyIndex) {
		return 1;
	}
	return 0;
}

dgInt32 dgWorldDynamicUpdate::SortClusters(const dgBodyCluster* const cluster, dgFloat32 timestep, dgInt32 threadID) const
{
	DG_TRACKTIME(__FUNCTION__);
	dgWorld* const world = (dgWorld*) this;
	dgBodyInfo* const bodyArray = &world->m_bodiesMemory[cluster->m_bodyStart];
	dgJointInfo* const constraintArray = &world->m_jointsMemory[cluster->m_jointStart];

	const dgInt32 bodyCount = cluster->m_bodyCount;
	const dgInt32 jointCount = cluster->m_jointCount;

	dgJointInfo* const tmpInfoList = dgAlloca(dgJointInfo, cluster->m_jointCount);
	dgJointInfo** queueBuffer = dgAlloca(dgJointInfo*, cluster->m_jointCount * 2 + 1024 * 8);
	dgBodyJacobianPair* const bodyJoint = dgAlloca (dgBodyJacobianPair, cluster->m_jointCount * 2);
	dgInt32* const bodyJointList = dgAlloca (dgInt32, bodyCount + 1);

	dgQueue<dgJointInfo*> queue(queueBuffer, cluster->m_jointCount * 2 + 1024 * 8);
	dgFloat32 heaviestMass = dgFloat32(1.0e20f);
	dgInt32 infoIndex = 0;
	dgInt32 activeJoints = 0;
	dgJointInfo* heaviestBody = NULL;

	for (dgInt32 i = 0; i < jointCount; i++) {
		dgJointInfo& jointInfo = constraintArray[i];
		tmpInfoList[i] = jointInfo;
		tmpInfoList[i].m_preconditioner0 = dgFloat32 (0.0f);

		jointInfo.m_joint->m_graphTagged = 0;
		const dgInt32 m0 = jointInfo.m_m0;
		const dgInt32 m1 = jointInfo.m_m1;
		dgBody* const body0 = bodyArray[m0].m_body;
		dgBody* const body1 = bodyArray[m1].m_body;

		const dgFloat32 invMass0 = body0->GetInvMass().m_w;
		const dgFloat32 invMass1 = body1->GetInvMass().m_w;

		dgInt32 resting = body0->m_equilibrium & body1->m_equilibrium;
		body0->m_resting &= resting | (invMass0 == dgFloat32 (0.0f));
		body1->m_resting &= resting | (invMass1 == dgFloat32 (0.0f));

		if ((invMass0 == dgFloat32 (0.0f)) || (invMass1 == dgFloat32 (0.0f))) {
			queue.Insert(&tmpInfoList[i]);
			tmpInfoList[i].m_preconditioner0 = dgFloat32 (1.0f);
		} else if (invMass0 && (heaviestMass > invMass0)) {
			heaviestMass = invMass0;
			heaviestBody = &tmpInfoList[i];
		} else if (invMass1 && (heaviestMass > invMass1)) {
			heaviestMass = invMass1;
			heaviestBody = &tmpInfoList[i];
		}

		bodyJoint[i * 2 + 0].m_bodyIndex = m0;
		bodyJoint[i * 2 + 0].m_JointIndex = i;
		bodyJoint[i * 2 + 1].m_bodyIndex = m1;
		bodyJoint[i * 2 + 1].m_JointIndex = i;
	}

	if (queue.IsEmpty()) {
		dgAssert(heaviestBody);
		queue.Insert(heaviestBody);
		heaviestBody->m_preconditioner0 = dgFloat32 (1.0f);
	}

	dgSort (bodyJoint, jointCount * 2, CompareBodyJacobianPair);
	memset (bodyJointList, 0, sizeof (dgInt32) * (cluster->m_bodyCount + 1));
	for (dgInt32 i = 0; i < jointCount * 2; i++) {
		dgInt32 index = bodyJoint[i].m_bodyIndex;
		bodyJointList[index] ++;
	}

	dgInt32 startIndex = 0;
	for (dgInt32 i = 0; i <= bodyCount; i++) {
		dgInt32 count = bodyJointList[i];
		bodyJointList[i] = startIndex;
		startIndex += count; 
	}

	while (!queue.IsEmpty()) {
		dgInt32 count = queue.m_firstIndex - queue.m_lastIndex;
		if (count < 0) {
			count += queue.m_mod;
		}

		dgInt32 index = queue.m_lastIndex;
		queue.Reset();

		for (dgInt32 i = 0; i < count; i++) {
			dgJointInfo* const jointInfo = queue.m_pool[index];
			dgConstraint* const constraint = jointInfo->m_joint;
			if (!constraint->m_graphTagged) {
				//dgAssert (dgInt32 (constraint->m_index) < cluster->m_jointCount);
				constraint->m_index = infoIndex;
				constraintArray[infoIndex] = *jointInfo;
				constraint->m_graphTagged = 1;
				infoIndex++;
				dgAssert(infoIndex <= cluster->m_jointCount);

				const dgInt32 m0 = jointInfo->m_m0;
				const dgInt32 m1 = jointInfo->m_m1;
				const dgBody* const body0 = bodyArray[m0].m_body;
				const dgBody* const body1 = bodyArray[m1].m_body;
				
				activeJoints += !(body0->m_resting & body1->m_resting);
				
				if (body0->GetInvMass().m_w > dgFloat32(0.0f)) {
					const dgInt32 endJoint = bodyJointList[m0 + 1];
					for (dgInt32 j = bodyJointList[m0]; j < endJoint; j ++ ) {
						dgJointInfo* const info = &tmpInfoList[bodyJoint[j].m_JointIndex];
						dgConstraint* const nextConstraint = info->m_joint;
						if (!nextConstraint->m_graphTagged)	{
							if (!info->m_preconditioner0) {
								queue.Insert(info);
								info->m_preconditioner0 = dgFloat32 (1.0f);
							}
						}
					}
				}

				if (body1->GetInvMass().m_w > dgFloat32(0.0f)) {
					const dgInt32 endJoint = bodyJointList[m1 + 1];
					for (dgInt32 j = bodyJointList[m1]; j < endJoint; j++) {
						dgJointInfo* const info = &tmpInfoList[bodyJoint[j].m_JointIndex];
						dgConstraint* const nextConstraint = info->m_joint;
						if (!nextConstraint->m_graphTagged)	{
							if (!info->m_preconditioner0) {
								queue.Insert(info);
								info->m_preconditioner0 = dgFloat32 (1.0f);
							}
						}
					}
				}

				if (infoIndex == cluster->m_jointCount) {
					queue.Reset();
					break;
				}
			}
			index++;
			if (index >= queue.m_mod) {
				index = 0;
			}
		}
	}

	dgAssert(infoIndex == cluster->m_jointCount);
	return activeJoints;
}


dgBody* dgWorldDynamicUpdate::GetClusterBody(const void* const clusterPtr, dgInt32 index) const
{
	const dgClusterCallbackStruct* const cluster = (dgClusterCallbackStruct*)clusterPtr;

	char* const ptr = &((char*)cluster->m_bodyArray)[cluster->m_strideInByte * index];
	dgBody** const bodyPtr = (dgBody**)ptr;
	return (index < cluster->m_count) ? ((index >= 0) ? *bodyPtr : NULL) : NULL;
}


// sort from high to low
dgInt32 dgWorldDynamicUpdate::CompareClusters(const dgBodyCluster* const clusterA, const dgBodyCluster* const clusterB, void* notUsed)
{
	dgInt32 countA = clusterA->m_jointCount + (clusterA->m_hasSoftBodies << 30);
	dgInt32 countB = clusterB->m_jointCount + (clusterB->m_hasSoftBodies << 30);

	if (countA < countB) {
		return 1;
	}
	if (countA > countB) {
		return -1;
	}
	return 0;
}


void dgWorldDynamicUpdate::CalculateClusterReactionForcesKernel (void* const context, void* const worldContext, dgInt32 threadID)
{
	dgWorldDynamicUpdateSyncDescriptor* const descriptor = (dgWorldDynamicUpdateSyncDescriptor*) context;

	dgFloat32 timestep = descriptor->m_timestep;
	dgWorld* const world = (dgWorld*) worldContext;
	dgInt32 count = descriptor->m_clusterCount;
	dgBodyCluster* const clusters = &world->m_clusterData[descriptor->m_firstCluster];

	for (dgInt32 i = dgAtomicExchangeAndAdd(&descriptor->m_atomicCounter, 1); i < count; i = dgAtomicExchangeAndAdd(&descriptor->m_atomicCounter, 1)) {
		dgBodyCluster* const cluster = &clusters[i]; 
		world->ResolveClusterForces (cluster, threadID, timestep);
	}
}

dgInt32 dgWorldDynamicUpdate::GetJacobianDerivatives(dgContraintDescritor& constraintParam, dgJointInfo* const jointInfo, dgConstraint* const constraint, dgLeftHandSide* const leftHandSide, dgRightHandSide* const rightHandSide, dgInt32 rowCount) const
{
	dgInt32 dof = dgInt32(constraint->m_maxDOF);
	dgAssert(dof <= DG_CONSTRAINT_MAX_ROWS);
	for (dgInt32 i = 0; i < dof; i++) {
		constraintParam.m_forceBounds[i].m_low = DG_MIN_BOUND;
		constraintParam.m_forceBounds[i].m_upper = DG_MAX_BOUND;
		constraintParam.m_forceBounds[i].m_jointForce = NULL;
		constraintParam.m_forceBounds[i].m_normalIndex = DG_INDEPENDENT_ROW;
	}

	dgAssert(constraint->m_body0);
	dgAssert(constraint->m_body1);

	dgBody* const body0 = constraint->m_body0;
	dgBody* const body1 = constraint->m_body1;

	dgAssert(body0->IsRTTIType(dgBody::m_dynamicBodyRTTI) || body0->IsRTTIType(dgBody::m_kinematicBodyRTTI));
	dgAssert(body1->IsRTTIType(dgBody::m_dynamicBodyRTTI) || body1->IsRTTIType(dgBody::m_kinematicBodyRTTI));

	body0->m_inCallback = true;
	body1->m_inCallback = true;
	dof = constraint->JacobianDerivative(constraintParam);
	body0->m_inCallback = false;
	body1->m_inCallback = false;

	if (constraint->GetId() == dgConstraint::m_contactConstraint) {
		dgContact* const contactJoint = (dgContact*)constraint;
		dgSkeletonContainer* const skeleton0 = body0->GetSkeleton();
		dgSkeletonContainer* const skeleton1 = body1->GetSkeleton();
		if (skeleton0 && (skeleton0 == skeleton1)) {
			skeleton0->AddSelfCollisionJoint(contactJoint);
		} else if (contactJoint->IsSkeleton()) {
			if (skeleton0 && !skeleton1) {
				skeleton0->AddSelfCollisionJoint((dgContact*)constraint);
			} else if (skeleton1 && !skeleton0) {
				skeleton1->AddSelfCollisionJoint((dgContact*)constraint);
			}
		}
	}

	jointInfo->m_pairCount = dof;
	jointInfo->m_pairStart = rowCount;
	//jointInfo->m_paddedPairCount = dgInt16(dof);
	for (dgInt32 i = 0; i < dof; i++) {
		dgAssert(constraintParam.m_forceBounds[i].m_jointForce);

		dgLeftHandSide* const row = &leftHandSide[rowCount];
		dgRightHandSide* const rhs = &rightHandSide[rowCount];
		
		row->m_Jt = constraintParam.m_jacobian[i];
		rhs->m_diagDamp = dgFloat32(0.0f);
		const dgFloat32 stiffCoef = dgFloat32(1.0f) - constraintParam.m_jointStiffness[i];
		rhs->m_stiffness = dgMax(DG_PSD_DAMP_TOL * stiffCoef, dgFloat32(1.0e-5f));

		dgAssert(rhs->m_stiffness >= dgFloat32(0.0f));
		dgAssert(constraintParam.m_jointStiffness[i] <= dgFloat32(1.0f));
		dgAssert((dgFloat32(1.0f) - constraintParam.m_jointStiffness[i]) >= dgFloat32(0.0f));
		rhs->m_coordenateAccel = constraintParam.m_jointAccel[i];
		rhs->m_restitution = constraintParam.m_restitution[i];
		rhs->m_penetration = constraintParam.m_penetration[i];
		rhs->m_penetrationStiffness = constraintParam.m_penetrationStiffness[i];
		rhs->m_lowerBoundFrictionCoefficent = constraintParam.m_forceBounds[i].m_low;
		rhs->m_upperBoundFrictionCoefficent = constraintParam.m_forceBounds[i].m_upper;
		rhs->m_jointFeebackForce = constraintParam.m_forceBounds[i].m_jointForce;

//		dgInt32 frictionIndex = constraintParam.m_forceBounds[i].m_normalIndex < 0 ? dof : constraintParam.m_forceBounds[i].m_normalIndex;
//		rhs->m_normalForceIndex = frictionIndex;
		dgAssert (constraintParam.m_forceBounds[i].m_normalIndex >= -1);
		rhs->m_normalForceIndex = constraintParam.m_forceBounds[i].m_normalIndex;
		rowCount++;
	}
//  we separate left and right hand side not to align row to near multiple of 4
//	rowCount = (rowCount & (dgInt32(sizeof(dgVector) / sizeof(dgFloat32)) - 1)) ? ((rowCount & (-dgInt32(sizeof(dgVector) / sizeof(dgFloat32)))) + dgInt32(sizeof(dgVector) / sizeof(dgFloat32))) : rowCount;
//	dgAssert((rowCount & (dgInt32(sizeof(dgVector) / sizeof(dgFloat32)) - 1)) == 0);
	constraint->ResetInverseDynamics();
	return rowCount;
}


void dgWorldDynamicUpdate::BuildJacobianMatrix(const dgBodyInfo* const bodyInfoArray, dgJointInfo* const jointInfo, dgJacobian* const internalForces, dgLeftHandSide* const leftHandSide, dgRightHandSide* const rightHandSide, dgFloat32 forceImpulseScale) const
{
	const dgInt32 index = jointInfo->m_pairStart;
	const dgInt32 count = jointInfo->m_pairCount;
	const dgInt32 m0 = jointInfo->m_m0;
	const dgInt32 m1 = jointInfo->m_m1;

	const dgBody* const body0 = bodyInfoArray[m0].m_body;
	const dgBody* const body1 = bodyInfoArray[m1].m_body;
	const bool isBilateral = jointInfo->m_joint->IsBilateral();

	const dgVector invMass0(body0->m_invMass[3]);
	const dgMatrix& invInertia0 = body0->m_invWorldInertiaMatrix;
	const dgVector invMass1(body1->m_invMass[3]);
	const dgMatrix& invInertia1 = body1->m_invWorldInertiaMatrix;

	dgVector force0(dgVector::m_zero);
	dgVector torque0(dgVector::m_zero);
	if (body0->IsRTTIType(dgBody::m_dynamicBodyRTTI)) {
		force0 = ((dgDynamicBody*)body0)->m_externalForce;
		torque0 = ((dgDynamicBody*)body0)->m_externalTorque;
	}

	dgVector force1(dgVector::m_zero);
	dgVector torque1(dgVector::m_zero);
	if (body1->IsRTTIType(dgBody::m_dynamicBodyRTTI)) {
		force1 = ((dgDynamicBody*)body1)->m_externalForce;
		torque1 = ((dgDynamicBody*)body1)->m_externalTorque;
	}

	jointInfo->m_preconditioner0 = dgFloat32(1.0f);
	jointInfo->m_preconditioner1 = dgFloat32(1.0f);
//	if ((invMass0.GetScalar() > dgFloat32(0.0f)) && (invMass1.GetScalar() > dgFloat32(0.0f))) {
	if ((invMass0.GetScalar() > dgFloat32(0.0f)) && (invMass1.GetScalar() > dgFloat32(0.0f)) && !(body0->GetSkeleton() && body1->GetSkeleton())) {
		const dgFloat32 mass0 = body0->GetMass().m_w;
		const dgFloat32 mass1 = body1->GetMass().m_w;
		if (mass0 > (DG_DIAGONAL_PRECONDITIONER * mass1)) {
			jointInfo->m_preconditioner0 = mass0 / (mass1 * DG_DIAGONAL_PRECONDITIONER);
		} else if (mass1 > (DG_DIAGONAL_PRECONDITIONER * mass0)) {
			jointInfo->m_preconditioner1 = mass1 / (mass0 * DG_DIAGONAL_PRECONDITIONER);
		}
	}

	dgJacobian forceAcc0;
	dgJacobian forceAcc1;
	const dgVector preconditioner0(jointInfo->m_preconditioner0);
	const dgVector preconditioner1(jointInfo->m_preconditioner1);
	forceAcc0.m_linear = dgVector::m_zero;
	forceAcc0.m_angular = dgVector::m_zero;
	forceAcc1.m_linear = dgVector::m_zero;
	forceAcc1.m_angular = dgVector::m_zero;

	for (dgInt32 i = 0; i < count; i++) {
		dgLeftHandSide* const row = &leftHandSide[index + i];
		dgRightHandSide* const rhs = &rightHandSide[index + i];

		row->m_JMinv.m_jacobianM0.m_linear = row->m_Jt.m_jacobianM0.m_linear * invMass0;
		row->m_JMinv.m_jacobianM0.m_angular = invInertia0.RotateVector(row->m_Jt.m_jacobianM0.m_angular);
		row->m_JMinv.m_jacobianM1.m_linear = row->m_Jt.m_jacobianM1.m_linear * invMass1;
		row->m_JMinv.m_jacobianM1.m_angular = invInertia1.RotateVector(row->m_Jt.m_jacobianM1.m_angular);

		dgVector tmpAccel(row->m_JMinv.m_jacobianM0.m_linear * force0 + row->m_JMinv.m_jacobianM0.m_angular * torque0 +
						  row->m_JMinv.m_jacobianM1.m_linear * force1 + row->m_JMinv.m_jacobianM1.m_angular * torque1);

		dgAssert(tmpAccel.m_w == dgFloat32(0.0f));
		dgFloat32 extenalAcceleration = -(tmpAccel.AddHorizontal()).GetScalar();
		rhs->m_deltaAccel = extenalAcceleration * forceImpulseScale;
		rhs->m_coordenateAccel += extenalAcceleration * forceImpulseScale;
		dgAssert(rhs->m_jointFeebackForce);
		const dgFloat32 force = rhs->m_jointFeebackForce->m_force * forceImpulseScale;
		rhs->m_force = isBilateral ? dgClamp(force, rhs->m_lowerBoundFrictionCoefficent, rhs->m_upperBoundFrictionCoefficent) : force;
		//rhs->m_force = 0.0f;
		rhs->m_maxImpact = dgFloat32(0.0f);

		dgVector jMinvM0linear(preconditioner0 * row->m_JMinv.m_jacobianM0.m_linear);
		dgVector jMinvM0angular(preconditioner0 * row->m_JMinv.m_jacobianM0.m_angular);
		dgVector jMinvM1linear(preconditioner1 * row->m_JMinv.m_jacobianM1.m_linear);
		dgVector jMinvM1angular(preconditioner1 * row->m_JMinv.m_jacobianM1.m_angular);

		dgVector tmpDiag(jMinvM0linear * row->m_Jt.m_jacobianM0.m_linear + jMinvM0angular * row->m_Jt.m_jacobianM0.m_angular +
						 jMinvM1linear * row->m_Jt.m_jacobianM1.m_linear + jMinvM1angular * row->m_Jt.m_jacobianM1.m_angular);

		dgAssert(tmpDiag.m_w == dgFloat32(0.0f));
		dgFloat32 diag = (tmpDiag.AddHorizontal()).GetScalar();
		dgAssert(diag > dgFloat32(0.0f));
		rhs->m_diagDamp = diag * rhs->m_stiffness;
		diag *= (dgFloat32(1.0f) + rhs->m_stiffness);
//		rhs->m_jinvMJt = diag;
		rhs->m_invJinvMJt = dgFloat32(1.0f) / diag;

		dgAssert(dgCheckFloat(rhs->m_force));
		dgVector val(rhs->m_force);
		forceAcc0.m_linear += row->m_Jt.m_jacobianM0.m_linear * val;
		forceAcc0.m_angular += row->m_Jt.m_jacobianM0.m_angular * val;
		forceAcc1.m_linear += row->m_Jt.m_jacobianM1.m_linear * val;
		forceAcc1.m_angular += row->m_Jt.m_jacobianM1.m_angular * val;
	}

	forceAcc0.m_linear = forceAcc0.m_linear * preconditioner0;
	forceAcc0.m_angular = forceAcc0.m_angular * preconditioner0;
	forceAcc1.m_linear = forceAcc1.m_linear * preconditioner1;
	forceAcc1.m_angular = forceAcc1.m_angular * preconditioner1;

	internalForces[m0].m_linear += forceAcc0.m_linear;
	internalForces[m0].m_angular += forceAcc0.m_angular;
	internalForces[m1].m_linear += forceAcc1.m_linear;
	internalForces[m1].m_angular += forceAcc1.m_angular;
}

void dgWorldDynamicUpdate::BuildJacobianMatrix(dgBodyCluster* const cluster, dgInt32 threadID, dgFloat32 timestep) const
{
	dgAssert(cluster->m_bodyCount >= 2);

	dgWorld* const world = (dgWorld*) this;
	const dgInt32 bodyCount = cluster->m_bodyCount;

	dgBodyInfo* const bodyArray = &world->m_bodiesMemory[cluster->m_bodyStart];
	dgJacobian* const internalForces = &m_solverMemory.m_internalForcesBuffer[cluster->m_bodyStart];

	dgAssert(((dgDynamicBody*)bodyArray[0].m_body)->IsRTTIType(dgBody::m_dynamicBodyRTTI));
	dgAssert((((dgDynamicBody*)bodyArray[0].m_body)->m_accel.DotProduct(((dgDynamicBody*)bodyArray[0].m_body)->m_accel)).GetScalar() == dgFloat32(0.0f));
	dgAssert((((dgDynamicBody*)bodyArray[0].m_body)->m_alpha.DotProduct(((dgDynamicBody*)bodyArray[0].m_body)->m_alpha)).GetScalar() == dgFloat32(0.0f));
	dgAssert((((dgDynamicBody*)bodyArray[0].m_body)->m_externalForce.DotProduct(((dgDynamicBody*)bodyArray[0].m_body)->m_externalForce)).GetScalar() == dgFloat32(0.0f));
	dgAssert((((dgDynamicBody*)bodyArray[0].m_body)->m_externalTorque.DotProduct(((dgDynamicBody*)bodyArray[0].m_body)->m_externalTorque)).GetScalar() == dgFloat32(0.0f));

	internalForces[0].m_linear = dgVector::m_zero;
	internalForces[0].m_angular = dgVector::m_zero;

	if (timestep != dgFloat32(0.0f)) {
		for (dgInt32 i = 1; i < bodyCount; i++) {
			dgBody* const body = (dgDynamicBody*)bodyArray[i].m_body;
			dgAssert(body->IsRTTIType(dgBody::m_dynamicBodyRTTI) || body->IsRTTIType(dgBody::m_kinematicBodyRTTI));
			if (!body->m_equilibrium) {
				dgAssert(body->m_invMass.m_w > dgFloat32(0.0f));
				body->AddDampingAcceleration(timestep);
				body->CalcInvInertiaMatrix();
			}

			// re use these variables for temp storage 
			body->m_accel = body->m_veloc;
			body->m_alpha = body->m_omega;

			internalForces[i].m_linear = dgVector::m_zero;
			internalForces[i].m_angular = dgVector::m_zero;
		}

	} else {
		for (dgInt32 i = 1; i < bodyCount; i++) {
			dgBody* const body = bodyArray[i].m_body;
			dgAssert(body->IsRTTIType(dgBody::m_dynamicBodyRTTI) || body->IsRTTIType(dgBody::m_kinematicBodyRTTI));
			if (!body->m_equilibrium) {
				dgAssert(body->m_invMass.m_w > dgFloat32(0.0f));
				body->CalcInvInertiaMatrix();
			}

			// re use these variables for temp storage 
			body->m_accel = body->m_veloc;
			body->m_alpha = body->m_omega;

			internalForces[i].m_linear = dgVector::m_zero;
			internalForces[i].m_angular = dgVector::m_zero;
		}
	}

	dgContraintDescritor constraintParams;

	constraintParams.m_world = world;
	constraintParams.m_threadIndex = threadID;
	constraintParams.m_timestep = timestep;
	constraintParams.m_invTimestep = (timestep > dgFloat32(1.0e-5f)) ? dgFloat32(1.0f / timestep) : dgFloat32(0.0f);
	const dgFloat32 forceOrImpulseScale = (timestep > dgFloat32(0.0f)) ? dgFloat32(1.0f) : dgFloat32(0.0f);

	dgJointInfo* const constraintArrayPtr = &world->m_jointsMemory[0];
	dgJointInfo* const constraintArray = &constraintArrayPtr[cluster->m_jointStart];
	dgLeftHandSide* const leftHandSide = &m_solverMemory.m_leftHandSizeBuffer[cluster->m_rowStart];
	dgRightHandSide* const rightHandSide = &m_solverMemory.m_righHandSizeBuffer[cluster->m_rowStart];

	dgInt32 rowCount = 0;
	const dgInt32 jointCount = cluster->m_jointCount;
	for (dgInt32 i = 0; i < jointCount; i++) {
		dgJointInfo* const jointInfo = &constraintArray[i];
		dgConstraint* const constraint = jointInfo->m_joint;

		dgAssert(dgInt32(constraint->m_index) == i);
		dgAssert(jointInfo->m_m0 < cluster->m_bodyCount);
		dgAssert(jointInfo->m_m1 < cluster->m_bodyCount);
		//dgAssert (constraint->m_index == dgUnsigned32(i));

		rowCount = GetJacobianDerivatives(constraintParams, jointInfo, constraint, leftHandSide, rightHandSide, rowCount);
		dgAssert(rowCount <= cluster->m_rowCount);

		dgAssert(jointInfo->m_m0 >= 0);
		dgAssert(jointInfo->m_m0 < bodyCount);
		dgAssert(jointInfo->m_m1 >= 0);
		dgAssert(jointInfo->m_m1 < bodyCount);
		BuildJacobianMatrix(bodyArray, jointInfo, internalForces, leftHandSide, rightHandSide, forceOrImpulseScale);
	}
}

void dgWorldDynamicUpdate::IntegrateVelocity(const dgBodyCluster* const cluster, dgFloat32 accelTolerance, dgFloat32 timestep, dgInt32 threadID) const
{
	dgWorld* const world = (dgWorld*) this;
	dgFloat32 velocityDragCoeff = DG_FREEZZING_VELOCITY_DRAG;
	dgBodyInfo* const bodyArray = &world->m_bodiesMemory[cluster->m_bodyStart + 1];

	dgInt32 count = cluster->m_bodyCount - 1;
	if (count <= 2) {
		//bool autosleep = bodyArray[0].m_body->m_autoSleep;
		bool equilibrium = bodyArray[0].m_body->m_equilibrium;
		if (count == 2) {
			equilibrium &= bodyArray[1].m_body->m_equilibrium;
		}
		if (!equilibrium) {
			velocityDragCoeff = dgFloat32(0.9999f);
		}
	}

	dgFloat32 maxAccel = dgFloat32(0.0f);
	dgFloat32 maxAlpha = dgFloat32(0.0f);
	dgFloat32 maxSpeed = dgFloat32(0.0f);
	dgFloat32 maxOmega = dgFloat32(0.0f);

	const dgFloat32 speedFreeze = world->m_freezeSpeed2;
	const dgFloat32 accelFreeze = world->m_freezeAccel2 * ((cluster->m_jointCount <= DG_SMALL_ISLAND_COUNT) ? dgFloat32(0.05f) : dgFloat32(1.0f));
	dgVector velocDragVect(velocityDragCoeff, velocityDragCoeff, velocityDragCoeff, dgFloat32(0.0f));

	bool stackSleeping = true;
	//bool isClusterResting = true;
	dgInt32 sleepCounter = 10000;
	for (dgInt32 i = 0; i < count; i++) {
		dgBody* const body = bodyArray[i].m_body;
		dgAssert(body->IsRTTIType(dgBody::m_dynamicBodyRTTI) || body->IsRTTIType(dgBody::m_kinematicBody));

		body->m_equilibrium = 1;
		dgVector isMovingMask(body->m_veloc + body->m_omega + body->m_accel + body->m_alpha);
		if ((isMovingMask.TestZero().GetSignMask() & 7) != 7) {
			dgAssert(body->m_invMass.m_w);
			if (body->IsRTTIType(dgBody::m_dynamicBodyRTTI)) {
				body->IntegrateVelocity(timestep);
			}

			dgAssert(body->m_accel.m_w == dgFloat32(0.0f));
			dgAssert(body->m_alpha.m_w == dgFloat32(0.0f));
			dgAssert(body->m_veloc.m_w == dgFloat32(0.0f));
			dgAssert(body->m_omega.m_w == dgFloat32(0.0f));
			dgFloat32 accel2 = body->m_accel.DotProduct(body->m_accel).GetScalar();
			dgFloat32 alpha2 = body->m_alpha.DotProduct(body->m_alpha).GetScalar();
			dgFloat32 speed2 = body->m_veloc.DotProduct(body->m_veloc).GetScalar();
			dgFloat32 omega2 = body->m_omega.DotProduct(body->m_omega).GetScalar();

			maxAccel = dgMax(maxAccel, accel2);
			maxAlpha = dgMax(maxAlpha, alpha2);
			maxSpeed = dgMax(maxSpeed, speed2);
			maxOmega = dgMax(maxOmega, omega2);
			bool equilibrium = (accel2 < accelFreeze) && (alpha2 < accelFreeze) && (speed2 < speedFreeze) && (omega2 < speedFreeze);
			if (equilibrium) {
				dgVector veloc(body->m_veloc * velocDragVect);
				dgVector omega(body->m_omega * velocDragVect);
				body->m_veloc = (veloc.DotProduct(veloc) > m_velocTol) & veloc;
				body->m_omega = (omega.DotProduct(omega) > m_velocTol) & omega;
			}

			body->m_equilibrium = equilibrium ? 1 : 0;
			//body->m_equilibrium &= body->m_autoSleep;
			stackSleeping &= equilibrium;
			//isClusterResting &= (body->m_autoSleep & equilibrium);
			if (body->IsRTTIType(dgBody::m_dynamicBodyRTTI)) {
				dgDynamicBody* const dynBody = (dgDynamicBody*)body;
				sleepCounter = dgMin(sleepCounter, dynBody->m_sleepingCounter);
				dynBody->m_sleepingCounter++;
			}

			body->UpdateCollisionMatrix(timestep, threadID);
		}
	}

//	if (isClusterResting && cluster->m_jointCount) {
	if (cluster->m_jointCount) {
		if (stackSleeping) {
			for (dgInt32 i = 0; i < count; i++) {
				dgBody* const body = bodyArray[i].m_body;
				dgAssert(body->IsRTTIType(dgBody::m_dynamicBodyRTTI) || body->IsRTTIType(dgBody::m_kinematicBodyRTTI));
				body->m_accel = dgVector::m_zero;
				body->m_alpha = dgVector::m_zero;
				body->m_veloc = dgVector::m_zero;
				body->m_omega = dgVector::m_zero;
				body->m_sleeping = body->m_autoSleep;
			}
		} else {
			bool state = (maxAccel > world->m_sleepTable[DG_SLEEP_ENTRIES - 1].m_maxAccel) ||
						 (maxAlpha > world->m_sleepTable[DG_SLEEP_ENTRIES - 1].m_maxAlpha) ||
						 (maxSpeed > world->m_sleepTable[DG_SLEEP_ENTRIES - 1].m_maxVeloc) || 
						 (maxOmega > world->m_sleepTable[DG_SLEEP_ENTRIES - 1].m_maxOmega);
			if (state) {
				for (dgInt32 i = 0; i < count; i++) {
					dgBody* const body = bodyArray[i].m_body;
					if (body->IsRTTIType(dgBody::m_dynamicBodyRTTI)) {
						dgDynamicBody* const dynBody = (dgDynamicBody*)body;
						dynBody->m_sleepingCounter = 0;
					}
				}
			} else {
				dgInt32 timeScaleSleepCount = dgInt32(dgFloat32(60.0f) * sleepCounter * timestep);

				dgInt32 index = DG_SLEEP_ENTRIES;
				for (dgInt32 i = 1; i < DG_SLEEP_ENTRIES; i ++) {
					if (world->m_sleepTable[i].m_steps > timeScaleSleepCount) {
						index = i;
						break;
					}
				}
				index --;

				bool state1 = (maxAccel < world->m_sleepTable[index].m_maxAccel) &&
							  (maxAlpha < world->m_sleepTable[index].m_maxAlpha) &&
					          (maxSpeed < world->m_sleepTable[index].m_maxVeloc) &&
					          (maxOmega < world->m_sleepTable[index].m_maxOmega);
				if (state1) {
					for (dgInt32 i = 0; i < count; i++) {
						dgBody* const body = bodyArray[i].m_body;
						body->m_accel = dgVector::m_zero;
						body->m_alpha = dgVector::m_zero;
						body->m_veloc = dgVector::m_zero;
						body->m_omega = dgVector::m_zero;
						body->m_sleeping = body->m_autoSleep;
						if (body->IsRTTIType(dgBody::m_dynamicBodyRTTI)) {
							dgDynamicBody* const dynBody = (dgDynamicBody*)body;
							dynBody->m_sleepingCounter = 0;
						}
					}
				}
			}
		}
	}
}
