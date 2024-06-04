#!/bin/sh

#
# (C) 2009-2011 by Pablo Neira Ayuso <pneira@us.es>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#

#
# Here, you can find the variables that you have to change.
#

# enable this for debugging
LOG_DEBUG=0

# number of cluster node (must be unique, from 1 to N cluster nodes)
NODE=1

# this is the real MAC address of eth1
REAL_HWADDR1=00:18:71:68:f2:34

# this is the real MAC address of eth2
REAL_HWADDR2=00:11:0a:60:e7:32

#
# These variables MUST have the same values in both cluster nodes
#

# number of nodes that belong this cluster
TOTAL_NODES=2

# this is the cluster multicast MAC address of eth1
MC_HWADDR1=01:00:5e:00:01:01

# this is the cluster multicast MAC address of eth2
MC_HWADDR2=01:00:5e:00:01:02

# cluster IP address of eth1
ADDR1=192.168.0.5/24

# cluster IP address of eth2
ADDR2=192.168.1.5/24

# random seed for hashing
SEED=0xdeadbeef

start_cluster_address()
{
	# set cluster IP addresses
	ip a a $ADDR1 dev eth1
	ip a a $ADDR2 dev eth2
	# set cluster multicast MAC addresses
	ip maddr add $MC_HWADDR1 dev eth1
	ip maddr add $MC_HWADDR2 dev eth2
	# mangle ARP replies to include the cluster multicast MAC addresses
	arptables -I OUTPUT -o eth1 --h-length 6 \
		-j mangle --mangle-mac-s $MC_HWADDR1
	# mangle ARP request to use the original MAC address (otherwise the
	# stack drops this packet).
	arptables -I INPUT -i eth1 --h-length 6 --destination-mac \
		$MC_HWADDR1 -j mangle --mangle-mac-d $REAL_HWADDR1
	arptables -I OUTPUT -o eth2 --h-length 6 \
		-j mangle --mangle-mac-s $MC_HWADDR2
	arptables -I INPUT -i eth2 --h-length 6 --destination-mac \
		$MC_HWADDR2 -j mangle --mangle-mac-d $REAL_HWADDR2
}

stop_cluster_address()
{
	# delete cluster IP addresses
	ip a d $ADDR1 dev eth1
	ip a d $ADDR2 dev eth2
	# delete cluster multicast MAC addresses
	ip maddr del $MC_HWADDR1 dev eth1
	ip maddr del $MC_HWADDR2 dev eth2
	# delete ARP replies mangling
	arptables -D OUTPUT -o eth1 --h-length 6 \
		-j mangle --mangle-mac-s $MC_HWADDR1
	# delete ARP requests mangling
	arptables -D INPUT -i eth1 --h-length 6 --destination-mac \
		$MC_HWADDR1 -j mangle --mangle-mac-d $REAL_HWADDR1
	arptables -D OUTPUT -o eth2 --h-length 6 \
		-j mangle --mangle-mac-s $MC_HWADDR2
	arptables -D INPUT -i eth2 --h-length 6 --destination-mac \
		$MC_HWADDR2 -j mangle --mangle-mac-d $REAL_HWADDR2
}

start_nat()
{
	iptables -A POSTROUTING -t nat -s 192.168.0.11 \
		-j SNAT --to-source 192.168.1.5
	iptables -A POSTROUTING -t nat -s 192.168.0.2 \
		-j SNAT --to-source 192.168.1.5
}

stop_nat()
{
	iptables -D POSTROUTING -t nat -s 192.168.0.11 \
		-j SNAT --to-source 192.168.1.5
	iptables -D POSTROUTING -t nat -s 192.168.0.2 \
		-j SNAT --to-source 192.168.1.5
}

iptables_start_cluster_rules()
{
	# mark packets that belong to this node (go direction)
	iptables -A CLUSTER-RULES -t mangle -i eth1 -m cluster \
		--cluster-total-nodes $TOTAL_NODES --cluster-local-node $1 \
		--cluster-hash-seed $SEED -j MARK --set-mark 0xffff

	# mark packet that belong to this node (reply direction)
	# note: we *do* need this to change the packet type to PACKET_HOST,
	# otherwise the stack silently drops the packet.
	iptables -A CLUSTER-RULES -t mangle -i eth2 -m cluster \
		--cluster-total-nodes $TOTAL_NODES --cluster-local-node $1 \
		--cluster-hash-seed $SEED -j MARK --set-mark 0xffff
}

