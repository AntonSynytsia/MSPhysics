/*

MSPhysics v1.1.0, 20 January 2018
Copyright Anton Synytsia (c) 2014-2018 (anton.synytsia@gmail.com)

Implementation:
  - Newton Dynamics Physics SDK 3.14, git e080b91
  - SDL 2.0.5
  - SDL_mixer 2.0.1

Semantics (unless otherwise stated):
  - length			: m
  - area			: m^2
  - volume			: m^3
  - density			: kg/m^3
  - mass			: kg
  - position		: in
  - velocity		: m/s
  - acceleration	: m/s/s
  - force			: Newtons (kg * m/s/s)
  - torque			: kg * m * m/s/s
  - angle			: rad
  - omega			: rad/s
  - lambda			: rad/s/s

When a body is created, we compute its volume and convert it to meters
We then comute the mass based on the given density.
The resulting mass would be the true (unscaled) mass of the body.

When a force is applied to a body, we know that force is mass * acceleration.
The mass is unscaled but the acceleration is scaled.
Because units are in inches, the acceleration is in kg * in/s/s
To convert it to Newtons (kg * m/s/s), we multiply the force by M_INCH_TO_METER.

When applying a force, we have to convert it to (kg * in/s/s).
To convert it, we multiply our force (presumably in Newtons) by M_METER_TO_INCH.

Same applies to torque:
  - When obtaining torque from engine, multiply it by M_INCH2_TO_METER2.
  - When passing torque to the engine, multiply it by M_METER2_TO_INCH2.

When retriving velocity from engine, multiple it by M_INCH_TO_METER.
When passing velocity to the engine, multiply it by M_METER_TO_INCH.
Same applies to acceleration.

Linear friction is controlled by force
Angular friction is controlled by torque

Do the following when updating NewtonDynamics:
  - File: Newton.cpp
	  Change NewtonMaterialSetContactSoftness min/max to 0.01f and 1.00f
  - File: NewtonClass.cpp
	  Comment out stiffness modification in SetRowStiffness function.
  - File: dgTypes.h
	  ~ Comment out all #define DG_SSE4_INSTRUCTIONS_SET
  - File: dgThread.h, dgTypes.h
	  Uncomment #define DG_USE_THREAD_EMULATION
  - dgBody.h
	  DG_MINIMUM_MASS to 1.0e-6f
  - dgDynamicBody.h
	  Change DG_FREEZE_MAG to 0.05f
	  Change DG_ERR_TOLERANCE to 1.0e-3f
  - File: dgCollisionCompound.cpp
	  Change DG_MAX_MIN_VOLUME to 1.0e-6f
  - File: dgBilateralConstraint.cpp
	  Comment out stiffness modification in SetSpringDamperAcceleration.
	  Make it: desc.m_jointStiffness[index] = rowStiffness;
  - dgWorld.h
	  Change DG_REDUCE_CONTACT_TOLERANCE to 5.0e-3f
	  Change DG_PRUNE_CONTACT_TOLERANCE to 5.0e-3f
  - File: dgWorldDynamicUpdate.cpp
	  Change DG_PARALLEL_JOINT_COUNT_CUT_OFF to 1024
  - File: dgWorldDynamicUpdate.h
	  Change DG_FREEZZING_VELOCITY_DRAG to 0.1f
	  Change DG_MAX_SKELETON_JOINT_COUNT to 1024
  - File: dgWorldDynamicSimpleSolver.cpp
	  Line 524, comment out dgAssert(rowCount <= cluster->m_rowsCount);
  - File: dgSkeletonContainer.cpp
	  Line 217, comment out dgAssert (m_dof > 0);
	  Line 218, comment out dgAssert (m_dof <= 6);
  - Revert to a custom dgConvexHull3d.cpp, located in custom folder.

*/

#ifndef MSP_H
#define MSP_H

#include "msp_util.h"
#include <set>
#include <map>
#include <vector>

// Comment out if SDL is not needed
#define MSP_USE_SDL

#define MSP_NON_COL_CONTACTS_CAPACITY 16
#define MSP_MAX_RAY_HITS			  256

namespace MSP {
	// Classes
	class Newton;
	class World;
	class Collision;
	class Body;
	class Bodies;
	class Joint;
	class Gear;
	class BallAndSocket;
	class Corkscrew;
	class Fixed;
	class Hinge;
	class Motor;
	class Piston;
	class Servo;
	class Slider;
	class Spring;
	class Universal;
	class UpVector;
	class CurvySlider;
	class CurvyPiston;
	class Plane;
	class PointToPoint;
	class SDL;
	class SDLMixer;
	class Sound;
	class Music;
	class Joystick;
	class Particle;

	// Structures

	// Ruby Functions
	VALUE rbf_is_sdl_used(VALUE self);

	// Main
	void init_ruby();
};

#endif	/* MSP_H */
