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
#include "dgConstraint.h"
#include "dgDynamicBody.h"
#include "dgSkeletonContainer.h"
#include "dgWorldDynamicUpdate.h"
#include "dgBilateralConstraint.h"

class dgSkeletonContainer::dgNodePair
{
	public:
	dgInt32 m_m0;
	dgInt32 m_m1;
};

DG_MSC_VECTOR_ALIGMENT
class dgSkeletonContainer::dgForcePair
{
	public:
	dgSpatialVector m_joint;
	dgSpatialVector m_body;
} DG_GCC_VECTOR_ALIGMENT;

DG_MSC_VECTOR_ALIGMENT 
class dgSkeletonContainer::dgMatriData
{
	public:
	dgSpatialMatrix m_jt;
	dgSpatialMatrix m_invMass;
} DG_GCC_VECTOR_ALIGMENT;

DG_MSC_VECTOR_ALIGMENT 
class dgSkeletonContainer::dgBodyJointMatrixDataPair
{
	public:
	dgMatriData m_body;
	dgMatriData m_joint;
} DG_GCC_VECTOR_ALIGMENT;

dgInt32 dgSkeletonContainer::m_lruMarker = 1;
dgInt32 dgSkeletonContainer::m_uniqueID = DG_SKELETON_BASE_UNIQUE_ID;

class dgSkeletonContainer::dgNode
{
	public:

	DG_CLASS_ALLOCATOR(allocator)
	dgNode(dgDynamicBody* const body)
		:m_body (body)
		,m_joint(NULL)
		,m_parent(NULL)
		,m_child(NULL)
		,m_sibling(NULL)
		,m_index(0)
		,m_dof(0)
		,m_swapJacobianBodiesIndex(0)
	{
	}

	dgNode (dgBilateralConstraint* const joint, dgNode* const parent)
		:m_body ((dgDynamicBody*) ((joint->GetBody0() == parent->m_body) ? joint->GetBody1() : joint->GetBody0()))
		,m_joint (joint)
		,m_parent(parent)
		,m_child(NULL)
		,m_sibling(NULL)
		,m_index(0)
		,m_dof(0)
		,m_swapJacobianBodiesIndex(joint->GetBody0() == parent->m_body)
	{
		dgAssert (m_parent);
		dgAssert (m_body->GetInvMass().m_w != dgFloat32 (0.0f));
		if (m_parent->m_child) {
			m_sibling = m_parent->m_child;
		}
		m_parent->m_child = this;
	}

	DG_INLINE ~dgNode()
	{
		m_body->SetSkeleton(NULL);

		dgNode* next;
		for (dgNode* ptr = m_child; ptr; ptr = next) {
			next = ptr->m_sibling;
			delete ptr;
		}
	}

	DG_INLINE void CalculateInertiaMatrix(dgSpatialMatrix* const bodyMassArray) const
	{
		dgSpatialMatrix& bodyMass = bodyMassArray[m_index];

		bodyMass = dgSpatialMatrix(dgFloat32(0.0f));
		if (m_body->GetInvMass().m_w != dgFloat32(0.0f)) {
			const dgFloat32 mass = m_body->GetMass().m_w;
			dgMatrix inertia(m_body->CalculateInertiaMatrix());
			for (dgInt32 i = 0; i < 3; i++) {
				bodyMass[i][i] = mass;
				for (dgInt32 j = 0; j < 3; j++) {
					bodyMass[i + 3][j + 3] = inertia[i][j];
				}
			}
		}
	}

	DG_INLINE void GetJacobians(const dgJointInfo* const jointInfo, const dgLeftHandSide* const leftHandSide, const dgRightHandSide* const rightHandSide, dgSpatialMatrix* const jointMassArray)
	{
		dgAssert(m_parent);
		dgAssert(jointInfo->m_joint == m_joint);

		dgSpatialMatrix& bodyJt = m_data.m_body.m_jt;
		dgSpatialMatrix& jointJ = m_data.m_joint.m_jt;
		dgSpatialMatrix& jointMass = jointMassArray[m_index];

		const dgInt32 start = jointInfo->m_pairStart;
		const dgSpatialVector zero (dgSpatialVector::m_zero);
		if (!m_swapJacobianBodiesIndex) {
			for (dgInt32 i = 0; i < m_dof; i++) {
				const dgInt32 k = m_sourceJacobianIndex[i];
				const dgRightHandSide* const rhs = &rightHandSide[start + k];
				const dgLeftHandSide* const row = &leftHandSide[start + k];
				jointMass[i] = zero;
				jointMass[i][i] = -rhs->m_diagDamp;
				bodyJt[i] = dgSpatialVector (row->m_Jt.m_jacobianM0.m_linear * dgVector::m_negOne, row->m_Jt.m_jacobianM0.m_angular * dgVector::m_negOne);
				jointJ[i] = dgSpatialVector (row->m_Jt.m_jacobianM1.m_linear * dgVector::m_negOne, row->m_Jt.m_jacobianM1.m_angular * dgVector::m_negOne);
			}
		} else {
			for (dgInt32 i = 0; i < m_dof; i++) {
				const dgInt32 k = m_sourceJacobianIndex[i];
				const dgRightHandSide* const rhs = &rightHandSide[start + k];
				const dgLeftHandSide* const row = &leftHandSide[start + k];
				jointMass[i] = zero;
				jointMass[i][i] = -rhs->m_diagDamp;
				bodyJt[i] = dgSpatialVector(row->m_Jt.m_jacobianM1.m_linear * dgVector::m_negOne, row->m_Jt.m_jacobianM1.m_angular * dgVector::m_negOne);
				jointJ[i] = dgSpatialVector(row->m_Jt.m_jacobianM0.m_linear * dgVector::m_negOne, row->m_Jt.m_jacobianM0.m_angular * dgVector::m_negOne);
			}
		}
	}

	//DG_INLINE dgInt32 Factorize(const dgJointInfo* const jointInfoArray, const dgLeftHandSide* const leftHandSide, const dgRightHandSide* const rightHandSide, dgSpatialMatrix* const bodyMassArray, dgSpatialMatrix* const jointMassArray)
	dgInt32 Factorize(const dgJointInfo* const jointInfoArray, const dgLeftHandSide* const leftHandSide, const dgRightHandSide* const rightHandSide, dgSpatialMatrix* const bodyMassArray, dgSpatialMatrix* const jointMassArray)
	{
		CalculateInertiaMatrix(bodyMassArray);

		m_ordinals = m_ordinalInit;
		dgInt32 boundedDof = 0;
		if (m_joint) {
			dgAssert (m_parent);
			const dgJointInfo* const jointInfo = &jointInfoArray[m_joint->m_index];
			dgAssert(jointInfo->m_joint == m_joint);

			m_dof = 0;
			dgInt32 count = jointInfo->m_pairCount;
			const dgInt32 first = jointInfo->m_pairStart;
			for (dgInt32 i = 0; i < count; i++) {
				dgInt32 k = m_sourceJacobianIndex[i];
				const dgRightHandSide* const rhs = &rightHandSide[k + first];
				if ((rhs->m_lowerBoundFrictionCoefficent <= dgFloat32 (-DG_LCP_MAX_VALUE)) && (rhs->m_upperBoundFrictionCoefficent >= dgFloat32 (DG_LCP_MAX_VALUE))) {
					m_dof ++;
				} else {
					dgSwap(m_sourceJacobianIndex[i], m_sourceJacobianIndex[count - 1]);
					i--;
					count--;
				}
			}
			dgAssert (m_dof > 0);
			dgAssert (m_dof <= 6);
			boundedDof += jointInfo->m_pairCount - count;
			GetJacobians(jointInfo, leftHandSide, rightHandSide, jointMassArray);
		}

		dgSpatialMatrix& bodyInvMass = m_data.m_body.m_invMass;
		const dgSpatialMatrix& bodyMass = bodyMassArray[m_index];
		if (m_body->GetInvMass().m_w != dgFloat32(0.0f)) {
			for (dgNode* child = m_child; child; child = child->m_sibling) {
				CalculateBodyDiagonal(child, bodyMassArray, jointMassArray);
			}
			bodyInvMass = bodyMass.Inverse(6);
		} else {
			bodyInvMass = dgSpatialMatrix(dgFloat32(0.0f));
		}

		if (m_joint) {
			dgSpatialMatrix& bodyJt = m_data.m_body.m_jt;
			dgAssert(m_parent);
			for (dgInt32 i = 0; i < m_dof; i++) {
				bodyJt[i] = bodyInvMass.VectorTimeMatrix(bodyJt[i]);
			}
			CalculateJointDiagonal(bodyMassArray, jointMassArray);
			CalculateJacobianBlock();
		}
		return boundedDof;
	}

