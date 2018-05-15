set opt(data_rate)                       0.03
set opt(packetsize)                      300
set opt(packetsize_bct)                  8
set opt(datarate_bct)                    0.02
set opt(distance)                        2000

set opt(chan)		Channel/UnderwaterChannel
set opt(prop)		Propagation/UnderwaterPropagation
set opt(netif)		Phy/UnderwaterPhy
set opt(mac)		Mac/UnderwaterMac/uw802_11
set opt(ifq)		Queue/DropTail/PriQueue
set opt(ll)		LL
set opt(energy)         EnergyModel
set opt(txpower)        30
set opt(rxpower)        1
set opt(initialenergy)  200000
set opt(idlepower)      0.2
set opt(ant)            Antenna/OmniAntenna  ;#we don't use it in underwater
set opt(filters)        GradientFilter    ;# options can be one or more of 
                                ;# TPP/OPP/Gear/Rmst/SourceRoute/Log/TagFilter



# the following parameters are set fot protocols
set opt(bit_rate)                     1000
set opt(encoding_efficiency)          1
set opt(guard_time)		      0.00001
set opt(max_backoff_slots)            4
set opt(max_burst)                    1


set opt(dz)                             10
set opt(ifqlen)		                50       ;# max packet in ifq
set opt(nn)	                	15	;# number of nodes in each layer
set opt(layers)                         1
set opt(x)	                	10000	;# X dimension of the topography
set opt(y)	                        7000 ;# Y dimension of the topography
set opt(z)                              [expr ($opt(layers)-1)*$opt(dz)]
set opt(seed)	                	348.88
set opt(stop)	                	2415	;# simulation time
set opt(prestop)                        2400 ;# time to prepare to stop
set opt(tr)	                	"802_11.tr"	;# trace file
set opt(nam)                            "802_11.nam"  ;# nam file
set opt(adhocRouting)                    Vectorbasedforward
set opt(width)                           20
set opt(adj)                             10
set opt(interval)                        0.01
#set opt(traf)	                	"diffusion-traf.tcl"      ;# traffic file
# ==================================================================
set start_time                  10

#LL set mindelay_		50us
#LL set delay_			25us
#LL set bandwidth_		0	;# not used

#Queue/DropTail/PriQueue set Prefer_Routing_Protocols    1

# unity gain, omni-directional antennas
# set up the antennas to be centered in the node and 1.5 meters above it
Antenna/OmniAntenna set X_ 0
Antenna/OmniAntenna set Y_ 0
Antenna/OmniAntenna set Z_ 1.5
Antenna/OmniAntenna set Z_ 0.05
Antenna/OmniAntenna set Gt_ 1.0
Antenna/OmniAntenna set Gr_ 1.0



Mac/UnderwaterMac set bit_rate_  $opt(bit_rate)
Mac/UnderwaterMac set encoding_efficiency_  $opt(encoding_efficiency)
Mac/UnderwaterMac/uw802_11 set dataRate_      1000
Mac/UnderwaterMac/uw802_11 set basicRate_     1000
Mac/UnderwaterMac/uw802_11 set PLCPDataRate_  1000000 
Mac/UnderwaterMac/uw802_11 set SlotTime_      4      ;# 0.0000020 20us\n\
Mac/UnderwaterMac/uw802_11 set SIFS_          2    ;# 10us\n\



# Initialize the SharedMedia interface with parameters to make
# it work like the 914MHz Lucent WaveLAN DSSS radio interface
Phy/UnderwaterPhy set CPThresh_ 10 ;#100  10.0
Phy/UnderwaterPhy set CSThresh_ 1.47  ;# 1.559e-11
Phy/UnderwaterPhy set RXThresh_ 1.47 ;#3.652e-10
#Phy/UnderwaterPhy set Rb_ 2*1e6
Phy/UnderwaterPhy set Pt_ 30;#0.2818
Phy/UnderwaterPhy set freq_ 10  ;#frequency range in khz 
Phy/UnderwaterPhy set K_ 2.0   ;#spherical spreading
Phy/UnderwaterPhy set setTxPower 30;
Phy/UnderwaterPhy set setRxPower 1;
Phy/UnderwaterPhy set setIdlePower 0.2;

# ==================================================================
# Main Program
# =================================================================

#
# Initialize Global Variables
# 
#set sink_ 1


