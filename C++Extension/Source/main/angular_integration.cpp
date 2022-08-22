/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "pch.h"
#include "angular_integration.h"

AngularIntegration::AngularIntegration() {
    set_angle(0.0);
}

AngularIntegration::AngularIntegration(dFloat angle) {
    set_angle(angle);
}

dFloat AngularIntegration::get_angle() const {
    return m_angle;
}

void AngularIntegration::set_angle(dFloat angle) {
    m_angle = angle;
    m_sin_angle = dSin(angle);
    m_cos_angle = dCos(angle);
}

dFloat AngularIntegration::update(dFloat new_angle_cos, dFloat new_angle_sin) {
    dFloat sin_da = new_angle_sin * m_cos_angle - new_angle_cos * m_sin_angle;
    dFloat cos_da = new_angle_cos * m_cos_angle + new_angle_sin * m_sin_angle;

    m_angle += dAtan2(sin_da, cos_da);
    m_cos_angle = new_angle_cos;
    m_sin_angle = new_angle_sin;

    return m_angle;
}

dFloat AngularIntegration::update(dFloat angle) {
    return update(dCos(angle), dSin(angle));
}

AngularIntegration AngularIntegration::operator+ (const AngularIntegration& angle) const {
    dFloat sin_da = angle.m_sin_angle * m_cos_angle + angle.m_cos_angle * m_sin_angle;
    dFloat cos_da = angle.m_cos_angle * m_cos_angle - angle.m_sin_angle * m_sin_angle;
    return AngularIntegration(m_angle + dAtan2(sin_da, cos_da));
}

AngularIntegration AngularIntegration::operator- (const AngularIntegration& angle) const {
    dFloat sin_da = angle.m_sin_angle * m_cos_angle - angle.m_cos_angle * m_sin_angle;
    dFloat cos_da = angle.m_cos_angle * m_cos_angle + angle.m_sin_angle * m_sin_angle;
    return AngularIntegration(dAtan2(sin_da, cos_da));
}
