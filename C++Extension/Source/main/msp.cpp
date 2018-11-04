/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "msp.h"

#include "msp_newton.h"
#include "msp_world.h"
#include "msp_collision.h"
#include "msp_body.h"
#include "msp_bodies.h"
#include "msp_joint.h"
#include "msp_gear.h"

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
#include "msp_joint_curvy_slider.h"
#include "msp_joint_curvy_piston.h"
#include "msp_joint_plane.h"
#include "msp_joint_point_to_point.h"

#ifdef MSP_USE_SDL
    #include "msp_sdl.h"
    #include "msp_sdl_mixer.h"
    #include "msp_sound.h"
    #include "msp_music.h"
    #include "msp_joystick.h"
#endif

#include "msp_particle.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::rbf_is_sdl_used(VALUE self) {
#ifdef MSP_USE_SDL
    return Qtrue;
#else
    return Qfalse;
#endif
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::init_ruby(VALUE mMSP) {
    VALUE mNewton = rb_define_module_under(mMSP, "Newton");
    VALUE mC = rb_define_module_under(mMSP, "C");

    Util::init_ruby();

    MSP::Newton::init_ruby(mNewton);
    MSP::World::init_ruby(mNewton);
    MSP::Collision::init_ruby(mNewton);
    MSP::Body::init_ruby(mNewton);
    MSP::Bodies::init_ruby(mNewton);
    MSP::Joint::init_ruby(mNewton);
    MSP::Gear::init_ruby(mNewton);

    MSP::BallAndSocket::init_ruby(mNewton);
    MSP::Corkscrew::init_ruby(mNewton);
    MSP::Fixed::init_ruby(mNewton);
    MSP::Hinge::init_ruby(mNewton);
    MSP::Motor::init_ruby(mNewton);
    MSP::Servo::init_ruby(mNewton);
    MSP::Slider::init_ruby(mNewton);
    MSP::Piston::init_ruby(mNewton);
    MSP::Spring::init_ruby(mNewton);
    MSP::UpVector::init_ruby(mNewton);
    MSP::Universal::init_ruby(mNewton);
    MSP::CurvySlider::init_ruby(mNewton);
    MSP::CurvyPiston::init_ruby(mNewton);
    MSP::Plane::init_ruby(mNewton);
    MSP::PointToPoint::init_ruby(mNewton);

    #ifdef MSP_USE_SDL
        MSP::SDL::init_ruby(mMSP);
        MSP::SDLMixer::init_ruby(mMSP);
        MSP::Sound::init_ruby(mMSP);
        MSP::Music::init_ruby(mMSP);
        MSP::Joystick::init_ruby(mMSP);
    #endif

    MSP::Particle::init_ruby(mC);

    rb_define_module_function(mMSP, "sdl_used?", VALUEFUNC(MSP::rbf_is_sdl_used), 0);
}
