#include "steer_s153_stall_diag.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 assert(M8S153_DiagGap(257,245)==12);
 assert(M8S153_DiagGap(2224,2225)==1);
 assert(M8S153_DiagGap(245,245)==0);
 assert(M8S153_DiagAgeMs(14396,13746)==650);
 assert(M8S153_DiagAgeMs(14396,14395)==1);
 assert(M8S153_DiagAgeMs(1000,1001)==0);
 assert(M8S153_DiagFuture(1000,1001)==1);
 assert(M8S153_DiagAgeMs(1000,0)==0);
 assert(M8S153_DiagAgeMs(0x10U,0xfffffff0U)==32);
 assert(M8S153_STOP5_NEUTRAL_REQUIRED!=M8S153_STOP5_DIRECT_REVERSAL);
 puts("M8-S15.3D HOST C PASS");return 0;
}
