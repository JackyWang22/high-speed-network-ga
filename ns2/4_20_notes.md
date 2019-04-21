# 4_20 Notes

## In the original ns2 project, we can implement token bucket in the diffserv folder. There is a policy named token bucket filter.
  *set qE1C [[$ns link $core $e2] queue]
  * Add entries in policy table
  $qE1C addPolicyEntry [$s1 id] [$dest id] TokenBucket 20 $cir0 $cbs0
  * Add Entries in policer table
  $qE1C addPolicerEntry TokenBucket 10 11
  *More examples under ~ns/tcl/ex/diffserv/


## Leaky bucket is just as simple as setting token rate= 1.
## Token bucket: Store packet (shaper)
Discard packet (policer)
##
*RFC 2697: A Single-Rate, Three-Color Marker
Committed Information Rate (CIR), 
Committed Burst Size (CBS), 
Excess Burst Size (EBS)
*RFC 2698: A Dual-Rate, Three-Color Marker
Peak Information Rate (PIR) 
Committed Information Rate (CIR), 
Committed Burst Size (CBS), 
Peak Burst Size (PBS)



## https://sites.google.com/site/linuxpebbles/Home/ns2/token-bucket-implementation-for-multiple-flows
  * This provides the method we used in our project and implement multi flow based on token bucket.
 
## I have some useful cc files need to read and learn from below:
