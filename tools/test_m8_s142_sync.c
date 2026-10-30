/* Tests of EXACT production pure planner, no HAL dependency. */
#include <assert.h>
#include <stdio.h>
#include "steer_s142_sync.h"

static void bounds(void)
{
  int e;
  for(e=-2000;e<=2000;e+=25)
  {
    M8S142SyncPlan_t p=M8S142_SyncPlan(405,175,450,200,175,50,
          (int16_t)e,4000,2500,400);
    assert(p.left_pwm>=175 && p.left_pwm<=450);
    assert(p.right_pwm>=50 && p.right_pwm<=200);
    assert(!(p.boost_l && p.boost_r));
    assert(!(p.trim_l && p.trim_r));
    assert(!(p.boost_l && p.trim_l));
    assert(!(p.boost_r && p.trim_r));
  }
}
int main(void)
{
  M8S142SyncPlan_t p;
  p=M8S142_SyncPlan(405,175,450,200,175,50,0,2800,2800,500);
  assert(p.left_pwm==405 && p.right_pwm==175);
  p=M8S142_SyncPlan(405,175,450,200,175,50,200,3800,3000,500);
  assert(p.boost_r>0 && p.trim_l==0 && p.right_pwm<=200);
  p=M8S142_SyncPlan(405,200,450,200,175,50,400,3800,2700,500);
  assert(p.sat_r && p.boost_r==0 && p.trim_l>0);
  p=M8S142_SyncPlan(405,200,450,200,175,50,400,2500,4000,500);
  assert(p.trim_l==0); /* follower is already catching up */
  p=M8S142_SyncPlan(405,175,450,200,175,50,-200,2900,3700,500);
  assert(p.boost_l>0 && p.trim_r==0);
  p=M8S142_SyncPlan(450,175,450,200,175,50,-400,2600,3800,500);
  assert(p.sat_l && p.trim_r>0);
  p=M8S142_SyncPlan(405,175,450,200,175,50,400,4000,2800,180);
  assert(p.left_pwm==405 && p.right_pwm==175);
  p=M8S142_SyncPlan(0,175,450,200,175,50,400,4000,2800,500);
  assert(p.left_pwm==0 && p.right_pwm==175);
  p=M8S142_SyncPlan(405,0,450,200,175,50,400,4000,2800,500);
  assert(p.left_pwm==405 && p.right_pwm==0);
  p=M8S142_SyncPlan(405,175,450,200,175,50,40,2800,2800,500);
  assert(p.left_pwm==405 && p.right_pwm==175);
  p=M8S142_SyncPlan(405,175,450,200,175,50,-40,2800,2800,500);
  assert(p.left_pwm==405 && p.right_pwm==175);
  bounds();
  puts("PASS S14.2 cross-coupled: boost first, saturation taper, coast and limits");
  return 0;
}
