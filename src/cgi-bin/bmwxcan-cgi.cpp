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

#include "../include/bmwxcan-cgi.h"

std::vector<std::string> explode(std::string const & s, char delim, unsigned long &size)
{
    std::vector<std::string> result;
    std::istringstream iss(s);
    
    size = 0;

    for (std::string token; std::getline(iss, token, delim); )
    {
        result.push_back(std::move(token));
        size++;
    }

    return result;
}

BMWXCANCGI::BMWXCANCGI()
{
  PATH = getenv("PATH");
  PWD = getenv("PWD");
  HOSTNAME = getenv("HOSTNAME");
  HOSTTYPE = getenv("HOSTTYPE");
  OSTYPE = getenv("OSTYPE");
  MACHTYPE = getenv("MACHTYPE");
  PPID = getenv("PPID");
  UID = getenv("UID");
  EUID = getenv("EUID");

  if (getenv("GATEWAY_INTERFACE"))
  {
    headers_needed = true;
    
    PATH_INFO = getenv("PATH_INFO");
    PATH_TRANSLATED = getenv("PATH_TRANSLATED");
    REMOTE_ADDR = getenv("REMOTE_ADDR");
    REMOTE_PORT = getenv("REMOTE_PORT");
    DOCUMENT_ROOT = getenv("DOCUMENT_ROOT");
    GATEWAY_INTERFACE = getenv("GATEWAY_INTERFACE");
    HTTP_ACCEPT = getenv("HTTP_ACCEPT");
    HTTP_ACCEPT_ENCODING = getenv("HTTP_ACCEPT_ENCODING");
    HTTP_ACCEPT_LANGUAGE = getenv("HTTP_ACCEPT_LANGUAGE");
    HTTP_CONNECTION = getenv("HTTP_CONNECTION");
    HTTP_COOKIE = getenv("HTTP_COOKIE");
    HTTP_HOST = getenv("HTTP_HOST");
    HTTP_USER_AGENT = getenv("HTTP_USER_AGENT");
    QUERY_STRING = getenv("QUERY_STRING");
    REQUEST_METHOD = getenv("REQUEST_METHOD");
    REQUEST_SCHEME = getenv("REQUEST_SCHEME");
    REQUEST_URI = getenv("REQUEST_URI");
    SCRIPT_FILENAME = getenv("SCRIPT_FILENAME");
    SCRIPT_NAME = getenv("SCRIPT_NAME");
    SERVER_ADDR = getenv("SERVER_ADDR");
    SERVER_NAME = getenv("SERVER_NAME");
    SERVER_ADMIN = getenv("SERVER_ADMIN");
    SERVER_PORT = getenv("SERVER_PORT");
    SERVER_PROTOCOL = getenv("SERVER_PROTOCOL");
    SERVER_SIGNATURE = getenv("SERVER_SIGNATURE");
    SERVER_SOFTWARE = getenv("SERVER_SOFTWARE");
    
//    syslog(LOG_INFO, "Access from %s to %s, server %s, url %s", REMOTE_ADDR, SERVER_ADDR, SERVER_NAME, SCRIPT_NAME);
  }
  else
  {
    syslog(LOG_INFO, "Access from console UID %d", UID);
  }
}

BMWXCANCGI::~BMWXCANCGI()
{
  if (!getenv("GATEWAY_INTERFACE"))
  {
    syslog(LOG_INFO, "Program exitted.");
  }
}

bool BMWXCANCGI::send_headers()
{
  if (headers_needed == false)
    return true;
  
  if (headers_sent == true)
    return false;
  
  headers_sent = true;
  
  std::cout << "Status: " << STATUS << "\r\n";
  std::cout << "Content-Type: " << CONTENT_TYPE << "\r\n";
  std::cout << "Content-Length: " << CONTENT_LENGTH << "\r\n";
  
  if (basename(PATH_INFO))
    std::cout << "Content-Disposition: inline; filename=" << basename(PATH_INFO) << "\r\n";
  
  std::cout << "Expires: now" << "\r\n";
  std::cout << "Pragma: no-cache, must-revalidate" << "\r\n";
  std::cout << "Copyright: 2017 Oliver Welter - All rights reserved" << "\r\n";
  std::cout << "P3P: CP=\"CURa ADMa DEVa PSAo PSDo OUR BUS UNI PUR INT DEM STA PRE COM NAV OTC NOI DSP COR\"" << "\r\n";
  //std::cout << "Content-Security-Policy: default-src http://" << SERVER_NAME << "; child-src 'none'; object-src 'none'" << "\r\n";
  std::cout << "\r\n";

  return headers_sent;
}

