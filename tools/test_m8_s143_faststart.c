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
 assert(!M8S143_Ready(&a,280,210,1874));
 assert(!M8S143_Ready(&a,211,211,1880));
 take(&a,230,1877);assert(!M8S143_Ready(&a,230,230,1877));
 take(&a,280,1877);take(&a,340,1877);assert(M8S143_Ready(&a,340,340,1877));
 M8S143_Observe(&a,345,340,1877,1);assert(M8S143_Ready(&a,345,340,1877));
 M8S143_Observe(&a,406,340,1877,1);assert(!M8S143_Ready(&a,406,340,1877));
 take(&a,500,1877);assert(!M8S143_Ready(&a,500,500,1877));
 take(&a,560,1877);take(&a,620,1877);assert(M8S143_Ready(&a,620,620,1877));
 M8S143_Observe(&a,640,640,1877,0);assert(!M8S143_Ready(&a,640,640,1877));
 M8S143_Reset(&a);assert(!M8S143_Ready(&a,640,640,1877));
 take(&b,100,4095);take(&b,150,0);take(&b,210,1);
 assert(M8S143_Ready(&b,210,210,1));
 take(&b,230,3);assert(!M8S143_Ready(&b,230,230,3));
 puts("PASS S14.3 quiet/fresh/coast/invalid/wrap tests");
 return 0;
}
