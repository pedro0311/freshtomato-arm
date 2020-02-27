/*
 * mysql.c
 *
 * Copyright (C) 2014 bwq518, Hyzoom
 *
 */
#include <stdlib.h>
#include <rc.h>
#include <shutils.h>
#include <utils.h>
#include <syslog.h>
#include <sys/stat.h>
#include <shared.h>

void start_mysql(void)
{
	FILE *fp;
	char pbi[128];
	char ppr[64];
	char pdatadir[256], ptmpdir[256];
	char full_datadir[256], full_tmpdir[256], basedir[256];
	char tmp1[256], tmp2[256];

	/* make sure its really stop */
	stop_mysql();

	/* only if enabled... */
	if (!nvram_match("mysql_enable", "1")) return;

	if (nvram_match( "mysql_binary", "internal")) {
		strcpy(pbi, "/usr/bin");
	}
	else if (nvram_match("mysql_binary", "optware")) {
		strcpy(pbi, "/opt/bin");
	}
	else {
		strcpy(pbi, nvram_safe_get("mysql_binary_custom"));
	}

	if (pbi[strlen(pbi)-1] =='/') pbi[strlen(pbi) - 1] ='\0';
	splitpath(pbi, basedir, tmp1);

	/* Generate download saved path based on USB partition (mysql_dlroot) and directory name (mysql_datadir) */
	if (nvram_match("mysql_usb_enable", "1")) {
		tmp1[0] = 0;
		tmp2[0] = 0;
		strcpy(tmp1,nvram_safe_get("mysql_dlroot"));
		trimstr(tmp1);
		if (tmp1[0] != '/') {
			sprintf(tmp2, "/%s", tmp1);
			strcpy(tmp1,tmp2);
		}
		strcpy(ppr, tmp1);
		if (ppr[strlen(ppr)-1] == '/') {
			ppr[strlen(ppr)-1] = 0;
		}
		if (strlen(ppr) == 0) {
			syslog(LOG_ERR, "Found no mounted USB partition. You must mount a USB disk first.");
			return;
		}
	}
	else {
		ppr[0] = '\0';
	}

	strcpy(pdatadir, nvram_safe_get("mysql_datadir"));
	trimstr(pdatadir);
	if (pdatadir[strlen(pdatadir)-1] == '/') {
		pdatadir[strlen(pdatadir)-1] = 0;
	}
	if (strlen(pdatadir) == 0) {
		strcpy (pdatadir, "data");
		nvram_set("mysql_dir", "data");
	}
	if (pdatadir[0] == '/') {
		sprintf(full_datadir, "%s%s", ppr, pdatadir);
	}
	else {
		sprintf(full_datadir, "%s/%s", ppr, pdatadir);
	}

	strcpy(ptmpdir, nvram_safe_get("mysql_tmpdir"));
	trimstr(ptmpdir);
	if (ptmpdir[strlen(ptmpdir)-1] == '/') {
		ptmpdir[strlen(ptmpdir)-1] = 0;
	}
	if (strlen(ptmpdir) == 0) {
		strcpy (ptmpdir, "tmp");
		nvram_set("mysql_tmpdir", "tmp");
	}
	if (ptmpdir[0] == '/') {
		sprintf(full_tmpdir, "%s%s", ppr, ptmpdir);
	}
	else {
		sprintf(full_tmpdir, "%s/%s", ppr, ptmpdir);
	}

	/* config file  /etc/my.cnf */
	if (!(fp = fopen("/etc/my.cnf", "w"))) {
		syslog(LOG_ERR, "Can not open /etc/my.cnf for writing.");
		return;
	}
	fprintf(fp,
		"[client]\n"
		"port            = %s\n"
		"socket          = /var/run/mysqld.sock\n\n"
		"[mysqld]\n"
		"user            = root\n"
		"socket          = /var/run/mysqld.sock\n"
		"port            = %s\n"
		"basedir         = %s\n\n"
		"datadir         = %s\n"
		"tmpdir          = %s\n\n"
		"skip-external-locking\n",
		nvram_safe_get("mysql_port"),
		nvram_safe_get("mysql_port"),
		basedir,
		full_datadir,
		full_tmpdir);

	if (nvram_match("mysql_allow_anyhost", "1"))
		fprintf(fp, "bind-address            = 0.0.0.0\n");
	else
		fprintf(fp, "bind-address            = 127.0.0.1\n");

	fprintf(fp,
		"key_buffer_size         = %sM\n"
		"max_allowed_packet      = %sM\n"
		"thread_stack            = %sK\n"
		"thread_cache_size       = %s\n\n"
		"table_open_cache        = %s\n"
		"sort_buffer_size        = %sK\n"
		"read_buffer_size        = %sK\n"
		"read_rnd_buffer_size    = %sK\n"
		"query_cache_size        = %sM\n"
		"max_connections         = %s\n"
		"#The following items are from mysql_server_custom\n"
		"%s\n"
		"#end of mysql_server_custom\n\n"
		"[mysqldump]\n"
		"quick\nquote-names\n"
		"max_allowed_packet      = 16M\n"
		"[mysql]\n\n"
		"[isamchk]\n"
		"key_buffer_size         = 8M\n"
		"sort_buffer_size        = 8M\n\n",
		nvram_safe_get("mysql_key_buffer"),
		nvram_safe_get("mysql_max_allowed_packet"),
		nvram_safe_get("mysql_thread_stack"),
		nvram_safe_get("mysql_thread_cache_size"),
		nvram_safe_get("mysql_table_open_cache"),
		nvram_safe_get("mysql_sort_buffer_size"),
		nvram_safe_get("mysql_read_buffer_size"),
		nvram_safe_get("mysql_read_rnd_buffer_size"),
		nvram_safe_get("mysql_query_cache_size"),
		nvram_safe_get("mysql_max_connections"),
		nvram_safe_get("mysql_server_custom"));
	fclose(fp);

	/* start file */
	if(!(fp = fopen( "/tmp/start_mysql.sh", "w"))) {
		syslog(LOG_ERR, "Can not open /tmp/start_mysql.sh for writing.");
		return;
	}

	fprintf(fp,
		"#!/bin/sh\n\n"
		"BINPATH=%s\n"
		"PID=/var/run/mysqld.pid\n"
		"NEW_INSTALL=0\n"
		"MYLOG=/var/log/mysql.log\n"
		"ROOTNAME=`nvram get mysql_username`\n"
		"ROOTPASS=`nvram get mysql_passwd`\n"
		"NGINX_DOCROOT=`nvram get nginx_docroot`\n"
		"ANYHOST=`nvram get mysql_allow_anyhost`\n"
		"alias elog=\"logger -t mysql -s\"\n"
		"sleep %s\n"
		"rm -f $MYLOG\n"
		"touch $MYLOG\n"
		"if [ ! -d \"%s\" ]; then\n"
		"  elog \"datadir in /etc/my.cnf doesn't exist. Creating ...\"\n"
		"  mkdir -p %s\n"
		"  if [ -d \"%s\" ]; then\n"
		"    elog \"Created successfully.\"\n"
		"  else\n"
		"    elog \"Created failed. exit.\"\n"
		"    exit 1\n"
		"  fi\n"
		"fi\n"
		"if [ ! -d \"%s\" ]; then\n"
		"  elog \"tmpdir in /etc/my.cnf doesn't exist. creating ...\"\n"
		"  mkdir -p %s\n"
		"  if [ -d \"%s\" ]; then\n"
		"    elog \"Created successfully.\"\n"
		"  else\n"
		"    elog \"Created failed. exit.\"\n"
		"    exit 1\n"
		"  fi\n"
		"fi\n"
		"if [ ! -f \"%s/mysql/tables_priv.MYD\" ]; then\n"
		"  NEW_INSTALL=1\n"
		"  echo \"=========Found NO tables_priv.MYD====================\" >> $MYLOG\n"
		"  echo \"This is new installed MySQL.\" >> $MYLOG\n"
		"fi\n"
		"REINIT_PRIV_TABLES=`nvram get mysql_init_priv`\n"
		"if [[ $REINIT_PRIV_TABLES -eq 1 || $NEW_INSTALL -eq 1 ]]; then\n"
		"  echo \"=========mysql_install_db====================\" >> $MYLOG\n"
		"  $BINPATH/mysql_install_db --user=root --force >> $MYLOG 2>&1\n"
		"  elog \"Privileges table was already initialized.\"\n"
		"  nvram set mysql_init_priv=0\n"
		"  nvram commit\n"
		"fi\n"
		"REINIT_ROOT_PASSWD=`nvram get mysql_init_rootpass`\n"
		"if [[ $REINIT_ROOT_PASSWD -eq 1 || $NEW_INSTALL -eq 1 ]]; then\n"
		"  echo \"=========mysqld skip-grant-tables==================\" >> $MYLOG\n"
		"  nohup $BINPATH/mysqld --skip-grant-tables --skip-networking --pid-file=$PID >> $MYLOG 2>&1 &\n"
		"  sleep 2\n"
		"  [ -f /tmp/setpasswd.sql ] && rm -f /tmp/setpasswd.sql\n"
		"  echo \"use mysql;\" > /tmp/setpasswd.sql\n"
		"  echo \"update user set password=password('$ROOTPASS') where user='root';\" >> /tmp/setpasswd.sql\n"
		"  echo \"flush privileges;\" >> /tmp/setpasswd.sql\n"
		"  echo \"=========mysql < /tmp/setpasswd.sql====================\" >> $MYLOG\n"
		"  $BINPATH/mysql < /tmp/setpasswd.sql >> $MYLOG 2>&1\n"
		"  echo \"=========mysqldadmin shutdown====================\" >> $MYLOG\n"
		"  $BINPATH/mysqladmin -uroot -p\"$ROOTPASS\" --shutdown_timeout=3 shutdown >> $MYLOG 2>&1\n"
		"  killall mysqld\n"
		"  rm -f $PID /tmp/setpasswd.sql\n"
		"  nvram set mysql_init_rootpass=0\n"
		"  nvram commit\n"
		"  elog \"root password was already re-initialized.\"\n"
		"fi\n\n"
		"echo \"=========mysqld startup====================\" >> $MYLOG\n"
		"nohup $BINPATH/mysqld --pid-file=$PID >> $MYLOG 2>&1 &\n"
		"if [ $ANYHOST -eq 1 ]; then\n"
		"  sleep 3\n"
		"  [ -f /tmp/setanyhost.sql ] && rm -f /tmp/setanyhost.sql\n"
		"  echo \"GRANT ALL PRIVILEGES ON *.* TO 'root'@'%%' WITH GRANT OPTION;\" >> /tmp/setanyhost.sql\n"
		"  echo \"flush privileges;\" >> /tmp/setanyhost.sql\n"
		"  echo \"=========mysql < /tmp/setanyhost.sql====================\" >> $MYLOG\n"
		"  $BINPATH/mysql -uroot -p\"$ROOTPASS\" < /tmp/setanyhost.sql >> $MYLOG 2>&1\n"
		"fi\n"
		"/usr/bin/mycheck addcru\n"
		"elog \"MySQL successfully started.\"\n"
		"mkdir -p $NGINX_DOCROOT\n"
		"cp -p /www/adminer.php $NGINX_DOCROOT/\n",
		pbi,
		nvram_safe_get("mysql_sleep"),
		full_datadir,
		full_datadir,
		full_datadir,
		full_tmpdir,
		full_tmpdir,
		full_tmpdir,
		full_datadir);
	fclose( fp );

	chmod("/tmp/start_mysql.sh", 0755);
	xstart("/tmp/start_mysql.sh");
	return;

}

void stop_mysql(void)
{
	FILE *fp;
	char pbi[128];

	if (nvram_match("mysql_binary", "internal")) {
		strcpy(pbi,"/usr/bin");
	}
	else if (nvram_match("mysql_binary", "optware")) {
		strcpy(pbi,"/opt/bin");
	}
	else {
		strcpy(pbi,nvram_safe_get("mysql_binary_custom"));
	}

	/* stop file */
	if (!(fp = fopen("/tmp/stop_mysql.sh", "w"))) {
		syslog(LOG_ERR, "Can not open /tmp/stop_mysql.sh for writing.");
		return;
	}

	fprintf(fp,
		"#!/bin/sh\n\n"
		"%s/mysqladmin -uroot -p\"%s\" --shutdown_timeout=3 shutdown\n"
		"killall mysqld\n"
		"logger \"MySQL successfully stopped\" \n"
		"sleep 1\n"
		"rm -f /var/run/mysql.pid\n"
		"/usr/bin/mycheck addcru\n",
		pbi,
		nvram_safe_get("mysql_passwd"));
	fclose(fp);

	chmod("/tmp/stop_mysql.sh", 0755);

	xstart("/tmp/stop_mysql.sh");
	return;
}