remove-all-packet-headers 
#remove-packet-header AODV ARP TORA  IMEP TFRC
add-packet-header Common UWVB Mac LL
set ns_ [new Simulator]
set topo  [new Topography]

$topo load_cubicgrid $opt(x) $opt(y) $opt(z)
#$ns_ use-newtrace
set tracefd	[open $opt(tr) w]
$ns_ trace-all $tracefd

set nf [open $opt(nam) w]
$ns_ namtrace-all-wireless $nf $opt(x) $opt(y)


set total_number [expr $opt(nn)-1]
set god_ [create-god $opt(nn)]

set chan_1_ [new $opt(chan)]


global defaultRNG
$defaultRNG seed $opt(seed)

$ns_ node-config -adhocRouting $opt(adhocRouting) \
		 -llType $opt(ll) \
		 -macType $opt(mac) \
		 -ifqType $opt(ifq) \
		 -ifqLen $opt(ifqlen) \
		 -antType $opt(ant) \
		 -propType $opt(prop) \
		 -phyType $opt(netif) \
		 #-channelType $opt(chan) \
		 -agentTrace OFF \
                 -routerTrace OFF \
                 -macTrace ON\
                 -topoInstance $topo\
                 -energyModel $opt(energy)\
                 -txpower $opt(txpower)\
                 -rxpower $opt(rxpower)\
                 -initialEnergy $opt(initialenergy)\
                 -idlePower $opt(idlepower)\
                 -channel $chan_1_

$god_ set_filename test_802.data

set node_(0) [$ns_  node 0]
$god_ new_node $node_(0)
$node_(0) set X_  1000
$node_(0) set Y_  [expr 1000+2*$opt(distance)] 
$node_(0) set Z_  0
$node_(0) set-cx  1000
$node_(0) set-cy  [expr 1000+2*$opt(distance)]
$node_(0) set-cz  0
$node_(0) set_next_hop  12
$node_(0) ismobile 0
$node_(0) addenergymodel [new $opt(energy) $node_(0) $opt(initialenergy)  $opt(txpower) $opt(rxpower)]
set a_(0) [new Agent/UWSink]
$ns_ attach-agent $node_(0) $a_(0)
$a_(0) setTargetAddress 12
$a_(0) attach-vectorbasedforward $opt(width)
$a_(0) set-range 3500
$a_(0) set-target-x   -5
$a_(0) set-target-y   5
$a_(0) set-target-z   0
$a_(0) set data_rate_  $opt(data_rate)
$a_(0) set-packetsize $opt(packetsize)
$a_(0) set-mactype 1
$a_(0) set-filename test_802.data

set node_(1) [$ns_  node 1]
$god_ new_node $node_(1)
$node_(1) set X_  1000
$node_(1) set Y_  [expr 1000+$opt(distance)]
$node_(1) set Z_  0
$node_(1) set-cx  1000
$node_(1) set-cy  [expr 1000+$opt(distance)]
$node_(1) set-cz  0
$node_(1) set_next_hop  12
$node_(1) ismobile 0
$node_(1) addenergymodel [new $opt(energy) $node_(1) $opt(initialenergy)  $opt(txpower) $opt(rxpower)]
set a_(1) [new Agent/UWSink]
$ns_ attach-agent $node_(1) $a_(1)
$a_(1) setTargetAddress 12
$a_(1) attach-vectorbasedforward $opt(width)
$a_(1) set-range 3500
$a_(1) set-target-x   -5
$a_(1) set-target-y   5
$a_(1) set-target-z   0
$a_(1) set data_rate_  $opt(data_rate)
$a_(1) set data_rate_bct $opt(datarate_bct)
$a_(1) set-packetsize $opt(packetsize)
$a_(1) set-mactype 1
$a_(1) set-filename test_802.data

set node_(2) [$ns_  node 2]
$god_ new_node $node_(2)
$node_(2) set X_  1000
$node_(2) set Y_  1000
$node_(2) set Z_  0
$node_(2) set-cx  1000
$node_(2) set-cy  1000
$node_(2) set-cz  0
$node_(2) set_next_hop  12
$node_(2) ismobile 0
$node_(2) addenergymodel [new $opt(energy) $node_(2) $opt(initialenergy)  $opt(txpower) $opt(rxpower)]
set a_(2) [new Agent/UWSink]
$ns_ attach-agent $node_(2) $a_(2)
$a_(2) cmd setTargetAddress 12
$a_(2) attach-vectorbasedforward $opt(width)
$a_(2) set-range 3500
$a_(2) set-target-x   -5
$a_(2) set-target-y   5
$a_(2) set-target-z   0
$a_(2) set data_rate_  $opt(data_rate)
$a_(2) set data_rate_bct $opt(datarate_bct)
$a_(2) set-packetsize $opt(packetsize)
$a_(2) set-mactype 1
$a_(2) set-filename test_802.data

