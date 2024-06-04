#!/bin/bash

DEFAULT_CT="../../src/conntrack"
DEFAULT_SPORT_COUNT=0xffff
DEFAULT_DPORT_COUNT=0x2
DEFAULT_TMP_FILE="./ct_data.txt"
DEFAULT_CT_ZONE=123
DEFAULT_GEN_ONLY=0
DEFAULT_CLEANUP_INDIVIDUAL=0

CT=$DEFAULT_CT
SPORT_COUNT=$DEFAULT_SPORT_COUNT
DPORT_COUNT=$DEFAULT_DPORT_COUNT
TMP_FILE=$DEFAULT_TMP_FILE
CT_ZONE=$DEFAULT_CT_ZONE
GEN_ONLY=$DEFAULT_GEN_ONLY
CLEANUP_INDIVIDUAL=$DEFAULT_CLEANUP_INDIVIDUAL

print_help()
{
	me=$(basename "$0")

	echo "Script for stress-testing bulk ct entries load (-R option)"
	echo ""
	echo "Usage: $me [options]"
	echo ""
	echo "Where options can be:"
	echo ""
	echo "-dpc <dst_port_count> -  number of destination port values."
	echo "                         Default is ${DEFAULT_DPORT_COUNT}."
	echo ""
	echo "-spc <src_port_count> -  number of source port values."
	echo "                         Default is ${DEFAULT_SPORT_COUNT}."
	echo ""
	echo "-ct <ct_tool_path>    -  path to the conntrack tool."
	echo "                         Default is ${DEFAULT_CT}."
	echo ""
	echo "-z <ct_zone>          -  ct zone to be used."
	echo "                         Default is ${DEFAULT_CT_ZONE}."
	echo ""
	echo "-f <tmp_file_name>    -  tmp file to be used to generate the ct data to."
	echo "                         Default is ${DEFAULT_TMP_FILE}."
	echo ""
	echo "-g                    -  Generate tmp file and exit."
	echo ""
	echo "-h                    -  Print this help and exit."
}

function ct_data_gen()
{
	for (( d = 1; d <= $DPORT_COUNT; d++ )) do
		for (( s = 1; s <= $SPORT_COUNT; s++ )) do
			echo "-I -w $CT_ZONE -s 1.1.1.1 -d 2.2.2.2 -p tcp --sport ${s} --dport ${d} --state LISTEN -u SEEN_REPLY -t 50"
		done
	done
}

if [ $UID -ne 0 ]
then
        echo "Run this test as root"
        exit 1
fi

while [ $# -gt 0 ]
do
	case "$1" in
	-spc)
		SPORT_COUNT=${2:-}
		if [ -z "$SPORT_COUNT" ]
		then
			echo "Source port must be specified!"
			print_help
			exit 1
		fi
		shift
		;;
	-dpc)
		DPORT_COUNT=${2:-}
		if [ -z "$DPORT_COUNT" ]
		then
			echo "Destination port must be specified!"
			print_help
			exit 1
		fi
		shift
		;;
	-ct)
		CT=${2:-}
		if [ -z "$CT" ]
		then
			echo "conntrack path must be specified!"
			print_help
			exit 1
		fi
		shift
		;;
	-z)
		CT_ZONE=${2:-}
		if [ -z "$CT_ZONE" ]
		then
			echo "ct zone must be specified!"
			print_help
			exit 1
		fi
		shift
		;;
	-f)
		TMP_FILE=${2:-}
		if [ -z "$TMP_FILE" ]
		then
			echo "Tmp file must be specified!"
			print_help
			exit 1
		fi
		shift
		;;
	-g)
		GEN_ONLY=1
		;;
	-ci)
		CLEANUP_INDIVIDUAL=1
		;;
	-h)
		print_help
		exit 1
		;;
	*)
		echo "Unknown paramerer \"$1\""
		print_help
		exit 1
		;;
	esac
	shift
done

ct_data_gen > $TMP_FILE

NUM_ENTRIES=$(cat ${TMP_FILE} | wc -l)

echo "File ${TMP_FILE} is generated, number of entries: ${NUM_ENTRIES}."

if [ "$GEN_ONLY" -eq "1" ]; then
	exit 0
fi

echo "Loading ${NUM_ENTRIES} entries from ${TMP_FILE} .."
time -p ${CT} -R $TMP_FILE

if [ "$CLEANUP_INDIVIDUAL" -eq "1" ]; then
	sed -i -e "s/-I/-D/g" -e "s/-t 50//g" $TMP_FILE

	NUM_ENTRIES=$(cat ${TMP_FILE} | wc -l)

	echo "File ${TMP_FILE} is updated, number of entries: ${NUM_ENTRIES}."

	echo "Cleaning ${NUM_ENTRIES} entries from ${TMP_FILE} .."
	time -p ${CT} -R $TMP_FILE
fi


echo "Cleaning up zone ${CT_ZONE}.."
time -p ${CT} -D -w $CT_ZONE > /dev/null
rm $TMP_FILE
