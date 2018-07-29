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

#define PROGNAME "BMWXCAN"
#define PROGVER "1.01-alpha"

#include "../include/libbmwxcan.h"
#include <termios.h>

BMWXCAN bmw;

struct termios tty;
struct termios tty_old;

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

INT32U hexstr2hex(std::string const hexstr)
{
    INT32U x;   
    std::stringstream ss;
    
    ss << std::hex << hexstr;
    ss >> x;
    
    return static_cast<INT32U>(x);
}

void senderr(char *msg)
{
    std::cerr << msg << std::endl;
}

int parse_line(std::string line)
{
    int result = 0x01;
    int cmp;
    unsigned long vsize = 0;
    char msg[128];

    auto v = explode(line, ':', vsize);
        
        sprintf(msg, ">> REQUEST: %s", line.c_str());
        std::cerr << msg << std::endl;

        if (vsize < 6)
        {
            result = 0x02;
            senderr("?? ERROR 0x02: Invalid message format");
            return result;
        }
        
        INT8U retval = hexstr2hex(v[0]);
        INT8U length = hexstr2hex(v[1]);
        
        if ((5 + length) != vsize)
        {
            result = 0x04;
            senderr("?? ERROR 0x04: Invalid payload length"); 
            return result;
        }
        
        INT32U canid = hexstr2hex(v[3]);
        INT8U checksum = hexstr2hex(v[vsize-1]);
        INT8U payload[9];
        INT8U csum = retval & 0xFF;
        
        csum = ((csum + length) & 0xFF);
        csum = ((csum + canid) & 0xFF);
        
        int i;

        for (i = 0; i < 8; i++)
            payload[i] = '\0';
        
        for (i = 4; i < vsize-1; i++)
        {
            payload[i-4] = hexstr2hex(v[i]);
            csum = ((csum + payload[i-4]) & 0xFF);
        }
/*        
        if (checksum != csum)
        {
            result = 0x08;
            sprintf(msg, "?? ERROR 0x08: Invalid checksum 0x%.2X must be 0x%.2X", checksum, csum);
            senderr(msg);
            return result;
        }
*/
        cmp = bmw.message_parse(canid, payload, length);
        
        if (cmp != retval)
        {
            result = 0x10;
            senderr("!! WARNING 0x10: Parsed return value does not match sent return value");
        }
        
        switch (cmp)
        {
            case BMWXCAN_CMP_OK:
                result = 0x00;
                sprintf(msg, "## SUCCESS: CAN-ID %.3lX parsed", canid);
                break;
#ifdef WITH_IGNORED_IDS
            case BMWXCAN_CMP_IGNORED_ID:
                result = 0x20;
                sprintf(msg, "!! WARNING 0x20: CAN-ID %.3lX ignored", canid);
                break;
#endif
#ifdef WITH_UNKNOWN_IDS
            case BMWXCAN_CMP_UNKNOWN_ID:
                result = 0x40;
                sprintf(msg, "!! WARNING 0x40: CAN-ID %.3lX is marked as unknown", canid);
                break;
#endif
            case BMWXCAN_CMP_INVALID_LENGTH:
                result = 0x80;
                sprintf(msg, "!! WARNING 0x40: CAN-ID %.3lX with invalid length (%d)", canid, length);
                break;
            case BMWXCAN_CMP_UNREGISTERED_ID:
                result = 0xF0;
                sprintf(msg, "!! WARNING 0xF0: CAN-ID %.3lX is not registered", canid);
                break;
            default:
                result = 0xFF;
                sprintf(msg, "!! WARNING 0xFF: CAN-ID %.3lX with invalid return value %.2X", canid, cmp);
                break;
        }
        
        senderr(msg);
        sprintf(msg, "<< ANSWER: 0x%.2X", result);
        senderr(msg);
	
	usleep(750);

	return result;
}

void restoretty(int device)
{  
  if ( tcgetattr ( device, &tty ) != 0 ) 
  {
   std::cerr << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
  }
  
  tty = tty_old;  
}

void inittty(int device)
{
  memset (&tty, 0, sizeof tty);
  
  if ( tcgetattr ( device, &tty ) != 0 ) 
  {
   std::cerr << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
  }
  
  tty_old = tty;
  
  cfsetospeed (&tty, (speed_t)B115200);
  cfsetispeed (&tty, (speed_t)B115200); 
  
  tty.c_cflag     &=  ~PARENB;            // Make 8n1
  tty.c_cflag     &=  ~CSTOPB;
  tty.c_cflag     &=  ~CSIZE;
  tty.c_cflag     |=  CS8;

  tty.c_cflag     &=  ~CRTSCTS;           // no flow control
  tty.c_cc[VMIN]   =  1;                  // read doesn't block
  tty.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout
  tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines  
  
  cfmakeraw(&tty);
  
  tcflush( device, TCIFLUSH );
  
  if ( tcsetattr ( device, TCSANOW, &tty ) != 0) 
  {
   std::cout << "Error " << errno << " from tcsetattr" << std::endl;
  }  
}

