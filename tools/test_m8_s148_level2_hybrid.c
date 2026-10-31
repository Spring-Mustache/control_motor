/* Host test against the exact M8-S14.8 production helper. */
#include <assert.h>
#include <stdio.h>
#include "steer_s148_level2_hybrid.h"
int main(void)
{
  M8S142SyncPlan_t pd={0};
  M8S148Plan_t p;
  /* Level2 owner405/175; max 450/200; P+D stronger than PI. */
  pd.boost_l=30U;pd.boost_r=25U;
  pd.trim_l=48U;pd.left_pwm=357U;
  p=M8S148_Merge(405,175,450,200,418,185,&pd,432,194);
  assert(p.left_pwm==430U&&p.right_pwm==197U);
  assert(p.pd_win_l&&p.pd_win_r);
  /* PI stronger: no stacking, cap and owner maintained. */
  pd.boost_l=4U;pd.boost_r=3U;
  p=M8S148_Merge(405,175,450,200,440,190,&pd,443,199);
  assert(p.left_pwm==443U&&p.right_pwm==199U);
  assert(p.pi_win_l&&p.pi_win_r);
  /* PI target is already met; P+D can still boost. */
  pd.boost_l=40U;pd.boost_r=22U;
  p=M8S148_Merge(405,175,450,200,440,195,&pd,405,175);
  assert(p.left_pwm==445U&&p.right_pwm==197U);
  /* Level1 caps. */
  pd.boost_l=45U;pd.boost_r=75U;
  p=M8S148_Merge(330,125,375,150,370,149,&pd,360,150);
  assert(p.left_pwm==375U&&p.right_pwm==150U);
  /* Stop before or after one axis completes: no re-energize. */
  p=M8S148_Merge(0,175,450,200,300,185,&pd,450,200);
  assert(p.left_pwm==0U&&p.right_pwm==175U);
  /* P+D trim is intentionally not applied, even if legacy asks. */
  pd.boost_l=0U;pd.boost_r=0U;pd.trim_l=48U;
  p=M8S148_Merge(405,175,450,200,405,175,&pd,405,175);
  assert(p.left_pwm==405U&&p.right_pwm==175U);
  /* +12 CCR additional rise; never reduce an owner that rose already. */
  pd.boost_l=45U;pd.boost_r=25U;
  p=M8S148_Merge(405,175,450,200,405,175,&pd,450,200);
  assert(p.left_pwm==417U&&p.right_pwm==187U);
  p=M8S148_Merge(405,175,450,200,300,100,&pd,450,200);
  assert(p.left_pwm==405U&&p.right_pwm==175U);
  puts("PASS M8-S14.8: Level1/2 MAX(PI,P+D) boost, never SUM,");
  puts("      no trim, caps 375/150 450/200, +12 rise and zero-CCR guard");
  return 0;
}