set node_(3) [$ns_  node 3]
$god_ new_node $node_(3)
$node_(3) set X_  [expr 1000+$opt(distance)]
$node_(3) set Y_  [expr 1000+2*$opt(distance)]
$node_(3) set Z_  0
$node_(3) set-cx  [expr 1000+$opt(distance)]
$node_(3) set-cy  [expr 1000+2*$opt(distance)]
$node_(3) set-cz  0
$node_(3) set_next_hop  12
$node_(3) ismobile 0
$node_(3) addenergymodel [new $opt(energy) $node_(3) $opt(initialenergy)  $opt(txpower) $opt(rxpower)]
set a_(3) [new Agent/UWSink]
$ns_ attach-agent $node_(3) $a_(3)
$a_(3) setTargetAddress 12
$a_(3) attach-vectorbasedforward $opt(width)
$a_(3) set-range 3500
$a_(3) set-target-x   2000
$a_(3) set-target-y   1000
$a_(3) set-target-z   0
$a_(3) set data_rate_  $opt(data_rate)
$a_(3) set data_rate_bct $opt(datarate_bct)
$a_(3) set-packetsize $opt(packetsize)
$a_(3) set-mactype 1
$a_(3) set-filename test_802.data

set node_(4) [$ns_  node 4]
$god_ new_node $node_(4)
$node_(4) set X_  [expr 1000+$opt(distance)]
$node_(4) set Y_  [expr 1000+$opt(distance)]
$node_(4) set Z_  0
$node_(4) set-cx  [expr 1000+$opt(distance)]
$node_(4) set-cy  [expr 1000+$opt(distance)]
$node_(4) set-cz  0
$node_(4) set_next_hop  12
$node_(4) ismobile 0
$node_(4) addenergymodel [new $opt(energy) $node_(4) $opt(initialenergy)  $opt(txpower) $opt(rxpower)]
set a_(4) [new Agent/UWSink]
$ns_ attach-agent $node_(4) $a_(4)
$a_(4) setTargetAddress 12
$a_(4) attach-vectorbasedforward $opt(width)
$a_(4) set-range 3500
$a_(4) set-target-x   -5
$a_(4) set-target-y   5
$a_(4) set-target-z   0
$a_(4) set data_rate_  $opt(data_rate)
$a_(4) set data_rate_bct $opt(datarate_bct)
$a_(4) set-packetsize $opt(packetsize)
$a_(4) set-mactype 1
$a_(4) set-filename test_802.data

set node_(5) [$ns_  node 5]
$god_ new_node $node_(5)
$node_(5) set X_  [expr 1000+$opt(distance)]
$node_(5) set Y_  1000
$node_(5) set Z_  0
$node_(5) set-cx  [expr 1000+$opt(distance)]
$node_(5) set-cy  1000
$node_(5) set-cz  0
$node_(5) set_next_hop 12
$node_(5) ismobile 0
$node_(5) addenergymodel [new $opt(energy) $node_(5) $opt(initialenergy)  $opt(txpower) $opt(rxpower)]
set a_(5) [new Agent/UWSink]
$ns_ attach-agent $node_(5) $a_(5)
$a_(5) setTargetAddress 12
$a_(5) attach-vectorbasedforward $opt(width)
$a_(5) set-range 3500
$a_(5) set data_rate_ 0.05
$a_(5) set-target-x   -5
$a_(5) set-target-y   5
$a_(5) set-target-z   0
$a_(5) set data_rate_  $opt(data_rate)
$a_(5) set data_rate_bct $opt(datarate_bct)
$a_(5) set-packetsize $opt(packetsize)
$a_(5) set-mactype 1
$a_(5) set-filename test_802.data

