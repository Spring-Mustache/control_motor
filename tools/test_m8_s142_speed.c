#include <assert.h>
#include <stdio.h>
#include "steer_s142_speed.h"
int main(void)
{
  assert(M8S142_Cap(1,0)==300&&M8S142_Cap(0,0)==100);
  assert(M8S142_Cap(1,1)==375&&M8S142_Cap(0,1)==150);
  assert(M8S142_Cap(1,2)==450&&M8S142_Cap(0,2)==200);
  assert(M8S142_Cap(1,3)==300);
  for(unsigned left=0;left<2;left++)
    for(unsigned level=0;level<3;level++)
      for(unsigned d=0;d<1000;d++)
      {
        uint16_t cap=M8S142_Envelope(left,level,d);
        assert(cap>=M8S142_Cap(left,0));
        assert(cap<=M8S142_Cap(left,level));
        if(d<=180)assert(cap==M8S142_Cap(left,0));
        if(d>=320)assert(cap==M8S142_Cap(left,level));
      }
  assert(M8S142_RX(175,450,0)==175);
  assert(M8S142_RX(175,450,117)==450);
  assert(M8S142_RX(50,200,117)==200);
  assert(M8S142_Rise(450,300,300)==312);
  assert(M8S142_Rise(200,100,100)==112);
  assert(M8S142_Rise(200,300,100)==200);
  puts("PASS S14.2 speed caps and SOFT envelope");return 0;
}