void BMWXCANCGI::send(char *content_type, std::string msg)
{
  CONTENT_TYPE = content_type;
  CONTENT_LENGTH = msg.length();
    
  send_headers();
  
  std::cout << msg << std::endl;
}

void BMWXCANCGI::index_html()
{
  std::stringstream html;
  
  html << "<!DOCTYPE html>"
  "<html>"
  "<head>"
  "<meta charset=\"utf-8\"/>"
  "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">"
  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
  "<title>BMW E6X CGI - Copyright &copy; 2017 Oliver Welter - All rights reserved</title>"
  "<link rel=\"stylesheet\" type=\"text/css\" href=\"" << SCRIPT_NAME << "/css/boot.css\"/>"
  "<script type=\"text/javascript\" src=\"" << SCRIPT_NAME << "/js/boot.js\"></script>"
  "</head>"
  "<body>"
  "<div id=\"background\"></div>"
  "<div id=\"main\"><img src=\"/assets/images/bmw-fire.gif\" alt=\"\" border=\"0\"/><br/>Starte System</div>"
  "</body>"
  "</html>";
  
  send("text/html", html.str());
}

void BMWXCANCGI::error_not_found()
{
  STATUS = "404 not found";
  send("text/html", "<!DOCTYPE html><html><head><title>404 - NOT FOUND</title></head><body><h1>404 - NOT FOUND</h1></body></html>");
}

void BMWXCANCGI::css_boot_css()
{
  std::stringstream css;
  
  css << "body { font-family: Verdana, Tahoma, Arial; font-size: 12px; margin: 0 auto; padding: 0 auto; background-color: #000 !important; color: #fff !important; width: 100%; position: relative; }"
  "#main { display: block; padding: 0 auto; margin: 0 auto; font-size: 32px; text-align: center; text-shadow: 1px 1px 4px #f630ca; }";

  send("text/css", css.str());
}

void BMWXCANCGI::dataframe_json(bool consoleout)
{
  std::stringstream json;

  MYSQL *db;
  
  db = mysql_init(NULL);
  
  if (db == NULL)
  {
    syslog(LOG_CRIT, "Unable to initialize MySQL: %s", mysql_error(db));

    json << "{\"error\":\"1\",\"errmsg\":\"" << mysql_error(db) << "\"}";

    if (consoleout == true)
      std::cout << json.str() << std::endl;
    else
      send("application/json", json.str());

    return;
  }
  else
  {
//    syslog(LOG_INFO, "Initialized %s", mysql_get_client_info());
  }

  if (mysql_real_connect(db, mysql_hostname, mysql_username, mysql_password, mysql_database, mysql_port, NULL, 0) == NULL)
  {
    syslog(LOG_CRIT, "Unable to connect to MySQL database: %s", mysql_error(db));

    json << "{\"error\":\"2\",\"errmsg\":\"" << mysql_error(db) << "\"}";

    if (consoleout == true)
      std::cout << json.str() << std::endl;
    else
      send("application/json", json.str());

    mysql_close(db);

    return;
  }
  
  char query[4096];  
  memset(query, '\0', sizeof query);
  bool ts_underscore = false;
  unsigned long vsize = 0;
  char qstr[2048];
  memset(qstr, '\0', sizeof qstr);

  if (QUERY_STRING)
  {
    sprintf(qstr, "%s", QUERY_STRING);
    std::vector<std::string> qtest = explode(qstr, '=', vsize);
    
    if (qtest[0] == "_")
      ts_underscore = true;
  }
  
  if ((QUERY_STRING) && (ts_underscore == false))
  {  
    
    std::vector<std::string> v = explode(qstr, ',', vsize);

    std::string a;
    int leng=v.size();

    for(int i=0; i<leng; i++)
    {
	a+= "'";
        a+= v[i];
	a+= "'";
        if (  i < (leng-1) )
            a+= ",";
    }
    
    sprintf(query, "SELECT `translated_key`,`timestamp_last_update`,`translated_value` FROM `kcan_data_translated` WHERE `translated_key` IN (%s)", a.c_str()); 
  }
  else
  {
    sprintf(query, "SELECT `translated_key`,`timestamp_last_update`,`translated_value` FROM `kcan_data_translated` WHERE '1'"); 
  }
  
  if (mysql_query(db, query))
  {
    syslog(LOG_CRIT, "Unable to execute select query for translated data: %s", mysql_error(db));

    json << "{\"error\":\"3\",\"errmsg\":\"" << mysql_error(db) << "\"}";

    if (consoleout == true)
      std::cout << json.str() << std::endl;
    else
      send("application/json", json.str());

    mysql_close(db);

    return;    
  }

  MYSQL_RES *result = mysql_store_result(db);

  if (result == NULL)
  {
    syslog(LOG_CRIT, "Unable to select translated data: %s", mysql_error(db));

    json << "{\"error\":\"4\",\"errmsg\":\"" << mysql_error(db) << "\"}";

    if (consoleout == true)
      std::cout << json.str() << std::endl;
    else
      send("application/json", json.str());

    mysql_close(db);

    return;    
  }
  
  MYSQL_ROW row;
  INT32U rowCount = 0;
  
  json << "{\"error\":\"0\"";

  while ((row = mysql_fetch_row(result)))
  {
    rowCount++;
    
    json << ",\"" << row[0] << "\":\"" << row[2] << "\",\"" << row[0] << "_timestamp\":\"" << row[1] << "\"";
  }
  
  json << ",\"rowCount\":\"" << rowCount << "\"}";
  
  mysql_free_result(result);
  mysql_close(db);

  if (consoleout == true)
    std::cout << json.str() << std::endl;
  else
    send("application/json", json.str());
}