	DG_INLINE void CalculateBodyDiagonal(dgNode* const child, dgSpatialMatrix* const bodyMassArray, const dgSpatialMatrix* const jointMassArray)
	{
		dgAssert(child->m_joint);
		
		dgSpatialMatrix copy (dgFloat32(0.0f));
		const dgInt32 dof = child->m_dof;
		const dgSpatialMatrix& jacobianMatrix = child->m_data.m_joint.m_jt;
		const dgSpatialMatrix& childDiagonal = jointMassArray[child->m_index];
		for (dgInt32 i = 0; i < dof ; i++) {
			const dgSpatialVector& jacobian = jacobianMatrix[i];
			for (dgInt32 j = 0; j < dof ; j++) {
				dgAssert(dgAreEqual (dgFloat64(childDiagonal[i][j]), dgFloat64(childDiagonal[j][i]), dgFloat64(1.0e-5f)));
				dgFloat64 val = childDiagonal[i][j];
				copy[j] = copy[j] + jacobian.Scale(val);
			}
		}

		dgSpatialMatrix& bodyMass = bodyMassArray[m_index];
		for (dgInt32 i = 0; i < dof; i++) {
			const dgSpatialVector& Jacobian = copy[i];
			const dgSpatialVector& JacobianTranspose = jacobianMatrix[i];
			for (dgInt32 j = 0; j < 6; j++) {
				dgFloat64 val = -Jacobian[j];
				bodyMass[j] = bodyMass[j] + JacobianTranspose.Scale(val);
			}
		}
	}

	DG_INLINE void CalculateJointDiagonal (const dgSpatialMatrix* const bodyMassArray, dgSpatialMatrix* const jointMassArray)
	{
		const dgSpatialMatrix& bodyMass = bodyMassArray[m_index];
		const dgSpatialMatrix& bodyJt = m_data.m_body.m_jt;

		dgSpatialMatrix tmp;
		for (dgInt32 i = 0; i < m_dof; i++) {
			tmp[i] = bodyMass.VectorTimeMatrix(bodyJt[i]);
		}

		dgSpatialMatrix& jointMass = jointMassArray[m_index];
		for (dgInt32 i = 0; i < m_dof; i++) {
			dgFloat64 a = bodyJt[i].DotProduct(tmp[i]);
			jointMass[i][i] -= a;
			for (dgInt32 j = i + 1; j < m_dof; j++) {
				a = - bodyJt[i].DotProduct(tmp[j]);
				jointMass[i][j] = a;
				jointMass[j][i] = a;
			}
		}

		dgSpatialMatrix& jointInvMass = m_data.m_joint.m_invMass;
		jointInvMass = jointMass.Inverse(m_dof);
	}

	DG_INLINE void CalculateJacobianBlock()
	{
		dgSpatialMatrix& jointJ = m_data.m_joint.m_jt;

		dgSpatialMatrix copy;
		const dgSpatialVector zero (dgSpatialVector::m_zero);
		for (dgInt32 i = 0; i < m_dof; i++) {
			copy[i] = jointJ[i];
			jointJ[i] = zero;
		}

		const dgSpatialMatrix& jointInvMass = m_data.m_joint.m_invMass;
		for (dgInt32 i = 0; i < m_dof; i++) {
			const dgSpatialVector& jacobian = copy[i];
			const dgSpatialVector& invDiagonalRow = jointInvMass[i];
			for (dgInt32 j = 0; j < m_dof; j++) {
				dgFloat64 val = invDiagonalRow[j];
				jointJ[j] = jointJ[j] + jacobian.Scale(val);
			}
		}
	}

	DG_INLINE void JointJacobianTimeMassForward (dgForcePair& force)
	{
		const dgSpatialMatrix& bodyJt = m_data.m_body.m_jt;
		for (dgInt32 i = 0; i < m_dof; i++) {
			force.m_joint[i] -= bodyJt[i].DotProduct(force.m_body);
		}
	}

	DG_INLINE void BodyJacobianTimeMassForward(const dgForcePair& force, dgForcePair& parentForce) const 
	{
		const dgSpatialMatrix& jointJ = m_data.m_joint.m_jt;
		for (dgInt32 i = 0; i < m_dof; i++) {
			parentForce.m_body = parentForce.m_body + jointJ[i].Scale(-force.m_joint[i]);
		}
	}

	DG_INLINE void JointJacobianTimeSolutionBackward(dgForcePair& force, const dgForcePair& parentForce)
	{
		const dgSpatialMatrix& jointJ = m_data.m_joint.m_jt;
		const dgSpatialVector& f = parentForce.m_body;
		for (dgInt32 i = 0; i < m_dof; i++) {
			force.m_joint[i] -= f.DotProduct(jointJ[i]);
		}
	}

	DG_INLINE void BodyJacobianTimeSolutionBackward(dgForcePair& force)
	{
		const dgSpatialMatrix& bodyJt = m_data.m_body.m_jt;
		for (dgInt32 i = 0; i < m_dof; i++) {
			force.m_body = force.m_body + bodyJt[i].Scale(-force.m_joint[i]);
		}
	}

	DG_INLINE void BodyDiagInvTimeSolution(dgForcePair& force)
	{
		const dgSpatialMatrix& bodyInvMass = m_data.m_body.m_invMass;
		force.m_body = bodyInvMass.VectorTimeMatrix(force.m_body);
	}

	DG_INLINE void JointDiagInvTimeSolution(dgForcePair& force)
	{
		const dgSpatialMatrix& jointInvMass = m_data.m_joint.m_invMass;
		force.m_joint = jointInvMass.VectorTimeMatrix(force.m_joint, m_dof);
	}