iptables_stop_cluster_rules()
{
	iptables -D CLUSTER-RULES -t mangle -i eth1 -m cluster \
		--cluster-total-nodes $TOTAL_NODES --cluster-local-node $1 \
		--cluster-hash-seed $SEED -j MARK --set-mark 0xffff

	iptables -D CLUSTER-RULES -t mangle -i eth2 -m cluster \
		--cluster-total-nodes $TOTAL_NODES --cluster-local-node $1 \
		--cluster-hash-seed $SEED -j MARK --set-mark 0xffff
}

start_cluster_ruleset() {
	iptables -N CLUSTER-RULES -t mangle

	iptables_start_cluster_rules $NODE

	iptables -A PREROUTING -t mangle -j CLUSTER-RULES

	if [ $LOG_DEBUG -eq 1 ]
	then
		iptables -A PREROUTING -t mangle -i eth1 -m mark \
			--mark 0xffff -j LOG --log-prefix "cluster-accept: "
		iptables -A PREROUTING -t mangle -i eth1 -m mark \
			! --mark 0xffff -j LOG --log-prefix "cluster-drop: "
		iptables -A PREROUTING -t mangle -i eth2 -m mark \
			--mark 0xffff \
			-j LOG --log-prefix "cluster-reply-accept: "
		iptables -A PREROUTING -t mangle -i eth2 -m mark \
			! --mark 0xffff \
			-j LOG --log-prefix "cluster-reply-drop: "
	fi

	# drop packets that don't belong to us (go direction)
	iptables -A PREROUTING -t mangle -i eth1 -m mark \
		! --mark 0xffff -j DROP

	# drop packets that don't belong to us (reply direction)
	iptables -A PREROUTING -t mangle -i eth2 -m mark \
		! --mark 0xffff -j DROP
}

stop_cluster_ruleset() {
	iptables -D PREROUTING -t mangle -j CLUSTER-RULES

	if [ $LOG_DEBUG -eq 1 ]
	then
		iptables -D PREROUTING -t mangle -i eth1 -m mark \
			--mark 0xffff -j LOG --log-prefix "cluster-accept: "
		iptables -D PREROUTING -t mangle -i eth1 -m mark \
			! --mark 0xffff -j LOG --log-prefix "cluster-drop: "
		iptables -D PREROUTING -t mangle -i eth2 -m mark \
			--mark 0xffff \
			-j LOG --log-prefix "cluster-reply-accept: "
		iptables -D PREROUTING -t mangle -i eth2 -m mark \
			! --mark 0xffff \
			-j LOG --log-prefix "cluster-reply-drop: "
	fi

	iptables -D PREROUTING -t mangle -i eth1 -m mark \
		! --mark 0xffff -j DROP

	iptables -D PREROUTING -t mangle -i eth2 -m mark \
		! --mark 0xffff -j DROP

	iptables_stop_cluster_rules $NODE

	iptables -F CLUSTER-RULES -t mangle
	iptables -X CLUSTER-RULES -t mangle
}

case "$1" in
start)
	echo "starting cluster configuration for node $NODE."

	# just in case that you forget it
	echo 1 > /proc/sys/net/ipv4/ip_forward

	# disable TCP pickup
	echo 0 > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_be_liberal
	echo 0 > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_loose

	start_cluster_address
	start_nat

	# drop invalid flows from eth2 (not allowed). This is mandatory
	# because traffic which does not belong to this node is always
	# labeled as INVALID by TCP and ICMP state tracking. For protocols like
	# UDP, you will have to drop NEW traffic from eth2, otherwise reply
	# traffic may be accepted by both nodes, thus duplicating the traffic.
	iptables -A PREROUTING -t mangle -i eth2 \
		-m state --state INVALID -j DROP

	start_cluster_ruleset
	;;
stop)
	echo "stopping cluster configuration for node $NODE."

	stop_cluster_address
	stop_nat

	iptables -D PREROUTING -t mangle -i eth2 \
		-m state --state INVALID -j DROP

	stop_cluster_ruleset
	;;
primary)
	logger "cluster-match-script: entering MASTER state for node $2"
	if [ -x $CONNTRACKD_SCRIPT ]
	then
		sh $CONNTRACKD_SCRIPT primary $NODE $2
	fi
	iptables_start_cluster_rules $2
	;;
backup)
	logger "cluster-match-script: entering BACKUP state for node $2"
	if [ -x $CONNTRACKD_SCRIPT ]
	then
		sh $CONNTRACKD_SCRIPT backup $NODE $2
	fi
	iptables_stop_cluster_rules $2
	;;
fault)
	logger "cluster-match-script: entering FAULT state for node $2"
	if [ -x $CONNTRACKD_SCRIPT ]
	then
		sh $CONNTRACKD_SCRIPT fault $NODE $2
	fi
	iptables_stop_cluster_rules $2
	;;
*)
	echo "$0 start|stop|add|del [nodeid]"
	;;
esac
