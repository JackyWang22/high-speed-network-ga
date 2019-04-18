# 4/18/2019 Note 

## Bind parameter between cpp file with tcl file.
And solve the problem for "no class variable".
** Use 	bind("recvpkt_", &received_packets); to add connection.

* The folder path is tcl/lib/ns-lib.tcl.
**  Within the _ns-default.tcl_ file. Add wherever, or search Queue to add following lines.
```
DSShaper set recvpkt_ 0
DSShaper set stpkt_ 0
DSShaper set shpkt_ 0
DSShaper set drpkt_ 0
```
** ##Important## Every new protocal needs to add
```
DSShaper set debug_ false
```

** By then, it will directly add 
``` C++
\n\
DSShaper set recvpkt_ 0\n\
DSShaper set stpkt_ 0\n\
DSShaper set shpkt_ 0\n\
DSShaper set drpkt_ 0\n\
\n\
````
For _ns_tcl.cc_ file. No worry for adding these lines manulally.