	DG_INLINE dgInt32 GetAuxiliaryRows(const dgJointInfo* const jointInfoArray, const dgRightHandSide* const rightHandSide) const
	{
		dgInt32 rowCount = 0;
		if (m_joint) {
			dgAssert(m_parent);
			const dgJointInfo* const jointInfo = &jointInfoArray[m_joint->m_index];
			dgAssert(jointInfo->m_joint == m_joint);
			dgInt32 count = jointInfo->m_pairCount;
			const dgInt32 first = jointInfo->m_pairStart;
			for (dgInt32 i = 0; i < count; i++) {
				const dgRightHandSide* const rhs = &rightHandSide[i + first];
				if (!((rhs->m_lowerBoundFrictionCoefficent <= dgFloat32 (-DG_LCP_MAX_VALUE)) && (rhs->m_upperBoundFrictionCoefficent >= dgFloat32 (DG_LCP_MAX_VALUE)))) {
					rowCount++;
				}
			}
		}
		return rowCount;
	}
	
	dgBodyJointMatrixDataPair m_data;
	dgDynamicBody* m_body;
	dgBilateralConstraint* m_joint;
	dgNode* m_parent;
	dgNode* m_child;
	dgNode* m_sibling;
	union
	{
		dgInt64 m_ordinals;
		dgInt8 m_sourceJacobianIndex[8];
	};
	dgInt16 m_index;
	dgInt8 m_dof;
	dgInt8 m_swapJacobianBodiesIndex;
	static dgInt64 m_ordinalInit;
};

dgInt64 dgSkeletonContainer::dgNode::m_ordinalInit = 0x050403020100ll;

dgSkeletonContainer::dgSkeletonContainer(dgWorld* const world, dgDynamicBody* const rootBody)
	:m_world(world)
	,m_skeleton(new (world->GetAllocator()) dgNode(rootBody))
	,m_nodesOrder(NULL)
	,m_pairs(NULL)
	,m_deltaForce(NULL)
	,m_massMatrix11(NULL)
	,m_massMatrix10(NULL)
	,m_rightHandSide(NULL)
	,m_leftHandSide(NULL)
	,m_matrixRowsIndex(NULL)
	,m_loopingJoints(world->GetAllocator())
	,m_auxiliaryMemoryBuffer(world->GetAllocator())
	,m_id(m_uniqueID)
	,m_lru(0)
	,m_nodeCount(1)
	,m_loopCount(0)
	,m_selfContactCount(0)
	,m_rowCount(0)
	,m_loopRowCount(0)
	,m_auxiliaryRowCount(0)
{
	if (rootBody->GetInvMass().m_w != dgFloat32 (0.0f)) {
		rootBody->SetSkeleton(this);
	}
	m_uniqueID++;
}

dgSkeletonContainer::~dgSkeletonContainer()
{
	for (dgInt32 i = 0; i < m_loopCount; i++) {
		dgConstraint* const joint = m_loopingJoints[i];
		joint->m_isInSkeleton = false;
	}

	for (dgInt32 i = 0; i < m_nodeCount - 1; i ++) {
		m_nodesOrder[i]->m_joint->m_isInSkeleton = false;
	}

	dgMemoryAllocator* const allocator = m_world->GetAllocator();
	if (m_nodesOrder) {
		allocator->Free(m_nodesOrder);
	}

	delete m_skeleton;
}

dgSkeletonContainer::dgNode* dgSkeletonContainer::GetRoot () const
{
	return m_skeleton;
}

dgSkeletonContainer::dgNode* dgSkeletonContainer::GetParent (dgNode* const node) const
{
	return node->m_parent;
}

dgDynamicBody* dgSkeletonContainer::GetBody(dgSkeletonContainer::dgNode* const node) const
{
	return node->m_body;
}

dgBilateralConstraint* dgSkeletonContainer::GetJoint(dgSkeletonContainer::dgNode* const node) const
{
	return node->m_joint;
}

dgSkeletonContainer::dgNode* dgSkeletonContainer::GetFirstChild(dgSkeletonContainer::dgNode* const parent) const
{
	return parent->m_child;
}

dgSkeletonContainer::dgNode* dgSkeletonContainer::GetNextSiblingChild(dgSkeletonContainer::dgNode* const sibling) const
{
	return sibling->m_sibling;
}

dgWorld* dgSkeletonContainer::GetWorld() const
{
	return m_world;
}

void dgSkeletonContainer::ResetUniqueId(dgInt32 id)
{
	m_uniqueID = id;
}

void dgSkeletonContainer::ClearSelfCollision()
{
	m_selfContactCount = 0;
}

void dgSkeletonContainer::AddSelfCollisionJoint(dgContact* const contact)
{
	m_world->GlobalLock();
	m_loopingJoints[m_loopCount + m_selfContactCount] = contact;
	m_selfContactCount++;
	m_world->GlobalUnlock();
}

void dgSkeletonContainer::SortGraph(dgNode* const root, dgInt32& index)
{
	for (dgNode* node = root->m_child; node; node = node->m_sibling) {
		SortGraph(node, index);
	}

	dgAssert((m_nodeCount - index - 1) >= 0);
	m_nodesOrder[index] = root;
	root->m_index = dgInt16(index);
	index++;
	dgAssert(index <= m_nodeCount);
}

dgSkeletonContainer::dgNode* dgSkeletonContainer::FindNode(dgDynamicBody* const body) const
{
	dgInt32 stack = 1;
	dgNode* stackPool[1024];

	stackPool[0] = m_skeleton;
	while (stack) {
		stack--;
		dgNode* const node = stackPool[stack];
		if (node->m_body == body) {
			return node;
		}

		for (dgNode* ptr = node->m_child; ptr; ptr = ptr->m_sibling) {
			stackPool[stack] = ptr;
			stack++;
			dgAssert(stack < dgInt32(sizeof (stackPool) / sizeof (stackPool[0])));
		}
	}
	return NULL;
}

dgSkeletonContainer::dgNode* dgSkeletonContainer::AddChild(dgBilateralConstraint* const joint, dgNode* const parent)
{
	dgAssert (m_skeleton->m_body);
	dgMemoryAllocator* const allocator = m_world->GetAllocator();
	dgNode* const node = new (allocator)dgNode(joint, parent);
	m_nodeCount ++;

	joint->m_isInSkeleton = true;
	dgAssert (m_world->GetSentinelBody() != node->m_body);
	node->m_body->SetSkeleton(this);
	return node;
}

void dgSkeletonContainer::RemoveLoopJoint(dgBilateralConstraint* const joint)
{
	for (dgInt32 i = 0; i < m_loopCount; i++) {
		if (m_loopingJoints[i] == joint) {
			joint->m_isInSkeleton = false;
			m_loopCount--;
			m_loopingJoints[i] = m_loopingJoints[m_loopCount];
			break;
		}
	}
}


