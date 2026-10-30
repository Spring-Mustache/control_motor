/* Host-runnable tests of the EXACT pure planner in Core/Inc. */
#include <assert.h>
#include <stdio.h>
#include "steer_s14_math.h"

int main(void)
{
  M8S14Plan_t p;
  p = M8S14_Plan(0, 800, 800, 400);
  assert(p.trim_left_pwm == 0 && p.trim_right_pwm == 0);
  p = M8S14_Plan(300, 900, 500, 400);
  assert(p.trim_left_pwm > 0 && p.trim_right_pwm == 0);
  p = M8S14_Plan(-300, 500, 900, 400);
  assert(p.trim_left_pwm == 0 && p.trim_right_pwm > 0);
  p = M8S14_Plan(0, 2000, 500, 400);
  assert(p.trim_left_pwm > 0 && p.trim_right_pwm == 0);
  p = M8S14_Plan(0, 500, 2000, 400);
  assert(p.trim_left_pwm == 0 && p.trim_right_pwm > 0);
  p = M8S14_Plan(600, 1500, 200, 100);
  assert(p.trim_left_pwm == 0 && p.trim_right_pwm == 0);
  p = M8S14_Plan(600, 1500, 200, 150);
  assert(p.trim_left_pwm > 0 && p.trim_left_pwm < 96);
  p = M8S14_Plan(6000, 5000, 0, 400);
  assert(p.trim_left_pwm == 96 && p.trim_right_pwm == 0);
  p = M8S14_Plan(-6000, 0, 5000, 400);
  assert(p.trim_right_pwm == 48 && p.trim_left_pwm == 0);
  assert(M8S14_Slew(0, 100, 8) == 8);
  assert(M8S14_Slew(80, 0, 8) == 72);
  assert(M8S14_Slew(0, 100, 100) == 12);
  assert(M8S14_Slew(10, 11, 8) == 11);
  puts("M8-S14 pure C planner: PASS");
  return 0;
}
