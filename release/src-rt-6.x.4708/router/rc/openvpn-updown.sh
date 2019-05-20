#!/bin/sh

#
# VPN Client up down script
#
# Thanks to Phil Wiggum <p1mail2015@mail.com> for general idea
#
# Edited/corrected/rewritten/tested by pedro - 2019
#
# Environmental Variables
# ref: # ref: https://openvpn.net/community-resources/reference-manual-for-openvpn-2-4/ (Scripting and Environmental Variables)
#


VPN_GW="$route_vpn_gateway"
PID=$$
IFACE=$dev
SERVICE=$(echo $dev | sed 's/\(tun\|tap\)1/client/;s/\(tun\|tap\)2/server/')
FIREWALL_ROUTING="/etc/openvpn/fw/$SERVICE-fw-routing.sh"
FIREWALL_VPN="/etc/openvpn/fw/$SERVICE-fw.sh"
DNSDIR="/etc/openvpn/dns"
DNSCONFFILE="$DNSDIR/$SERVICE.conf"
DNSRESOLVFILE="$DNSDIR/$SERVICE.resolv"
DNSMASQ_IPSET="/etc/dnsmasq.ipset"
RESTART_DNSMASQ=0
FOREIGN_OPTIONS=$(set | grep "^foreign_option_" | sed "s/^\(.*\)=.*$/\1/g")
LOGS="logger -t openvpn-updown.sh[$PID][$IFACE]"
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
	ip rule add fwmark $ID table $ID priority 1000

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
	$LOGS "Running firewall routing rules for $SERVICE"
	$FIREWALL_ROUTING
	$LOGS "Done running firewall routing rules for $SERVICE"

	[ "$DNSMASQ" -eq 1 ] && {
		nvram set vpn_client"${ID#??}"_rdnsmasq=1
		RESTART_DNSMASQ=1
	}

	$LOGS "Completed routing policy configuration for $SERVICE"
}

stopRouting() {
	cleanupRouting
}

startFirewall() {
	local ip_mask INBOUND="DROP" i

	[ "$(NV vpn_"$SERVICE"_firewall)" == "custom" ] && return
	[ "$(NV vpn_"$SERVICE"_fw)" -eq 0 ] && INBOUND="ACCEPT"

	$LOGS "Creating firewall rules for $SERVICE"
	echo "#!/bin/sh" > $FIREWALL_VPN
	echo "iptables -I INPUT -i $IFACE -m state --state NEW -j $INBOUND" >> $FIREWALL_VPN
	echo "iptables -I FORWARD -i $IFACE -m state --state NEW -j $INBOUND" >> $FIREWALL_VPN

	[ -n "$ifconfig_ipv6_remote" ] && {
		$LOGS "startFirewall(): ifconfig_ipv6_remote=$ifconfig_ipv6_remote for $SERVICE"

		echo "ip6tables -I INPUT -i $IFACE -m state --state NEW -j $INBOUND" >> $FIREWALL_VPN
		echo "ip6tables -I FORWARD -i $IFACE -m state --state NEW -j $INBOUND" >> $FIREWALL_VPN
	}

	[ "$(NV vpn_"$SERVICE"_nat)" -eq 1 ] && ([ "$(NV vpn_"$SERVICE"_if)" == "tun" ] || [ "$(NV vpn_"$SERVICE"_bridge)" -eq 0 ]) && {
		ip_mask="$(NV 'lan_ipaddr')"/"$(NV 'lan_netmask')"

		# Add the nat for the main lan addresses
		echo "iptables -t nat -I POSTROUTING -s $ip_mask -o $IFACE -j MASQUERADE" >> $FIREWALL_VPN

		# Add the nat for other bridges, too
		for i in 1 2 3; do
			ip_mask="$(NV lan"$i"_ipaddr)/$(NV lan"$i"_netmask)"
			expr "$ip_mask" : '[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\/[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*$' > /dev/null || continue

			echo "iptables -t nat -I POSTROUTING -s $ip_mask -o $IFACE -j MASQUERADE" >> $FIREWALL_VPN
		done
	}

	chmod +x $FIREWALL_VPN
	$LOGS "Running firewall rules for $SERVICE"
	$FIREWALL_VPN
}

stopFirewall() {
	deleteRules $FIREWALL_VPN
	rm -f $FIREWALL_VPN > /dev/null 2>&1
}

startAdns() {
	local fileexists="" optionname option

	[ "$(NV "vpn_"$SERVICE"_adns")" -eq 0 ] && return

	[ ! -d $DNSDIR ] && mkdir $DNSDIR
	[ -f $DNSCONFFILE ] && {
		rm $DNSCONFFILE
		fileexists=1
	}
	[ -f $DNSRESOLVFILE ] && {
		rm $DNSRESOLVFILE
		fileexists=1
	}

	[ -n "$FOREIGN_OPTIONS" ] & {
		$LOGS "FOREIGN_OPTIONS: $FOREIGN_OPTIONS"
		for optionname in $FOREIGN_OPTIONS; do
			option=$(eval "echo \\$$optionname")
			$LOGS "Optionname: $optionname, Option: $option"
			if echo $option | grep "dhcp-option WINS ";	then echo $option | sed "s/ WINS /=44,/" >> $DNSCONFFILE; fi
			if echo $option | grep "dhcp-option DNS";	then echo $option | sed "s/dhcp-option DNS/nameserver/" >> $DNSRESOLVFILE; fi
			if echo $option | grep "dhcp-option DOMAIN";	then echo $option | sed "s/dhcp-option DOMAIN/search/" >> $DNSRESOLVFILE; fi
		done
	}

	[ -f $DNSCONFFILE -o -f $DNSRESOLVFILE -o -n "$fileexists" ] && RESTART_DNSMASQ=1
}

stopAdns() {
	local fileexists=""

	[ -f $DNSCONFFILE ] && {
		rm $DNSCONFFILE
		fileexists=1
	}
	[ -f $DNSRESOLVFILE ] && {
		rm $DNSRESOLVFILE
		fileexists=1
	}

	[ -n "$fileexists" ] && RESTART_DNSMASQ=1
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


if [ "$script_type" == "up" ]; then
	find_iface

	checkPid
	startAdns
	startFirewall
	startRouting
	checkRestart

elif [ "$script_type" == "down" ]; then
	find_iface

	checkPid
	stopAdns
	stopFirewall
	stopRouting
	checkRestart

else
	$LOGS "Unsupported command"
	exit 0
fi


rm -f $PIDFILE > /dev/null 2>&1
