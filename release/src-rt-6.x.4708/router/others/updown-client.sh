#!/bin/sh

#
# VPN Client up down script
#
# 2019 - 2020 by pedro
#


PID=$$
IFACE=$dev
SERVICE=$(echo $dev | sed 's/\(tun\|tap\)1/client/;s/\(tun\|tap\)2/server/')
DNSDIR="/etc/openvpn/dns"
DNSCONFFILE="$DNSDIR/$SERVICE.conf"
DNSRESOLVFILE="$DNSDIR/$SERVICE.resolv"
RESTART_DNSMASQ=0
FOREIGN_OPTIONS=$(set | grep "^foreign_option_" | sed "s/^\(.*\)=.*$/\1/g")
LOGS="logger -t openvpn-updown-client.sh[$PID][$IFACE]"


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
}

startAdns() {
	local fileexists="" optionname option

	[ "$(nvram get "vpn_"$SERVICE"_adns")" -eq 0 ] && return

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
			option=$(eval "echo \$$optionname")
			$LOGS "Optionname: $optionname, Option: $option"
			if echo $option | grep "dhcp-option WINS ";	then echo $option | sed "s/ WINS /=44,/" >> $DNSCONFFILE; fi
			if echo $option | grep "dhcp-option DNS";	then echo $option | sed "s/dhcp-option DNS/nameserver/" >> $DNSRESOLVFILE; fi
			if echo $option | grep "dhcp-option DOMAIN";	then echo $option | sed "s/dhcp-option DOMAIN/search/" >> $DNSRESOLVFILE; fi
		done
	}

	[ ! -f $DNSCONFFILE -a ! -f $DNSRESOLVFILE ] && {
		$LOGS "Warning: 'Accept DNS configuration' is enabled but no foreign options (push dhcp-option) have been received from the OpenVPN server!"
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
	[ "$RESTART_DNSMASQ" -eq 1 ] && service dnsmasq restart
}


###################################################


find_iface

[ "$script_type" == "up" ] && {
	startAdns
}

[ "$script_type" == "down" ] && {
	stopAdns
}

checkRestart

exit 0
