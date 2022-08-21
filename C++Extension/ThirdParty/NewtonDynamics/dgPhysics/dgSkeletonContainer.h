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

#ifndef _DG_SKELETON_CONTAINER_H__
#define _DG_SKELETON_CONTAINER_H__

#include "dgConstraint.h"
#include "dgContact.h"
#include "dgBilateralConstraint.h"

class dgDynamicBody;

class dgSkeletonContainer
{
	public:
	class dgNode;
	class dgNodePair;
	class dgForcePair;
	class dgMatriData;
	class dgBodyJointMatrixDataPair;

	DG_CLASS_ALLOCATOR(allocator)
	dgSkeletonContainer(dgWorld* const world, dgDynamicBody* const rootBody);
	virtual ~dgSkeletonContainer();

	dgWorld* GetWorld() const; 
	dgInt32 GetJointCount () const {return m_nodeCount - 1;}
	dgNode* AddChild (dgBilateralConstraint* const joint, dgNode* const parent);
	void RemoveLoopJoint(dgBilateralConstraint* const joint);  
	void Finalize (dgInt32 loopJoints, dgBilateralConstraint** const loopJointArray);
	
	dgNode* GetRoot () const;
	dgDynamicBody* GetBody(dgNode* const node) const;
	dgBilateralConstraint* GetJoint(dgNode* const node) const;
	dgNode* GetParent (dgNode* const node) const;
	dgNode* GetFirstChild (dgNode* const parent) const;
	dgNode* GetNextSiblingChild (dgNode* const sibling) const;

	void ClearSelfCollision();
	void AddSelfCollisionJoint(dgConstraint* const joint);

	dgInt32 GetLru() const { return m_lru; }
	void SetLru(dgInt32 lru) { m_lru = lru; }

	virtual void CalculateJointForce (dgJointInfo* const jointInfoArray, const dgBodyInfo* const bodyArray, dgJacobian* const internalForces);
	virtual void InitMassMatrix (const dgJointInfo* const jointInfoArray, const dgLeftHandSide* const matrixRow, dgRightHandSide* const rightHandSide, bool m_consideredCloseLoop = true);
	
	private:
	bool SanityCheck(const dgForcePair* const force, const dgForcePair* const accel) const;
	void ConditionMassMatrix() const;
	void RebuildMassMatrix(const dgFloat32* const diagDamp) const;
	void CalculateLoopMassMatrixCoefficients(dgFloat32* const diagDamp);
	
	DG_INLINE void CalculateForce(dgForcePair* const force, const dgForcePair* const accel) const;
	DG_INLINE void SolveBackward(dgForcePair* const force) const;
	DG_INLINE void SolveForward(dgForcePair* const force, const dgForcePair* const accel, dgInt32 startNode = 0) const;
	DG_INLINE void UpdateForces(dgJointInfo* const jointInfoArray, dgJacobian* const internalForces, const dgForcePair* const force) const;
	DG_INLINE void CalculateJointAccel (dgJointInfo* const jointInfoArray, const dgJacobian* const internalForces, dgForcePair* const accel) const;

	dgNode* FindNode(dgDynamicBody* const node) const;
	void SortGraph(dgNode* const root, dgInt32& index);
		
	void InitLoopMassMatrix (const dgJointInfo* const jointInfoArray);
	dgInt8* CalculateBufferSizeInBytes (const dgJointInfo* const jointInfoArray);
	void SolveAuxiliary (const dgJointInfo* const jointInfoArray, dgJacobian* const internalForces, const dgForcePair* const accel, dgForcePair* const force) const;
	void SolveLcp(dgInt32 stride, dgInt32 size, const dgFloat32* const matrix, const dgFloat32* const x0, dgFloat32* const x, const dgFloat32* const b, const dgFloat32* const low, const dgFloat32* const high, const dgInt32* const normalIndex) const;
	void SolveBlockLcp(dgInt32 size, dgInt32 blockSize, const dgFloat32* const x0, dgFloat32* const x, dgFloat32* const b, const dgFloat32* const low, const dgFloat32* const high, const dgInt32* const normalIndex) const;
	void FactorizeMatrix(dgInt32 size, dgInt32 stride, dgFloat32* const matrix, dgFloat32* const diagDamp) const;

	dgWorld* m_world;
	dgNode* m_skeleton;
	dgNode** m_nodesOrder;
	dgNodePair* m_pairs;
	dgFloat32* m_deltaForce;
	dgFloat32* m_massMatrix11;
	dgFloat32* m_massMatrix10;

	dgRightHandSide* m_rightHandSide;
	const dgLeftHandSide* m_leftHandSide;
	dgInt32* m_frictionIndex;
	dgInt32* m_matrixRowsIndex;
	dgSkeletonList::dgListNode* m_listNode;
	dgArray<dgConstraint*> m_loopingJoints;
	dgArray<dgInt8> m_auxiliaryMemoryBuffer;
	dgInt32 m_lru;
	dgInt32 m_blockSize;
	dgInt16 m_nodeCount;
	dgInt16 m_loopCount;
	dgInt16 m_dynamicsLoopCount;
	dgInt16 m_rowCount;
	dgInt16 m_loopRowCount;
	dgInt16 m_auxiliaryRowCount;
	dgInt16 m_consideredCloseLoop;

	friend class dgWorld;
	friend class dgParallelBodySolver;
	friend class dgWorldDynamicUpdate;
};

#endif