set node_(6) [$ns_  node 6]
$god_ new_node $node_(6)
$node_(6) set X_  [expr 1000+2*$opt(distance)]
$node_(6) set Y_  [expr 1000+2*$opt(distance)]
$node_(6) set Z_  0
$node_(6) set-cx  [expr 1000+2*$opt(distance)]
$node_(6) set-cy  [expr 1000+2*$opt(distance)]
$node_(6) set-cz  0
$node_(6) set_next_hop  13
$node_(6) ismobile 0
$node_(6) addenergymodel [new $opt(energy) $node_(6) $opt(initialenergy)  $opt(txpower) $opt(rxpower)]
set a_(6) [new Agent/UWSink]
$ns_ attach-agent $node_(6) $a_(6)
$a_(6) cmd setTargetAddress 12
$a_(6) attach-vectorbasedforward $opt(width)
$a_(6) set-range 3500
$a_(6) set-target-x   6000
$a_(6) set-target-y   1000
$a_(6) set-target-z   0
$a_(6) set data_rate_  $opt(data_rate)
$a_(6) set data_rate_bct $opt(datarate_bct)
$a_(6) set-packetsize $opt(packetsize)
$a_(6) set-mactype 1
$a_(6) set-filename test_802.data

set node_(7) [$ns_  node 7]
$god_ new_node $node_(7)
$node_(7) set X_  [expr 1000+2*$opt(distance)]
$node_(7) set Y_  [expr 1000+$opt(distance)]
$node_(7) set Z_  0
$node_(7) set-cx  [expr 1000+2*$opt(distance)]
$node_(7) set-cy  [expr 1000+$opt(distance)]
$node_(7) set-cz  0
$node_(7) set_next_hop  13
$node_(7) ismobile 0
$node_(7) addenergymodel [new $opt(energy) $node_(7) $opt(initialenergy)  $opt(txpower) $opt(rxpower)]
set a_(7) [new Agent/UWSink]
$ns_ attach-agent $node_(7) $a_(7)
$a_(7) setTargetAddress 13
$a_(7) attach-vectorbasedforward $opt(width)
$a_(7) set-range 3500
$a_(7) set-target-x   -5
$a_(7) set-target-y   5
$a_(7) set-target-z   0
$a_(7) set data_rate_  $opt(data_rate)
$a_(7) set data_rate_bct $opt(datarate_bct)
$a_(7) set-packetsize $opt(packetsize)
$a_(7) set-mactype 1
$a_(7) set-filename test_802.data

set node_(8) [$ns_  node 8]
$god_ new_node $node_(8)
$node_(8) set X_  [expr 1000+2*$opt(distance)]
$node_(8) set Y_  1000
$node_(8) set Z_  0
$node_(8) set-cx  [expr 1000+2*$opt(distance)]
$node_(8) set-cy  1000
$node_(8) set-cz  0
$node_(8) set_next_hop  13
$node_(8) ismobile 0
$node_(8) addenergymodel [new $opt(energy) $node_(8) $opt(initialenergy)  $opt(txpower) $opt(rxpower)]
set a_(8) [new Agent/UWSink]
$ns_ attach-agent $node_(8) $a_(8)
$a_(8) setTargetAddress 13
$a_(8) attach-vectorbasedforward $opt(width)
$a_(8) set-range 3500
$a_(8) set-target-x   -5
$a_(8) set-target-y   5
$a_(8) set-target-z   0
$a_(8) set data_rate_  $opt(data_rate)
$a_(8) set data_rate_bct $opt(datarate_bct)
$a_(8) set-packetsize $opt(packetsize)
$a_(8) set-mactype 1
$a_(8) set-filename test_802.data

set node_(9) [$ns_  node 9]
$god_ new_node $node_(9)
$node_(9) set X_  [expr 1000+3*$opt(distance)]
$node_(9) set Y_  [expr 1000+2*$opt(distance)]
$node_(9) set Z_  0
$node_(9) set-cx  [expr 1000+3*$opt(distance)]
$node_(9) set-cy  [expr 1000+2*$opt(distance)]
$node_(9) set-cz  0
$node_(9) set_next_hop  13
$node_(9) ismobile 0
$node_(9) addenergymodel [new $opt(energy) $node_(9) $opt(initialenergy)  $opt(txpower) $opt(rxpower)]
set a_(9) [new Agent/UWSink]
$ns_ attach-agent $node_(9) $a_(9)
$a_(9) cmd setTargetAddress 13
$a_(9) attach-vectorbasedforward $opt(width)
$a_(9) set-range 3500
$a_(9) set-target-x   -5
$a_(9) set-target-y   5
$a_(9) set-target-z   0
$a_(9) set data_rate_  $opt(data_rate)
$a_(9) set-packetsize $opt(packetsize)
$a_(9) set data_rate_bct $opt(datarate_bct)
$a_(9) set-mactype 1
$a_(9) set-filename test_802.data

