#if defined(M9B1_USE_SERVO_PROFILE)
 /* A small persistent position error builds a load-compensating output.
  * Then exactly zero position error must retain that output for60s. */
 steer_pos_left_raw=1882;steer_pos_right_raw=666;sample_at(0);begin_center(2);
 for(ms=10;ms<=1000;ms+=10)pass(ms);
 assert(m9_h2b1_hold.axis[0].integral_q8>0 && m9_h2b1_hold.axis[1].integral_q8<0);
 steer_pos_left_raw=1886;steer_pos_right_raw=662;
 for(;ms<=2000;ms+=10)pass(ms);
 {uint16_t left=hardware_pwm[0],right=hardware_pwm[1];
  assert(left && right);
  for(;ms<=62000;ms+=10){
   pass(ms);assert(hardware_pwm[0]==left && hardware_pwm[1]==right);
   assert(m9_h2b1_hold.state==M9B1_HOLDING && !m9_h2b1_hold.retry_count);
  }
 }
 /* External force holding a nonzero error never triggers a timed rest. */
 steer_pos_left_raw=1866;steer_pos_right_raw=682;
 for(;ms<=122000;ms+=10){pass(ms);assert(m9_h2b1_hold.active && hardware_pwm[0] && hardware_pwm[1]);}
 assert(!m9_h2b1_hold.fault_axes && m9_h2b1_hold.state==M9B1_HOLDING);
 /* Fault/owner cleanup clears both learned load commands. */
 Turn_Motors_Stop();assert(!m9_h2b1_hold.active && !hardware_pwm[0] && !hardware_pwm[1]);
 assert(!m9_h2b1_hold.axis[0].integral_q8 && !m9_h2b1_hold.axis[1].integral_q8);
 sample_at(clock_ms);begin_center(2);
 for(unsigned t=0;t<200;t++){SysTick_Handler();if(t%10==0)pass(clock_ms);}
 assert(hardware_pwm[0] && hardware_pwm[1]);
 for(unsigned t=0;t<50;t++)SysTick_Handler();
 assert(!hardware_pwm[0] && !hardware_pwm[1]);pass(clock_ms);
 assert(!m9_h2b1_hold.active && m9_h2b1_hold.reason==M9B1_SERVICE_GAP);
 /* Sensor freshness deadline remains independent of fast main service. */
 sample_at(clock_ms);begin_center(2);
 for(unsigned t=0;t<200;t++){SysTick_Handler();if(t%10==0)pass(clock_ms);}
 for(unsigned t=0;t<80;t++){SysTick_Handler();M9B1_Update();}
 assert(!m9_h2b1_hold.active && !hardware_pwm[0] && !hardware_pwm[1]);
 sample_at(clock_ms);begin_center(2);
 for(unsigned t=0;t<200;t++){SysTick_Handler();if(t%10==0)pass(clock_ms);}
 Steering_SafetyOnLimitInterrupt(STEER_LIMIT_L_MIN_MASK);M9B1_PreOwnerService();
 assert(!m9_h2b1_hold.active && !hardware_pwm[0] && !hardware_pwm[1]);
 assert(!m9_h2b1_hold.axis[0].integral_q8 && !m9_h2b1_hold.axis[1].integral_q8);
 puts("PASS continuous SERVO actual HAL/main: load output retained at EXACT goal60s, forced error60s without rest, STOP/NC clear integral, lease50ms, stale80ms, Rotate handoff");
 return 0;
#endif