void copyright(bool stderr)
{
  ((stderr == true) ? std::cerr : std::cout) << "SpeedBull(r) " << PROGNAME << " ver. " << PROGVER << std::endl << "Copyright (c) 2017 Oliver Welter - All rights reserved!" << std::endl << std::endl;

  ((stderr == true) ? std::cerr : std::cout) << "**************************************************************************************" << std::endl;
  
  if (bmw.check_license() == true)
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
    std::cerr << " --stdin                        Read CAN data from STDIN" << std::endl;
    std::cerr << " --serial-input <dev>           Use serial device <dev> for CAN data input" << std::endl;
    std::cerr << " --enable-data-output           Enable data output to FIFO and MySQL database" << std::endl;
    std::cerr << " --disable-fifo-writing         Disable data output to FIFO" << std::endl;
    std::cerr << " --disable-dbdata-writing       Disable data output to MySQL database" << std::endl;
    std::cerr << " --disable-can-deduplication    Disable MySQL CAN write deduplication" << std::endl;
    std::cerr << " --disable-raw-payload-table    Disable MySQL writes to raw payload table" << std::endl;
    std::cerr << " --disable-translation-table    Disable MySQL writes to translation table" << std::endl;
    std::cerr << " --disable-crash-snapshots      Disable MySQL CAN snapshots on vehicle crash" << std::endl;
    std::cerr << " --mysql-hostname <host>        Set hostname for MySQL to <host> (default localhost)" << std::endl;
    std::cerr << " --mysql-username <user>        Set username for MySQL to <user> (default bmwxcan)" << std::endl;
    std::cerr << " --mysql-password <pass>        Set password for MySQL to <pass> (default bmwxcan)" << std::endl;
    std::cerr << " --mysql-database <name>        Set database for MySQL to <name> (default bmwxcan)" << std::endl;
    std::cerr << " --mysql-port <port>            Set port for MySQL to <port> (default 3306)" << std::endl;
    std::cerr << " --fifo-path <path>             Set FIFO path to <path> (default /var/run/bmwxcan)" << std::endl;
    std::cerr << "======================================================================================" << std::endl;
  }
    
  exit(1);
}

int main(int argc, char **argv)
{
    int result = 0x01;
    int i;
    
    bool have_device = false;
    bool have_stdin = false;
    
    char *device;
    
    for (i = 1; i < argc; i++)
    {
	if (strcmp(argv[i], "--help") == 0)
	{
	    usage(argv[0], true);
	}
	else if (strcmp(argv[i], "--serial-input") == 0)
	{
	    i++;
	    device = argv[i];
	    have_device = true;
	}
	else if (strcmp(argv[i], "--enable-data-output") == 0)
	{
	    bmw.message_parse_output = true;
	}
	else if (strcmp(argv[i], "--disable-fifo-writing") == 0)
	{
	    bmw.fifo_writing_enabled = false;
	}
	else if (strcmp(argv[i], "--disable-dbdata-writing") == 0)
	{
	    bmw.dbdata_writing_enabled = false;
	}
	else if (strcmp(argv[i], "--disable-can-deduplication") == 0)
	{
	    bmw.buffering_memory_prefilter_enabled = false;
	}
	else if (strcmp(argv[i], "--disable-raw-payload-table") == 0)
	{
	    bmw.buffering_raw_payload_table_enabled = false;
	}
	else if (strcmp(argv[i], "--disable-translation-table") == 0)
	{
	    bmw.buffering_translation_table_enabled = false;
	}
	else if (strcmp(argv[i], "--disable-crash-snapshots") == 0)
	{
	    bmw.dbdata_crash_snapshots = false;
	}
	else if (strcmp(argv[i], "--mysql-hostname") == 0)
	{
	    i++;
	    sprintf(bmw.mysql_hostname, "%s", argv[i]);
	}
	else if (strcmp(argv[i], "--mysql-username") == 0)
	{
	    i++;
	    sprintf(bmw.mysql_username, "%s", argv[i]);
	}
	else if (strcmp(argv[i], "--mysql-password") == 0)
	{
	    i++;
	    sprintf(bmw.mysql_password, "%s", argv[i]);
	}
	else if (strcmp(argv[i], "--mysql-database") == 0)
	{
	    i++;
	    sprintf(bmw.mysql_database, "%s", argv[i]);
	}
	else if (strcmp(argv[i], "--mysql-port") == 0)
	{
	    i++;
	    bmw.mysql_port = atoi(argv[i]);
	}
	else if (strcmp(argv[i], "--fifo-path") == 0)
	{
	    i++;
	    sprintf(bmw.path_realtime_data, "%s", argv[i]);
	}
	else if (strcmp(argv[i], "--stdin") == 0)
	{
	    have_stdin = true;
	}
    }
    
    if (bmw.check_license() == false)
    {
	usage(argv[0], false);
    }

    if (have_device == true)
    {
      copyright(true);
      std::cerr << "Reading from device " << device << std::endl;
      
      int dev = open(device, O_RDONLY | O_NOCTTY | O_SYNC);
      inittty(dev);
                  
      while (1)
      {
	int n = 0, spot = 0;
	char buf = '\0';
	char response[1024];

	memset(response, '\0', sizeof response);
	
	do {
	  n = read( dev, &buf, 1 );
	  sprintf( &response[spot], "%c", buf );
	  spot += n;
	} while( buf != '\r' && n > 0);

	if (n < 0) 
	{
	  std::cerr << "Error reading: " << strerror(errno) << std::endl;
	  break;
	}
	else if (n == 0) 
	{
	  std::cerr << "Read nothing!" << std::endl;
	  break;
	}
	else 
	{
	  std::cerr << "Response " << response << std::endl;
	  result = parse_line(std::string(response));
	}
      }
      restoretty(dev);
      close(dev);
    }
    else if (have_stdin == true)
    {
      copyright(true);
      std::cerr << "Reading from standard input" << std::endl;

      for (std::string line; std::getline(std::cin, line);) 
      {
	result = parse_line(line);
      }
    }
    else
    {
      usage(argv[0], false);
    }
    
    if (result == 0x01)
    {
        senderr("?? ERROR 0x01: No parseable message");
    }
    
    return result;
}