void BMWXCANCGI::rawframe_json(bool consoleout)
{
  std::stringstream json;

  MYSQL *db;
  
  db = mysql_init(NULL);
  
  if (db == NULL)
  {
    syslog(LOG_CRIT, "Unable to initialize MySQL: %s", mysql_error(db));

    json << "{\"error\":\"1\",\"errmsg\":\"" << mysql_error(db) << "\"}";

    if (consoleout == true)
      std::cout << json.str() << std::endl;
    else
      send("application/json", json.str());

    return;
  }
  else
  {
//    syslog(LOG_INFO, "Initialized %s", mysql_get_client_info());
  }

  if (mysql_real_connect(db, mysql_hostname, mysql_username, mysql_password, mysql_database, mysql_port, NULL, 0) == NULL)
  {
    syslog(LOG_CRIT, "Unable to connect to MySQL database: %s", mysql_error(db));

    json << "{\"error\":\"2\",\"errmsg\":\"" << mysql_error(db) << "\"}";

    if (consoleout == true)
      std::cout << json.str() << std::endl;
    else
      send("application/json", json.str());

    mysql_close(db);

    return;
  }

  char query[4096];  
  memset(query, '\0', sizeof query);
  bool ts_underscore = false;
  unsigned long vsize = 0;
  char qstr[2048];
  memset(qstr, '\0', sizeof qstr);

  if (QUERY_STRING)
  {
    sprintf(qstr, "%s", QUERY_STRING);
    std::vector<std::string> qtest = explode(qstr, '=', vsize);
    
    if (qtest[0] == "_")
      ts_underscore = true;
  }
  
  if ((QUERY_STRING) && (ts_underscore == false))
    sprintf(query, "SELECT `can_id`,`timestamp_last_update`,`can_length`,`can_byte_1`,`can_byte_2`,`can_byte_3`,`can_byte_4`,`can_byte_5`,`can_byte_6`,`can_byte_7`,`can_byte_8`,`can_retval` FROM `kcan_raw_messages` WHERE `can_id` IN (%s)", QUERY_STRING); 
  else
    sprintf(query, "SELECT `can_id`,`timestamp_last_update`,`can_length`,`can_byte_1`,`can_byte_2`,`can_byte_3`,`can_byte_4`,`can_byte_5`,`can_byte_6`,`can_byte_7`,`can_byte_8`,`can_retval` FROM `kcan_raw_messages` WHERE '1'"); 

  if (mysql_query(db, query))
  {
    syslog(LOG_CRIT, "Unable to execute select query for translated data: %s", mysql_error(db));

    json << "{\"error\":\"3\",\"errmsg\":\"" << mysql_error(db) << "\"}";

    if (consoleout == true)
      std::cout << json.str() << std::endl;
    else
      send("application/json", json.str());

    mysql_close(db);

    return;    
  }

  MYSQL_RES *result = mysql_store_result(db);

  if (result == NULL)
  {
    syslog(LOG_CRIT, "Unable to select translated data: %s", mysql_error(db));

    json << "{\"error\":\"4\",\"errmsg\":\"" << mysql_error(db) << "\"}";

    if (consoleout == true)
      std::cout << json.str() << std::endl;
    else
      send("application/json", json.str());

    mysql_close(db);

    return;    
  }
  
  MYSQL_ROW row;
  INT32U rowCount = 0;
  
  json << "{\"error\":\"0\"";

  while ((row = mysql_fetch_row(result)))
  {
    rowCount++;
        
    json << ", \"" << row[0] << "\": { \"can_id\": \"" << row[0] << "\", \"can_last_update\": \"" << row[1] << "\", \"can_length\": \"" << row[2] << "\", \"can_byte_1\": \"" << row[3] << "\", \"can_byte_2\": \"" << row[4] << "\", \"can_byte_3\": \"" << row[5] << "\", \"can_byte_4\": \"" << row[6] << "\", \"can_byte_5\": \"" << row[7] << "\", \"can_byte_6\": \"" << row[8] << "\", \"can_byte_7\": \"" << row[9] << "\", \"can_byte_8\": \"" << row[10] << "\"}";
  }
  
  json << ",\"rowCount\":\"" << rowCount << "\"}";
  
  mysql_free_result(result);
  mysql_close(db);

  if (consoleout == true)
    std::cout << json.str() << std::endl;
  else
    send("application/json", json.str());
}