void dgSkeletonContainer::Finalize(dgInt32 loopJointsCount, dgBilateralConstraint** const loopJointArray)
{
	dgAssert(m_nodeCount >= 1);

	const dgDynamicBody* const rootBody = m_skeleton->m_body;
	dgAssert (((rootBody->GetInvMass().m_w == dgFloat32 (0.0f)) && (m_skeleton->m_child->m_sibling == NULL)) || (m_skeleton->m_body->GetInvMass().m_w != dgFloat32 (0.0f)));

	dgMemoryAllocator* const allocator = rootBody->GetWorld()->GetAllocator();
	m_nodesOrder = (dgNode**)allocator->Malloc(m_nodeCount * sizeof (dgNode*));

	dgInt32 index = 0;
	SortGraph(m_skeleton, index);
	dgAssert(index == m_nodeCount);

	if (loopJointsCount) {
		//dgTree<dgInt32, dgDynamicBody*> filter (allocator);
		for (dgInt32 i = 0; i < loopJointsCount; i++) {
			dgBilateralConstraint* const joint = loopJointArray[i];
			dgAssert(joint->GetBody0()->IsRTTIType(dgBody::m_dynamicBodyRTTI));
			dgAssert(joint->GetBody1()->IsRTTIType(dgBody::m_dynamicBodyRTTI));
			dgAssert((FindNode((dgDynamicBody*)joint->GetBody0()) || FindNode((dgDynamicBody*)joint->GetBody1())));
			joint->m_isInSkeleton = true;

			//dgDynamicBody* const body0 = (dgDynamicBody*)joint->GetBody0();
			//dgDynamicBody* const body1 = (dgDynamicBody*)joint->GetBody1();
			//dgNode* const node0 = FindNode(body0);
			//dgNode* const node1 = FindNode(body1);
			//if (node0) {
			//	filter.Insert(node0->m_index, node0->m_body);
			//}
			//if (node1) {
			//	filter.Insert(node1->m_index, node1->m_body);
			//}
			//dgTree<dgInt32, dgDynamicBody*>::dgTreeNode* index0 = filter.Find(body0);
			//if (!index0) {
			//	index0 = filter.Insert(-1, body0);
			//}
			//dgTree<dgInt32, dgDynamicBody*>::dgTreeNode* index1 = filter.Find(body1);
			//if (!index1) {
			//	index1 = filter.Insert(-1, body1);
			//}
			m_loopingJoints[m_loopCount] = joint;
			m_loopCount++;
		}

		//for (dgInt32 i = 0; i < m_nodeCount; i++) {
		//	filter.Remove(m_nodesOrder[i]->m_body);
		//}
		//dgTree<dgDynamicBody*, dgInt32> bodyOrder (allocator);
		//dgTree<dgInt32, dgDynamicBody*>::Iterator iter(filter);
		//for (iter.Begin(); iter; iter ++) {
		//	bodyOrder.Insert (iter.GetKey(), iter.GetNode()->GetInfo());
		//}
	}
}

DG_INLINE void dgSkeletonContainer::CalculateLoopMassMatrixCoefficients(dgFloat32* const diagDamp)
{
	const dgInt32 primaryCount = m_rowCount - m_auxiliaryRowCount;
	for (dgInt32 i = 0; i < m_auxiliaryRowCount; i++) {
		const dgInt32 ii = m_matrixRowsIndex[primaryCount + i];
		const dgLeftHandSide* const row_i = &m_leftHandSide[ii];
		const dgRightHandSide* const rhs_i = &m_rightHandSide[ii];
		dgFloat32* const matrixRow11 = &m_massMatrix11[m_auxiliaryRowCount * i];

		dgJacobian JMinvM0(row_i->m_JMinv.m_jacobianM0);
		dgJacobian JMinvM1(row_i->m_JMinv.m_jacobianM1);

		dgVector element(JMinvM0.m_linear * row_i->m_Jt.m_jacobianM0.m_linear + JMinvM0.m_angular * row_i->m_Jt.m_jacobianM0.m_angular +
						 JMinvM1.m_linear * row_i->m_Jt.m_jacobianM1.m_linear + JMinvM1.m_angular * row_i->m_Jt.m_jacobianM1.m_angular);
		element = element.AddHorizontal();

		// I know I am doubling the matrix regularizer, but this makes the solution more robust.
		dgFloat32 diagonal = element.GetScalar() + rhs_i->m_diagDamp;
		matrixRow11[i] = diagonal + rhs_i->m_diagDamp;
		diagDamp[i] = matrixRow11[i] * (DG_PSD_DAMP_TOL * dgFloat32(4.0f));

		const dgInt32 m0_i = m_pairs[primaryCount + i].m_m0;
		const dgInt32 m1_i = m_pairs[primaryCount + i].m_m1;
		for (dgInt32 j = i + 1; j < m_auxiliaryRowCount; j++) {
			const dgInt32 jj = m_matrixRowsIndex[primaryCount + j];
			const dgLeftHandSide* const row_j = &m_leftHandSide[jj];

			const dgInt32 k = primaryCount + j;
			dgVector acc(dgVector::m_zero);
			const dgInt32 m0_j = m_pairs[k].m_m0;
			const dgInt32 m1_j = m_pairs[k].m_m1;
			bool hasEffect = false;
			if (m0_i == m0_j) {
				hasEffect = true;
				acc += JMinvM0.m_linear * row_j->m_Jt.m_jacobianM0.m_linear + JMinvM0.m_angular * row_j->m_Jt.m_jacobianM0.m_angular;
			} else if (m0_i == m1_j) {
				hasEffect = true;
				acc += JMinvM0.m_linear * row_j->m_Jt.m_jacobianM1.m_linear + JMinvM0.m_angular * row_j->m_Jt.m_jacobianM1.m_angular;
			}

			if (m1_i == m1_j) {
				hasEffect = true;
				acc += JMinvM1.m_linear * row_j->m_Jt.m_jacobianM1.m_linear + JMinvM1.m_angular * row_j->m_Jt.m_jacobianM1.m_angular;
			} else if (m1_i == m0_j) {
				hasEffect = true;
				acc += JMinvM1.m_linear * row_j->m_Jt.m_jacobianM0.m_linear + JMinvM1.m_angular * row_j->m_Jt.m_jacobianM0.m_angular;
			}

			if (hasEffect) {
				acc = acc.AddHorizontal();
				dgFloat32 offDiagValue = acc.GetScalar();
				matrixRow11[j] = offDiagValue;
				m_massMatrix11[j * m_auxiliaryRowCount + i] = offDiagValue;
			}
		}

		dgFloat32* const matrixRow10 = &m_massMatrix10[primaryCount * i];
		for (dgInt32 j = 0; j < primaryCount; j++) {
			const dgInt32 jj = m_matrixRowsIndex[j];
			const dgLeftHandSide* const row_j = &m_leftHandSide[jj];

			const dgInt32 m0_j = m_pairs[j].m_m0;
			const dgInt32 m1_j = m_pairs[j].m_m1;
			dgVector acc(dgVector::m_zero);
			bool hasEffect = false;
			if (m0_i == m0_j) {
				hasEffect = true;
				acc += JMinvM0.m_linear * row_j->m_Jt.m_jacobianM0.m_linear + JMinvM0.m_angular * row_j->m_Jt.m_jacobianM0.m_angular;
			} else if (m0_i == m1_j) {
				hasEffect = true;
				acc += JMinvM0.m_linear * row_j->m_Jt.m_jacobianM1.m_linear + JMinvM0.m_angular * row_j->m_Jt.m_jacobianM1.m_angular;
			}

			if (m1_i == m1_j) {
				hasEffect = true;
				acc += JMinvM1.m_linear * row_j->m_Jt.m_jacobianM1.m_linear + JMinvM1.m_angular * row_j->m_Jt.m_jacobianM1.m_angular;
			} else if (m1_i == m0_j) {
				hasEffect = true;
				acc += JMinvM1.m_linear * row_j->m_Jt.m_jacobianM0.m_linear + JMinvM1.m_angular * row_j->m_Jt.m_jacobianM0.m_angular;
			}

			if (hasEffect) {
				acc = acc.AddHorizontal();
				dgFloat32 val = acc.GetScalar();
				matrixRow10[j] = val;
			}
		}
	}
}


