#ifndef ANGULAR_INTEGRATION_H
#define ANGULAR_INTEGRATION_H

#include "msp_util.h"

class AngularIntegration {
public:
	AngularIntegration();
	AngularIntegration(dFloat angle);

	dFloat get_angle() const;
	void set_angle(dFloat angle);
	dFloat update(dFloat new_angle_cos, dFloat new_angle_sin);
	dFloat update(dFloat angle);

	AngularIntegration operator+ (const AngularIntegration& angle) const;
	AngularIntegration operator- (const AngularIntegration& angle) const;

private:
	dFloat m_angle;
	dFloat m_sin_angle;
	dFloat m_cos_angle;
};

#endif	/* ANGULAR_INTEGRATION_H */
