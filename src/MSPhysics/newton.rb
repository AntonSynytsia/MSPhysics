module MSPhysics
  # @private
  module Newton
    extend AMS::FFI::Library

    dir = File.dirname(__FILE__)
    lib = File.join(dir, 'win32/newton.dll')
    ffi_lib lib
    #ffi_convention :stdcall

    # --------------------------------------------------------------------------
    # Structures
    # --------------------------------------------------------------------------
    class Point3d < AMS::FFI::Struct
      layout :x, :float,
        :y, :float,
        :z, :float
    end

    class Vector3d < AMS::FFI::Struct
      layout :x, :float,
        :y, :float,
        :z, :float
    end

    class WorldConvexCastReturnInfo < AMS::FFI::Struct
      layout :point, Point3d.ptr,
        :normal, Vector3d.ptr,
        :contactID, :long,
        :_body_ptr, :pointer,
        :penetration, :float
    end

    class UserMeshCollisionRayHitDesc < AMS::FFI::Struct
      layout :p0, Point3d.ptr,
        :p1, Point3d.ptr,
        :normalOut, Vector3d.ptr,
        :userIdOut, :long,
        :userData, :pointer
    end

    class UserMeshCollisionCollideDesc < AMS::FFI::Struct
      layout :boxP0, Point3d.ptr,
        :boxP0, Point3d.ptr,
        :boxDistanceTravel, Point3d.ptr,
        :threadNumber, :int,
        :faceCount, :int,
        :vertexStrideInBytes, :int,
        :skinThickness, :float,
        :userData, :pointer,
        :objBody, :pointer,
        :polySoupBody, :pointer,
        :objCollision, :pointer,
        :polySoupCollision, :pointer,
        :vertex, :pointer,
        :faceIndexCount, :pointer,
        :faceVertexIndex, :pointer
    end

    class NewtonJointRecord < AMS::FFI::Struct
      layout :attachmentMatrix0, :pointer,
        :attachmentMatrix1, :pointer,
        :minLinearDof, :pointer,
        :maxLinearDof, :pointer,
        :minAngularDof, :pointer,
        :maxAngularDof, :pointer,
        :attachmentBody0, :pointer,
        :attachmentBody1, :pointer,
        :extraParameters, :pointer,
        :bodiesCollisionOn, :int,
        :descriptionType, :pointer
    end

    # --------------------------------------------------------------------------
    # Callbacks
    # --------------------------------------------------------------------------
    callback :allocMemory, [:int], :pointer
    callback :freeMemory, [:pointer, :int], :void

    callback :worldDestructorCallback, [:pointer], :void
    callback :worldUpdateListenerCallback, [:pointer, :pointer, :float], :void
    callback :worldDestroyListenerCallback, [:pointer, :pointer], :void

    callback :getTicksCountCallback, [], :uint

    callback :serializeCallback, [:pointer, :pointer, :int], :void
    callback :deserializeCallback, [:pointer, :pointer, :int], :void
    callback :onBodySerializationCallback, [:pointer, :serializeCallback, :pointer], :void
    callback :onBodyDeserializationCallback, [:pointer, :deserializeCallback, :pointer], :void
    callback :onUserCollisionSerializationCallback, [:pointer, :serializeCallback, :pointer], :void

    callback :userMeshCollisionDestroyCallback, [:pointer], :void
    callback :userMeshCollisionRayHitCallback, [:pointer], :float
    callback :userMeshCollisionGetCollisionInfo, [:pointer, :pointer], :void
    callback :userMeshCollisionAABBTest, [:pointer, :pointer, :pointer], :int
    callback :userMeshCollisionGetFacesInAABB, [:pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :int, :pointer], :int
    callback :userMeshCollisionCollideCallback, [:pointer, :pointer], :void

    callback :treeCollisionFaceCallback, [:pointer, :pointer, :int, :pointer, :int], :int

    callback :collisionTreeRayCastCallback, [:pointer, :pointer, :float, :pointer, :int, :pointer], :float
    callback :heightFieldRayCastCallback, [:pointer, :pointer, :float, :int, :int, :pointer, :int, :pointer], :float

    callback :collisionCopyConstructionCallback, [:pointer, :pointer, :pointer], :void
    callback :collisionDestructorCallback, [:pointer, :pointer], :void

    callback :treeCollisionCallback, [:pointer, :pointer, :int, :int, :pointer, :int], :void

    callback :bodyDestructor, [:pointer], :void
    callback :applyForceAndTorque, [:pointer, :float, :int], :void
    callback :setTransform, [:pointer, :pointer, :int], :void

    callback :islandUpdate, [:pointer, :pointer, :int], :int

    callback :fractureCompoundCollisionOnEmitChunk, [:pointer, :pointer, :pointer], :void
    callback :fractureCompoundCollisionReconstructMainMeshCallBack, [:pointer, :pointer, :pointer], :void

    callback :worldRayPrefilterCallback, [:pointer, :pointer, :pointer], :uint
    callback :worldRayFilterCallback, [:pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :float], :float

    callback :contactsProcess, [:pointer, :float, :int], :void
    callback :onAABBOverlap, [:pointer, :pointer, :pointer, :int], :int
    callback :onCompoundSubCollisionAABBOverlap, [:pointer, :pointer, :pointer, :pointer, :pointer, :int], :int

    callback :bodyIterator, [:pointer, :pointer], :int
    callback :jointIterator, [:pointer, :pointer], :void
    callback :collisionIterator, [:pointer, :int, :pointer, :int], :void

    callback :ballCallback, [:pointer, :float], :void
    callback :hingeCallback, [:pointer, :pointer], :uint
    callback :sliderCallback, [:pointer, :pointer], :uint
    callback :universalCallback, [:pointer, :pointer], :uint
    callback :corkscrewCallback, [:pointer, :pointer], :uint

    callback :userBilateralCallback, [:pointer, :float, :int], :void
    callback :userBilateralGetInfoCallback, [:pointer, :pointer], :void

    callback :constraintDestructor, [:pointer], :void

    callback :jobTask, [:pointer, :pointer, :int], :void

    callback :reportProgress, [:float, :pointer], :bool


    # --------------------------------------------------------------------------
    # World control functions
    # --------------------------------------------------------------------------
    attach_function :worldGetVersion, :NewtonWorldGetVersion, [], :int
    attach_function :worldFloatSize, :NewtonWorldFloatSize, [], :int

    attach_function :getMemoryUsed, :NewtonGetMemoryUsed, [], :int
    attach_function :setMemorySystem, :NewtonSetMemorySystem, [:allocMemory, :freeMemory], :void

    attach_function :create, :NewtonCreate, [], :pointer
    attach_function :destroy, :NewtonDestroy, [:pointer], :void
    attach_function :destroyAllBodies, :NewtonDestroyAllBodies, [:pointer], :void

    attach_function :alloc, :NewtonAlloc, [:int], :pointer
    attach_function :free, :NewtonFree, [:pointer], :void

    attach_function :enumrateDevices, :NewtonEnumrateDevices, [:pointer], :int
    attach_function :getCurrentDevice, :NewtonGetCurrentDevice, [:pointer], :int
    attach_function :setCurrentDevice, :NewtonSetCurrentDevice, [:pointer, :int], :void
    attach_function :getDeviceString, :NewtonGetDeviceString, [:pointer, :int, :string, :int], :void

    attach_function :invalidateCache, :NewtonInvalidateCache, [:pointer], :void
    attach_function :setSolverModel, :NewtonSetSolverModel, [:pointer, :int], :void

    attach_function :setMultiThreadSolverOnSingleIsland, :NewtonSetMultiThreadSolverOnSingleIsland, [:pointer, :int], :void
    attach_function :getMultiThreadSolverOnSingleIsland, :NewtonGetMultiThreadSolverOnSingleIsland, [:pointer], :int

    attach_function :setPerformanceClock, :NewtonSetPerformanceClock, [:pointer, :getTicksCountCallback], :void
    attach_function :readPerformanceTicks, :NewtonReadPerformanceTicks, [:pointer, :uint], :uint

    attach_function :getBroadphaseAlgorithm, :NewtonGetBroadphaseAlgorithm, [:pointer], :int
    attach_function :selectBroadphaseAlgorithm, :NewtonSelectBroadphaseAlgorithm, [:pointer, :int], :void

    attach_function :update, :NewtonUpdate, [:pointer, :float], :void
    attach_function :updateAsync, :NewtonUpdateAsync, [:pointer, :float], :void
    attach_function :waitForUpdateToFinish, :NewtonWaitForUpdateToFinish, [:pointer], :void

    attach_function :serializeToFile, :NewtonSerializeToFile, [:pointer, :string], :void
    attach_function :serializeBodyArray, :NewtonSerializeBodyArray, [:pointer, :pointer, :int, :onBodySerializationCallback, :serializeCallback, :pointer], :void
    attach_function :deserializeBodyArray, :NewtonDeserializeBodyArray, [:pointer, :onBodyDeserializationCallback, :deserializeCallback, :pointer], :void

    attach_function :readThreadPerformanceTicks, :NewtonReadThreadPerformanceTicks, [:pointer, :uint], :uint

    attach_function :worldCriticalSectionLock, :NewtonWorldCriticalSectionLock, [:pointer, :int], :void
    attach_function :worldCriticalSectionUnlock, :NewtonWorldCriticalSectionUnlock, [:pointer], :void
    attach_function :setThreadsCount, :NewtonSetThreadsCount, [:pointer, :int], :void
    attach_function :getThreadsCount, :NewtonGetThreadsCount, [:pointer], :int
    attach_function :getMaxThreadsCount, :NewtonGetMaxThreadsCount, [:pointer], :int
    attach_function :dispachThreadJob, :NewtonDispachThreadJob, [:pointer, :jobTask, :pointer], :void
    attach_function :syncThreadJobs, :NewtonSyncThreadJobs, [:pointer], :void

    attach_function :atomicAdd, :NewtonAtomicAdd, [:pointer, :int], :int
    attach_function :atomicSwap, :NewtonAtomicSwap, [:pointer, :int], :int
    attach_function :yield, :NewtonYield, [], :void

    attach_function :setFrictionModel, :NewtonSetFrictionModel, [:pointer, :int], :void
    attach_function :setMinimumFrameRate, :NewtonSetMinimumFrameRate, [:pointer, :float], :void
    attach_function :setIslandUpdateEvent, :NewtonSetIslandUpdateEvent, [:pointer, :islandUpdate], :void

    attach_function :worldForEachJointDo, :NewtonWorldForEachJointDo, [:pointer, :jointIterator, :pointer], :void
    attach_function :worldForEachBodyInAABBDo, :NewtonWorldForEachBodyInAABBDo, [:pointer, :pointer, :pointer, :bodyIterator, :pointer], :void

    attach_function :worldSetUserData, :NewtonWorldSetUserData, [:pointer, :pointer], :void
    attach_function :worldGetUserData, :NewtonWorldGetUserData, [:pointer], :pointer

    attach_function :worldGetListenerUserData, :NewtonWorldGetListenerUserData, [:pointer, :pointer], :pointer
    attach_function :worldGetPreListener, :NewtonWorldGetPreListener, [:pointer, :pointer], :pointer
    attach_function :worldAddPreListener, :NewtonWorldAddPreListener, [:pointer, :pointer, :pointer, :worldUpdateListenerCallback, :worldDestroyListenerCallback], :pointer

    attach_function :worldGetPostListener, :NewtonWorldGetPostListener, [:pointer, :pointer], :pointer
    attach_function :worldAddPostListener, :NewtonWorldAddPostListener, [:pointer, :pointer, :pointer, :worldUpdateListenerCallback, :worldDestroyListenerCallback], :pointer

    attach_function :worldSetDestructorCallback, :NewtonWorldSetDestructorCallback, [:pointer, :worldDestructorCallback], :void
    attach_function :worldGetDestructorCallback, :NewtonWorldGetDestructorCallback, [:pointer], :worldDestructorCallback

    attach_function :worldSetCollisionConstructorDestuctorCallback, :NewtonWorldSetCollisionConstructorDestuctorCallback, [:pointer, :collisionCopyConstructionCallback, :collisionDestructorCallback], :void

    attach_function :worldRayCast, :NewtonWorldRayCast, [:pointer, :pointer, :pointer, :worldRayFilterCallback, :pointer, :worldRayPrefilterCallback, :int], :void
    attach_function :worldConvexRayCast, :NewtonWorldConvexRayCast, [:pointer, :pointer, :pointer, :pointer, :worldRayFilterCallback, :pointer, :worldRayPrefilterCallback, :int], :void

    attach_function :worldCollide, :NewtonWorldCollide, [:pointer, :pointer, :pointer, :pointer, :worldRayPrefilterCallback, :pointer, :int, :int], :int
    attach_function :worldConvexCast, :NewtonWorldConvexCast, [:pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :worldRayPrefilterCallback, :pointer, :int, :int], :int

    attach_function :worldGetBodyCount, :NewtonWorldGetBodyCount, [:pointer], :int
    attach_function :worldGetConstraintCount, :NewtonWorldGetConstraintCount, [:pointer], :int


    # --------------------------------------------------------------------------
    # Simulation island functions
    # --------------------------------------------------------------------------
    attach_function :islandGetBody, :NewtonIslandGetBody, [:pointer, :int], :pointer
    attach_function :islandGetBodyAABB, :NewtonIslandGetBodyAABB, [:pointer, :int, :pointer, :pointer], :void


    # --------------------------------------------------------------------------
    # Physics material section functions
    # --------------------------------------------------------------------------
    attach_function :materialCreateGroupID, :NewtonMaterialCreateGroupID, [:pointer], :int
    attach_function :materialGetDefaultGroupID, :NewtonMaterialGetDefaultGroupID, [:pointer], :int
    attach_function :materialDestroyAllGroupID, :NewtonMaterialDestroyAllGroupID, [:pointer], :void

    attach_function :materialGetUserData, :NewtonMaterialGetUserData, [:pointer, :int, :int], :pointer
    attach_function :materialSetSurfaceThickness, :NewtonMaterialSetSurfaceThickness, [:pointer, :int, :int, :float], :void

    attach_function :materialSetCollisionCallback, :NewtonMaterialSetCollisionCallback, [:pointer, :int, :int, :pointer, :onAABBOverlap, :contactsProcess], :void
    attach_function :materialSetCompoundCollisionCallback, :NewtonMaterialSetCompoundCollisionCallback, [:pointer, :int, :int, :onCompoundSubCollisionAABBOverlap], :void

    attach_function :materialSetDefaultSoftness, :NewtonMaterialSetDefaultSoftness, [:pointer, :int, :int, :float], :void
    attach_function :materialSetDefaultElasticity, :NewtonMaterialSetDefaultElasticity, [:pointer, :int, :int, :float], :void
    attach_function :materialSetDefaultCollidable, :NewtonMaterialSetDefaultCollidable, [:pointer, :int, :int, :int], :void
    attach_function :materialSetDefaultFriction, :NewtonMaterialSetDefaultFriction, [:pointer, :int, :int, :float, :float], :void

    attach_function :worldGetFirstMaterial, :NewtonWorldGetFirstMaterial, [:pointer], :pointer
    attach_function :worldGetNextMaterial, :NewtonWorldGetNextMaterial, [:pointer, :pointer], :pointer

    attach_function :worldGetFirstBody, :NewtonWorldGetFirstBody, [:pointer], :pointer
    attach_function :worldGetNextBody, :NewtonWorldGetNextBody, [:pointer, :pointer], :pointer


    # --------------------------------------------------------------------------
    # Physics contact control functions
    # --------------------------------------------------------------------------
    attach_function :materialGetMaterialPairUserData, :NewtonMaterialGetMaterialPairUserData, [:pointer], :void
    attach_function :materialGetContactFaceAttribute, :NewtonMaterialGetContactFaceAttribute, [:pointer], :uint
    attach_function :materialGetBodyCollidingShape, :NewtonMaterialGetBodyCollidingShape, [:pointer, :pointer], :pointer
    attach_function :materialGetContactNormalSpeed, :NewtonMaterialGetContactNormalSpeed, [:pointer], :float
    attach_function :materialGetContactForce, :NewtonMaterialGetContactForce, [:pointer, :pointer, :pointer], :void
    attach_function :materialGetContactPositionAndNormal, :NewtonMaterialGetContactPositionAndNormal, [:pointer, :pointer, :pointer, :pointer], :void
    attach_function :materialGetContactTangentDirections, :NewtonMaterialGetContactTangentDirections, [:pointer, :pointer, :pointer, :pointer], :void
    attach_function :materialGetContactTangentSpeed, :NewtonMaterialGetContactTangentSpeed, [:pointer, :int], :float

    attach_function :materialSetContactSoftness, :NewtonMaterialSetContactSoftness, [:pointer, :float], :void
    attach_function :materialSetContactElasticity, :NewtonMaterialSetContactElasticity, [:pointer, :float], :void
    attach_function :materialSetContactFrictionState, :NewtonMaterialSetContactFrictionState, [:pointer, :int, :int], :void
    attach_function :materialSetContactFrictionCoef, :NewtonMaterialSetContactFrictionCoef, [:pointer, :float, :float, :int], :void

    attach_function :materialSetContactNormalAcceleration, :NewtonMaterialSetContactNormalAcceleration, [:pointer, :float], :void
    attach_function :materialSetContactNormalDirection, :NewtonMaterialSetContactNormalDirection, [:pointer, :pointer], :void

    attach_function :materialSetContactTangentAcceleration, :NewtonMaterialSetContactTangentAcceleration, [:pointer, :float, :int], :void
    attach_function :materialContactRotateTangentDirections, :NewtonMaterialContactRotateTangentDirections, [:pointer, :pointer], :void


    # --------------------------------------------------------------------------
    # Convex collision primitives creation functions
    # --------------------------------------------------------------------------
    attach_function :createNull, :NewtonCreateNull, [:pointer], :pointer
    attach_function :createSphere, :NewtonCreateSphere, [:pointer, :float, :int, :pointer], :pointer
    attach_function :createBox, :NewtonCreateBox, [:pointer, :float, :float, :float, :int, :pointer], :pointer
    attach_function :createCone, :NewtonCreateCone, [:pointer, :float, :float, :int, :pointer], :pointer
    attach_function :createCapsule, :NewtonCreateCapsule, [:pointer, :float, :float, :int, :pointer], :pointer
    attach_function :createCylinder, :NewtonCreateCylinder, [:pointer, :float, :float, :int, :pointer], :pointer
    attach_function :createTaperedCapsule, :NewtonCreateTaperedCapsule, [:pointer, :float, :float, :float, :int, :pointer], :pointer
    attach_function :createTaperedCylinder, :NewtonCreateTaperedCylinder, [:pointer, :float, :float, :float, :int, :pointer], :pointer
    attach_function :createChamferCylinder, :NewtonCreateChamferCylinder, [:pointer, :float, :float, :int, :pointer], :pointer
    attach_function :createConvexHull, :NewtonCreateConvexHull, [:pointer, :int, :pointer, :int, :float, :int, :pointer], :pointer
    attach_function :createConvexHullFromMesh, :NewtonCreateConvexHullFromMesh, [:pointer, :pointer, :float, :int], :pointer

    attach_function :collisionGetMode, :NewtonCollisionGetMode, [:pointer], :int
    attach_function :collisionSetCollisonMode, :NewtonCollisionSetCollisonMode, [:pointer, :int], :void

    attach_function :convexHullGetFaceIndices, :NewtonConvexHullGetFaceIndices, [:pointer, :int, :pointer], :int
    attach_function :convexHullGetVetexData, :NewtonConvexHullGetVetexData, [:pointer, :pointer, :pointer], :int

    attach_function :convexCollisionCalculateVolume, :NewtonConvexCollisionCalculateVolume, [:pointer], :float
    attach_function :convexCollisionCalculateInertialMatrix, :NewtonConvexCollisionCalculateInertialMatrix, [:pointer, :pointer, :pointer], :void
    attach_function :convexCollisionCalculateBuoyancyAcceleration, :NewtonConvexCollisionCalculateBuoyancyAcceleration, [:pointer, :pointer, :pointer, :pointer, :pointer, :float, :float, :pointer, :pointer], :void

    attach_function :collisionDataPointer, :NewtonCollisionDataPointer, [:pointer], :int


    # --------------------------------------------------------------------------
    # Compound collision primitives creation functions
    # --------------------------------------------------------------------------
    attach_function :createCompoundCollision, :NewtonCreateCompoundCollision, [:pointer, :int], :pointer
    attach_function :createCompoundCollisionFromMesh, :NewtonCreateCompoundCollisionFromMesh, [:pointer, :pointer, :float, :int, :int], :pointer

    attach_function :compoundCollisionBeginAddRemove, :NewtonCompoundCollisionBeginAddRemove, [:pointer], :void
    attach_function :compoundCollisionAddSubCollision, :NewtonCompoundCollisionAddSubCollision, [:pointer, :pointer], :pointer
    attach_function :compoundCollisionRemoveSubCollision, :NewtonCompoundCollisionRemoveSubCollision, [:pointer, :pointer], :void
    attach_function :compoundCollisionRemoveSubCollisionByIndex, :NewtonCompoundCollisionRemoveSubCollisionByIndex, [:pointer, :int], :void
    attach_function :compoundCollisionSetSubCollisionMatrix, :NewtonCompoundCollisionSetSubCollisionMatrix, [:pointer, :pointer, :pointer], :void
    attach_function :compoundCollisionEndAddRemove, :NewtonCompoundCollisionEndAddRemove, [:pointer], :void
    attach_function :compoundCollisionGetFirstNode, :NewtonCompoundCollisionGetFirstNode, [:pointer], :pointer
    attach_function :compoundCollisionGetNextNode, :NewtonCompoundCollisionGetNextNode, [:pointer, :pointer], :pointer

    attach_function :compoundCollisionGetNodeByIndex, :NewtonCompoundCollisionGetNodeByIndex, [:pointer, :int], :pointer
    attach_function :compoundCollisionGetNodeIndex, :NewtonCompoundCollisionGetNodeIndex, [:pointer, :pointer], :int
    attach_function :compoundCollisionGetCollisionFromNode, :NewtonCompoundCollisionGetCollisionFromNode, [:pointer, :pointer], :pointer


    # --------------------------------------------------------------------------
    # Scene collision functions
    # --------------------------------------------------------------------------
    attach_function :createSceneCollision, :NewtonCreateSceneCollision, [:pointer, :int], :pointer

    attach_function :sceneCollisionBeginAddRemove, :NewtonSceneCollisionBeginAddRemove, [:pointer], :void
    attach_function :sceneCollisionAddSubCollision, :NewtonSceneCollisionAddSubCollision, [:pointer, :pointer], :pointer
    attach_function :sceneCollisionRemoveSubCollision, :NewtonSceneCollisionRemoveSubCollision, [:pointer, :pointer], :void
    attach_function :sceneCollisionRemoveSubCollisionByIndex, :NewtonSceneCollisionRemoveSubCollisionByIndex, [:pointer, :int], :void
    attach_function :sceneCollisionSetSubCollisionMatrix, :NewtonSceneCollisionSetSubCollisionMatrix, [:pointer, :pointer, :pointer], :void
    attach_function :sceneCollisionEndAddRemove, :NewtonSceneCollisionEndAddRemove, [:pointer], :void

    attach_function :sceneCollisionGetFirstNode, :NewtonSceneCollisionGetFirstNode, [:pointer], :pointer
    attach_function :sceneCollisionGetNextNode, :NewtonSceneCollisionGetNextNode, [:pointer, :pointer], :pointer

    attach_function :sceneCollisionGetNodeByIndex, :NewtonSceneCollisionGetNodeByIndex, [:pointer, :int], :pointer
    attach_function :sceneCollisionGetNodeIndex, :NewtonSceneCollisionGetNodeIndex, [:pointer, :pointer], :int
    attach_function :sceneCollisionGetCollisionFromNode, :NewtonSceneCollisionGetCollisionFromNode, [:pointer, :pointer], :pointer


    # --------------------------------------------------------------------------
    # Fractured compound collision primitives interface
    # --------------------------------------------------------------------------
    attach_function :createFracturedCompoundCollision, :NewtonCreateFracturedCompoundCollision, [:pointer, :pointer, :int, :int, :int, :pointer, :int, :int, :pointer, :fractureCompoundCollisionReconstructMainMeshCallBack, :fractureCompoundCollisionOnEmitChunk], :pointer

    attach_function :fracturedCompoundGetMainMesh, :NewtonFracturedCompoundGetMainMesh, [:pointer], :pointer
    attach_function :fracturedCompoundGetFirstSubMesh, :NewtonFracturedCompoundGetFirstSubMesh, [:pointer], :pointer
    attach_function :fracturedCompoundGetNextSubMesh, :NewtonFracturedCompoundGetNextSubMesh, [:pointer, :pointer], :pointer

    attach_function :fracturedCompoundCollisionGetVertexCount, :NewtonFracturedCompoundCollisionGetVertexCount, [:pointer, :pointer], :int
    attach_function :fracturedCompoundCollisionGetVertexPositions, :NewtonFracturedCompoundCollisionGetVertexPositions, [:pointer, :pointer], :pointer
    attach_function :fracturedCompoundCollisionGetVertexNormals, :NewtonFracturedCompoundCollisionGetVertexNormals, [:pointer, :pointer], :pointer
    attach_function :fracturedCompoundCollisionGetVertexUVs, :NewtonFracturedCompoundCollisionGetVertexUVs, [:pointer, :pointer], :pointer
    attach_function :fracturedCompoundMeshPartGetIndexStream, :NewtonFracturedCompoundMeshPartGetIndexStream, [:pointer, :pointer, :pointer, :pointer], :int

    attach_function :fracturedCompoundMeshPartGetFirstSegment, :NewtonFracturedCompoundMeshPartGetFirstSegment, [:pointer], :pointer
    attach_function :fracturedCompoundMeshPartGetNextSegment, :NewtonFracturedCompoundMeshPartGetNextSegment, [:pointer], :pointer
    attach_function :fracturedCompoundMeshPartGetMaterial, :NewtonFracturedCompoundMeshPartGetMaterial, [:pointer], :int
    attach_function :fracturedCompoundMeshPartGetIndexCount, :NewtonFracturedCompoundMeshPartGetIndexCount, [:pointer], :int


    # --------------------------------------------------------------------------
    # User Static mesh collision interface
    # --------------------------------------------------------------------------
    attach_function :createUserMeshCollision, :NewtonCreateUserMeshCollision, [:pointer, :pointer, :pointer, :pointer, :userMeshCollisionCollideCallback, :userMeshCollisionRayHitCallback, :userMeshCollisionDestroyCallback, :userMeshCollisionGetCollisionInfo, :userMeshCollisionAABBTest, :userMeshCollisionGetFacesInAABB, :onUserCollisionSerializationCallback, :int], :pointer
    attach_function :userMeshCollisionContinueOveralapTest, :NewtonUserMeshCollisionContinueOveralapTest, [:pointer, :pointer, :pointer, :pointer], :int


    # --------------------------------------------------------------------------
    # Collision serialization functions
    # --------------------------------------------------------------------------
    attach_function :createCollisionFromSerialization, :NewtonCreateCollisionFromSerialization, [:pointer, :deserializeCallback, :pointer], :pointer
    attach_function :collisionSerialize, :NewtonCollisionSerialize, [:pointer, :pointer, :serializeCallback, :pointer], :void
    attach_function :collisionGetInfo, :NewtonCollisionGetInfo, [:pointer, :pointer], :void


    # --------------------------------------------------------------------------
    # Static collision shapes functions
    # --------------------------------------------------------------------------
    attach_function :createHeightFieldCollision, :NewtonCreateHeightFieldCollision, [:pointer, :int, :int, :int, :int, :pointer, :pointer, :float, :float, :int], :pointer
    attach_function :heightFieldSetUserRayCastCallback, :NewtonHeightFieldSetUserRayCastCallback, [:pointer, :heightFieldRayCastCallback], :void

    attach_function :createTreeCollision, :NewtonCreateTreeCollision, [:pointer, :int], :pointer
    attach_function :createTreeCollisionFromMesh, :NewtonCreateTreeCollisionFromMesh, [:pointer, :pointer, :int], :pointer

    attach_function :treeCollisionBeginBuild, :NewtonTreeCollisionBeginBuild, [:pointer], :void
    attach_function :treeCollisionAddFace, :NewtonTreeCollisionAddFace, [:pointer, :int, :pointer, :int, :int], :void
    attach_function :treeCollisionEndBuild, :NewtonTreeCollisionEndBuild, [:pointer, :int], :void

    attach_function :treeCollisionGetFaceAtribute, :NewtonTreeCollisionGetFaceAtribute, [:pointer, :pointer, :int], :int
    attach_function :treeCollisionSetFaceAtribute, :NewtonTreeCollisionSetFaceAtribute, [:pointer, :pointer, :int, :int], :void
    attach_function :treeCollisionForEachFace, :NewtonTreeCollisionForEachFace, [:pointer, :treeCollisionFaceCallback, :pointer], :void

    attach_function :treeCollisionGetVertexListTriangleListInAABB, :NewtonTreeCollisionGetVertexListTriangleListInAABB, [:pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :int, :pointer], :void

    attach_function :staticCollisionSetDebugCallback, :NewtonStaticCollisionSetDebugCallback, [:pointer, :treeCollisionCallback], :void


    # --------------------------------------------------------------------------
    # General purpose collision library functions
    # --------------------------------------------------------------------------
    attach_function :collisionCreateInstance, :NewtonCollisionCreateInstance, [:pointer], :pointer
    attach_function :collisionGetType, :NewtonCollisionGetType, [:pointer], :int

    attach_function :collisionSetUserData, :NewtonCollisionSetUserData, [:pointer, :pointer], :void
    attach_function :collisionGetUserData, :NewtonCollisionGetUserData, [:pointer], :pointer

    attach_function :collisionSetUserID, :NewtonCollisionSetUserID, [:pointer, :uint], :void
    attach_function :collisionGetUserID, :NewtonCollisionGetUserID, [:pointer], :uint

    attach_function :collisionSetMatrix, :NewtonCollisionSetMatrix, [:pointer, :pointer], :void
    attach_function :collisionGetMatrix, :NewtonCollisionGetMatrix, [:pointer, :pointer], :void

    attach_function :collisionSetScale, :NewtonCollisionSetScale, [:pointer, :float, :float, :float], :void
    attach_function :collisionGetScale, :NewtonCollisionGetScale, [:pointer, :pointer, :pointer, :pointer], :void

    attach_function :destroyCollision, :NewtonDestroyCollision, [:pointer], :void

    attach_function :collisionGetSkinThickness, :NewtonCollisionGetSkinThickness, [:pointer], :float

    attach_function :collisionIntersectionTest, :NewtonCollisionIntersectionTest, [:pointer, :pointer, :pointer, :pointer, :pointer, :int], :int
    attach_function :collisionPointDistance, :NewtonCollisionPointDistance, [:pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :int], :int
    attach_function :collisionClosestPoint, :NewtonCollisionClosestPoint, [:pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :int], :int
    attach_function :collisionCollide, :NewtonCollisionCollide, [:pointer, :int, :pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :int], :int
    attach_function :collisionCollideContinue, :NewtonCollisionCollideContinue, [:pointer, :int, :float, :pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :pointer, :int], :int
    attach_function :collisionSupportVertex, :NewtonCollisionSupportVertex, [:pointer, :pointer, :pointer], :void
    attach_function :collisionRayCast, :NewtonCollisionRayCast, [:pointer, :pointer, :pointer, :pointer, :pointer], :float
    attach_function :collisionCalculateAABB, :NewtonCollisionCalculateAABB, [:pointer, :pointer, :pointer, :pointer], :void
    attach_function :collisionForEachPolygonDo, :NewtonCollisionForEachPolygonDo, [:pointer, :pointer, :collisionIterator, :pointer], :void


    # --------------------------------------------------------------------------
    # Transforms utility functions
    # --------------------------------------------------------------------------
    attach_function :getEulerAngle, :NewtonGetEulerAngle, [:pointer, :pointer], :void
    attach_function :setEulerAngle, :NewtonSetEulerAngle, [:pointer, :pointer], :void
    attach_function :calculateSpringDamperAcceleration, :NewtonCalculateSpringDamperAcceleration, [:float, :float, :float, :float, :float], :void


    # --------------------------------------------------------------------------
    # Body manipulation functions
    # --------------------------------------------------------------------------
    attach_function :createDynamicBody, :NewtonCreateDynamicBody, [:pointer, :pointer, :pointer], :pointer
    attach_function :createKinematicBody, :NewtonCreateKinematicBody, [:pointer, :pointer, :pointer], :pointer
    attach_function :createDeformableBody, :NewtonCreateDeformableBody, [:pointer, :pointer, :pointer], :pointer

    attach_function :destroyBody, :NewtonDestroyBody, [:pointer], :void

    attach_function :bodyEnableSimulation, :NewtonBodyEnableSimulation, [:pointer], :void
    attach_function :bodyDisableSimulation, :NewtonBodyDisableSimulation, [:pointer], :void

    attach_function :bodyGetType, :NewtonBodyGetType, [:pointer], :int
    attach_function :kinematicBodyGetCollidable, :NewtonKinematicBodyGetCollidable, [:pointer], :int
    attach_function :kinematicBodySetCollidable, :NewtonKinematicBodySetCollidable, [:pointer, :int], :void

    attach_function :bodyAddForce, :NewtonBodyAddForce, [:pointer, :pointer], :void
    attach_function :bodyAddTorque, :NewtonBodyAddTorque, [:pointer, :pointer], :void
    attach_function :bodyCalculateInverseDynamicsForce, :NewtonBodyCalculateInverseDynamicsForce, [:pointer, :float, :pointer, :pointer], :void

    attach_function :bodySetCentreOfMass, :NewtonBodySetCentreOfMass, [:pointer, :pointer], :void
    attach_function :bodySetMassMatrix, :NewtonBodySetMassMatrix, [:pointer, :float, :float, :float, :float], :void

    attach_function :bodySetMassProperties, :NewtonBodySetMassProperties, [:pointer, :float, :pointer], :void

    attach_function :bodySetMatrix, :NewtonBodySetMatrix, [:pointer, :pointer], :void

    attach_function :bodySetMaterialGroupID, :NewtonBodySetMaterialGroupID, [:pointer, :int], :void
    attach_function :bodySetContinuousCollisionMode, :NewtonBodySetContinuousCollisionMode, [:pointer, :uint], :void
    attach_function :bodySetJointRecursiveCollision, :NewtonBodySetJointRecursiveCollision, [:pointer, :uint], :void
    attach_function :bodySetOmega, :NewtonBodySetOmega, [:pointer, :pointer], :void
    attach_function :bodySetVelocity, :NewtonBodySetVelocity, [:pointer, :pointer], :void
    attach_function :bodySetForce, :NewtonBodySetForce, [:pointer, :pointer], :void
    attach_function :bodySetTorque, :NewtonBodySetTorque, [:pointer, :pointer], :void

    attach_function :bodySetLinearDamping, :NewtonBodySetLinearDamping, [:pointer, :float], :void
    attach_function :bodySetAngularDamping, :NewtonBodySetAngularDamping, [:pointer, :pointer], :void
    attach_function :bodySetCollision, :NewtonBodySetCollision, [:pointer, :pointer], :void
    attach_function :bodySetCollisionScale, :NewtonBodySetCollisionScale, [:pointer, :float, :float, :float], :void

    attach_function :bodyGetSleepState, :NewtonBodyGetSleepState, [:pointer], :int
    attach_function :bodySetSleepState, :NewtonBodySetSleepState, [:pointer, :int], :void

    attach_function :bodyGetAutoSleep, :NewtonBodyGetAutoSleep, [:pointer], :int
    attach_function :bodySetAutoSleep, :NewtonBodySetAutoSleep, [:pointer, :int], :void

    attach_function :bodyGetFreezeState, :NewtonBodyGetFreezeState, [:pointer], :int
    attach_function :bodySetFreezeState, :NewtonBodySetFreezeState, [:pointer, :int], :void

    attach_function :bodySetDestructorCallback, :NewtonBodySetDestructorCallback, [:pointer, :bodyDestructor], :void
    attach_function :bodyGetDestructorCallback, :NewtonBodyGetDestructorCallback, [:pointer], :bodyDestructor

    attach_function :bodySetTransformCallback, :NewtonBodySetTransformCallback, [:pointer, :setTransform], :void
    attach_function :bodyGetTransformCallback, :NewtonBodyGetTransformCallback, [:pointer], :setTransform

    attach_function :bodySetForceAndTorqueCallback, :NewtonBodySetForceAndTorqueCallback, [:pointer, :applyForceAndTorque], :void
    attach_function :bodyGetForceAndTorqueCallback, :NewtonBodyGetForceAndTorqueCallback, [:pointer], :applyForceAndTorque

    attach_function :bodyGetID, :NewtonBodyGetID, [:pointer], :int

    attach_function :bodySetUserData, :NewtonBodySetUserData, [:pointer, :pointer], :void
    attach_function :bodyGetUserData, :NewtonBodyGetUserData, [:pointer], :pointer

    attach_function :bodyGetWorld, :NewtonBodyGetWorld, [:pointer], :pointer
    attach_function :bodyGetCollision, :NewtonBodyGetCollision, [:pointer], :pointer
    attach_function :bodyGetMaterialGroupID, :NewtonBodyGetMaterialGroupID, [:pointer], :int

    attach_function :bodyGetContinuousCollisionMode, :NewtonBodyGetContinuousCollisionMode, [:pointer], :int
    attach_function :bodyGetJointRecursiveCollision, :NewtonBodyGetJointRecursiveCollision, [:pointer], :int

    attach_function :bodyGetMatrix, :NewtonBodyGetMatrix, [:pointer, :pointer], :void
    attach_function :bodyGetRotation, :NewtonBodyGetRotation, [:pointer, :pointer], :void
    attach_function :bodyGetMassMatrix, :NewtonBodyGetMassMatrix, [:pointer, :pointer, :pointer, :pointer, :pointer], :void
    attach_function :bodyGetInvMass, :NewtonBodyGetInvMass, [:pointer, :pointer, :pointer, :pointer, :pointer], :void
    attach_function :bodyGetInertiaMatrix, :NewtonBodyGetInertiaMatrix, [:pointer, :pointer], :void
    attach_function :bodyGetInvInertiaMatrix, :NewtonBodyGetInvInertiaMatrix, [:pointer, :pointer], :void
    attach_function :bodyGetOmega, :NewtonBodyGetOmega, [:pointer, :pointer], :void
    attach_function :bodyGetVelocity, :NewtonBodyGetVelocity, [:pointer, :pointer], :void
    attach_function :bodyGetForce, :NewtonBodyGetForce, [:pointer, :pointer], :void
    attach_function :bodyGetTorque, :NewtonBodyGetTorque, [:pointer, :pointer], :void
    attach_function :bodyGetForceAcc, :NewtonBodyGetForceAcc, [:pointer, :pointer], :void
    attach_function :bodyGetTorqueAcc, :NewtonBodyGetTorqueAcc, [:pointer, :pointer], :void
    attach_function :bodyGetCentreOfMass, :NewtonBodyGetCentreOfMass, [:pointer, :pointer], :void

    attach_function :bodyGetPointVelocity, :NewtonBodyGetPointVelocity, [:pointer, :pointer, :pointer], :void
    attach_function :bodyAddImpulse, :NewtonBodyAddImpulse, [:pointer, :pointer, :pointer], :void
    attach_function :bodyApplyImpulseArray, :NewtonBodyApplyImpulseArray, [:pointer, :int, :int, :pointer, :pointer], :void

    attach_function :bodyApplyImpulsePair, :NewtonBodyApplyImpulsePair, [:pointer, :pointer, :pointer], :void

    attach_function :bodyIntegrateVelocity, :NewtonBodyIntegrateVelocity, [:pointer, :float], :void

    attach_function :bodyGetLinearDamping, :NewtonBodyGetLinearDamping, [:pointer], :float
    attach_function :bodyGetAngularDamping, :NewtonBodyGetAngularDamping, [:pointer, :pointer], :void
    attach_function :bodyGetAABB, :NewtonBodyGetAABB, [:pointer, :pointer, :pointer], :void

    attach_function :bodyGetFirstJoint, :NewtonBodyGetFirstJoint, [:pointer], :pointer
    attach_function :bodyGetNextJoint, :NewtonBodyGetNextJoint, [:pointer, :pointer], :pointer
    attach_function :bodyGetFirstContactJoint, :NewtonBodyGetFirstContactJoint, [:pointer], :pointer
    attach_function :bodyGetNextContactJoint, :NewtonBodyGetNextContactJoint, [:pointer, :pointer], :pointer


    # --------------------------------------------------------------------------
    # Contact joints interface
    # --------------------------------------------------------------------------
    attach_function :contactJointGetFirstContact, :NewtonContactJointGetFirstContact, [:pointer], :pointer
    attach_function :contactJointGetNextContact, :NewtonContactJointGetNextContact, [:pointer, :pointer], :pointer

    attach_function :contactJointGetContactCount, :NewtonContactJointGetContactCount, [:pointer], :int
    attach_function :contactJointRemoveContact, :NewtonContactJointRemoveContact, [:pointer, :pointer], :void

    attach_function :contactGetMaterial, :NewtonContactGetMaterial, [:pointer], :pointer

    attach_function :contactGetCollision0, :NewtonContactGetCollision0, [:pointer], :pointer
    attach_function :contactGetCollision1, :NewtonContactGetCollision1, [:pointer], :pointer

    attach_function :contactGetCollisionID0, :NewtonContactGetCollisionID0, [:pointer], :pointer
    attach_function :contactGetCollisionID1, :NewtonContactGetCollisionID1, [:pointer], :pointer


    # --------------------------------------------------------------------------
    # Common joint functions
    # --------------------------------------------------------------------------
    attach_function :jointGetUserData, :NewtonJointGetUserData, [:pointer], :pointer
    attach_function :jointSetUserData, :NewtonJointSetUserData, [:pointer, :pointer], :void

    attach_function :jointGetBody0, :NewtonJointGetBody0, [:pointer], :pointer
    attach_function :jointGetBody1, :NewtonJointGetBody1, [:pointer], :pointer

    attach_function :jointGetInfo, :NewtonJointGetInfo, [:pointer, :pointer], :void
    attach_function :jointGetCollisionState, :NewtonJointGetCollisionState, [:pointer], :int
    attach_function :jointSetCollisionState, :NewtonJointSetCollisionState, [:pointer, :int], :void

    attach_function :jointGetStiffness, :NewtonJointGetStiffness, [:pointer], :float
    attach_function :jointSetStiffness, :NewtonJointSetStiffness, [:pointer, :float], :void

    attach_function :destroyJoint, :NewtonDestroyJoint, [:pointer, :pointer], :void
    attach_function :jointSetDestructor, :NewtonJointSetDestructor, [:pointer, :constraintDestructor], :void


    # --------------------------------------------------------------------------
    # Particle system interface
    # --------------------------------------------------------------------------
    attach_function :createClothPatch, :NewtonCreateClothPatch, [:pointer, :pointer, :int, :pointer, :pointer], :pointer
    attach_function :createDeformableMesh, :NewtonCreateDeformableMesh, [:pointer, :pointer, :int], :pointer

    attach_function :deformableMeshCreateClusters, :NewtonDeformableMeshCreateClusters, [:pointer, :int, :float], :void

    attach_function :deformableMeshGetParticleCount, :NewtonDeformableMeshGetParticleCount, [:pointer], :int
    attach_function :deformableMeshGetParticlePosition, :NewtonDeformableMeshGetParticlePosition, [:pointer, :int, :pointer], :void

    attach_function :deformableMeshBeginConfiguration, :NewtonDeformableMeshBeginConfiguration, [:pointer], :void
    attach_function :deformableMeshUnconstraintParticle, :NewtonDeformableMeshUnconstraintParticle, [:pointer, :int], :void
    attach_function :deformableMeshConstraintParticle, :NewtonDeformableMeshConstraintParticle, [:pointer, :int, :pointer, :pointer], :void
    attach_function :deformableMeshEndConfiguration, :NewtonDeformableMeshEndConfiguration, [:pointer], :void

    attach_function :deformableMeshSetSkinThickness, :NewtonDeformableMeshSetSkinThickness, [:pointer, :float], :void

    attach_function :deformableMeshUpdateRenderNormals, :NewtonDeformableMeshUpdateRenderNormals, [:pointer], :void
    attach_function :deformableMeshGetVertexCount, :NewtonDeformableMeshGetVertexCount, [:pointer], :int
    attach_function :deformableMeshGetVertexStreams, :NewtonDeformableMeshGetVertexStreams, [:pointer, :int, :pointer, :int, :pointer, :int, :pointer, :int, :pointer], :void
    attach_function :deformableMeshGetFirstSegment, :NewtonDeformableMeshGetFirstSegment, [:pointer], :pointer
    attach_function :deformableMeshGetNextSegment, :NewtonDeformableMeshGetNextSegment, [:pointer, :pointer], :pointer

    attach_function :deformableMeshSegmentGetMaterialID, :NewtonDeformableMeshSegmentGetMaterialID, [:pointer, :pointer], :int
    attach_function :deformableMeshSegmentGetIndexCount, :NewtonDeformableMeshSegmentGetIndexCount, [:pointer, :pointer], :int
    attach_function :deformableMeshSegmentGetIndexList, :NewtonDeformableMeshSegmentGetIndexList, [:pointer, :pointer], :pointer


    # --------------------------------------------------------------------------
    # Ball and Socket joint functions
    # --------------------------------------------------------------------------
    attach_function :constraintCreateBall, :NewtonConstraintCreateBall, [:pointer, :pointer, :pointer, :pointer], :pointer

    attach_function :ballSetUserCallback, :NewtonBallSetUserCallback, [:pointer, :ballCallback], :void
    attach_function :ballGetJointAngle, :NewtonBallGetJointAngle, [:pointer, :pointer], :void
    attach_function :ballGetJointOmega, :NewtonBallGetJointOmega, [:pointer, :pointer], :void
    attach_function :ballGetJointForce, :NewtonBallGetJointForce, [:pointer, :pointer], :void
    attach_function :ballSetConeLimits, :NewtonBallSetConeLimits, [:pointer, :pointer, :float, :float], :void


    # --------------------------------------------------------------------------
    # Hinge joint functions
    # --------------------------------------------------------------------------
    attach_function :constraintCreateHinge, :NewtonConstraintCreateHinge, [:pointer, :pointer, :pointer, :pointer, :pointer], :pointer

    attach_function :hingeSetUserCallback, :NewtonHingeSetUserCallback, [:pointer, :hingeCallback], :void
    attach_function :hingeGetJointAngle, :NewtonHingeGetJointAngle, [:pointer], :float
    attach_function :hingeGetJointOmega, :NewtonHingeGetJointOmega, [:pointer], :float
    attach_function :hingeGetJointForce, :NewtonHingeGetJointForce, [:pointer, :pointer], :void
    attach_function :hingeCalculateStopAlpha, :NewtonHingeCalculateStopAlpha, [:pointer, :pointer, :float], :float


    # --------------------------------------------------------------------------
    # Slider joint functions
    # --------------------------------------------------------------------------
    attach_function :constraintCreateSlider, :NewtonConstraintCreateSlider, [:pointer, :pointer, :pointer, :pointer, :pointer], :pointer

    attach_function :sliderSetUserCallback, :NewtonSliderSetUserCallback, [:pointer, :sliderCallback], :void
    attach_function :sliderGetJointPosit, :NewtonSliderGetJointPosit, [:pointer], :float
    attach_function :sliderGetJointVeloc, :NewtonSliderGetJointVeloc, [:pointer], :float
    attach_function :sliderGetJointForce, :NewtonSliderGetJointForce, [:pointer, :pointer], :void
    attach_function :sliderCalculateStopAccel, :NewtonSliderCalculateStopAccel, [:pointer, :pointer, :float], :float


    # --------------------------------------------------------------------------
    # Corkscrew joint functions
    # --------------------------------------------------------------------------
    attach_function :constraintCreateCorkscrew, :NewtonConstraintCreateCorkscrew, [:pointer, :pointer, :pointer, :pointer, :pointer], :pointer

    attach_function :corkscrewSetUserCallback, :NewtonCorkscrewSetUserCallback, [:pointer, :corkscrewCallback], :void
    attach_function :corkscrewGetJointPosit, :NewtonCorkscrewGetJointPosit, [:pointer], :float
    attach_function :corkscrewGetJointAngle, :NewtonCorkscrewGetJointAngle, [:pointer], :float
    attach_function :corkscrewGetJointOmega, :NewtonCorkscrewGetJointOmega, [:pointer], :float
    attach_function :corkscrewGetJointForce, :NewtonCorkscrewGetJointForce, [:pointer, :pointer], :void
    attach_function :corkscrewCalculateStopAlpha, :NewtonCorkscrewCalculateStopAlpha, [:pointer, :pointer, :float], :float
    attach_function :corkscrewCalculateStopAccel, :NewtonCorkscrewCalculateStopAccel, [:pointer, :pointer, :float], :float


    # --------------------------------------------------------------------------
    # Universal joint functions
    # --------------------------------------------------------------------------
    attach_function :constraintCreateCorkscrew, :NewtonConstraintCreateCorkscrew, [:pointer, :pointer, :pointer, :pointer, :pointer, :pointer], :pointer

    attach_function :universalSetUserCallback, :NewtonUniversalSetUserCallback, [:pointer, :universalCallback], :void
    attach_function :universalGetJointAngle0, :NewtonUniversalGetJointAngle0, [:pointer], :float
    attach_function :universalGetJointAngle1, :NewtonUniversalGetJointAngle1, [:pointer], :float
    attach_function :universalGetJointOmega0, :NewtonUniversalGetJointOmega0, [:pointer], :float
    attach_function :universalGetJointOmega1, :NewtonUniversalGetJointOmega1, [:pointer], :float
    attach_function :universalGetJointForce, :NewtonUniversalGetJointForce, [:pointer, :pointer], :void
    attach_function :universalCalculateStopAlpha0, :NewtonUniversalCalculateStopAlpha0, [:pointer, :pointer, :float], :float
    attach_function :universalCalculateStopAlpha1, :NewtonUniversalCalculateStopAlpha1, [:pointer, :pointer, :float], :float


    # --------------------------------------------------------------------------
    # Up vector joint functions
    # --------------------------------------------------------------------------
    attach_function :constraintCreateUpVector, :NewtonConstraintCreateUpVector, [:pointer, :pointer, :pointer], :pointer

    attach_function :upVectorGetPin, :NewtonUpVectorGetPin, [:pointer, :pointer], :void
    attach_function :upVectorSetPin, :NewtonUpVectorSetPin, [:pointer, :pointer], :void


    # --------------------------------------------------------------------------
    # User defined bilateral joint
    # --------------------------------------------------------------------------
    attach_function :constraintCreateUserJoint, :NewtonConstraintCreateUserJoint, [:pointer, :int, :userBilateralCallback, :userBilateralGetInfoCallback, :pointer, :pointer], :pointer

    attach_function :userJointSetFeedbackCollectorCallback, :NewtonUserJointSetFeedbackCollectorCallback, [:pointer, :userBilateralCallback], :void
    attach_function :userJointAddLinearRow, :NewtonUserJointAddLinearRow, [:pointer, :pointer, :pointer, :pointer], :void
    attach_function :userJointAddAngularRow, :NewtonUserJointAddAngularRow, [:pointer, :float, :pointer], :void
    attach_function :userJointAddGeneralRow, :NewtonUserJointAddGeneralRow, [:pointer, :pointer, :pointer], :void
    attach_function :userJointSetRowMinimumFriction, :NewtonUserJointSetRowMinimumFriction, [:pointer, :float], :void
    attach_function :userJointSetRowMaximumFriction, :NewtonUserJointSetRowMaximumFriction, [:pointer, :float], :void
    attach_function :userJointSetRowAcceleration, :NewtonUserJointSetRowAcceleration, [:pointer, :float], :void
    attach_function :userJointSetRowSpringDamperAcceleration, :NewtonUserJointSetRowSpringDamperAcceleration, [:pointer, :float, :float], :void
    attach_function :userJointSetRowStiffness, :NewtonUserJointSetRowStiffness, [:pointer, :float], :void
    attach_function :userJointGetRowForce, :NewtonUserJointGetRowForce, [:pointer, :int], :float

    attach_function :userJointSetSolver, :NewtonUserJointSetSolver, [:pointer, :int, :int], :void


    # --------------------------------------------------------------------------
    # Mesh joint functions
    # --------------------------------------------------------------------------
    attach_function :meshCreate, :NewtonMeshCreate, [:pointer], :pointer
    attach_function :meshCreateFromMesh, :NewtonMeshCreateFromMesh, [:pointer], :pointer
    attach_function :meshCreateFromCollision, :NewtonMeshCreateFromCollision, [:pointer], :pointer
    attach_function :meshCreateConvexHull, :NewtonMeshCreateConvexHull, [:pointer, :int, :pointer, :int, :float], :pointer
    attach_function :meshCreateDelaunayTetrahedralization, :NewtonMeshCreateDelaunayTetrahedralization, [:pointer, :int, :pointer, :int, :int, :pointer], :pointer
    attach_function :meshCreateVoronoiConvexDecomposition, :NewtonMeshCreateVoronoiConvexDecomposition, [:pointer, :int, :pointer, :int, :int, :pointer], :pointer
    attach_function :meshDestroy, :NewtonMeshDestroy, [:pointer], :void

    attach_function :meshApplyTransform, :NewtonMeshApplyTransform, [:pointer, :pointer], :void
    attach_function :meshCalculateOOBB, :NewtonMeshCalculateOOBB, [:pointer, :pointer, :pointer, :pointer, :pointer], :void

    attach_function :meshCalculateVertexNormals, :NewtonMeshCalculateVertexNormals, [:pointer, :float], :void
    attach_function :meshApplySphericalMapping, :NewtonMeshApplySphericalMapping, [:pointer, :int], :void
    attach_function :meshApplyCylindricalMapping, :NewtonMeshApplyCylindricalMapping, [:pointer, :int, :int], :void
    attach_function :meshApplyBoxMapping, :NewtonMeshApplyBoxMapping, [:pointer, :int, :int, :int], :void
    attach_function :meshApplyAngleBasedMapping, :NewtonMeshApplyAngleBasedMapping, [:pointer, :int, :reportProgress, :pointer], :void

    attach_function :meshIsOpenMesh, :NewtonMeshIsOpenMesh, [:pointer], :int
    attach_function :meshFixTJoints, :NewtonMeshFixTJoints, [:pointer], :void

    attach_function :meshPolygonize, :NewtonMeshPolygonize, [:pointer], :void
    attach_function :meshTriangulate, :NewtonMeshTriangulate, [:pointer], :void
    attach_function :meshUnion, :NewtonMeshUnion, [:pointer, :pointer, :pointer], :pointer
    attach_function :meshDifference, :NewtonMeshDifference, [:pointer, :pointer, :pointer], :pointer
    attach_function :meshIntersection, :NewtonMeshIntersection, [:pointer, :pointer, :pointer], :pointer
    attach_function :meshClip, :NewtonMeshClip, [:pointer, :pointer, :pointer, :pointer, :pointer], :void

    attach_function :meshConvexMeshIntersection, :NewtonMeshConvexMeshIntersection, [:pointer, :pointer], :pointer

    attach_function :meshSimplify, :NewtonMeshSimplify, [:pointer, :int, :reportProgress, :pointer], :pointer
    attach_function :meshApproximateConvexDecomposition, :NewtonMeshApproximateConvexDecomposition, [:pointer, :float, :float, :int, :int, :reportProgress, :pointer], :pointer

    attach_function :removeUnusedVertices, :NewtonRemoveUnusedVertices, [:pointer, :pointer], :void

    attach_function :meshBeginFace, :NewtonMeshBeginFace, [:pointer], :void
    attach_function :meshAddFace, :NewtonMeshAddFace, [:pointer, :int, :pointer, :int, :int], :void
    attach_function :meshEndFace, :NewtonMeshEndFace, [:pointer], :void

    attach_function :meshBuildFromVertexListIndexList, :NewtonMeshBuildFromVertexListIndexList, [:pointer, :int, :pointer, :pointer, :pointer, :int, :pointer, :pointer, :int, :pointer, :pointer, :int, :pointer, :pointer, :int, :pointer], :void

    attach_function :meshGetVertexStreams, :NewtonMeshGetVertexStreams, [:pointer, :int, :pointer, :int, :pointer, :int, :pointer, :int, :pointer], :void

    attach_function :meshGetIndirectVertexStreams, :NewtonMeshGetIndirectVertexStreams, [:pointer, :int, :pointer, :pointer, :pointer, :int, :pointer, :pointer, :pointer, :int, :pointer, :pointer, :pointer, :int, :pointer, :pointer, :pointer], :void

    attach_function :meshBeginHandle, :NewtonMeshBeginHandle, [:pointer], :pointer
    attach_function :meshEndHandle, :NewtonMeshEndHandle, [:pointer, :pointer], :void
    attach_function :meshFirstMaterial, :NewtonMeshFirstMaterial, [:pointer, :pointer], :int
    attach_function :meshNextMaterial, :NewtonMeshNextMaterial, [:pointer, :pointer], :int
    attach_function :meshMaterialGetMaterial, :NewtonMeshMaterialGetMaterial, [:pointer, :pointer, :int], :int
    attach_function :meshMaterialGetIndexCount, :NewtonMeshMaterialGetIndexCount, [:pointer, :pointer, :int], :int
    attach_function :meshMaterialGetIndexStream, :NewtonMeshMaterialGetIndexStream, [:pointer, :pointer, :int, :pointer], :void
    attach_function :meshMaterialGetIndexStreamShort, :NewtonMeshMaterialGetIndexStreamShort, [:pointer, :pointer, :int, :pointer], :void

    attach_function :meshCreateFirstSingleSegment, :NewtonMeshCreateFirstSingleSegment, [:pointer], :pointer
    attach_function :meshCreateNextSingleSegment, :NewtonMeshCreateNextSingleSegment, [:pointer, :pointer], :pointer

    attach_function :meshCreateFirstLayer, :NewtonMeshCreateFirstLayer, [:pointer], :pointer
    attach_function :meshCreateNextLayer, :NewtonMeshCreateNextLayer, [:pointer, :pointer], :pointer

    attach_function :meshGetTotalFaceCount, :NewtonMeshGetTotalFaceCount, [:pointer], :int
    attach_function :meshGetTotalIndexCount, :NewtonMeshGetTotalIndexCount, [:pointer], :int
    attach_function :meshGetFaces, :NewtonMeshGetFaces, [:pointer, :pointer, :pointer, :pointer], :void

    attach_function :meshGetPointCount, :NewtonMeshGetPointCount, [:pointer], :int
    attach_function :meshGetPointStrideInByte, :NewtonMeshGetPointStrideInByte, [:pointer], :int
    attach_function :meshGetPointArray, :NewtonMeshGetPointArray, [:pointer], :pointer
    attach_function :meshGetNormalArray, :NewtonMeshGetNormalArray, [:pointer], :pointer
    attach_function :meshGetUV0Array, :NewtonMeshGetUV0Array, [:pointer], :pointer
    attach_function :meshGetUV1Array, :NewtonMeshGetUV1Array, [:pointer], :pointer

    attach_function :meshGetVertexCount, :NewtonMeshGetVertexCount, [:pointer], :int
    attach_function :meshGetVertexStrideInByte, :NewtonMeshGetVertexStrideInByte, [:pointer], :int
    attach_function :meshGetVertexArray, :NewtonMeshGetVertexArray, [:pointer], :pointer

    attach_function :meshGetFirstVertex, :NewtonMeshGetFirstVertex, [:pointer], :pointer
    attach_function :meshGetNextVertex, :NewtonMeshGetNextVertex, [:pointer, :pointer], :pointer
    attach_function :meshGetVertexIndex, :NewtonMeshGetVertexIndex, [:pointer, :pointer], :int

    attach_function :meshGetFirstPoint, :NewtonMeshGetFirstPoint, [:pointer], :pointer
    attach_function :meshGetNextPoint, :NewtonMeshGetNextPoint, [:pointer, :pointer], :pointer
    attach_function :meshGetPointIndex, :NewtonMeshGetPointIndex, [:pointer, :pointer], :int
    attach_function :meshGetVertexIndexFromPoint, :NewtonMeshGetVertexIndexFromPoint, [:pointer, :pointer], :int

    attach_function :meshGetFirstEdge, :NewtonMeshGetFirstEdge, [:pointer], :pointer
    attach_function :meshGetNextEdge, :NewtonMeshGetNextEdge, [:pointer, :pointer], :pointer
    attach_function :meshGetEdgeIndices, :NewtonMeshGetEdgeIndices, [:pointer, :pointer, :pointer, :pointer], :void

    attach_function :meshGetFirstFace, :NewtonMeshGetFirstFace, [:pointer], :pointer
    attach_function :meshGetNextFace, :NewtonMeshGetNextFace, [:pointer, :pointer], :pointer
    attach_function :meshIsFaceOpen, :NewtonMeshIsFaceOpen, [:pointer, :pointer], :int
    attach_function :meshGetFaceMaterial, :NewtonMeshGetFaceMaterial, [:pointer, :pointer], :int
    attach_function :meshGetFaceIndexCount, :NewtonMeshGetFaceIndexCount, [:pointer, :pointer], :int
    attach_function :meshGetFaceIndices, :NewtonMeshGetFaceIndices, [:pointer, :pointer, :pointer], :void
    attach_function :meshGetFacePointIndices, :NewtonMeshGetFacePointIndices, [:pointer, :pointer, :pointer], :void
    attach_function :meshCalculateFaceNormal, :NewtonMeshCalculateFaceNormal, [:pointer, :pointer, :pointer], :void

    attach_function :meshSetFaceMaterial, :NewtonMeshSetFaceMaterial, [:pointer, :pointer, :int], :void

  end # module Newton
end # module MSPhysics