void dgSkeletonContainer::InitLoopMassMatrix(const dgJointInfo* const jointInfoArray)
{
	const dgInt32 primaryCount = m_rowCount - m_auxiliaryRowCount;
	dgInt8* const memoryBuffer = CalculateBufferSizeInBytes(jointInfoArray);

	m_matrixRowsIndex = (dgInt32*)memoryBuffer;
	m_pairs = (dgNodePair*)&m_matrixRowsIndex[m_rowCount];
	m_massMatrix11 = (dgFloat32*)&m_pairs[m_rowCount];
	m_massMatrix10 = (dgFloat32*)&m_massMatrix11[m_auxiliaryRowCount * m_auxiliaryRowCount];
	m_deltaForce = &m_massMatrix10[m_auxiliaryRowCount * primaryCount];

	dgForcePair* const forcePair = dgAlloca(dgForcePair, m_nodeCount);
	dgForcePair* const accelPair = dgAlloca(dgForcePair, m_nodeCount);
	dgFloat32* const diagDamp = dgAlloca(dgFloat32, m_auxiliaryRowCount);

	dgInt32 primaryIndex = 0;
	dgInt32 auxiliaryIndex = 0;
	for (dgInt32 i = 0; i < m_nodeCount - 1; i++) {
		const dgNode* const node = m_nodesOrder[i];
		const dgJointInfo* const jointInfo = &jointInfoArray[node->m_joint->m_index];

		const dgInt32 m0 = jointInfo->m_m0;
		const dgInt32 m1 = jointInfo->m_m1;

		const dgInt32 primaryDof = node->m_dof;
		const dgInt32 first = jointInfo->m_pairStart;

		for (dgInt32 j = 0; j < primaryDof; j++) {
			const dgInt32 index = node->m_sourceJacobianIndex[j];
			m_pairs[primaryIndex].m_m0 = m0;
			m_pairs[primaryIndex].m_m1 = m1;
			m_matrixRowsIndex[primaryIndex] = first + index;
			primaryIndex++;
		}

		const dgInt32 auxiliaryDof = jointInfo->m_pairCount - primaryDof;
		for (dgInt32 j = 0; j < auxiliaryDof; j++) {
			const dgInt32 index = node->m_sourceJacobianIndex[primaryDof + j];

			m_pairs[auxiliaryIndex + primaryCount].m_m0 = m0;
			m_pairs[auxiliaryIndex + primaryCount].m_m1 = m1;
			m_matrixRowsIndex[auxiliaryIndex + primaryCount] = first + index;
			auxiliaryIndex++;
		}
	}
	dgAssert (m_loopRowCount == (m_auxiliaryRowCount - auxiliaryIndex));
	
	const dgInt32 loopCount = m_loopCount + m_selfContactCount;
	for (dgInt32 j = 0; j < loopCount; j ++) {
		const dgConstraint* const joint = m_loopingJoints[j];
		const dgJointInfo* const jointInfo = &jointInfoArray[joint->m_index];

		const dgInt32 m0 = jointInfo->m_m0;
		const dgInt32 m1 = jointInfo->m_m1;
		const dgInt32 first = jointInfo->m_pairStart;
		const dgInt32 auxiliaryDof = jointInfo->m_pairCount;

		for (dgInt32 i = 0; i < auxiliaryDof; i++) {
			m_pairs[auxiliaryIndex + primaryCount].m_m0 = m0;
			m_pairs[auxiliaryIndex + primaryCount].m_m1 = m1;
			m_matrixRowsIndex[auxiliaryIndex + primaryCount] = first + i;
			auxiliaryIndex++;
		}
	}
	dgAssert(primaryIndex == primaryCount);
	dgAssert(auxiliaryIndex == m_auxiliaryRowCount);
	memset(m_massMatrix10, 0, primaryCount * m_auxiliaryRowCount * sizeof(dgFloat32));
	memset(m_massMatrix11, 0, m_auxiliaryRowCount * m_auxiliaryRowCount * sizeof(dgFloat32));

	CalculateLoopMassMatrixCoefficients(diagDamp);

	const dgSpatialVector zero (dgSpatialVector::m_zero);
	accelPair[m_nodeCount - 1].m_body = zero;
	accelPair[m_nodeCount - 1].m_joint = zero;

	for (dgInt32 i = 0; i < m_auxiliaryRowCount; i++) {
		dgInt32 entry = 0;
		dgInt32 startjoint = m_nodeCount;
		const dgFloat32* const matrixRow10 = &m_massMatrix10[i * primaryCount];
		for (dgInt32 j = 0; j < m_nodeCount - 1; j++) {
			const dgNode* const node = m_nodesOrder[j];
			const dgInt32 index = node->m_index;
			accelPair[index].m_body = zero;
			dgSpatialVector& a = accelPair[index].m_joint;

			const int count = node->m_dof;
			for (dgInt32 k = 0; k < count; k++) {
				const dgFloat32 value = matrixRow10[entry];
				a[k] = value;
				startjoint = (value == 0.0f) ? startjoint : dgMin (startjoint, index);
				entry++;
			}
		}

		entry = 0;
		startjoint = (startjoint == m_nodeCount) ? 0 : startjoint;
		dgAssert (startjoint < m_nodeCount);
		SolveForward(forcePair, accelPair, startjoint);
		SolveBackward(forcePair, forcePair);

		dgFloat32* const deltaForcePtr = &m_deltaForce[i * primaryCount];
		for (dgInt32 j = 0; j < m_nodeCount - 1; j++) {
			const dgNode* const node = m_nodesOrder[j];
			const dgInt32 index = node->m_index;
			const dgSpatialVector& f = forcePair[index].m_joint;
			const int count = node->m_dof;
			for (dgInt32 k = 0; k < count; k++) {
				deltaForcePtr[entry] = dgFloat32(f[k]);
				entry++;
			}
		}
	}

	dgInt16* const indexList = dgAlloca(dgInt16, primaryCount);
	for (dgInt32 i = 0; i < m_auxiliaryRowCount; i++) {
		const dgFloat32* const matrixRow10 = &m_massMatrix10[i * primaryCount];
		const dgFloat32* const deltaForcePtr = &m_deltaForce[i * primaryCount];
		dgFloat32* const matrixRow11 = &m_massMatrix11[i * m_auxiliaryRowCount];

		dgInt32 indexCount = 0;
		for (dgInt32 k = 0; k < primaryCount; k++) {
			indexList[indexCount] = dgInt16(k);
			indexCount += (matrixRow10[k] != dgFloat32(0.0f)) ? 1 : 0;
		}

		dgFloat32 diagonal = matrixRow11[i];
		for (dgInt32 k = 0; k < indexCount; k++) {
			dgInt32 index = indexList[k];
			diagonal += matrixRow10[index] * deltaForcePtr[index];
		}
		matrixRow11[i] = dgMax(diagonal, diagDamp[i]);

		for (dgInt32 j = i + 1; j < m_auxiliaryRowCount; j++) {
			dgFloat32 offDiagonal = matrixRow11[j];
			const dgFloat32* const row10 = &m_deltaForce[j * primaryCount];
			for (dgInt32 k = 0; k < indexCount; k++) {
				dgInt32 index = indexList[k];
				offDiagonal += matrixRow10[index] * row10[index];
			}
			matrixRow11[j] = offDiagonal;
			m_massMatrix11[j * m_auxiliaryRowCount + i] = offDiagonal;
		}
	}

	dgCholeskyApplyRegularizer(m_auxiliaryRowCount, m_massMatrix11, diagDamp);
}


