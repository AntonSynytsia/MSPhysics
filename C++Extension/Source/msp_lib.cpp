/*
The C++ version of MSPhysics implements the following:
  - Newton Dynamics Physics SDK 3.14 by Juleo Jerez and Alain Suero.
  - V-HACD 2.2 by Khaled Mamou.
  - SDL 1.2.15 / 2.0.4
  - SDL_mixer 1.2.12 / 2.0.1

Implementation by Anton Synytsia

Do the following when updating NewtonDynamics:
* File: dgBody.h
	Change DG_MINIMUM_MASS to 1.0e-6f
* File: dgBody.cpp, lines 496-498
	~ Remove the clamping of inirtia values.
* File: dgCollisionBox.cpp, line 322
	~ Change tiltAngle to 0.785398f. (45 degrees)
* File: dgCollisionCompound.cpp
	Change DG_MAX_MIN_VOLUME to 1.0e-6f
* File: dgBilateralConstraint.cpp, line 237
	Comment out desc.m_jointStiffness[index] = - den / DG_PSD_DAMP_TOL ;
* File: dgContact.h
	Change DG_MAX_CONTATCS to 4096
* File: dgBroadPhase.h
	Change DG_BROADPHASE_MAX_STACK_DEPTH to 4096
* File: dgWorldDynamicUpdate.h
	Change DG_MAX_SKELETON_JOINT_COUNT to 4096
* File: dgWorldDynamicUpdate.cpp
	Change DG_PARALLEL_JOINT_COUNT_CUT_OFF to 4096
* File: dgThread.h, line 27
	Uncomment #define DG_USE_THREAD_EMULATION
* File: Newton.cpp, line 2091
	Change NewtonMaterialSetContactSoftness min/max to 0.01f and 1.00f
* File: NewtonClass.h
	Change min and max timestep to 1/30 and 1/1200

To Do:
- body_recalculate_volume(body)
- body_get_moments_of_inertia(body)
- body_set_moments_of_inertia(body, ixx, iyy, izz)
- User Mesh
- Fractured Compounds
- Vehicles
- Ragdols
- Cloth
- Kinematic bodies
*/

#include "msp_util.h"
#include "msp_newton.h"
#include "msp_world.h"
#include "msp_collision.h"
#include "msp_body.h"
#include "msp_bodies.h"
#include "msp_joint.h"

#include "msp_joint_ball_and_socket.h"
#include "msp_joint_corkscrew.h"
#include "msp_joint_fixed.h"
#include "msp_joint_hinge.h"
#include "msp_joint_motor.h"
#include "msp_joint_piston.h"
#include "msp_joint_servo.h"
#include "msp_joint_slider.h"
#include "msp_joint_spring.h"
#include "msp_joint_universal.h"
#include "msp_joint_up_vector.h"

#include "msp_sdl.h"
#include "msp_sdl_mixer.h"
#include "msp_sound.h"
#include "msp_music.h"

void Init_msp_lib() {
	VALUE mMSPhysics = rb_define_module("MSPhysics");
	VALUE mNewton = rb_define_module_under(mMSPhysics, "Newton");

	Init_msp_util(mMSPhysics);
	Init_msp_newton(mNewton);
	Init_msp_world(mNewton);
	Init_msp_collision(mNewton);
	Init_msp_body(mNewton);
	Init_msp_bodies(mNewton);
	Init_msp_joint(mNewton);

	Init_msp_ball_and_socket(mNewton);
	Init_msp_fixed(mNewton);
	Init_msp_hinge(mNewton);
	Init_msp_motor(mNewton);
	Init_msp_servo(mNewton);
	Init_msp_slider(mNewton);
	Init_msp_piston(mNewton);
	Init_msp_spring(mNewton);
	Init_msp_up_vector(mNewton);

	Init_msp_sdl(mMSPhysics);
	Init_msp_sdl_mixer(mMSPhysics);
	Init_msp_sound(mMSPhysics);
	Init_msp_music(mMSPhysics);
}
