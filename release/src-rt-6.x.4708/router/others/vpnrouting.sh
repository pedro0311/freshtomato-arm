#!/bin/sh

#
# VPN Client selective routing up down script
#
# Copyright by pedro 2019
#


VPN_GW="$route_vpn_gateway"
PID=$$
IFACE=$dev
SERVICE=$(echo $dev | sed 's/\(tun\|tap\)1/client/;s/\(tun\|tap\)2/server/')
FIREWALL_ROUTING="/etc/openvpn/fw/$SERVICE-fw-routing.sh"
DNSMASQ_IPSET="/etc/dnsmasq.ipset"
RESTART_DNSMASQ=0
LOGS="logger -t openvpn-vpnrouting.sh[$PID][$IFACE]"
[ -d /etc/openvpn/fw ] || mkdir -m 0700 "/etc/openvpn/fw"


NV() {
	nvram get "$1"
}

find_iface() {
	if [ "$SERVICE" == "client1" ]; then
		ID="311"
	elif [ "$SERVICE" == "client2" ]; then
		ID="312"
	elif [ "$SERVICE" == "client3" ]; then
		ID="313"
	else
		$LOGS "Interface not found!"
		exit 0
	fi

	PIDFILE="/var/run/vpnrouting$ID.pid"
}

deleteRules() {
	sed -i 's/-A/-D/g;s/-I/-D/g' "$1"
	$1
}

cleanupRouting() {
	$LOGS "Clean-up routing"

	ip route flush table $ID
	ip route flush cache

	[ "$(ip rule | grep "lookup $ID" | wc -l)" -gt 0 ] && {
		ip rule del fwmark $ID table $ID
	}

	deleteRules $FIREWALL_ROUTING
	rm -f $FIREWALL_ROUTING > /dev/null 2>&1

	ipset destroy vpnrouting$ID
	sed -i $DNSMASQ_IPSET -e "/vpnrouting$ID/d"
}

startRouting() {
	local DNSMASQ=0 VAL1 VAL2 VAL3 ROUTE

	cleanupRouting
	nvram set vpn_client"${ID#??}"_rdnsmasq=0

	$LOGS "Starting routing policy for VPN $SERVICE - Interface $IFACE - Table $ID - GW $VPN_GW"

	ip route add table $ID default via $VPN_GW dev $IFACE
	ip rule add fwmark $ID table $ID priority 90

	# copy routes from main routing table (exclude vpns and default gateway)
	ip route | grep -Ev 'tun11|tun12|tun13|^default ' | while read ROUTE; do
		ip route add table $ID $ROUTE
	done

	modprobe xt_set
	modprobe ip_set
	modprobe ip_set_hash_ip
	ipset create vpnrouting$ID hash:ip

	echo "#!/bin/sh" > $FIREWALL_ROUTING
	echo "echo 0 > /proc/sys/net/ipv4/conf/$IFACE/rp_filter" >> $FIREWALL_ROUTING
	echo "echo 0 > /proc/sys/net/ipv4/conf/all/rp_filter" >> $FIREWALL_ROUTING
	echo "iptables -t mangle -A PREROUTING -m set --match-set vpnrouting$ID dst,src -j MARK --set-mark $ID" >> $FIREWALL_ROUTING

	# example of routing_val: 1<2<8.8.8.8<1>1<1<1.2.3.4<0>1<3<domain.com<0>
	for i in $(echo "$(NV vpn_"$SERVICE"_routing_val)" | tr ">" "\n"); do
		VAL1=$(echo $i | cut -d "<" -f1)
		VAL2=$(echo $i | cut -d "<" -f2)
		VAL3=$(echo $i | cut -d "<" -f3)

		# only if rule is enabled
		[ "$VAL1" -eq 1 ] && {
			case "$VAL2" in
				1)	# from source
					$LOGS "Type: $VAL2 - add $VAL3"
					echo "iptables -t mangle -A PREROUTING -s $VAL3 -j MARK --set-mark $ID" >> $FIREWALL_ROUTING
				;;
				2)	# to destination
					$LOGS "Type: $VAL2 - add $VAL3"
					echo "iptables -t mangle -A PREROUTING -d $VAL3 -j MARK --set-mark $ID" >> $FIREWALL_ROUTING
				;;
				3)	# to domain
					$LOGS "Type: $VAL2 - add $VAL3"
					echo "ipset=/$VAL3/vpnrouting$ID" >> $DNSMASQ_IPSET
					# try to add ipset rule using forced query to DNS server
					nslookup $VAL3 127.0.0.1 > /dev/null

					DNSMASQ=1
				;;
				*) continue ;;
			esac
		}
	done

	chmod +x $FIREWALL_ROUTING
	service firewall restart

	[ "$DNSMASQ" -eq 1 ] && {
		nvram set vpn_client"${ID#??}"_rdnsmasq=1
		RESTART_DNSMASQ=1
	}

	$LOGS "Completed routing policy configuration for $SERVICE"
}

checkRestart() {
	[ "$RESTART_DNSMASQ" -eq 1 -o "$(NV "vpn_client"${ID#??}"_rdnsmasq")" -eq 1 ] && service dnsmasq restart
}

checkPid() {
	local PIDNO

	[ -f $PIDFILE ] && {
		PIDNO=$(cat $PIDFILE)
		cat "/proc/$PIDNO/cmdline" > /dev/null 2>&1

		[ $? -eq 0 ] && {
			# priority has the last process
			$LOGS "Killing previous process ..."
			kill -9 $PIDNO
			echo $PID > $PIDFILE

			[ $? -ne 0 ] && {
				$LOGS "Could not create PID file"
				exit 0
			}
		} || {
			# process not found assume not running
			echo $PID > $PIDFILE
			[ $? -ne 0 ] && {
				$LOGS "Could not create PID file"
				exit 0
			}
		}
	} || {
		echo $PID > $PIDFILE
		[ $? -ne 0 ] && {
			$LOGS "Could not create PID file"
			exit 0
		}
	}
}


###################################################


find_iface
checkPid

[ "$script_type" == "route-up" -a "$(NV vpn_"$SERVICE"_rgw)" -lt 2 ] && {
	$LOGS "Skipping, $SERVICE not in routing policy mode"
	checkRestart
	exit 0
}

[ "$script_type" == "route-pre-down" ] && {
	cleanupRouting
}

[ "$script_type" == "route-up" ] && {
	startRouting
}

checkRestart

ip route flush cache

rm -f $PIDFILE > /dev/null 2>&1

exit 0
