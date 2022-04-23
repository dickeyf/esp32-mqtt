
void init_servos();

/**
 * Sets a servo motor at a relative percent between its minimum and maximum angle.
 *
 * @param angle It's angle in fractional of its movement.  0.0 being its minimal position, and 1.0 its maximum position.
 */
void set_servo_angle(double angle);