bool dgSkeletonContainer::SanityCheck(const dgForcePair* const force, const dgForcePair* const accel) const
{
	return true;
}

DG_INLINE void dgSkeletonContainer::SolveForward(dgForcePair* const force, const dgForcePair* const accel, dgInt32 startNode) const
{
	dgSpatialVector zero (dgSpatialVector::m_zero);
	for (dgInt32 i = 0; i < startNode; i++) {
		force[i].m_body = zero;
		force[i].m_joint = zero;
	}
	for (dgInt32 i = startNode; i < m_nodeCount - 1; i++) {
		dgNode* const node = m_nodesOrder[i];
		dgAssert(node->m_joint);
		dgAssert(node->m_index == i);
		dgForcePair& f = force[i];
		const dgForcePair& a = accel[i];
		f.m_body = a.m_body;
		f.m_joint = a.m_joint;
		for (dgNode* child = node->m_child; child; child = child->m_sibling) {
			dgAssert(child->m_joint);
			dgAssert(child->m_parent->m_index == i);
			child->BodyJacobianTimeMassForward(force[child->m_index], f);
		}
		node->JointJacobianTimeMassForward(f);
	}

	force[m_nodeCount - 1] = accel[m_nodeCount - 1];
	for (dgNode* child = m_nodesOrder[m_nodeCount - 1]->m_child; child; child = child->m_sibling) {
		child->BodyJacobianTimeMassForward(force[child->m_index], force[child->m_parent->m_index]);
	}

	for (dgInt32 i = startNode; i < m_nodeCount - 1; i++) {
		dgNode* const node = m_nodesOrder[i];
		dgForcePair& f = force[i];
		node->BodyDiagInvTimeSolution(f);
		node->JointDiagInvTimeSolution(f);
	}
	m_nodesOrder[m_nodeCount - 1]->BodyDiagInvTimeSolution(force[m_nodeCount - 1]);
}


DG_INLINE void dgSkeletonContainer::SolveBackward(dgForcePair* const force, const dgForcePair* const accel) const
{
	for (dgInt32 i = m_nodeCount - 2; i >= 0; i--) {
		dgNode* const node = m_nodesOrder[i];
		dgAssert(node->m_index == i);
		dgForcePair& f = force[i];
		node->JointJacobianTimeSolutionBackward(f, force[node->m_parent->m_index]);
		node->BodyJacobianTimeSolutionBackward(f);
	}
}


DG_INLINE void dgSkeletonContainer::CalculateForce (dgForcePair* const force, const dgForcePair* const accel) const
{
	SolveForward(force, accel);
	SolveBackward(force, accel);
}

DG_INLINE void dgSkeletonContainer::UpdateForces (dgJointInfo* const jointInfoArray, dgJacobian* const internalForces, const dgForcePair* const force) const
{
	dgVector zero (dgVector::m_zero);
	for (dgInt32 i = 0; i < (m_nodeCount - 1)  ; i ++) {
		dgNode* const node = m_nodesOrder[i];
		const dgJointInfo* const jointInfo = &jointInfoArray[node->m_joint->m_index];

		dgJacobian y0;
		dgJacobian y1;
		y0.m_linear = zero;
		y0.m_angular = zero;
		y1.m_linear = zero;
		y1.m_angular = zero;
		dgAssert(i == node->m_index);

		const dgSpatialVector& f = force[i].m_joint;
		dgAssert(jointInfo->m_joint == node->m_joint);
		const dgInt32 first = jointInfo->m_pairStart;
		const dgInt32 count = node->m_dof;
		for (dgInt32 j = 0; j < count; j ++) {
			const dgInt32 k = node->m_sourceJacobianIndex[j];
			dgRightHandSide* const rhs = &m_rightHandSide[first + k];
			const dgLeftHandSide* const row = &m_leftHandSide[first + k];

			rhs->m_force += dgFloat32(f[j]);
			dgVector jointForce = dgFloat32 (f[j]);
			y0.m_linear += row->m_Jt.m_jacobianM0.m_linear * jointForce;
			y0.m_angular += row->m_Jt.m_jacobianM0.m_angular * jointForce;
			y1.m_linear += row->m_Jt.m_jacobianM1.m_linear * jointForce;
			y1.m_angular += row->m_Jt.m_jacobianM1.m_angular * jointForce;
		}

		const dgInt32 m0 = jointInfo->m_m0;
		const dgInt32 m1 = jointInfo->m_m1;
		internalForces[m0].m_linear += y0.m_linear;
		internalForces[m0].m_angular += y0.m_angular;
		internalForces[m1].m_linear += y1.m_linear;
		internalForces[m1].m_angular += y1.m_angular;
	}
}

DG_INLINE void dgSkeletonContainer::CalculateJointAccel(dgJointInfo* const jointInfoArray, const dgJacobian* const internalForces, dgForcePair* const accel) const
{
	const dgSpatialVector zero (dgSpatialVector::m_zero);
	for (dgInt32 i = 0; i < m_nodeCount - 1; i++) {
		dgNode* const node = m_nodesOrder[i];
		dgAssert(i == node->m_index);

		dgForcePair& a = accel[i];
		dgAssert(node->m_body);
		a.m_body = zero;
		a.m_joint = zero;

		dgAssert(node->m_joint);
		const dgJointInfo* const jointInfo = &jointInfoArray[node->m_joint->m_index];
		dgAssert(jointInfo->m_joint == node->m_joint);

		const dgInt32 first = jointInfo->m_pairStart;
		const dgInt32 dof = jointInfo->m_pairCount;
		const dgInt32 m0 = jointInfo->m_m0;
		const dgInt32 m1 = jointInfo->m_m1;
		const dgJacobian& y0 = internalForces[m0];
		const dgJacobian& y1 = internalForces[m1];

		for (dgInt32 j = 0; j < dof; j++) {
			const dgInt32 k = node->m_sourceJacobianIndex[j];
			const dgLeftHandSide* const row = &m_leftHandSide[first + k];
			const dgRightHandSide* const rhs = &m_rightHandSide[first + k];
			dgVector diag(row->m_JMinv.m_jacobianM0.m_linear * y0.m_linear + row->m_JMinv.m_jacobianM0.m_angular * y0.m_angular +
						  row->m_JMinv.m_jacobianM1.m_linear * y1.m_linear + row->m_JMinv.m_jacobianM1.m_angular * y1.m_angular);
			a.m_joint[j] = -(rhs->m_coordenateAccel - rhs->m_force * rhs->m_diagDamp - diag.AddHorizontal().GetScalar());
		}
	}
	dgAssert((m_nodeCount - 1) == m_nodesOrder[m_nodeCount - 1]->m_index);
	accel[m_nodeCount - 1].m_body = zero;
	accel[m_nodeCount - 1].m_joint = zero;
}