void BMWXCANCGI::js_boot_js()
{
  std::stringstream js;
  
  js << "// Boot script - (c) 2017 Oliver Welter - All rights reserved\n"
  "function load_js(script_filename) { document.writeln(unescape('%3C') + 'script type=\"text/javascript\" src=\"' + script_filename + '\"' + unescape('%3E%3C') + '/script' + unescape('%3E')); }\n"
  "function load_css(stylesheet_filename) { document.writeln(unescape('%3C') + 'link rel=\"stylesheet\" href=\"' + stylesheet_filename + '\"/' + unescape('%3E')); }\n"
  "load_js('/assets/js/jquery.js');\n"
  "load_js('/assets/js/jquery.number.min.js');\n"
  "load_js('/assets/js/jquery-ui.min.js');\n"
  "load_js('/assets/js/bootstrap.min.js');\n"
  "load_js('/assets/js/gauge.min.js');\n"
  "load_js('/assets/js/bmwxcan.js');\n"
  "load_css('/assets/css/jquery-ui.min.css');\n"
  "load_css('/assets/css/bootstrap.min.css');\n"
  "load_css('/assets/css/font-awesome.min.css');\n"
  "load_css('/assets/css/bmwxcan.css');\n";
  
  send("text/javascript", js.str());
}

bool BMWXCANCGI::check_license()
{
#ifndef LICENSED_FOR_VIN
  return false;
#else
  std::stringstream lc;
  
  lc << LICENSED_FOR_VIN << "BMWXCANCAN" << LICENSED_FOR_NAME << "Oliver Welter" << std::endl;
  
  std::string str(lc.str());
  
  if (str.size() > 10) return true;
#endif
  
  return false;
}

BMWXCANCGI cgi;

void copyright(bool stderr)
{
  ((stderr == true) ? std::cerr : std::cout) << "SpeedBull(r) " << PROGNAME << " ver. " << PROGVER << std::endl << "Copyright (c) 2017 Oliver Welter - All rights reserved!" << std::endl << std::endl;

  ((stderr == true) ? std::cerr : std::cout) << "**************************************************************************************" << std::endl;
  
  if (cgi.check_license() == true)
  {
#ifndef LICENSED_FOR_VIN
    ((stderr == true) ? std::cerr : std::cout) << "* UNLICENSED VERSION !! YOU ARE NOT ALLOWED TO USE THIS PROGRAM!" << std::endl;
#else
    ((stderr == true) ? std::cerr : std::cout) << "* Licensed to " << LICENSED_FOR_NAME << " for VIN " << LICENSED_FOR_VIN << " (last 7 digits only)" << std::endl;
#endif
  }
  else
  {
#ifndef LICENSED_FOR_NAME
    ((stderr == true) ? std::cerr : std::cout) << "* UNLICENSED VERSION !! YOU ARE NOT ALLOWED TO USE THIS PROGRAM!" << std::endl;
#else
    ((stderr == true) ? std::cerr : std::cout) << "* Invalid license for " << LICENSED_FOR_NAME << "! Program use is unauthorized!" << std::endl;
#endif
  }
  
  ((stderr == true) ? std::cerr : std::cout) << "* Any unauthorized use, copy or distribution is strictly prohibited!" << std::endl;
  ((stderr == true) ? std::cerr : std::cout) << "**************************************************************************************" << std::endl << std::endl;
}

