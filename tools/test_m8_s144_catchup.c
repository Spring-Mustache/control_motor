/* Exact production pure M8-S14.4/5 helper + S14.2 planner. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "steer_s144_rmin_catchup.h"
#include "steer_s142_sync.h"
static void blockers(void)
{
 int d;
 for(d=-500;d<=500;d+=10)
 {
  uint16_t val=M8S144_RminBias(1,1,1,1,500,500,(int16_t)d);
  assert(val<=200U);
  if(d< -250)assert(val==0U);
 }
 assert(M8S144_RminBias(0,1,1,1,600,600,0)==0U);
 assert(M8S144_RminBias(1,0,1,1,600,600,0)==0U);
 assert(M8S144_RminBias(1,2,1,1,600,600,0)==0U); /* Level 2 not proven */
 assert(M8S144_RminBias(1,1,0,1,600,600,0)==0U); /* RX/R1/R2 */
 assert(M8S144_RminBias(1,1,1,0,600,600,0)==0U); /* RIGHT MAX */
 assert(M8S144_RminBias(1,1,1,1,320,600,0)==0U);
 assert(M8S144_RminBias(1,1,1,1,600,320,0)==0U);
 assert(M8S144_RminBias(1,1,1,1,321,700,0)==3U);
 assert(M8S144_RminBias(1,1,1,1,350,700,0)==100U);
 assert(M8S144_RminBias(1,1,1,1,380,700,0)==200U);
 assert(M8S144_RminBias(1,1,1,1,700,700,-251)==0U);
 assert(M8S144_RminBias(1,1,1,1,700,700,-250)==200U);
}
int main(void)
{
  M8S142SyncPlan_t old,improved;
  uint16_t bias=M8S144_RminBias(1,1,1,1,600,600,-120);
  assert(bias==200U);
  old=M8S142_SyncPlan(330,125,375,150,175,50,-120,2800,2800,600);
  improved=M8S142_SyncPlan(330,125,375,150,175,50,
                   (int16_t)(-120+bias),2800,2800,600);
  assert(old.boost_l>0U && old.boost_r==0U);
  assert(improved.boost_r>0U && improved.boost_l==0U);
  assert(improved.right_pwm>old.right_pwm);
  assert(improved.right_pwm<=150U && improved.left_pwm<=375U);
  /* Old S14.2 last-180 RAW coast zone is untouched. */
  for(uint16_t dist=0;dist<=320;dist++)
    assert(M8S144_RminBias(1,1,1,1,dist,600,0)==0U);
  blockers();
  puts("PASS S14.5: 2-deg early RIGHT MIN priority, fades before SOFT, all gates");
  return 0;
}
