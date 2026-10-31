#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "steer_s1410_right_terminal_shadow.h"
int main(void)
{
  M8S1410Shadow_t p;
  p=M8S1410_Plan(180U,250U,100U,100U,1U,20U,1U);
  assert(p.valid&&p.candidate_pwm==100U&&p.target_raw_s==510U);
  assert(p.would_raise==0U); /* original cap never surpassed */
  p=M8S1410_Plan(100U,180U,55U,100U,1U,10U,1U);
  assert(p.valid&&p.target_raw_s==350U);
  assert(p.candidate_pwm==63U&&p.would_raise);
  assert(p.forecast_50ms_raw==9U);
  assert(p.estimated_to_soft_ms==555U);
  p=M8S1410_Plan(60U,0U,60U,100U,1U,10U,1U);
  assert(p.valid&&p.candidate_pwm==73U);
  assert(p.estimated_to_soft_ms==65535U);
  p=M8S1410_Plan(40U,0U,50U,100U,1U,10U,1U);
  assert(p.valid&&!p.would_raise&&p.candidate_pwm==50U);
  p=M8S1410_Plan(20U,900U,50U,100U,1U,10U,1U);
  assert(p.valid&&p.distance_proxy_warning&&!p.would_raise);
  p=M8S1410_Plan(100U,300U,60U,100U,1U,101U,1U);
  assert(!p.valid&&p.candidate_pwm==60U);
  p=M8S1410_Plan(100U,300U,60U,100U,0U,0U,1U);
  assert(!p.valid&&p.candidate_pwm==60U);
  p=M8S1410_Plan(100U,300U,60U,100U,1U,0U,0U);
  assert(!p.valid&&p.candidate_pwm==60U);
  p=M8S1410_Plan(181U,300U,100U,200U,1U,0U,1U);
  assert(!p.valid&&p.candidate_pwm==100U);
  p=M8S1410_Plan(100U,300U,0U,100U,1U,0U,1U);
  assert(!p.valid&&p.candidate_pwm==0U);
  p=M8S1410_Plan(100U,0U,80U,79U,1U,0U,1U);
  assert(!p.valid&&p.candidate_pwm==80U);
  p=M8S1410_Plan(80U,10000U,60U,100U,1U,0U,1U);
  assert(p.valid&&p.distance_proxy_warning);
  p=M8S1410_Plan(80U,10001U,60U,100U,1U,0U,1U);
  assert(!p.valid);
  puts("PASS M8-S14.10: read-only speed-adaptive shadow, freshness,");
  puts("      40RAW no-raise, existing cap, PWM0, proxy not coast.");
  return 0;
}
