#include "../main/msp.h"

extern "C"

void Init_msp_lib() {
	VALUE mMSP = rb_define_module("MSPhysics");
	MSP::init_ruby(mMSP);
}
