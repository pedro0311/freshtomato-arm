#!/bin/sh

cd /etc

cp -L openssl.cnf openssl.config

NVCN=$(nvram get https_crt_cn)
LANIP=$(nvram get lan_ipaddr)
LANHOSTNAME=$(nvram get lan_hostname)

if [ "$NVCN" == "" ]; then
	NVCN=$(nvram get router_name)
fi

I=0
for CN in $NVCN; do
	echo "$I.commonName=CN" >> openssl.config
	echo "$I.commonName_value=$CN" >> openssl.config
	echo "$I.organizationName=O" >> /etc/openssl.config
	echo "$I.organizationName_value=$(uname -o)" >> /etc/openssl.config
	I=$(($I + 1))
done

I=0
# Start of SAN extensions
sed -i "/\[ CA_default \]/acopy_extensions = copy" openssl.config
sed -i "/\[ v3_ca \]/asubjectAltName = @alt_names" openssl.config
sed -i "/\[ v3_req \]/asubjectAltName = @alt_names" openssl.config
echo "[alt_names]" >> openssl.config

# IP
echo "IP.0 = $LANIP" >> openssl.config
echo "DNS.$I = $LANIP" >> openssl.config # For broken clients like IE
I=$(($I + 1))

# hostnames
echo "DNS.$I = $LANHOSTNAME" >> openssl.config
I=$(($I + 1))

# create the key
openssl genrsa -out key.pem 2048 -config /etc/openssl.config
# create certificate request and sign it
openssl req -startdate 170101000000Z -enddate 261231235959Z -new -x509 -key key.pem -sha256 -out cert.pem -set_serial $1 -config /etc/openssl.config

# server.pem for WebDav SSL
cat key.pem cert.pem > server.pem

rm -f /tmp/cert.csr /tmp/privkey.pem openssl.config
