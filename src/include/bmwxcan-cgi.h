 /*****************************************************************
 *                                                                *
 * Copyright (C) 2017-2018 Oliver Welter <contact@verbotene.zone> *
 *                                                                *
 * This is free software, for non-commercial use, licensed under  *
 * The Non-Profit Open Software License version 3.0 (NPOSL-3.0).  *
 *                                                                *
 * See /LICENSE for more information.                             *
 *                                                                *
 *****************************************************************/

#ifndef bmwxcan_cgi_h
#define bmwxcan_cgi_h
#include "../include/defines.h"

#define PROGNAME "BMWXCAN-CGI"
#define PROGVER "1.0-alpha"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sstream>
#include <ostream>
#include <iterator>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <utility>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <mysql/mysql.h>

class BMWXCANCGI
{
private:
  
  char *STATUS = "200 OK";
  char *CONTENT_TYPE = "text/html";
  unsigned long CONTENT_LENGTH = 0;
  
  bool headers_sent = false;
  bool headers_needed = false;
  
  bool send_headers();

public:
  char *REMOTE_ADDR;
  char *REMOTE_PORT;
  char *HOSTNAME;
  char *HOSTTYPE;
  char *OSTYPE;
  char *MACHTYPE;
  char *DOCUMENT_ROOT;
  char *GATEWAY_INTERFACE;
  char *HTTP_ACCEPT;
  char *HTTP_ACCEPT_ENCODING;
  char *HTTP_ACCEPT_LANGUAGE;
  char *HTTP_CONNECTION;
  char *HTTP_COOKIE;
  char *HTTP_HOST;
  char *HTTP_USER_AGENT;
  char *PATH;
  char *PATH_INFO;
  char *PATH_TRANSLATED;
  char *PWD;
  char *QUERY_STRING;
  char *REQUEST_METHOD;
  char *REQUEST_SCHEME;
  char *REQUEST_URI;
  char *SCRIPT_FILENAME;
  char *SCRIPT_NAME;
  char *SERVER_ADDR;
  char *SERVER_NAME;
  char *SERVER_ADMIN;
  char *SERVER_PORT;
  char *SERVER_PROTOCOL;
  char *SERVER_SIGNATURE;
  char *SERVER_SOFTWARE;
  char *PPID;
  char *UID;
  char *EUID;

  BMWXCANCGI();
  ~BMWXCANCGI();
  
  char *mysql_hostname = "localhost";
  char *mysql_username = "bmwxcan";
  char *mysql_password = "bmwxcan";
  char *mysql_database = "bmwxcan";
  int mysql_port = 3306;

  void send(char *content_type, std::string msg);
  void index_html();
  void error_not_found();
  void js_boot_js();
  void css_boot_css();
  void dataframe_json(bool consoleout);
  void rawframe_json(bool consoleout);
  bool check_license();
};
#endif