void dgSkeletonContainer::SolveAuxiliary(const dgJointInfo* const jointInfoArray, dgJacobian* const internalForces, const dgForcePair* const accel, dgForcePair* const force) const
{
	dgFloat32* const f = dgAlloca(dgFloat32, m_rowCount);
	dgFloat32* const u = dgAlloca(dgFloat32, m_auxiliaryRowCount);
	dgFloat32* const b = dgAlloca(dgFloat32, m_auxiliaryRowCount);
	dgFloat32* const low = dgAlloca(dgFloat32, m_auxiliaryRowCount);
	dgFloat32* const high = dgAlloca(dgFloat32, m_auxiliaryRowCount);
	dgFloat32* const massMatrix11 = dgAlloca(dgFloat32, m_auxiliaryRowCount * m_auxiliaryRowCount);

	dgInt32 primaryIndex = 0;
	dgInt32 auxiliaryIndex = 0;
	const dgInt32 primaryCount = m_rowCount - m_auxiliaryRowCount;
	
	for (dgInt32 i = 0; i < m_nodeCount - 1; i++) { 
		const dgNode* const node = m_nodesOrder[i];
		const dgJointInfo* const jointInfo = &jointInfoArray[node->m_joint->m_index];
		const dgInt32 first = jointInfo->m_pairStart;

		const dgInt32 primaryDof = node->m_dof;
		const dgSpatialVector& accelSpatial = accel[i].m_joint;
		const dgSpatialVector& forceSpatial = force[i].m_joint;

		for (dgInt32 j = 0; j < primaryDof; j++) {
			f[primaryIndex] = dgFloat32(forceSpatial[j]);
			primaryIndex++;
		}

		const dgInt32 auxiliaryDof = jointInfo->m_pairCount - primaryDof;
		for (dgInt32 j = 0; j < auxiliaryDof; j++) {
			const dgInt32 index = node->m_sourceJacobianIndex[primaryDof + j];
			dgRightHandSide* const rhs = &m_rightHandSide[first + index];
			f[auxiliaryIndex + primaryCount] = dgFloat32 (0.0f);
			b[auxiliaryIndex] = -dgFloat32(accelSpatial[primaryDof + j]);
			dgAssert (rhs->m_force >= rhs->m_lowerBoundFrictionCoefficent * dgFloat32 (2.0f));
			dgAssert (rhs->m_force <= rhs->m_upperBoundFrictionCoefficent * dgFloat32 (2.0f));
			low[auxiliaryIndex] = dgClamp (rhs->m_lowerBoundFrictionCoefficent - rhs->m_force, -DG_MAX_BOUND, dgFloat32 (0.0f));
			high[auxiliaryIndex] = dgClamp (rhs->m_upperBoundFrictionCoefficent - rhs->m_force, dgFloat32 (0.0f), DG_MAX_BOUND);
			auxiliaryIndex++;
		}
	}

	for (dgInt32 j = 0; j < m_loopCount; j++) {
		const dgConstraint* const joint = m_loopingJoints[j];
		const dgJointInfo* const jointInfo = &jointInfoArray[joint->m_index];

		const dgInt32 m0 = jointInfo->m_m0;
		const dgInt32 m1 = jointInfo->m_m1;
		const dgInt32 first = jointInfo->m_pairStart;
		const dgInt32 auxiliaryDof = jointInfo->m_pairCount;
		const dgJacobian& y0 = internalForces[m0];
		const dgJacobian& y1 = internalForces[m1];

		for (dgInt32 i = 0; i < auxiliaryDof; i++) {
			const dgRightHandSide* const rhs = &m_rightHandSide[first + i];
			const dgLeftHandSide* const row = &m_leftHandSide[first + i];
			f[auxiliaryIndex + primaryCount] = dgFloat32 (0.0f);
			dgVector acc(row->m_JMinv.m_jacobianM0.m_linear * y0.m_linear + row->m_JMinv.m_jacobianM0.m_angular * y0.m_angular +
						 row->m_JMinv.m_jacobianM1.m_linear * y1.m_linear + row->m_JMinv.m_jacobianM1.m_angular * y1.m_angular);
			b[auxiliaryIndex] = rhs->m_coordenateAccel - acc.AddHorizontal().GetScalar();
			
			dgAssert(rhs->m_force >= rhs->m_lowerBoundFrictionCoefficent * dgFloat32(2.0f));
			dgAssert(rhs->m_force <= rhs->m_upperBoundFrictionCoefficent * dgFloat32(2.0f));
			low[auxiliaryIndex] = dgClamp (rhs->m_lowerBoundFrictionCoefficent - rhs->m_force, -DG_MAX_BOUND, dgFloat32 (0.0f));
			high[auxiliaryIndex] = dgClamp (rhs->m_upperBoundFrictionCoefficent - rhs->m_force, dgFloat32 (0.0f), DG_MAX_BOUND);
			auxiliaryIndex++;
		}
	}

	for (dgInt32 j = 0; j < m_selfContactCount; j++) {
		const dgConstraint* const joint = m_loopingJoints[m_loopCount + j];
		const dgJointInfo* const jointInfo = &jointInfoArray[joint->m_index];

		const dgInt32 m0 = jointInfo->m_m0;
		const dgInt32 m1 = jointInfo->m_m1;
		const dgInt32 first = jointInfo->m_pairStart;
		const dgInt32 auxiliaryDof = jointInfo->m_pairCount;
		const dgJacobian& y0 = internalForces[m0];
		const dgJacobian& y1 = internalForces[m1];

		dgFloat32 normalForce[DG_CONSTRAINT_MAX_ROWS + 1];
		normalForce[0] = dgFloat32 (1.0f);
		for (dgInt32 i = 0; i < auxiliaryDof; i++) {
			const dgLeftHandSide* const row = &m_leftHandSide[first + i];
			const dgRightHandSide* const rhs = &m_rightHandSide[first + i];

			f[auxiliaryIndex + primaryCount] = dgFloat32(0.0f);
			dgVector acc(row->m_JMinv.m_jacobianM0.m_linear * y0.m_linear + row->m_JMinv.m_jacobianM0.m_angular * y0.m_angular +
						 row->m_JMinv.m_jacobianM1.m_linear * y1.m_linear + row->m_JMinv.m_jacobianM1.m_angular * y1.m_angular);
			b[auxiliaryIndex] = rhs->m_coordenateAccel - acc.AddHorizontal().GetScalar();

			dgAssert(rhs->m_normalForceIndex >= -1);
			dgAssert(rhs->m_normalForceIndex <= auxiliaryDof);
			dgInt32 frictionIndex = rhs->m_normalForceIndex + 1;
			dgFloat32 frictionNormal = normalForce[frictionIndex];
			dgFloat32 lowerFrictionForce = frictionNormal * rhs->m_lowerBoundFrictionCoefficent;
			dgFloat32 upperFrictionForce = frictionNormal * rhs->m_upperBoundFrictionCoefficent;

			//dgAssert(rhs->m_force >= rhs->m_lowerBoundFrictionCoefficent * dgFloat32(2.0f));
			//dgAssert(rhs->m_force <= row->m_upperBoundFrictionCoefficent * dgFloat32(2.0f));
			//low[auxiliaryIndex] = dgClamp(rhs->m_lowerBoundFrictionCoefficent - rhs->m_force, -DG_MAX_BOUND, dgFloat32(0.0f));
			//high[auxiliaryIndex] = dgClamp(rhs->m_upperBoundFrictionCoefficent - rhs->m_force, dgFloat32(0.0f), DG_MAX_BOUND);
			low[auxiliaryIndex] = dgClamp(lowerFrictionForce - rhs->m_force, -DG_MAX_BOUND, dgFloat32(0.0f));
			high[auxiliaryIndex] = dgClamp(upperFrictionForce - rhs->m_force, dgFloat32(0.0f), DG_MAX_BOUND);

			normalForce[i + 1] = rhs->m_force;
			auxiliaryIndex++;
		}
	}

	memcpy (massMatrix11, m_massMatrix11, sizeof (dgFloat32) * m_auxiliaryRowCount * m_auxiliaryRowCount);
	for (dgInt32 i = 0; i < m_auxiliaryRowCount; i ++) {
		dgFloat32* const matrixRow10 = &m_massMatrix10[i * primaryCount];
		//u[i] = dgFloat32(0.0f);
		dgFloat32 r = dgFloat32(0.0f);
		for (dgInt32 j = 0; j < primaryCount; j++) {
			r += matrixRow10[j] * f[j];
		}
		b[i] -= r;
	}
	
	//dgSolveDantzigLCP(m_auxiliaryRowCount, massMatrix11, lowerTriangularMassMatrix11, u, b, low, high);
	dgSolveDantzigLCP(m_auxiliaryRowCount, massMatrix11, u, b, low, high);

	for (dgInt32 i = 0; i < m_auxiliaryRowCount; i++) {
		const dgFloat32 s = u[i];
		f[primaryCount + i] = s;
		const dgFloat32* const deltaForce = &m_deltaForce[i * primaryCount];
		for (dgInt32 j = 0; j < primaryCount; j++) {
			f[j] += deltaForce[j] * s;
		}
	}

	for (dgInt32 i = 0; i < m_rowCount; i++) {
		dgInt32 index = m_matrixRowsIndex[i];
		dgRightHandSide* const rhs = &m_rightHandSide[index];
		const dgLeftHandSide* const row = &m_leftHandSide[index];
		const dgInt32 m0 = m_pairs[i].m_m0;
		const dgInt32 m1 = m_pairs[i].m_m1;

		rhs->m_force += f[i];
		dgVector jointForce(f[i]);
		internalForces[m0].m_linear += row->m_Jt.m_jacobianM0.m_linear * jointForce;
		internalForces[m0].m_angular += row->m_Jt.m_jacobianM0.m_angular * jointForce;
		internalForces[m1].m_linear += row->m_Jt.m_jacobianM1.m_linear * jointForce;
		internalForces[m1].m_angular += row->m_Jt.m_jacobianM1.m_angular * jointForce;
	}
}


