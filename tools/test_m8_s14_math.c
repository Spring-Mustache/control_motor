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

  /* M8-S14.1: stronger LEFT taper only under the saturated-RIGHT gate. */
  {
    M8S14Plan_t base = M8S14_Plan(150, 1000, 800, 400);
    M8S14Plan_t tuned = M8S141_PlanLeftSaturated(150, 1000, 800, 400, 1);
    M8S14Plan_t no_cap = M8S141_PlanLeftSaturated(150, 1000, 800, 400, 0);
    assert(tuned.trim_left_pwm > base.trim_left_pwm);
    assert(tuned.trim_right_pwm == base.trim_right_pwm);
    assert(no_cap.trim_left_pwm == base.trim_left_pwm);
    assert(no_cap.trim_right_pwm == base.trim_right_pwm);
    p = M8S141_PlanLeftSaturated(-300, 500, 900, 400, 1);
    assert(p.trim_left_pwm == 0 && p.trim_right_pwm > 0);
    p = M8S141_PlanLeftSaturated(150, 1000, 800, 100, 1);
    assert(p.trim_left_pwm == 0 && p.trim_right_pwm == 0);
    p = M8S141_PlanLeftSaturated(150, 1000, 800, 150, 1);
    assert(p.trim_left_pwm > 0 && p.trim_left_pwm < tuned.trim_left_pwm);
    p = M8S141_PlanLeftSaturated(7000, 6000, 0, 400, 1);
    assert(p.trim_left_pwm == 96 && p.trim_right_pwm == 0);
    p = M8S141_PlanLeftSaturated(0, 800, 800, 400, 1);
    assert(p.trim_left_pwm == 0 && p.trim_right_pwm == 0);
    p = M8S141_PlanLeftSaturated(0, 1500, 500, 400, 1);
    assert(p.trim_left_pwm > 0);
  }
  puts("M8-S14 and M8-S14.1 C planner: PASS");
  return 0;
}
