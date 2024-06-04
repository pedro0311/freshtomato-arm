#!/bin/bash

SPORT_COUNT=128
DPORT_COUNT=128

function ct_data_gen()
{
	for (( d = 1; d <= $DPORT_COUNT; d++ )) do
		for (( s = 1; s <= $SPORT_COUNT; s++ )) do
			ip netns exec ct-ns-test conntrack -I -s 1.1.1.1 -d 2.2.2.2 -p tcp --sport ${s} --dport ${d} --state LISTEN -u SEEN_REPLY -t 300 &> /dev/null
			if [ $? -ne 0 ]
			then
				echo "[FAILED] cannot insert conntrack entries"
				exit 1
			fi
		done
	done
}

ip netns add ct-ns-test

if [ $UID -ne 0 ]
then
	echo "Run this test as root"
	exit 1
fi

echo "Creating conntrack entries, please wait..."
ct_data_gen
ip netns exec ct-ns-test conntrack -U -p tcp -m 1
if [ $? -ne 0 ]
then
	echo "[FAILED] cannot update conntrack entries"
	exit 1
fi

COUNT=`ip netns exec ct-ns-test conntrack -L | wc -l`
if [ $COUNT -ne 16384 ]
then
	echo "$COUNT entries, expecting 131072"
	exit 1
fi

ip netns exec ct-ns-test conntrack -F
if [ $? -ne 0 ]
then
	echo "[FAILED] faild to flush conntrack entries"
	exit 1
fi

COUNT=`ip netns exec ct-ns-test conntrack -L | wc -l`
if [ $COUNT -ne 0 ]
then
	echo "$COUNT entries, expecting 0"
	exit 1
fi

ip netns del ct-ns-test

echo "[OK] test successful"

exit 0