set node_(10) [$ns_  node 10]
$god_ new_node $node_(10)
$node_(10) set X_  [expr 1000+3*$opt(distance)]
$node_(10) set Y_  [expr 1000+$opt(distance)]
$node_(10) set Z_  0
$node_(10) set-cx  [expr 1000+3*$opt(distance)]
$node_(10) set-cy  [expr 1000+$opt(distance)]
$node_(10) set-cz  0
$node_(10) set_next_hop  13
$node_(10) ismobile 0
$node_(10) addenergymodel [new $opt(energy) $node_(10) $opt(initialenergy)  $opt(txpower) $opt(rxpower)]
set a_(10) [new Agent/UWSink]
$ns_ attach-agent $node_(10) $a_(10)
$a_(10) setTargetAddress 13
$a_(10) attach-vectorbasedforward $opt(width)
$a_(10) set-range 3500
$a_(10) set data_rate_ 0.05
$a_(10) set-target-x   -5
$a_(10) set-target-y   5
$a_(10) set-target-z   0
$a_(10) set data_rate_  $opt(data_rate)
$a_(10) set data_rate_bct $opt(datarate_bct)
$a_(10) set-packetsize $opt(packetsize)
$a_(10) set-mactype 1
$a_(10) set-filename test_802.data

set node_(11) [$ns_  node 11]
$god_ new_node $node_(11)
$node_(11) set X_ [expr  1000+3*$opt(distance)]
$node_(11) set Y_  1000
$node_(11) set Z_  0
$node_(11) set-cx  [expr 1000+3*$opt(distance)]
$node_(11) set-cy  1000
$node_(11) set-cz  0
$node_(11) set_next_hop  13
$node_(11) ismobile 0
$node_(11) addenergymodel [new $opt(energy) $node_(11) $opt(initialenergy)  $opt(txpower) $opt(rxpower)]
set a_(11) [new Agent/UWSink]
$ns_ attach-agent $node_(11) $a_(11)
$a_(11) setTargetAddress 13
$a_(11) attach-vectorbasedforward $opt(width)
$a_(11) set-range 3500
$a_(11) set data_rate_ 0.05
$a_(11) set-target-x   -5
$a_(11) set-target-y   5
$a_(11) set-target-z   0
$a_(11) set data_rate_ $opt(data_rate)
$a_(11) set data_rate_bct $opt(datarate_bct)
$a_(11) set-packetsize $opt(packetsize)
$a_(11) set-mactype 1
$a_(11) set-filename test_802.data

#move
set node_(12) [$ns_  node 12]
$god_ new_node $node_(12)
#1750 3250
#node 3 2500 4000
$node_(12) set X_  [expr  1000+0.5*$opt(distance)]
$node_(12) set Y_  6000
$node_(12) set Z_   0
$node_(12) set-cx  [expr  1000+0.5*$opt(distance)]
$node_(12) set-cy  6000
$node_(12) set-cz  0
$node_(12) set_next_hop -1;
$node_(12) random-motion 1
$node_(12) ismobile 1
$node_(12) addenergymodel [new $opt(energy) $node_(12) $opt(initialenergy)  $opt(txpower) $opt(rxpower)]
$ns_ at 15.1 "$node_(12) setdest [expr  1000+0.5*$opt(distance)] 10 2.5"
set a_(12) [new Agent/UWSink]
$ns_ attach-agent $node_(12) $a_(12)
$a_(12) attach-vectorbasedforward $opt(width)
$a_(12) setTargetAddress -1
$a_(12) set-range 3500
$a_(12) set data_rate_ $opt(datarate_bct)
$a_(12) set data_rate_bct $opt(datarate_bct)
$a_(12) set-target-x   2400
$a_(12) set-target-y   4000
$a_(12) set-target-z   0
$a_(12) set-packetsize-bct $opt(packetsize_bct)
$a_(12) set-mactype 1
$a_(12) set-filename test_802.data