dgInt8* dgSkeletonContainer::CalculateBufferSizeInBytes (const dgJointInfo* const jointInfoArray)
{
	dgInt32 rowCount = 0;
	dgInt32 auxiliaryRowCount = 0;
	if (m_nodesOrder) {
		for (dgInt32 i = 0; i < m_nodeCount - 1; i++) {
			dgNode* const node = m_nodesOrder[i];
			rowCount += jointInfoArray[node->m_joint->m_index].m_pairCount;
			auxiliaryRowCount += node->GetAuxiliaryRows(jointInfoArray, m_rightHandSide);
		}
	}

	dgInt32 extraAuxiliaryRows = 0;
	const dgInt32 loopCount = m_loopCount + m_selfContactCount;
	for (dgInt32 j = 0; j < loopCount; j++) {
		const dgConstraint* const joint = m_loopingJoints[j];
		extraAuxiliaryRows += jointInfoArray[joint->m_index].m_pairCount;
	}
	rowCount += extraAuxiliaryRows;
	auxiliaryRowCount+= extraAuxiliaryRows;

//	dgInt32 size = sizeof (dgLeftHandSide*) * rowCount;
//	size += sizeof (dgRightHandSide*) * rowCount;
	dgInt32 size = sizeof (dgInt32) * rowCount;
	size += sizeof (dgNodePair) * rowCount;
	size += sizeof(dgFloat32) * auxiliaryRowCount * auxiliaryRowCount;		// matrix11[auxiliaryRowCount * auxiliaryRowCount]
//	size += sizeof (dgFloat32) * auxiliaryRowCount * auxiliaryRowCount;		// matrixLowerTraingular [auxiliaryRowCount * auxiliaryRowCount]
	size += sizeof (dgFloat32) * auxiliaryRowCount * (rowCount - auxiliaryRowCount);
	size += sizeof (dgFloat32) * auxiliaryRowCount * (rowCount - auxiliaryRowCount);
	size = (size + 1024) & -0x10;
	m_auxiliaryMemoryBuffer.ResizeIfNecessary((size + 1024) & -0x10);
	return &m_auxiliaryMemoryBuffer[0];
}


void dgSkeletonContainer::InitMassMatrix(const dgJointInfo* const jointInfoArray, const dgLeftHandSide* const leftHandSide, dgRightHandSide* const rightHandSide)
{
	dgInt32 rowCount = 0;
	dgInt32 auxiliaryCount = 0;
	m_leftHandSide = leftHandSide;
	m_rightHandSide = rightHandSide;

	dgSpatialMatrix* const bodyMassArray = dgAlloca (dgSpatialMatrix, m_nodeCount);
	dgSpatialMatrix* const jointMassArray = dgAlloca (dgSpatialMatrix, m_nodeCount);

	if (m_nodesOrder) {
		for (dgInt32 i = 0; i < m_nodeCount - 1; i++) {
			dgNode* const node = m_nodesOrder[i];
			const dgJointInfo& info = jointInfoArray[node->m_joint->m_index];
			rowCount += info.m_pairCount;
			auxiliaryCount += node->Factorize(jointInfoArray, leftHandSide, rightHandSide, bodyMassArray, jointMassArray);
		}
		m_nodesOrder[m_nodeCount - 1]->Factorize(jointInfoArray, leftHandSide, rightHandSide, bodyMassArray, jointMassArray);
	}
	m_rowCount = dgInt16 (rowCount);
	m_auxiliaryRowCount = dgInt16 (auxiliaryCount);

	dgInt32 loopRowCount = 0;
	const dgInt32 loopCount = m_loopCount + m_selfContactCount;
	for (dgInt32 j = 0; j < loopCount; j++) {
		const dgConstraint* const joint = m_loopingJoints[j];
		loopRowCount += jointInfoArray[joint->m_index].m_pairCount;
	}
	m_loopRowCount = dgInt16 (loopRowCount);
	m_rowCount += m_loopRowCount;
	m_auxiliaryRowCount += m_loopRowCount;

	if (m_auxiliaryRowCount) {
		InitLoopMassMatrix(jointInfoArray);
	}
}

void dgSkeletonContainer::CalculateJointForce(dgJointInfo* const jointInfoArray, const dgBodyInfo* const bodyArray, dgJacobian* const internalForces)
{
	dgForcePair* const force = dgAlloca(dgForcePair, m_nodeCount);
	dgForcePair* const accel = dgAlloca(dgForcePair, m_nodeCount);

	CalculateJointAccel(jointInfoArray, internalForces, accel);
	CalculateForce(force, accel);
	if (m_auxiliaryRowCount) {
		SolveAuxiliary (jointInfoArray, internalForces, accel, force);
	} else {
		UpdateForces(jointInfoArray, internalForces, force);
	}
}

