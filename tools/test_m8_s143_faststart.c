#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "steer_s143_faststart.h"
static void take(M8S143_Quiet *q,uint32_t ms,uint16_t raw){
 M8S143_Observe(q,ms,ms,raw,1U);
}
int main(void){
 M8S143_Quiet a={0},b={0};
 take(&a,100,1873);assert(!M8S143_Ready(&a,100,100,1873));
 take(&a,150,1873);assert(!M8S143_Ready(&a,150,150,1873));
 take(&a,210,1874);assert(M8S143_Ready(&a,210,210,1874));
 assert(!M8S143_Ready(&a,351,210,1874)); /* >140ms stale */
 assert(!M8S143_Ready(&a,211,211,1880));
 take(&a,230,1877);assert(!M8S143_Ready(&a,230,230,1877));
 take(&a,280,1877);take(&a,340,1877);assert(M8S143_Ready(&a,340,340,1877));
 M8S143_Observe(&a,345,340,1877,1);assert(M8S143_Ready(&a,345,340,1877));
 M8S143_Observe(&a,481,340,1877,1);assert(!M8S143_Ready(&a,481,340,1877));
 take(&a,500,1877);assert(!M8S143_Ready(&a,500,500,1877));
 take(&a,560,1877);take(&a,620,1877);assert(M8S143_Ready(&a,620,620,1877));
 M8S143_Observe(&a,640,640,1877,0);assert(!M8S143_Ready(&a,640,640,1877));
 M8S143_Reset(&a);assert(!M8S143_Ready(&a,640,640,1877));
 take(&b,100,4095);take(&b,150,0);take(&b,210,1);
 assert(M8S143_Ready(&b,210,210,1));
 take(&b,230,3);assert(!M8S143_Ready(&b,230,230,3));
 /* Regression: command loop took 'now' BEFORE blocking I2C and the
  * sensor timestamp was set AFTER I2C. Underflow must fail closed,
  * but the firmware now uses a re-read HAL tick AFTER the I2C.
  */
 M8S143_Quiet pre={0};
 take(&pre,100U,1873U);take(&pre,200U,1873U);take(&pre,300U,1873U);
 assert(M8S143_Ready(&pre,300U,300U,1873U));
 assert(!M8S143_Ready(&pre,300U,302U,1873U)); /* buggy old clock */
 assert(M8S143_Ready(&pre,303U,302U,1873U));  /* corrected clock */
 /* Two AS5600s read in alternating >=50ms slots: each axis ~100ms. */
 M8S143_Quiet left={0},right={0};
 uint32_t last_l=0U,last_r=0U;
 uint16_t raw_l=1873U,raw_r=662U;
 for(uint32_t now=100U;now<=360U;now+=10U){
   if(now==100U||now==200U||now==300U)last_l=now;
   if(now==150U||now==250U||now==350U)last_r=now;
   M8S143_Observe(&left,now,last_l,raw_l,1U);
   M8S143_Observe(&right,now,last_r,raw_r,1U);
 }
 assert(left.distinct_samples==3U && right.distinct_samples==3U);
 assert(M8S143_Ready(&left,360U,last_l,raw_l));
 assert(M8S143_Ready(&right,360U,last_r,raw_r));
 /* Coast invalidates the moving axis, while the other is still quiet. */
 M8S143_Observe(&right,370U,370U,(uint16_t)(raw_r+5U),1U);
 assert(!M8S143_Ready(&right,370U,370U,(uint16_t)(raw_r+5U)));
 assert(M8S143_Ready(&left,370U,last_l,raw_l));
 assert(!M8S143_Ready(&left,441U,last_l,raw_l));
 puts("PASS S14.3 quiet/fresh/coast/invalid/wrap/alternating-poll tests");
 return 0;
}