set node_(13) [$ns_  node 13]
$god_ new_node $node_(13)
$node_(13) set X_  [expr  1000+2.5*$opt(distance)]
$node_(13) set Y_  6000
$node_(13) set Z_   0
$node_(13) set-cx  [expr  1000+2.5*$opt(distance)]
$node_(13) set-cy  6000
$node_(13) set-cz  0
$node_(13) ismobile 1
$node_(13) set_next_hop -1;
$node_(13) random-motion 1
$node_(13) addenergymodel [new $opt(energy) $node_(13) $opt(initialenergy)  $opt(txpower) $opt(rxpower)]
$ns_ at 15.1 "$node_(13) setdest [expr  1000+2.5*$opt(distance)] 10 2.5"
set a_(13) [new Agent/UWSink]
$ns_ attach-agent $node_(13) $a_(13)
$a_(13) attach-vectorbasedforward $opt(width)
$a_(13) setTargetAddress -1
$a_(13) set-range 3500
$a_(13) set data_rate_ $opt(datarate_bct)
$a_(13) set data_rate_bct $opt(datarate_bct)
$a_(13) set-target-x   -5
$a_(13) set-target-y   5
$a_(13) set-target-z   0
$a_(13) set-packetsize-bct $opt(packetsize_bct)
$a_(13) set-mactype 1
$a_(13) set-filename test_802.data

set node_(14) [$ns_  node 14]
$god_ new_node $node_(14)
$node_(14) set X_  [expr  1000+1.5*$opt(distance)]
$node_(14) set Y_  10
$node_(14) set Z_   0
$node_(14) set-cx  [expr  1000+1.5*$opt(distance)]
$node_(14) set-cy  10
$node_(14) set-cz  0
$node_(14) ismobile 1
$node_(14) set_next_hop -1;
$node_(14) random-motion 1
$node_(14) addenergymodel [new $opt(energy) $node_(14) $opt(initialenergy)  $opt(txpower) $opt(rxpower)]
$ns_ at 15.1 "$node_(14) setdest [expr  1000+1.5*$opt(distance)] 6000 2.5"
set a_(14) [new Agent/UWSink]
$ns_ attach-agent $node_(14) $a_(14)
$a_(14) attach-vectorbasedforward $opt(width)
$a_(14) setTargetAddress -1
$a_(14) set-range 3500
$a_(14) set data_rate_ $opt(datarate_bct)
$a_(14) set data_rate_bct $opt(datarate_bct)
$a_(14) set-target-x   -5
$a_(14) set-target-y   5
$a_(14) set-target-z   0
$a_(14) set-packetsize-bct $opt(packetsize_bct)
$a_(14) set-mactype 1
$a_(14) set-filename test_802.data


$ns_ at 15 "$a_(14) bct-start"
$ns_ at 15 "$a_(13) bct-start"
$ns_ at 15 "$a_(12) bct-start"


set node_size 1000
for {set k 0} { $k<$opt(nn) } { incr k } {
	$ns_ initial_node_pos $node_($k) $node_size
}

puts "+++++++AFTER ANNOUNCE++++++++++++++"





$ns_ at $opt(stop).001 "$a_(0) terminate"
$ns_ at $opt(stop).001 "$a_(1) terminate"
$ns_ at $opt(stop).001 "$a_(2) terminate"
$ns_ at $opt(stop).001 "$a_(3) terminate"
$ns_ at $opt(stop).001 "$a_(4) terminate"
$ns_ at $opt(stop).001 "$a_(5) terminate"
$ns_ at $opt(stop).001 "$a_(6) terminate"
$ns_ at $opt(stop).001 "$a_(7) terminate"
$ns_ at $opt(stop).001 "$a_(8) terminate"
$ns_ at $opt(stop).001 "$a_(9) terminate"
$ns_ at $opt(stop).001 "$a_(10) terminate"
$ns_ at $opt(stop).001 "$a_(11) terminate"
$ns_ at $opt(stop).001 "$a_(12) terminate"
$ns_ at $opt(stop).001 "$a_(13) terminate"
$ns_ at $opt(stop).001 "$a_(14) terminate"
$ns_ at $opt(stop).003  "$god_ compute_energy"
$ns_ at $opt(stop).004  "$ns_ nam-end-wireless $opt(stop)"
#$ns_ at $opt(stop).005 "exec nam 802_11.nam"
$ns_ at $opt(stop).005 "puts \"NS EXISTING...\"; $ns_ halt"
 
 puts $tracefd "vectorbased"
 puts $tracefd "M 0.0 nn $opt(nn) x $opt(x) y $opt(y) z $opt(z)"
 puts $tracefd "M 0.0 prop $opt(prop) ant $opt(ant)"
 puts "starting Simulation..."
 $ns_ run


