#!/bin/sh

PID=$$
PIDFILE="/var/run/gencert.pid"
WAITTIMER=0

while [ -f "$PIDFILE" -a $WAITTIMER -lt 14 ]; do
	WAITTIMER=$((WAITTIMER+2))
	sleep $WAITTIMER
done
touch $PIDFILE

OPENSSL=/usr/sbin/openssl

LANCN=$(nvram get https_crt_cn)
LANIP=$(nvram get lan_ipaddr)
LANHOSTNAME=$(nvram get lan_hostname)
KEYNAME="key.pem"
CERTNAME="cert.pem"
OPENSSLCNF="/etc/openssl.config.$PID"

[ "$(date +%s)" -gt 946684800 ] && {
	nvram set https_crt_timeset=1
	DAYS=398
} || {
	nvram set https_crt_timeset=0
	DAYS=3653
}

cd /etc

cp -L /etc/ssl/openssl.cnf $OPENSSLCNF

[ "$LANCN" != "" ] && {
	I=0
	for CN in $LANCN; do
		echo "$I.commonName=CN" >> $OPENSSLCNF
		echo "$I.commonName_value=$CN" >> $OPENSSLCNF
		echo "$I.organizationName=O" >> $OPENSSLCNF
		echo "$I.organizationName_value=FreshTomato" >> $OPENSSLCNF
		echo "$I.organizationalUnitName=OU" >> $OPENSSLCNF
		echo "$I.organizationalUnitName_value=FreshTomato Team" >> $OPENSSLCNF
		echo "$I.emailAddress=E" >> $OPENSSLCNF
		echo "$I.emailAddress_value=root@localhost" >> $OPENSSLCNF
		I=$(($I + 1))
	done
} || {
	LANRN=$(nvram get router_name)
	[ "$LANRN" == "" ] && {
		LANRN=$LANIP
	} || {
		RAND=$(printf '%s' $RANDOM | md5sum | cut -c -8)
		LANRN=$LANRN"-"$RAND	# fix problems in FF
	}
	echo "0.commonName=CN" >> $OPENSSLCNF
	echo "0.commonName_value=$LANRN" >> $OPENSSLCNF
	echo "0.organizationName=O" >> $OPENSSLCNF
	echo "0.organizationName_value=FreshTomato" >> $OPENSSLCNF
	echo "0.organizationalUnitName=OU" >> $OPENSSLCNF
	echo "0.organizationalUnitName_value=FreshTomato Team" >> $OPENSSLCNF
	echo "0.emailAddress=E" >> $OPENSSLCNF
	echo "0.emailAddress_value=root@localhost" >> $OPENSSLCNF
}

# Required extension
sed -i "/\[ v3_ca \]/aextendedKeyUsage = serverAuth" $OPENSSLCNF

I=0
# Start of SAN extensions
sed -i "/\[ CA_default \]/acopy_extensions = copy" $OPENSSLCNF
sed -i "/\[ v3_ca \]/asubjectAltName = @alt_names" $OPENSSLCNF
sed -i "/\[ v3_req \]/asubjectAltName = @alt_names" $OPENSSLCNF
echo "[alt_names]" >> $OPENSSLCNF

# IP
echo "IP.0 = $LANIP" >> $OPENSSLCNF
echo "DNS.$I = $LANIP" >> $OPENSSLCNF # For broken clients like IE
I=$(($I + 1))

# User-defined CN (if we have any)
[ "$LANCN" != "" ] && {
	for CN in $LANCN; do
		echo "DNS.$I = $CN" >> $OPENSSLCNF
		I=$(($I + 1))
	done
}

# hostnames
[ "$LANHOSTNAME" != "" ] && {
	echo "DNS.$I = $LANHOSTNAME" >> $OPENSSLCNF
	I=$(($I + 1))
}

# create the key
$OPENSSL genpkey -out $KEYNAME.$PID -algorithm rsa -pkeyopt rsa_keygen_bits:2048
# create certificate request and sign it
$OPENSSL req -days $DAYS -new -x509 -key $KEYNAME.$PID -sha256 -out $CERTNAME.$PID -set_serial $1 -config $OPENSSLCNF

# server.pem for WebDav SSL
cat $KEYNAME.$PID $CERTNAME.$PID > server.pem

mv $KEYNAME.$PID $KEYNAME
mv $CERTNAME.$PID $CERTNAME

chmod 640 $KEYNAME
chmod 640 $CERTNAME

rm -f /tmp/privkey.pem.$PID $OPENSSLCNF $PIDFILE
