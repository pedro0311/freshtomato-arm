# PDU Reader
Reader and parser of 2/3/4G modems SMS storage.
Application reads an standard ETSI AT+CMGL AT command output.

This program was designed to read Huawei modems SMS inbox on embedded
systems - such as Tomato, OpenWRT etc.

Application is very simple, just build it by typing
** make
and to run, just pass output from your favorite serial application, for ex. gcom:
gcom -d /dev/ttyUSB0 -s /etc/gcom/getsmses.gcom | ./pdureader

or dump response to file, to parse it later:
gcom -d /dev/ttyUSB0 -s /etc/gcom/getsmses.gcom > pdudump.txt
./pdureader -f ./pdudump.txt

Based on Huawei AT command set.
GSM 7-bit decoding algorithm implementation borrowed from:
https://github.com/SiteView/ecc