void usage(char *filename, bool help)
{
  copyright(true);
  std::cerr << "Usage: " << filename << " <options> (--help for more details)" << std::endl;

  if (help == true)
  {
    std::cerr << "Help context for: " << filename << std::endl;
    std::cerr << "======================================================================================" << std::endl;
    std::cerr << " --dataframe                    Retrieve a JSON data frame" << std::endl;
    std::cerr << " --rawframe                     Retrieve a JSON raw frame" << std::endl;
    std::cerr << " --query <val1,val2,...>        Add a query to frame request" << std::endl;
    std::cerr << " --mysql-hostname <host>        Set hostname for MySQL to <host> (default localhost)" << std::endl;
    std::cerr << " --mysql-username <user>        Set username for MySQL to <user> (default bmwxcan)" << std::endl;
    std::cerr << " --mysql-password <pass>        Set password for MySQL to <pass> (default bmwxcan)" << std::endl;
    std::cerr << " --mysql-database <name>        Set database for MySQL to <name> (default bmwxcan)" << std::endl;
    std::cerr << " --mysql-port <port>            Set port for MySQL to <port> (default 3306)" << std::endl;
    std::cerr << "======================================================================================" << std::endl;
  }
    
  exit(1);
}

int main (int argc, char **argv)
{
    if (!getenv("GATEWAY_INTERFACE"))
    {
      bool rawframe = false;
      bool dataframe = false;
      char *query[1024];
      
      for (int i = 1; i < argc; i++)
      {
	if (strcmp(argv[i], "--help") == 0)
	{
	    usage(argv[0], true);
	}
	else if (strcmp(argv[i], "--mysql-hostname") == 0)
	{
	    i++;
	    sprintf(cgi.mysql_hostname, "%s", argv[i]);
	}
	else if (strcmp(argv[i], "--mysql-username") == 0)
	{
	    i++;
	    sprintf(cgi.mysql_username, "%s", argv[i]);
	}
	else if (strcmp(argv[i], "--mysql-password") == 0)
	{
	    i++;
	    sprintf(cgi.mysql_password, "%s", argv[i]);
	}
	else if (strcmp(argv[i], "--mysql-database") == 0)
	{
	    i++;
	    sprintf(cgi.mysql_database, "%s", argv[i]);
	}
	else if (strcmp(argv[i], "--mysql-port") == 0)
	{
	    i++;
	    cgi.mysql_port = atoi(argv[i]);
	}	
	else if (strcmp(argv[i], "--query") == 0)
	{
	    i++;
	    cgi.QUERY_STRING = argv[i];
	}	
	else if (strcmp(argv[i], "--rawframe") == 0)
	{
	    rawframe = true;
	}	
	else if (strcmp(argv[i], "--dataframe") == 0)
	{
	    dataframe = true;
	}	
      }
      
      if (cgi.check_license() == false)
      {
	usage(argv[0], false);
      }
      
      if (dataframe == true)
      {
	copyright(true);
	cgi.dataframe_json(true);
      }
      else if (rawframe == true)
      {
	copyright(true);
	cgi.rawframe_json(true);
      }
      else
      {
	usage(argv[0], false);
      }
      
      return 0;
    }
    
    if (!getenv("PATH_INFO"))
    {
      std::cout << "Location: " << getenv("SCRIPT_NAME") << "/index.html" << "\r\n\r\n";
      return 0;
    }
    
    if (getenv("MYSQL_HOSTNAME"))
      cgi.mysql_hostname = getenv("MYSQL_HOSTNAME");

    if (getenv("MYSQL_USERNAME"))
      cgi.mysql_username = getenv("MYSQL_USERNAME");

    if (getenv("MYSQL_PASSWORD"))
      cgi.mysql_password = getenv("MYSQL_PASSWORD");

    if (getenv("MYSQL_DATABASE"))
      cgi.mysql_database = getenv("MYSQL_DATABASE");

    if (getenv("MYSQL_PORT"))
      cgi.mysql_port = atoi(getenv("MYSQL_PORT"));

    if (!strcmp(cgi.PATH_INFO, "/index.html"))
    {
      cgi.index_html();
    }
    else if (!strcmp(cgi.PATH_INFO, "/js/boot.js"))
    {
      cgi.js_boot_js();
    }
    else if (!strcmp(cgi.PATH_INFO, "/css/boot.css"))
    {
      cgi.css_boot_css();
    }
    else if (!strcmp(cgi.PATH_INFO, "/dataframe.json"))
    {
      cgi.dataframe_json(false);
    }
    else if (!strcmp(cgi.PATH_INFO, "/rawframe.json"))
    {
      cgi.rawframe_json(false);
    }
    else
    {
      cgi.error_not_found();
    }
}

