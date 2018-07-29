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

#ifdef WITH_TARGET_ARDUINO
#include <libbmwxcan.h>
#else
#include "libbmwxcan.h"
#endif

BMWXCAN::BMWXCAN()
{
	memset(last_message_data, '\0', sizeof(last_message_data));
	memset(last_message_source, '\0', sizeof(last_message_source));
	memset(value_gear_lever, '\0', sizeof(value_gear_lever));
	sprintf(value_gear_lever, "--");
	
#ifndef WITH_TARGET_ARDUINO
	// SET THE DEFAULTS
	message_parse_output = false;
	fifo_writing_enabled = true;
	dbdata_writing_enabled = true;

	// MUST BE SET TO TRUE
	buffering_enabled = true;

	buffering_memory_prefilter_enabled = true;
	buffering_translation_table_enabled = true;
	buffering_raw_payload_table_enabled = true;
	
	dbdata_crash_snapshots = true;
	
	setlogmask(LOG_UPTO (LOG_NOTICE));
	openlog(PROGNAME, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
	
	syslog(LOG_NOTICE, "Program version %s started by user %d", PROGVER, getuid());
#ifndef LICENSED_FOR_VIN
	syslog(LOG_EMERG, "=== YOU ARE USING AN UNLICENSED COPY - THIS IS PROHIBITED ===");
#endif
#endif
}

#ifndef WITH_TARGET_ARDUINO
BMWXCAN::~BMWXCAN()
{	
	if (buffering_db != NULL)
	{
	  mysql_close(buffering_db);
	}

#ifndef LICENSED_FOR_VIN
	syslog(LOG_EMERG, "=== YOU ARE USING AN UNLICENSED COPY - THIS IS PROHIBITED ===");
#endif
	syslog(LOG_NOTICE, "Program exitted");
	closelog();
}

void BMWXCAN::buffering_db_open()
{
	if (buffering_enabled == false)
		return;
	
	if (buffering_db != NULL)
		return;
		
	buffering_db = mysql_init(NULL);
	
	if (buffering_db == NULL)
	{
	  syslog(LOG_CRIT, "Unable to initialize MySQL: %s - buffering disabled", mysql_error(buffering_db));
	  buffering_enabled = false;

	  return;
	}
	else
	{
	  syslog(LOG_INFO, "Initialized %s", mysql_get_client_info());
	}

	if (mysql_real_connect(buffering_db, mysql_hostname, mysql_username, mysql_password, NULL, mysql_port, NULL, 0) == NULL)
	{
	  syslog(LOG_CRIT, "Unable to connect to MySQL database: %s - buffering disabled", mysql_error(buffering_db));
	  buffering_enabled = false;

	  mysql_close(buffering_db);

	  return;
	}
	
	char sql[1024];
	memset(sql, '\0', sizeof sql);
	sprintf(sql, "CREATE DATABASE IF NOT EXISTS `%s`", mysql_database); 
	
	if (mysql_query(buffering_db, sql))
	{
	  syslog(LOG_CRIT, "Unable to create database %s: %s - buffering disabled", mysql_database, mysql_error(buffering_db));
	  buffering_enabled = false;
	  
	  mysql_close(buffering_db);
	  
	  return;
	}
	
	if (mysql_select_db(buffering_db, mysql_database))
	{
	  syslog(LOG_CRIT, "Unable to select database %s: %s - buffering disabled", mysql_database, mysql_error(buffering_db));
	  buffering_enabled = false;
	  
	  mysql_close(buffering_db);
	  
	  return;
	}
	
	mysql_query(buffering_db, "DROP TABLE IF EXISTS `kcan_raw_messages`");
	mysql_query(buffering_db, "DROP TABLE IF EXISTS `kcan_data_translated`");

	if (mysql_query(buffering_db, "CREATE TABLE IF NOT EXISTS `kcan_raw_messages` (`can_id` smallint(5) unsigned NOT NULL, `timestamp_last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, `can_length` tinyint(3) unsigned NOT NULL, `can_byte_1` tinyint(3) unsigned NOT NULL, `can_byte_2` tinyint(3) unsigned NOT NULL, `can_byte_3` tinyint(3) unsigned NOT NULL, `can_byte_4` tinyint(3) unsigned NOT NULL, `can_byte_5` tinyint(3) unsigned NOT NULL, `can_byte_6` tinyint(3) unsigned NOT NULL, `can_byte_7` tinyint(3) unsigned NOT NULL, `can_byte_8` tinyint(3) unsigned NOT NULL, `can_retval` tinyint(3) unsigned NOT NULL, PRIMARY KEY (`can_id`)) ENGINE=MEMORY DEFAULT CHARSET=utf8;"))
	{
	  syslog(LOG_CRIT, "Unable to create table for raw messages: %s - buffering disabled", mysql_error(buffering_db));
	  buffering_enabled = false;
	  
	  mysql_close(buffering_db);
	  
	  return;
	}
		
	if (mysql_query(buffering_db, "CREATE TABLE IF NOT EXISTS `kcan_data_translated` (`translated_key` varchar(128) NOT NULL, `timestamp_last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, `translated_value` varchar(128) NOT NULL, PRIMARY KEY (`translated_key`)) ENGINE=MEMORY DEFAULT CHARSET=utf8;"))
	{
	  syslog(LOG_CRIT, "Unable to create table for translated data: %s - buffering disabled", mysql_error(buffering_db));
	  buffering_enabled = false;
	  
	  mysql_close(buffering_db);
	  
	  return;
	}
		
	if (mysql_query(buffering_db, "CREATE TABLE IF NOT EXISTS `kcan_snapshots` (`uid` bigint(20) unsigned NOT NULL AUTO_INCREMENT, `timestamp_registration` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP, `can_id` smallint(5) unsigned NOT NULL, `timestamp_last_update` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00', `can_length` tinyint(3) unsigned NOT NULL, `can_byte_1` tinyint(3) unsigned NOT NULL, `can_byte_2` tinyint(3) unsigned NOT NULL, `can_byte_3` tinyint(3) unsigned NOT NULL, `can_byte_4` tinyint(3) unsigned NOT NULL, `can_byte_5` tinyint(3) unsigned NOT NULL, `can_byte_6` tinyint(3) unsigned NOT NULL, `can_byte_7` tinyint(3) unsigned NOT NULL, `can_byte_8` tinyint(3) unsigned NOT NULL, PRIMARY KEY (`uid`)) ENGINE=MyISAM DEFAULT CHARSET=utf8"))
	{
	  syslog(LOG_CRIT, "Unable to create table for snapshots: %s - buffering disabled", mysql_error(buffering_db));
	  buffering_enabled = false;
	  
	  mysql_close(buffering_db);
	  
	  return;
	}
	
	mysql_autocommit(buffering_db, true);
		
	return;
}

void BMWXCAN::snapshot_buffering_db()
{
	if (dbdata_crash_snapshots == false)
	    return;
	
	if (buffering_enabled == false)
	    return;
	
	if (buffering_db == NULL)
	    return;

	if (mysql_query(buffering_db, "INSERT INTO `kcan_snapshots` (`can_id`,`timestamp_last_update`,`can_length`,`can_byte_1`,`can_byte_2`,`can_byte_3`,`can_byte_4`,`can_byte_5`,`can_byte_6`,`can_byte_7`,`can_byte_8`) SELECT * FROM `kcan_raw_messages` WHERE '1'"))
	{
	  syslog(LOG_CRIT, "Unable to write snapshot data: %s", mysql_error(buffering_db));
	}
}

bool BMWXCAN::database_write(INT32U can_id, INT8U *payload, INT8U length, INT8U retval)
{
	if (buffering_raw_payload_table_enabled == false)
		return true;
	
	if (dbdata_writing_enabled == false)
		return true;

	if (buffering_enabled == false)
		return false;

	if (buffering_db == NULL)
		return false;
	
	if (buffering_memory_prefilter_enabled == true)
	{
	  int v = 0, l = 0;
	  
	  for (l = 0; l < length; l++)
	  {
	    if (buffering_memory_payload[can_id][l] == payload[l])
	      v++;
	  }
	  
	  if (v == length)
	    return true;
	  
	  for (l = 0; l < length; l++)
	  {
	    buffering_memory_payload[can_id][l] = payload[l];
	  }
	}
	
	char sql[1024];
	
	memset(sql, '\0', sizeof(sql));
	sprintf(sql, "INSERT INTO `kcan_raw_messages` (`can_id`, `can_length`, `can_byte_1`, `can_byte_2`, `can_byte_3`, `can_byte_4`, `can_byte_5`, `can_byte_6`, `can_byte_7`, `can_byte_8`, `can_retval`) VALUES ('%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d') ON DUPLICATE KEY UPDATE `can_retval` = '%d', `can_length` = '%d', `can_byte_1` = '%d', `can_byte_2` = '%d', `can_byte_3` = '%d', `can_byte_4` = '%d', `can_byte_5` = '%d', `can_byte_6` = '%d', `can_byte_7` = '%d', `can_byte_8` = '%d'", can_id, length, payload[0], payload[1], payload[2], payload[3], payload[4], payload[5], payload[6], payload[7], retval, retval, length, payload[0], payload[1], payload[2], payload[3], payload[4], payload[5], payload[6], payload[7]);

	if (mysql_query(buffering_db, sql))
	{
	   syslog(LOG_CRIT, "Unable to store can message %.3lX: %s", can_id, mysql_error(buffering_db));
	   return false;
	}
		
	return true;
}

bool BMWXCAN::database_write(char* key, char* value)
{
	if (buffering_translation_table_enabled == false)
		return true;

	if (dbdata_writing_enabled == false)
		return true;

	if (buffering_enabled == false)
		return false;
	
	char sql[1024];
	
	memset(sql, '\0', sizeof(sql));
	sprintf(sql, "INSERT INTO `kcan_data_translated` (`translated_key`,`translated_value`) VALUES ('%s','%s') ON DUPLICATE KEY UPDATE `translated_value` = '%s'", key, value, value);

	if (mysql_query(buffering_db, sql))
	{
	   syslog(LOG_CRIT, "Unable to store data for %s: %s", key, mysql_error(buffering_db));
	   return false;
	}
	
	return true;
}

boolean BMWXCAN::fifo_write(char *fifo, char *msg)
{
	if (fifo_writing_enabled == false)
		return true;
		
	int fd;
	int i;
	int len = 0;
	char fifo_file[512];
	
	memset(fifo_file, '\0', sizeof(fifo_file));	
	sprintf(fifo_file, "%s/%s", path_realtime_data, fifo);
	
	for (i = 0; i < sizeof(msg); i++)
	{
		if (msg[i] != '\0')
			len++;
	}
	
	char fifo_data[len];
	sprintf(fifo_data, "%s", msg);
	
	mkdir(path_realtime_data, 0750);
	mkfifo(fifo_file, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
	
	fd = open(fifo_file, O_RDWR);
	
	if (!fd)
	{
		syslog(LOG_EMERG, "Unable to write realtime data to %s", fifo_file);
		return false;
	}
		
	write(fd, &fifo_data, sizeof(fifo_data));
	close(fd);
	
	return true;
}

boolean BMWXCAN::data_write(char *key, char *value)
{
	int retval_fifo;
	int retval_db;
  
	if ((strstr(key, "last_")) || (strstr(key, "program_")) || (strstr(key, "is_parsing")))
	{
	  // DO NOTHING
	}
	else
	{
	  char dbg[512];

	  memset(dbg, '\0', sizeof(dbg));
	  sprintf(dbg, ":: %s = %s", key, value);
	  std::cerr << dbg << std::endl;
	}

	if (fifo_writing_enabled == true)
	{
		retval_fifo = fifo_write(key, value);
	}
	else
	{
		retval_fifo = 1;
	}
		
	if (dbdata_writing_enabled == true)
	{
		retval_db = database_write(key, value);
	}
	else
	{
		retval_db = 1;
	}
	
	return (retval_fifo && retval_db) ? true : false;
}

boolean BMWXCAN::data_write(char *key, int d)
{
	char msg[11];
	
	memset(msg, '\0', sizeof(msg));
	sprintf(msg, "%d", key, d);
	
	return data_write(key, msg);
}

boolean BMWXCAN::data_write(char *key, INT8U d)
{
	char msg[3];
	
	memset(msg, '\0', sizeof(msg));
	sprintf(msg, "%d", d);
	
	return data_write(key, msg);
}

boolean BMWXCAN::data_write(char *key, INT32U d)
{
	char msg[32];

	memset(msg, '\0', sizeof(msg));
	sprintf(msg, "%d", d);
		
	return data_write(key, msg);
}

boolean BMWXCAN::data_write(char *key, float f)
{
	char msg[64];
	
	memset(msg, '\0', sizeof(msg));
	sprintf(msg, "%f", f);
		
	return data_write(key, msg);
}

boolean BMWXCAN::data_write(char *key, boolean b)
{
	char msg[2];

	memset(msg, '\0', sizeof(msg));
	sprintf(msg, "%1d", b);
		
	return data_write(key, msg);
}

boolean BMWXCAN::data_write(char *key, long l)
{
	char msg[32];

	memset(msg, '\0', sizeof(msg));
	sprintf(msg, "%d", l);
		
	return data_write(key, msg);
}

boolean BMWXCAN::data_write(char *key, unsigned long long l)
{
	char msg[64];

	memset(msg, '\0', sizeof(msg));
	sprintf(msg, "%d", l);
		
	return data_write(key, msg);
}
#endif

char * BMWXCAN::int2bin(unsigned int x)
{
	static char buffer[17];
	for (int i=0; i<16; i++) buffer[15-i] = '0' + ((x & (1 << i)) > 0);
	buffer[16] ='\0';
	return buffer;
}

void BMWXCAN::bytecpy(byte *dst, byte *src)
{
	int i;
	
	for (i = 0; i < sizeof(src); i++)
	{
		dst[i] = src[i];
	}
}

void BMWXCAN::bytecpy(char *dst, char *src)
{
	int i;
	
	for (i = 0; i < sizeof(src); i++)
	{
		dst[i] = src[i];
	}
}

boolean BMWXCAN::writing_allowed(INT32U can_id)
{
	for (int i = 0; i < sizeof(write_protected_can_ids); i++)
	{
		if (can_id == write_protected_can_ids[i])
			return true;
	}
	
	return false;
}

#if (defined(WITH_CAN_ID_THROTTLE_AND_RPM_F) || defined(WITH_CAN_ID_ALL)) && defined(WITH_F_SERIES)
INT8U BMWXCAN::parse_throttle_and_rpm_f(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_throttle_position = (((payload[4] << 8) + payload[3]) - offset_throttle);
	value_rpm = (((payload[6] << 8) + payload[5]) / 4);
	
	status_engine_running = ((value_rpm > 0) ? true : false);
	
	status_accelerator_full_pushed = ((value_throttle_position == 65064) ? true : false);
	status_accelerator_foot_on = ((value_throttle_position > 255) ? true : false);

#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("rpm_value", value_rpm);
		this->data_write("throttle_position", value_throttle_position);
		this->data_write("engine_running", status_engine_running);
		this->data_write("accelerator_full_pushed", status_accelerator_full_pushed);
		this->data_write("accelerator_foot_on", status_accelerator_foot_on);	  
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_TORQUE_CLUTCH_BRAKE))
INT8U BMWXCAN::parse_torque_clutch_brake(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_torque = (((payload[2] * 256) + payload[1]) / 32);
	value_power_kw = ((value_rpm * value_torque) / (60 / (2 * PI)) / 1000);
	value_power_hp = (value_power_kw * 1.35962);
	value_clutch = payload[5];
	value_brake = payload[7];
	
	status_brake_pressed = ((value_brake > 20) ? true : false);
	status_brake_foot_on = ((value_brake > 0) ? true : false);
	status_clutch_depressed = ((value_clutch == 241) ? true : false);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("torque_nm", value_torque);
		this->data_write("power_kw", value_power_kw);
		this->data_write("power_hp", value_power_hp);
		this->data_write("brake_value", value_brake);
		this->data_write("clutch_value", value_clutch);
		this->data_write("brake_pressed", (bool)((status_brake_pressed == true) ? 1 : 0));
		this->data_write("brake_foot_on", (bool)((status_brake_foot_on == true) ? 1 : 0));
		this->data_write("clutch_pressed", (bool)((status_clutch_depressed == true) ? 0 : 1));
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if defined(WITH_E_SERIES) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_RPM_AND_THROTTLE_POSITION))
INT8U BMWXCAN::parse_rpm_and_throttle_position(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_rpm = (((payload[5] * 256) + payload[4]) / 4);
	value_throttle_position = ((payload[3] * 256) + payload[2]);
	
	status_engine_running = ((value_rpm > 0) ? true : false);

	status_accelerator_full_pushed = ((value_throttle_position == 65064) ? true : false);
	status_accelerator_foot_on = ((payload[7] == 148) ? true : false);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("rpm_value", value_rpm);
		this->data_write("throttle_position", value_throttle_position);
		this->data_write("engine_running", status_engine_running);
		this->data_write("accelerator_full_pushed", status_accelerator_full_pushed);
		this->data_write("accelerator_foot_on", status_accelerator_foot_on);	  
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_ALIVE_COUNTER))
INT8U BMWXCAN::parse_alive_counter(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_alive_counter = payload[1];
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("alive_counter", value_alive_counter);
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_ABS_BRAKE_COUNTER))
INT8U BMWXCAN::parse_abs_brake_counter(INT8U *payload)
{
	value_abs_brake_counter = payload[1];
	
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "JBE");

#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("abs_brake_counter", value_abs_brake_counter);
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_STEERING_WHEEL_POS_A) || defined(WITH_CAN_ID_STEERING_WHEEL_POS_B))
INT8U BMWXCAN::parse_steering_wheel_position(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_steering_wheel_pos_change = ((payload[4] << 8) + payload[3]);
	
	if (value_steering_wheel_pos_absolute > 32767)
		value_steering_wheel_position_degree = (value_steering_wheel_pos_absolute - 65535) / 23;
	else
		value_steering_wheel_position_degree = value_steering_wheel_pos_absolute / 23;
	
	if (value_steering_wheel_pos_change > 32767)
		value_steering_wheel_change_degree = (value_steering_wheel_pos_change - 65535) / 23;
	else
		value_steering_wheel_change_degree = value_steering_wheel_pos_change / 23;
	
	if (value_steering_wheel_change_degree == 0)
	{
		status_steering_wheel_turn_right = false;
		status_steering_wheel_turn_left = false;
	}
	else if (value_steering_wheel_change_degree < 0)
	{
		status_steering_wheel_turn_right = false;
		status_steering_wheel_turn_left = true;
	}
	else
	{
		status_steering_wheel_turn_right = true;
		status_steering_wheel_turn_left = false;
	}

#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("steering_wheel_pos_absolute", value_steering_wheel_pos_absolute);
		this->data_write("steering_wheel_pos_change", value_steering_wheel_pos_change);
		this->data_write("steering_wheel_position_degree", value_steering_wheel_position_degree);
		this->data_write("steering_wheel_change_degree", value_steering_wheel_change_degree);
		this->data_write("steering_wheel_turn_right", status_steering_wheel_turn_right);
		this->data_write("steering_wheel_turn_left", status_steering_wheel_turn_left);
	}
#endif
	return BMWXCAN_CMP_OK;
}

#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_STEERING_WHEEL_POS_A))
INT8U BMWXCAN::parse_steering_wheel_pos_a(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_steering_wheel_pos_absolute = ((payload[2] << 8) + payload[1]);
	
	return parse_steering_wheel_position(payload);
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_STEERING_WHEEL_POS_B))
INT8U BMWXCAN::parse_steering_wheel_pos_b(INT8U *payload)
{
	value_steering_wheel_pos_absolute = ((payload[1] << 8) + payload[0]);

	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "SZL");
	
	return parse_steering_wheel_position(payload);
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_INDIVIDUAL_WHEEL_SPEED))
INT8U BMWXCAN::parse_individual_wheel_speed(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_wheel_speed_1 = (((payload[1] << 8) + payload[0]) / 24);
	value_wheel_speed_2 = (((payload[3] << 8) + payload[2]) / 24);
	value_wheel_speed_3 = (((payload[5] << 8) + payload[4]) / 24);
	value_wheel_speed_4 = (((payload[7] << 8) + payload[6]) / 24);
	
	if ((value_wheel_speed_1 + value_wheel_speed_2 + value_wheel_speed_3 + value_wheel_speed_4) == 0)
		status_car_is_rolling = false;
	else
		status_car_is_rolling = true;
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("wheel_speed_1", value_wheel_speed_1);
		this->data_write("wheel_speed_2", value_wheel_speed_2);
		this->data_write("wheel_speed_3", value_wheel_speed_3);
		this->data_write("wheel_speed_4", value_wheel_speed_4);
		this->data_write("car_is_rolling", status_car_is_rolling);
	}
#endif
	return parse_steering_wheel_position(payload);
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_AIRBAG_SEATBELT_COUNTER))
INT8U BMWXCAN::parse_airbag_seatbelt_counter(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_airbag_seatbelt_counter = payload[0];
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("airbag_seatbelt_counter", value_airbag_seatbelt_counter);
#endif	
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_DOOR_STATUS_FRONT_PASSENGER))
INT8U BMWXCAN::parse_door_status_front_passenger(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "JBE");

	status_door_front_passenger_open = ((payload[3] == 253) ? true : false);
	status_door_front_passenger_locked = (((payload[0] == 130) || (payload[0] == 131)) ? true : false);
	status_door_front_passenger_alert_active = ((payload[0] == 131) ? true : false);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("door_front_passenger_open", status_door_front_passenger_open);
		this->data_write("door_front_passenger_locked", status_door_front_passenger_locked);
		this->data_write("door_front_passenger_alert_active", status_door_front_passenger_alert_active);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_DOOR_STATUS_REAR_PASSENGER))
INT8U BMWXCAN::parse_door_status_rear_passenger(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "JBE");

	status_door_rear_passenger_open = ((payload[3] == 253) ? true : false);
	status_door_rear_passenger_locked = (((payload[0] == 130) || (payload[0] == 131)) ? true : false);
	status_door_rear_passenger_alert_active = ((payload[0] == 131) ? true : false);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("door_rear_passenger_open", status_door_rear_passenger_open);
		this->data_write("door_rear_passenger_locked", status_door_rear_passenger_locked);
		this->data_write("door_rear_passenger_alert_active", status_door_rear_passenger_alert_active);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_DOOR_STATUS_FRONT_DRIVER))
INT8U BMWXCAN::parse_door_status_front_driver(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "JBE");

	status_door_front_driver_open = ((payload[3] == 253) ? true : false);
	status_door_front_driver_locked = (((payload[0] == 130) || (payload[0] == 131)) ? true : false);
	status_door_front_driver_alert_active = ((payload[0] == 131) ? true : false);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("door_front_driver_open", status_door_front_driver_open);
		this->data_write("door_front_driver_locked", status_door_front_driver_locked);
		this->data_write("door_front_driver_alert_active", status_door_front_driver_alert_active);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_DOOR_STATUS_REAR_DRIVER))
INT8U BMWXCAN::parse_door_status_rear_driver(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "JBE");

	status_door_rear_driver_open = ((payload[3] == 253) ? true : false);
	status_door_rear_driver_locked = (((payload[0] == 130) || (payload[0] == 131)) ? true : false);
	status_door_rear_driver_alert_active = ((payload[0] == 131) ? true : false);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("door_rear_driver_open", status_door_rear_driver_open);
		this->data_write("door_rear_driver_locked", status_door_rear_driver_locked);
		this->data_write("door_rear_driver_alert_active", status_door_rear_driver_alert_active);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_BOOT_STATUS))
INT8U BMWXCAN::parse_boot_status(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "JBE");

	status_boot_open = ((payload[3] == 193) ? true : false);
	status_boot_locked = (((payload[0] == 242) || (payload[0] == 243)) ? true : false);
	status_boot_alert_active = ((payload[0] == 243) ? true : false);
	status_boot_release_button_pressed = ((payload[2] == 136) ? true : false);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("boot_open", status_boot_open);
		this->data_write("boot_locked", status_boot_locked);
		this->data_write("boot_alert_active", status_boot_alert_active);
		this->data_write("boot_release_button_pressed", status_boot_release_button_pressed);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_WINDOW_LIFTER_CONTROL_FRONT_DRIVER))
INT8U BMWXCAN::parse_window_lifter_control_front_driver(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	status_window_lifter_control_front_driver_button_front_driver_up_position_1 = false;
	status_window_lifter_control_front_driver_button_front_driver_up_position_2 = false;
	status_window_lifter_control_front_driver_button_rear_driver_up_position_1 = false;
	status_window_lifter_control_front_driver_button_rear_driver_up_position_2 = false;
	status_window_lifter_control_front_driver_button_front_passenger_up_position_1 = false;
	status_window_lifter_control_front_driver_button_front_passenger_up_position_2 = false;
	status_window_lifter_control_front_driver_button_rear_passenger_up_position_1 = false;
	status_window_lifter_control_front_driver_button_rear_passenger_up_position_2 = false;

	status_window_lifter_control_front_driver_button_front_driver_down_position_1 = false;
	status_window_lifter_control_front_driver_button_front_driver_down_position_2 = false;
	status_window_lifter_control_front_driver_button_rear_driver_down_position_1 = false;
	status_window_lifter_control_front_driver_button_rear_driver_down_position_2 = false;
	status_window_lifter_control_front_driver_button_front_passenger_down_position_1 = false;
	status_window_lifter_control_front_driver_button_front_passenger_down_position_2 = false;
	status_window_lifter_control_front_driver_button_rear_passenger_down_position_1 = false;
	status_window_lifter_control_front_driver_button_rear_passenger_down_position_2 = false;

	switch(payload[0])
	{
		case 8:
			status_window_lifter_control_front_driver_button_rear_passenger_down_position_1 = true;
			break;
		case 16:
			status_window_lifter_control_front_driver_button_rear_passenger_down_position_2 = true;
			break;
		case 24:
			status_window_lifter_control_front_driver_button_rear_passenger_up_position_1 = true;
			break;
		case 32:
			status_window_lifter_control_front_driver_button_rear_passenger_up_position_2 = true;
			break;
		case 200:
			status_window_lifter_control_front_driver_button_front_passenger_down_position_1 = true;
			break;
		case 208:
			status_window_lifter_control_front_driver_button_front_passenger_down_position_2 = true;
			break;
		case 216:
			status_window_lifter_control_front_driver_button_front_passenger_up_position_1 = true;
			break;
		case 224:
			status_window_lifter_control_front_driver_button_front_passenger_up_position_2 = true;
			break;
	}
	
	switch (payload[1])
	{
		case 1:
			status_window_lifter_control_front_driver_button_rear_driver_down_position_1 = true;
			break;
		case 2:
			status_window_lifter_control_front_driver_button_rear_driver_down_position_2 = true;
			break;
		case 3:
			status_window_lifter_control_front_driver_button_rear_driver_up_position_1 = true;
			break;
		case 4:
			status_window_lifter_control_front_driver_button_rear_driver_up_position_2 = true;
			break;
		case 193:
			status_window_lifter_control_front_driver_button_front_driver_down_position_1 = true;
			break;
		case 194:
			status_window_lifter_control_front_driver_button_front_driver_down_position_2 = true;
			break;
		case 195:
			status_window_lifter_control_front_driver_button_front_driver_up_position_1 = true;
			break;
		case 196:
			status_window_lifter_control_front_driver_button_front_driver_up_position_2 = true;
			break;
	}
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("window_lifter_control_front_driver_button_front_driver_up_position_1", status_window_lifter_control_front_driver_button_front_driver_up_position_1);
		this->data_write("window_lifter_control_front_driver_button_front_driver_up_position_2", status_window_lifter_control_front_driver_button_front_driver_up_position_2);
		this->data_write("window_lifter_control_front_driver_button_rear_driver_up_position_1", status_window_lifter_control_front_driver_button_rear_driver_up_position_1);
		this->data_write("window_lifter_control_front_driver_button_rear_driver_up_position_2", status_window_lifter_control_front_driver_button_rear_driver_up_position_2);
		this->data_write("window_lifter_control_front_driver_button_front_passenger_up_position_1", status_window_lifter_control_front_driver_button_front_passenger_up_position_1);
		this->data_write("window_lifter_control_front_driver_button_front_passenger_up_position_2", status_window_lifter_control_front_driver_button_front_passenger_up_position_2);
		this->data_write("window_lifter_control_front_driver_button_rear_passenger_up_position_1", status_window_lifter_control_front_driver_button_rear_passenger_up_position_1);
		this->data_write("window_lifter_control_front_driver_button_rear_passenger_up_position_2", status_window_lifter_control_front_driver_button_rear_passenger_up_position_2);

		this->data_write("window_lifter_control_front_driver_button_front_driver_down_position_1", status_window_lifter_control_front_driver_button_front_driver_down_position_1);
		this->data_write("window_lifter_control_front_driver_button_front_driver_down_position_2", status_window_lifter_control_front_driver_button_front_driver_down_position_2);
		this->data_write("window_lifter_control_front_driver_button_rear_driver_down_position_1", status_window_lifter_control_front_driver_button_rear_driver_down_position_1);
		this->data_write("window_lifter_control_front_driver_button_rear_driver_down_position_2", status_window_lifter_control_front_driver_button_rear_driver_down_position_2);
		this->data_write("window_lifter_control_front_driver_button_front_passenger_down_position_1", status_window_lifter_control_front_driver_button_front_passenger_down_position_1);
		this->data_write("window_lifter_control_front_driver_button_front_passenger_down_position_2", status_window_lifter_control_front_driver_button_front_passenger_down_position_2);
		this->data_write("window_lifter_control_front_driver_button_rear_passenger_down_position_1", status_window_lifter_control_front_driver_button_rear_passenger_down_position_1);
		this->data_write("window_lifter_control_front_driver_button_rear_passenger_down_position_2", status_window_lifter_control_front_driver_button_rear_passenger_down_position_2);	  
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_WINDOW_LIFTER_CONTROL_FRONT_PASSENGER))
INT8U BMWXCAN::parse_window_lifter_control_front_passenger(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	status_window_lifter_control_front_passenger_button_front_passenger_up_position_1 = false;
	status_window_lifter_control_front_passenger_button_front_passenger_up_position_2 = false;

	status_window_lifter_control_front_passenger_button_front_passenger_down_position_1 = false;
	status_window_lifter_control_front_passenger_button_front_passenger_down_position_2 = false;

	switch(payload[0])
	{
		case 207:
			status_window_lifter_control_front_passenger_button_front_passenger_down_position_1 = true;
			break;
		case 215:
			status_window_lifter_control_front_passenger_button_front_passenger_down_position_2 = true;
			break;
		case 223:
			status_window_lifter_control_front_passenger_button_front_passenger_up_position_1 = true;
			break;
		case 231:
			status_window_lifter_control_front_passenger_button_front_passenger_up_position_2 = true;
			break;
	}	
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("window_lifter_control_front_passenger_button_front_passenger_up_position_1", status_window_lifter_control_front_passenger_button_front_passenger_up_position_1);
		this->data_write("window_lifter_control_front_passenger_button_front_passenger_up_position_2", status_window_lifter_control_front_passenger_button_front_passenger_up_position_2);

		this->data_write("window_lifter_control_front_passenger_button_front_passenger_down_position_1", status_window_lifter_control_front_passenger_button_front_passenger_down_position_1);
		this->data_write("window_lifter_control_front_passenger_button_front_passenger_down_position_2", status_window_lifter_control_front_passenger_button_front_passenger_down_position_2);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_WINDOW_LIFTER_CONTROL_REAR_PASSENGER))
INT8U BMWXCAN::parse_window_lifter_control_rear_passenger(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	status_window_lifter_control_rear_passenger_button_rear_passenger_up_position_1 = false;
	status_window_lifter_control_rear_passenger_button_rear_passenger_up_position_2 = false;

	status_window_lifter_control_rear_passenger_button_rear_passenger_down_position_1 = false;
	status_window_lifter_control_rear_passenger_button_rear_passenger_down_position_2 = false;

	switch(payload[0])
	{
		case 207:
			status_window_lifter_control_rear_passenger_button_rear_passenger_down_position_1 = true;
			break;
		case 215:
			status_window_lifter_control_rear_passenger_button_rear_passenger_down_position_2 = true;
			break;
		case 223:
			status_window_lifter_control_rear_passenger_button_rear_passenger_up_position_1 = true;
			break;
		case 231:
			status_window_lifter_control_rear_passenger_button_rear_passenger_up_position_2 = true;
			break;
	}	
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("window_lifter_control_rear_passenger_button_rear_passenger_up_position_1", status_window_lifter_control_rear_passenger_button_rear_passenger_up_position_1);
		this->data_write("window_lifter_control_rear_passenger_button_rear_passenger_up_position_2", status_window_lifter_control_rear_passenger_button_rear_passenger_up_position_2);

		this->data_write("window_lifter_control_rear_passenger_button_rear_passenger_down_position_1", status_window_lifter_control_rear_passenger_button_rear_passenger_down_position_1);
		this->data_write("window_lifter_control_rear_passenger_button_rear_passenger_down_position_2", status_window_lifter_control_rear_passenger_button_rear_passenger_down_position_2);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_WINDOW_LIFTER_CONTROL_REAR_DRIVER))
INT8U BMWXCAN::parse_window_lifter_control_rear_driver(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	status_window_lifter_control_rear_driver_button_rear_driver_up_position_1 = false;
	status_window_lifter_control_rear_driver_button_rear_driver_up_position_2 = false;

	status_window_lifter_control_rear_driver_button_rear_driver_down_position_1 = false;
	status_window_lifter_control_rear_driver_button_rear_driver_down_position_2 = false;

	switch(payload[0])
	{
		case 207:
			status_window_lifter_control_rear_driver_button_rear_driver_down_position_1 = true;
			break;
		case 215:
			status_window_lifter_control_rear_driver_button_rear_driver_down_position_2 = true;
			break;
		case 223:
			status_window_lifter_control_rear_driver_button_rear_driver_up_position_1 = true;
			break;
		case 231:
			status_window_lifter_control_rear_driver_button_rear_driver_up_position_2 = true;
			break;
	}	
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("window_lifter_control_rear_driver_button_rear_driver_up_position_1", status_window_lifter_control_rear_driver_button_rear_driver_up_position_1);
		this->data_write("window_lifter_control_rear_driver_button_rear_driver_up_position_2", status_window_lifter_control_rear_driver_button_rear_driver_up_position_2);

		this->data_write("window_lifter_control_rear_driver_button_rear_driver_down_position_1", status_window_lifter_control_rear_driver_button_rear_driver_down_position_1);
		this->data_write("window_lifter_control_rear_driver_button_rear_driver_down_position_2", status_window_lifter_control_rear_driver_button_rear_driver_down_position_2);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_IGNITION_STATUS))
INT8U BMWXCAN::parse_ignition_status(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "CAS");
	
	status_ignition_key_beeing_inserted = false;
	status_ignition_key_is_in_slot = false;
	status_ignition_key_position_1 = false;
	status_ignition_key_position_2 = false;
	status_ignition_key_start_engine = false;
	
	switch (payload[0])
	{
		case 0x40:
			status_ignition_key_beeing_inserted = true;
			break;
		case 0x41:
			status_ignition_key_position_1 = true;
			break;
		case 0x45:
			status_ignition_key_position_2 = true;
			break;
		case 0x55:
			status_ignition_key_start_engine = true;
			break;
	}
	
	switch (payload[1])
	{
		case 0x40:
			status_ignition_key_is_in_slot = true;
			break;
	}
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("ignition_key_beeing_inserted", status_ignition_key_beeing_inserted);
		this->data_write("ignition_key_is_in_slot", status_ignition_key_is_in_slot);
		this->data_write("ignition_key_position_1", status_ignition_key_position_1);
		this->data_write("ignition_key_position_2", status_ignition_key_position_2);
		this->data_write("ignition_key_start_engine", status_ignition_key_start_engine);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_TIMER_AND_CRUISE_CONTROL_STATUS))
INT8U BMWXCAN::parse_timer_and_cruise_control_status(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");
	
	value_cruise_control_timer = payload[0];
	value_cruise_control_speed = payload[1];
	
	status_cruise_control_active = ((payload[5] == 88) ? true : false);
	status_cruise_control_correcting_speed = ((payload[6] == 1) ? true : false);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("cruise_control_timer", value_cruise_control_timer);
		this->data_write("cruise_control_speed", value_cruise_control_speed);
		this->data_write("cruise_control_active", status_cruise_control_active);
		this->data_write("cruise_control_correcting_speed", status_cruise_control_correcting_speed);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_ABS_BRAKE))
INT8U BMWXCAN::parse_abs_brake(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "DSC");
	
	value_abs_timer = payload[2];
	value_abs_counter = payload[7];
	value_brake_pressure = payload[6];
	
	status_braking = ((payload[5] == 0) ? false : true);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("ignition_key_beeing_inserted", status_ignition_key_beeing_inserted);
		this->data_write("ignition_key_position_1", status_ignition_key_position_1);
		this->data_write("ignition_key_position_2", status_ignition_key_position_2);
		this->data_write("ignition_key_start_engine", status_ignition_key_start_engine);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_SPEED))
INT8U BMWXCAN::parse_speed(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_speed_1 = ((payload[1] * 256) + payload[0]);
	value_speed_2 = ((payload[3] * 256) + payload[2]);
	value_speed_3 = ((payload[5] * 256) + payload[4]);
	value_speed_counter = ((payload[6] * 256) + payload[5]);	
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("speed_1", value_speed_1);
		this->data_write("speed_2", value_speed_2);
		this->data_write("speed_3", value_speed_3);
		this->data_write("speed_counter", value_speed_counter);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_SPEED_HANDBRAKE))
INT8U BMWXCAN::parse_speed_handbrake(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "KOMBI");

	status_handbrake_active = ((payload[5] == 50) ? true : false);
	value_speed_mph = ((((payload[1] - speed_correction_value) * 256) + payload[0]) / 16);
	value_speed_kmh = (value_speed_mph * 1.60934);
	value_speed_fps = (value_speed_mph * 1.46667);
	value_speed_mps = (value_speed_mph * 0.44704);
	value_speed_knots = (value_speed_mph * 0.868976);

#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("handbrake_active", status_handbrake_active);
		this->data_write("speed_mph", value_speed_mph);
		this->data_write("speed_kmh", value_speed_kmh);
		this->data_write("speed_fps", value_speed_fps);
		this->data_write("speed_mps", value_speed_mps);
		this->data_write("speed_knots", value_speed_knots);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_I_DRIVE_CONTROL))
INT8U BMWXCAN::parse_i_drive_control(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "CON");
	
	status_i_drive_button_menu = false;
	status_i_drive_button_voice = false;
	status_i_drive_pressed = false;
	status_i_drive_up = false;
	status_i_drive_down = false;
	status_i_drive_left = false;
	status_i_drive_right = false;
	
	if ((payload[0] == 0x0C) && (payload[1] == 0xC0))
	{
		return BMWXCAN_CMP_OK;
	}
	else if (payload[1] == 0xC0)
	{
		switch (payload[0])
		{
			case 0x00:
				status_i_drive_up = true;
				break;
			case 0x04:
				status_i_drive_down = true;
				break;
			case 0x06:
				status_i_drive_left = true;
				break;
			case 0x02:
				status_i_drive_right = true;
				break;
		}
	}
	else if (payload[1] == 0xC1)
	{
		if (payload[0] == 0x0F)
			status_i_drive_pressed = true;
	}
	else if (payload[1] == 0xC4)
	{
		if (payload[0] == 0x0F)
			status_i_drive_button_menu = true;
	}
	else if (payload[1] == 0x0D)
	{
		if (payload[0] == 0x0F)
			status_i_drive_button_voice = true;
	}
	
	value_i_drive_spin_sensitive = payload[2];
	value_i_drive_spin_rough = payload[3];
	value_i_drive_last_action = millis();
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("i_drive_button_menu", status_i_drive_button_menu);
		this->data_write("i_drive_button_voice", status_i_drive_button_voice);
		this->data_write("i_drive_pressed", status_i_drive_pressed);
		this->data_write("i_drive_up", status_i_drive_up);
		this->data_write("i_drive_down", status_i_drive_down);
		this->data_write("i_drive_left", status_i_drive_left);
		this->data_write("i_drive_right", status_i_drive_right);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_PDC_SENSOR_DATA))
INT8U BMWXCAN::parse_pdc_sensor_data(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");
	
	value_pdc_sensor_front_1 = payload[0];
	value_pdc_sensor_front_2 = payload[1];
	value_pdc_sensor_front_3 = payload[2];
	value_pdc_sensor_front_4 = payload[3];

	value_pdc_sensor_rear_5 = payload[4];
	value_pdc_sensor_rear_6 = payload[5];
	value_pdc_sensor_rear_7 = payload[6];
	value_pdc_sensor_rear_8 = payload[7];
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("pdc_sensor_front_1", value_pdc_sensor_front_1);
		this->data_write("pdc_sensor_front_2", value_pdc_sensor_front_2);
		this->data_write("pdc_sensor_front_3", value_pdc_sensor_front_3);
		this->data_write("pdc_sensor_front_4", value_pdc_sensor_front_4);

		this->data_write("pdc_sensor_rear_5", value_pdc_sensor_rear_5);
		this->data_write("pdc_sensor_rear_6", value_pdc_sensor_rear_6);
		this->data_write("pdc_sensor_rear_7", value_pdc_sensor_rear_7);
		this->data_write("pdc_sensor_rear_8", value_pdc_sensor_rear_8);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_PDC_BUTTON))
INT8U BMWXCAN::parse_pdc_button(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");
	
	if ((payload[2] == 0x52) && (payload[3] == 0x52))
	{
	  status_pdc_button_pressed = true;
	}
	else
	{
	  status_pdc_button_pressed = false;
	}
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("pdc_button_pressed", status_pdc_button_pressed);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_ENGINE_TEMP_AND_PRESSURE))
INT8U BMWXCAN::parse_engine_temp_and_pressure(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_engine_temperature = payload[0] - 48;
	value_engine_oil_pressure_hpa = (payload[4] * 18.47619);
// POSSIBLY OIL TEMP, WATER TEMP AND TURBO PRESSURE ARE HERE TOO?	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("engine_temperature", value_engine_temperature);
		this->data_write("engine_oil_pressure_hpa", value_engine_oil_pressure_hpa);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_STEERING_WHEEL_BUTTONS))
INT8U BMWXCAN::parse_steering_wheel_buttons(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "SZL");

	status_steering_wheel_button_volume_up = false;
	status_steering_wheel_button_volume_down = false;
	status_steering_wheel_button_up = false;
	status_steering_wheel_button_down = false;
	status_steering_wheel_button_telephone = false;
	status_steering_wheel_button_voice = false;
	status_steering_wheel_button_diamond = false;
	status_steering_wheel_button_star = false;
	status_steering_wheel_button_horn = false;
	
	if ((payload[0] == 0xC0) && (payload[1] == 0x00))
	{
		return BMWXCAN_CMP_OK;
	}
	else if (payload[1] == 0x00)
	{
		switch (payload[0])
		{
			case 0xC8:
				status_steering_wheel_button_volume_up = true;
				break;
			case 0xC4:
				status_steering_wheel_button_volume_down = true;
				break;
			case 0xE0:
				status_steering_wheel_button_up = true;
				break;
			case 0xD0:
				status_steering_wheel_button_down = true;
				break;
			case 0xC1:
				status_steering_wheel_button_telephone = true;
				break;
		}
	}
	else if (payload[1] == 0x01)
	{
		if (payload[0] == 0xC9)
			status_steering_wheel_button_voice = true;
	}
	else if (payload[1] == 0x40)
	{
		if (payload[0] == 0xC0)
			status_steering_wheel_button_diamond = true;
	}
	else if (payload[1] == 0x10)
	{
		if (payload[0] == 0xC0)
			status_steering_wheel_button_star = true;
	}
	else if (payload[1] == 0x04)
	{
		if (payload[0] == 0xC0)
			status_steering_wheel_button_horn = true;
	}
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("steering_wheel_button_volume_up", status_steering_wheel_button_volume_up);
		this->data_write("steering_wheel_button_volume_down", status_steering_wheel_button_volume_down);
		this->data_write("steering_wheel_button_up", status_steering_wheel_button_up);
		this->data_write("steering_wheel_button_down", status_steering_wheel_button_down);
		this->data_write("steering_wheel_button_telephone", status_steering_wheel_button_telephone);
		this->data_write("steering_wheel_button_voice", status_steering_wheel_button_voice);
		this->data_write("steering_wheel_button_diamond", status_steering_wheel_button_diamond);
		this->data_write("steering_wheel_button_star", status_steering_wheel_button_star);
		this->data_write("steering_wheel_button_horn", status_steering_wheel_button_horn);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_INTERIOR_LIGHT_SWITCH))
INT8U BMWXCAN::parse_interior_light_switch(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	status_interior_light_switch = ((payload[0] == 0xF1) ? true : false);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("interior_light_switch", status_interior_light_switch);
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_SEAT_HEATING_DRIVER))
INT8U BMWXCAN::parse_seat_heating_driver(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	status_seat_heating_button_driver_more = ((payload[0] == 0xF1) ? true : false);
	status_seat_heating_button_driver_off = ((payload[0] == 0xF0) ? true : false);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("seat_heating_button_driver_more", status_seat_heating_button_driver_more);
		this->data_write("seat_heating_button_driver_off", status_seat_heating_button_driver_off);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_SEAT_HEATING_PASSENGER))
INT8U BMWXCAN::parse_seat_heating_passenger(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	status_seat_heating_button_passenger_more = ((payload[0] == 0xF1) ? true : false);
	status_seat_heating_button_passenger_off = ((payload[0] == 0xF0) ? true : false);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("seat_heating_button_passenger_more", status_seat_heating_button_passenger_more);
		this->data_write("seat_heating_button_passenger_off", status_seat_heating_button_passenger_off);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_ADJUST_STEERING_WHEEL))
INT8U BMWXCAN::parse_adjust_steering_wheel(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	status_adjust_steering_wheel_up = false;
	status_adjust_steering_wheel_down = false;
	status_adjust_steering_wheel_closer = false;
	status_adjust_steering_wheel_farther = false;
	
	switch (payload[0])
	{
		case 0xF4:
			status_adjust_steering_wheel_up = true;
			break;
		case 0xF8:
			status_adjust_steering_wheel_down = true;
			break;
		case 0xF2:
			status_adjust_steering_wheel_closer = true;
			break;
		case 0xF1:
			status_adjust_steering_wheel_farther = true;
			break;
	}
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("adjust_steering_wheel_up", status_adjust_steering_wheel_up);
		this->data_write("adjust_steering_wheel_down", status_adjust_steering_wheel_down);
		this->data_write("adjust_steering_wheel_closer", status_adjust_steering_wheel_closer);
		this->data_write("adjust_steering_wheel_farther", status_adjust_steering_wheel_farther);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_INDICATOR_STALK))
INT8U BMWXCAN::parse_indicator_stalk(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	boolean status_indicator_stalk_up_position_1		= false;
	boolean status_indicator_stalk_up_position_2		= false;
	boolean status_indicator_stalk_down_position_1		= false;
	boolean status_indicator_stalk_down_position_2		= false;
	boolean status_indicator_stalk_high_beam			= false;
	boolean status_indicator_stalk_flash				= false;
	boolean status_indicator_stalk_button_check			= false;
	boolean status_indicator_stalk_button_bc			= false;

	switch (payload[0])
	{
		case 0x01:
			status_indicator_stalk_up_position_1 = true;
			break;
		case 0x02:
			status_indicator_stalk_up_position_2 = true;
			break;
		case 0x04:
			status_indicator_stalk_down_position_1 = true;
			break;
		case 0x08:
			status_indicator_stalk_down_position_2 = true;
			break;
		case 0x10:
			status_indicator_stalk_high_beam = true;
			break;
		case 0x20:
			status_indicator_stalk_flash = true;
			break;
		case 0x40:
			status_indicator_stalk_button_bc = true;
			break;
		case 0x80:
			status_indicator_stalk_button_check = true;
			break;
	}
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("indicator_stalk_up_position_1", status_indicator_stalk_up_position_1);
		this->data_write("indicator_stalk_up_position_2", status_indicator_stalk_up_position_2);
		this->data_write("indicator_stalk_down_position_1", status_indicator_stalk_down_position_1);
		this->data_write("indicator_stalk_down_position_2", status_indicator_stalk_down_position_2);
		this->data_write("indicator_stalk_high_beam", status_indicator_stalk_high_beam);
		this->data_write("indicator_stalk_flash", status_indicator_stalk_flash);
		this->data_write("indicator_stalk_button_bc", status_indicator_stalk_button_bc);
		this->data_write("indicator_stalk_button_check", status_indicator_stalk_button_check);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_SEAT_ADJUSTMENT_DRIVER))
INT8U BMWXCAN::parse_seat_adjustment_driver(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");
	
	status_seat_adjustment_driver_release_button = false;
	status_seat_adjustment_driver_m = false;
	status_seat_adjustment_driver_1 = false;
	status_seat_adjustment_driver_2 = false;
	
	switch (payload[0])
	{
		case 0xF8:
			status_seat_adjustment_driver_release_button = true;
			break;
		case 0xFC:
			status_seat_adjustment_driver_m = true;
			break;
		case 0xF9:
			status_seat_adjustment_driver_1 = true;
			break;
		case 0xFA:
			status_seat_adjustment_driver_2 = true;
			break;
	}

#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("seat_adjustment_driver_release_button", status_seat_adjustment_driver_release_button);
		this->data_write("seat_adjustment_driver_m", status_seat_adjustment_driver_m);
		this->data_write("seat_adjustment_driver_1", status_seat_adjustment_driver_1);
		this->data_write("seat_adjustment_driver_2", status_seat_adjustment_driver_2);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_INDICATORS))
INT8U BMWXCAN::parse_indicators(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");
	
	status_indicators_left = false;
	status_indicators_right = false;
	status_indicators_hazzard = false;

	switch(payload[0])
	{
		case 0x91:
			status_indicators_left = true;
			break;
		case 0xA1:
			status_indicators_right = true;
			break;
		case 0xB1:
			status_indicators_hazzard = true;
			break;
	}

#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("indicators_left", status_indicators_left);
		this->data_write("indicators_right", status_indicators_right);
		this->data_write("indicators_hazzard", status_indicators_hazzard);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

INT8U BMWXCAN::parse_crash(INT8U *payload)
{
#ifndef WITH_TARGET_ARDUINO
	if (snapshot_created == false)
	{
	  if (payload[1] > 0)
	  {
	    syslog(LOG_EMERG, "EMERGENCY ALERT!! CRASH #%d DETECTED!!", payload[1]);
	    snapshot_buffering_db();
	    snapshot_created = true;
	  }
	}
#endif
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	status_crash = ((payload[0] == 0xC0) ? false : true);
	value_crash_counter = payload[1];

#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("crash", status_crash);
		this->data_write("crash_counter", value_crash_counter);
	}
#endif
	return BMWXCAN_CMP_OK;
}

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_CRUISE_CONTROL_SPEED_STEP))
INT8U BMWXCAN::parse_cruise_control_speed_step(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_cruise_control_speed_step = payload[1];

#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("cruise_control_speed_step", value_cruise_control_speed_step);
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_INSTRUMENTS_DIMMER))
INT8U BMWXCAN::parse_instruments_dimmer(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "KOMBI");

	value_instruments_dimmer = payload[0];
	status_instrument_lights_active = ((payload[0] == 254) ? false : true);

#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("instruments_dimmer", value_instruments_dimmer);
		this->data_write("instrument_lights_active", status_instrument_lights_active);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_LIGHTS_SWITCH))
INT8U BMWXCAN::parse_lights_switch(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");
	
	status_lights_switch_zero = ((payload[0] == 0x00) && (payload[1] == 0x30) && (payload[2] == 0xFC)) ? true : false;
	status_lights_switch_position = ((payload[0] == 0x04) && (payload[1] == 0x32) && (payload[2] == 0xFC)) ? true : false;
	status_lights_switch_crossing = ((payload[0] == 0x05) && (payload[1] == 0x32) && (payload[2] == 0xFC)) ? true : false;

	// CHECK THIS !! COULD BE WRONG BECAUSE DOCUMENTED WITH 0x05 WHICH IS LIGHTCROSSING ALSO
	status_lights_switch_auto = ((payload[0] == 0x02) && (payload[1] == 0x32) && (payload[2] == 0xFC)) ? true : false;
	
	status_lights_fog_front_pressed = (((payload[0] == 0x25) || (payload[0] == 0x24) || (payload[0] == 0x20)) && (payload[1] == 0x32) && (payload[2] == 0xFC)) ? true : false;
	status_lights_fog_rear_pressed = (((payload[0] == 0x45) || (payload[0] == 0x44) || (payload[0] == 0x40)) && (payload[1] == 0x32) && (payload[2] == 0xFC)) ? true : false;
	status_lights_fog_both_pressed = (((payload[0] == 0x65) || (payload[0] == 0x64) || (payload[0] == 0x60)) && (payload[1] == 0x32) && (payload[2] == 0xFC)) ? true : false;

	byte bm_brakelight 	= B10000000;
	byte bm_rearfogs 	= B01000000;
	byte bm_frontfogs 	= B00100000;
	byte bm_backlights	= B00010000;
	byte bm_sidelights	= B00000100;
	byte bm_mainbeams	= B00000010;
	byte bm_dipbeams	= B00000001;
	
	byte bm_interior	= B00010000;
	byte bm_exterior	= B00000010;
	
	status_lights_brakelight = false;
	status_lights_rear_foglight = false;
	status_lights_front_foglight = false;
	status_lights_backlight = false;
	status_lights_sidelight = false;
	status_lights_dipbeam = false;
	status_lights_mainbeam = false;
	
	status_lights_interior = false;
	status_lights_exterior = false;
	
	if (payload[0] & bm_brakelight == bm_brakelight)
		status_lights_brakelight = true;
	if (payload[0] & bm_rearfogs == bm_rearfogs)
		status_lights_rear_foglight = true;
	if (payload[0] & bm_frontfogs == bm_frontfogs)
		status_lights_front_foglight = true;
	if (payload[0] & bm_backlights == bm_backlights)
		status_lights_backlight = true;
	if (payload[0] & bm_sidelights == bm_sidelights)
		status_lights_sidelight = true;
	if (payload[0] & bm_dipbeams == bm_dipbeams)
		status_lights_dipbeam = true;
	if (payload[0] & bm_mainbeams == bm_mainbeams)
		status_lights_mainbeam = true;
	
	if (payload[1] & bm_interior == bm_interior)
		status_lights_interior = true;
	if (payload[1] & bm_exterior == bm_exterior)
		status_lights_exterior = true;

#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("lights_switch_zero", status_lights_switch_zero);
		this->data_write("lights_switch_position", status_lights_switch_position);
		this->data_write("lights_switch_crossing", status_lights_switch_crossing);
		this->data_write("lights_switch_auto", status_lights_switch_auto);

		this->data_write("lights_fog_front_pressed", status_lights_fog_front_pressed);
		this->data_write("lights_fog_rear_pressed", status_lights_fog_rear_pressed);
		this->data_write("lights_fog_both_pressed", status_lights_fog_both_pressed);

		this->data_write("lights_brakelight", status_lights_brakelight);
		this->data_write("lights_rear_foglight", status_lights_rear_foglight);
		this->data_write("lights_front_foglight", status_lights_front_foglight);
		this->data_write("lights_backlight", status_lights_backlight);
		this->data_write("lights_sidelight", status_lights_sidelight);
		this->data_write("lights_dipbeam", status_lights_dipbeam);
		this->data_write("lights_mainbeam", status_lights_mainbeam);
		this->data_write("lights_interior", status_lights_interior);
		this->data_write("lights_exterior", status_lights_exterior);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_CARKEY_BUTTON))
INT8U BMWXCAN::parse_carkey_button(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");
	
	status_carkey_button_lock = false;
	status_carkey_button_unlock = false;
	status_carkey_button_boot = false;
	
	if (payload[0] == 0x30)
	{
		switch (payload[1])
		{
			case 0x01:
				status_carkey_button_unlock = true;
				break;
			case 0x04:
				status_carkey_button_lock = true;
				break;
			case 0x10:
				status_carkey_button_boot = true;
				break;
		}
	}

#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("carkey_button_lock", status_carkey_button_lock);
		this->data_write("carkey_button_unlock", status_carkey_button_unlock);
		this->data_write("carkey_button_boot", status_carkey_button_boot);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_AIRCONDITION))
INT8U BMWXCAN::parse_aircondition(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");
	
	status_aircondition_on = false;
	status_aircondition_cooling = false;
	status_aircondition_circulate_auto = false;
	status_aircondition_circulate_manual = false;
	
	if (payload[0] == 0xF1)
	{
		switch (payload[1])
		{
			case 0x00:
				status_aircondition_on = true;
				break;
			case 0x01:
				status_aircondition_cooling = true;
				break;
			case 0x10:
				status_aircondition_circulate_auto = true;
				break;
			case 0x20:
				status_aircondition_circulate_manual = true;
				break;
		}
	}

#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("aircondition_on", status_aircondition_on);
		this->data_write("aircondition_cooling", status_aircondition_cooling);
		this->data_write("aircondition_circulate_auto", status_aircondition_circulate_auto);
		this->data_write("aircondition_circulate_manual", status_aircondition_circulate_manual);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_AIRCONDITION_DEMISTER))
INT8U BMWXCAN::parse_aircondition_demister(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");
	
	status_aircondition_rear_demister_on = false;
	
	if (payload[0] == 0xF3)
	{
		switch (payload[1])
		{
			case 127:
				status_aircondition_rear_demister_on = true;
				break;
		}
	}

#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("aircondition_rear_demister_on", status_aircondition_rear_demister_on);
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_PDC))
INT8U BMWXCAN::parse_pdc(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");
	
	status_pdc_on = ((payload[0] == 0xf5) ? true : false);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("pdc_on", status_pdc_on);
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_WINDSCREEN_WIPER_STATUS))
INT8U BMWXCAN::parse_windscreen_wiper_status(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "JBE");
	
	status_windscreen_wiper_going_up = ((payload[0] == 0xCD) ? true : false);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("windscreen_wiper_going_up", status_windscreen_wiper_going_up);
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_I_DRIVEH_CONTROLLER_ROTARY_CONTROL))
INT8U BMWXCAN::parse_i_driveh_controller_rotary_control(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "CON");
	
	value_i_driveh_rotary_upcounter = payload[2];
	value_i_driveh_rotary_direction_counter = payload[3];
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("i_driveh_rotary_upcounter", value_i_driveh_rotary_upcounter);
		this->data_write("i_driveh_rotary_direction_counter", value_i_driveh_rotary_direction_counter);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_I_DRIVEH_CONTROLLER_DIRECTION_BUTTONS))
INT8U BMWXCAN::parse_i_driveh_controller_direction_buttons(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "CON");
	
	status_i_driveh_button_back_pressed = false;
	status_i_driveh_button_back_long = false;
	status_i_driveh_button_option_pressed = false;
	status_i_driveh_button_option_long = false;
	status_i_driveh_button_radio_pressed = false;
	status_i_driveh_button_radio_long = false;
	status_i_driveh_button_cd_pressed = false;
	status_i_driveh_button_cd_long = false;
	status_i_driveh_button_nav_pressed = false;
	status_i_driveh_button_nav_long = false;
	status_i_driveh_button_tel_pressed = false;
	status_i_driveh_button_tel_long = false;
	status_i_driveh_joystick_or_menu_pressed = false;
	status_i_driveh_joystick_or_menu_long = false;
	status_i_driveh_joystick_up = false;
	status_i_driveh_joystick_up_long = false;
	status_i_driveh_joystick_right = false;
	status_i_driveh_joystick_right_long = false;
	status_i_driveh_joystick_down = false;
	status_i_driveh_joystick_down_long = false;
	status_i_driveh_joystick_left = false;
	status_i_driveh_joystick_left_long = false;
	
	switch (payload[5])
	{
		case 0x01:
			status_i_driveh_joystick_or_menu_pressed = ((payload[3] != 0x00) ? true : false);
			status_i_driveh_joystick_or_menu_long = ((payload[3] == 0x02) ? true : false);
			break;
		case 0x02:
			status_i_driveh_button_back_pressed = ((payload[3] != 0x00) ? true : false);
			status_i_driveh_button_back_long = ((payload[3] == 0x02) ? true : false);
			break;
		case 0x04:
			status_i_driveh_button_option_pressed = ((payload[3] != 0x00) ? true : false);
			status_i_driveh_button_option_long = ((payload[3] == 0x02) ? true : false);
			break;
		case 0x08:
			status_i_driveh_button_radio_pressed = ((payload[3] != 0x00) ? true : false);
			status_i_driveh_button_radio_long = ((payload[3] == 0x02) ? true : false);
			break;
		case 0x10:
			status_i_driveh_button_cd_pressed = ((payload[3] != 0x00) ? true : false);
			status_i_driveh_button_cd_long = ((payload[3] == 0x02) ? true : false);
			break;
		case 0x20:
			status_i_driveh_button_nav_pressed = ((payload[3] != 0x00) ? true : false);
			status_i_driveh_button_nav_long = ((payload[3] == 0x02) ? true : false);
			break;
		case 0x40:
			status_i_driveh_button_tel_pressed = ((payload[3] != 0x00) ? true : false);
			status_i_driveh_button_tel_long = ((payload[3] == 0x02) ? true : false);
			break;
	}
	
	switch (payload[3])
	{
		case 0x11:
			status_i_driveh_joystick_up = true;
			break;
		case 0x12:
			status_i_driveh_joystick_up_long = true;
			break;
		case 0x21:
			status_i_driveh_joystick_right = true;
			break;
		case 0x22:
			status_i_driveh_joystick_right_long = true;
			break;
		case 0x41:
			status_i_driveh_joystick_down = true;
			break;
		case 0x42:
			status_i_driveh_joystick_down_long = true;
			break;
		case 0x81:
			status_i_driveh_joystick_left = true;
			break;
		case 0x82:
			status_i_driveh_joystick_left_long = true;
			break;
	}
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("i_driveh_button_back_pressed", status_i_driveh_button_back_pressed);
		this->data_write("i_driveh_button_back_long", status_i_driveh_button_back_long);
		this->data_write("i_driveh_button_option_pressed", status_i_driveh_button_option_pressed);
		this->data_write("i_driveh_button_option_long", status_i_driveh_button_option_long);
		this->data_write("i_driveh_button_radio_pressed", status_i_driveh_button_radio_pressed);
		this->data_write("i_driveh_button_radio_long", status_i_driveh_button_radio_long);
		this->data_write("i_driveh_button_cd_pressed", status_i_driveh_button_cd_pressed);
		this->data_write("i_driveh_button_cd_long", status_i_driveh_button_cd_long);
		this->data_write("i_driveh_button_nav_pressed", status_i_driveh_button_nav_pressed);
		this->data_write("i_driveh_button_nav_long", status_i_driveh_button_nav_long);
		this->data_write("i_driveh_button_tel_pressed", status_i_driveh_button_tel_pressed);
		this->data_write("i_driveh_button_tel_long", status_i_driveh_button_tel_long);
		this->data_write("i_driveh_joystick_or_menu_pressed", status_i_driveh_joystick_or_menu_pressed);
		this->data_write("i_driveh_joystick_or_menu_long", status_i_driveh_joystick_or_menu_long);
		this->data_write("i_driveh_joystick_up", status_i_driveh_joystick_up);
		this->data_write("i_driveh_joystick_up_long", status_i_driveh_joystick_up_long);
		this->data_write("i_driveh_joystick_right", status_i_driveh_joystick_right);
		this->data_write("i_driveh_joystick_right_long", status_i_driveh_joystick_right_long);
		this->data_write("i_driveh_joystick_down", status_i_driveh_joystick_down);
		this->data_write("i_driveh_joystick_down_long", status_i_driveh_joystick_down_long);
		this->data_write("i_driveh_joystick_left", status_i_driveh_joystick_left);
		this->data_write("i_driveh_joystick_left_long", status_i_driveh_joystick_left_long);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_WIPER_STALK))
INT8U BMWXCAN::parse_wiper_stalk(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "SZL");
	
	status_wiper_intermittant = false;
	status_wiper_continuous_standard = false;
	status_wiper_continuous_fast = false;
	status_wiper_single_wipe = false;
	status_wiper_rear_active = false;
	status_wiper_rear_wash = false;
	
	switch (payload[0])
	{
		case 0x01:
			status_wiper_intermittant = true;
			value_wiper_intermittant_speed = payload[1];
			break;
		case 0x02:
			status_wiper_continuous_standard = true;
			break;
		case 0x03:
			status_wiper_continuous_fast = true;
			break;
		case 0x08:
			status_wiper_single_wipe = true;
			break;
		case 0x10:
			status_wiper_wash = true;
			break;
		case 0x40:
			status_wiper_rear_active = true;
			break;
		case 0x81:
			status_wiper_rear_wash = true;
			break;
	}
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("wiper_intermittant", status_wiper_intermittant);
		this->data_write("wiper_intermittant_speed", value_wiper_intermittant_speed);
		this->data_write("wiper_continuous_standard", status_wiper_continuous_standard);
		this->data_write("wiper_continuous_fast", status_wiper_continuous_fast);
		this->data_write("wiper_single_wipe", status_wiper_single_wipe);
		this->data_write("wiper_wash", status_wiper_wash);
		this->data_write("wiper_rear_active", status_wiper_rear_active);
		this->data_write("wiper_rear_wash", status_wiper_rear_wash);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_RADIO_STATUS))
INT8U BMWXCAN::parse_radio_status(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "RADIO");
	
	value_radio_status = payload[7];
		
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("radio_status", value_radio_status);
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_REAR_VIEW_MIRROR_LIGHT_SENSOR))
INT8U BMWXCAN::parse_rear_view_mirror_light_sensor(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");
	
	value_rear_view_mirror_light_sensor = payload[0];
		
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("rear_view_mirror_light_sensor", value_rear_view_mirror_light_sensor);
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_STOP_WATCH))
INT8U BMWXCAN::parse_stop_watch(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");
	
	value_stop_watch = ((payload[3] * 256 * 256 * 256) + (payload[2] * 256 * 256) + (payload[1] * 256) + payload[0]);
	value_stop_watch_heartbeat = payload[4];
		
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("stop_watch", value_stop_watch);
		this->data_write("stop_watch_heartbeat", value_stop_watch_heartbeat);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_DOOR_LOCKING_VALIDATION))
INT8U BMWXCAN::parse_door_locking_validation(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_door_locking_valid_unlock_time = 0;
	value_door_locking_valid_lock_time = 0;

	switch (payload[1])
	{
		case 0xF2:
			value_door_locking_valid_unlock_time = millis();
			break;
		case 0xF1:
			value_door_locking_valid_lock_time = millis();
			break;
	}
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("door_locking_valid_unlock_time", value_door_locking_valid_unlock_time);
		this->data_write("door_locking_valid_lock_time", value_door_locking_valid_lock_time);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_BC_RESET_AVERAGE_SPEED_MPG))
INT8U BMWXCAN::parse_bc_reset_average_speed_mpg(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_bc_reset_average_speed_time = 0;
	value_bc_reset_average_mpg_time = 0;

	switch (payload[5])
	{
		case 0xE3:
			value_bc_reset_average_mpg_time = millis();
			break;
		case 0xE4:
			value_bc_reset_average_speed_time = millis();
			break;
	}
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("bc_reset_average_speed_time", value_bc_reset_average_speed_time);
		this->data_write("bc_reset_average_mpg_time", value_bc_reset_average_mpg_time);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_OUTSIDE_TEMPERATURE))
INT8U BMWXCAN::parse_outside_temperature(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_outside_temperature = ((float)(payload[0] - 80) / 2);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("outside_temperature", value_outside_temperature);
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_STATUS_REAR_WINDOW_HEATING))
INT8U BMWXCAN::parse_status_rear_window_heating(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	status_rear_window_heating = ((payload[0] == 0xFC) ? false : true);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("rear_window_heating", status_rear_window_heating);
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_AIRCONDITION_STATUS))
INT8U BMWXCAN::parse_aircondition_status(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	status_aircondition_active = ((payload[0] == 0xFD) ? true : false);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("aircondition_active", status_aircondition_active);
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_AIRCONDITION_DRIVER))
INT8U BMWXCAN::parse_aircondition_driver(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_aircondition_temperature_driver = (payload[7] / 2);
	value_aircondition_fan_speed = payload[5];
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("aircondition_temperature_driver", value_aircondition_temperature_driver);
		this->data_write("aircondition_temperature_fan_speed", value_aircondition_fan_speed);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_AIRCONDITION_PASSENGER))
INT8U BMWXCAN::parse_aircondition_passenger(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_aircondition_temperature_passenger = (payload[7] / 2);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("aircondition_temperature_passenger", value_aircondition_temperature_passenger);
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_INSIDE_LIGHTS))
INT8U BMWXCAN::parse_inside_lights(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	status_inside_lights = ((payload[0] == 0xF4) ? true : false);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("inside_lights", status_inside_lights);
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_DATE_TIME))
INT8U BMWXCAN::parse_date_time(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "KOMBI");

	value_date_time_hour = payload[1];
	value_date_time_minute = payload[2];
	value_date_time_day = payload[3];
	value_date_time_month = (payload[4] - 0x0F) >> 4;	
	value_date_time_year = payload[6] * 256 + payload[5];

	status_date_time_is_pm = ((payload[0] == 0x00) ? true : false);
	status_date_time_valid = ((payload[7] == 0xFD) ? true : false);

#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("date_time_hour", value_date_time_hour);
		this->data_write("date_time_minute", value_date_time_minute);
		this->data_write("date_time_day", value_date_time_day);
		this->data_write("date_time_month", value_date_time_month);
		this->data_write("date_time_year", value_date_time_year);
		this->data_write("date_time_valid", status_date_time_valid);
		this->data_write("date_time_is_pm", status_date_time_is_pm);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_SEATBELT_SENSORS))
INT8U BMWXCAN::parse_seatbelt_sensors(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");
	
	status_seatbelt_sensor_triggered_driver = ((payload[0] == 0xFC) ? false : true);
	status_seatbelt_sensor_triggered_passenger = ((payload[0] == 0xFC) ? false : true);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("seatbelt_sensor_triggered_driver", status_seatbelt_sensor_triggered_driver);
		this->data_write("seatbelt_sensor_triggered_passenger", status_seatbelt_sensor_triggered_passenger);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_DOOR_BOOT_BONNET_STATUS))
INT8U BMWXCAN::parse_door_boot_bonnet_status(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "CAS");
	
	// Doors and boot already checked by 0xE...
	if (payload[2] == 0x01)
		status_bonnet_open = true;
	else
		status_bonnet_open = false;	
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("bonnet_open", status_bonnet_open);
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_GEAR_LEVER))
INT8U BMWXCAN::parse_gear_lever(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	status_gear_lever_p							= false;
	status_gear_lever_r							= false;
	status_gear_lever_n							= false;
	status_gear_lever_d							= false;
	status_gear_lever_ds						= false;
	status_gear_lever_m1						= false;
	status_gear_lever_m2						= false;
	status_gear_lever_m3						= false;
	status_gear_lever_m4						= false;
	status_gear_lever_m5						= false;
	status_gear_lever_m6						= false;
	status_gear_lever_m7						= false;
	
	memset(value_gear_lever, '\0', sizeof(value_gear_lever));
	
	switch (payload[0])
	{
		case 0xE3:
			status_gear_lever_p = true;
			sprintf(value_gear_lever, "p");
			break;
		case 0xC1:
			status_gear_lever_r = true;
			sprintf(value_gear_lever, "r");
			break;
		case 0xD1:
			status_gear_lever_n = true;
			sprintf(value_gear_lever, "n");
			break;
		case 0xC2:
			status_gear_lever_d = true;
			sprintf(value_gear_lever, "d");
			break;
		case 0xC3:
			status_gear_lever_ds = true;
			sprintf(value_gear_lever, "ds");
			break;
		case 0xC4:
			status_gear_lever_m1 = true;
			sprintf(value_gear_lever, "m1");
			break;
		case 0xC5:
			status_gear_lever_m2 = true;
			sprintf(value_gear_lever, "m2");
			break;
		case 0xC6:
			status_gear_lever_m3 = true;
			sprintf(value_gear_lever, "m3");
			break;
		case 0xC7:
			status_gear_lever_m4 = true;
			sprintf(value_gear_lever, "m4");
			break;
		case 0xC8:
			status_gear_lever_m5 = true;
			sprintf(value_gear_lever, "m5");
			break;
		case 0xC9:
			status_gear_lever_m6 = true;
			sprintf(value_gear_lever, "m6");
			break;
		case 0xCA:
			status_gear_lever_m7 = true;
			sprintf(value_gear_lever, "m7");
			break;
		default:
			sprintf(value_gear_lever, "--");
			break;
	}
	
	value_gear_lever[3] = '\0';
		
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("gear_lever", value_gear_lever);
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_CAR_TILT))
INT8U BMWXCAN::parse_car_tilt(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_car_tilt_rl = payload[0];
	value_car_tilt_fr = payload[1];
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("car_tilt_rl", value_car_tilt_rl);
		this->data_write("car_tilt_fr", value_car_tilt_fr);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_STATUS_TIRE_SPAN_DISPLAY))
INT8U BMWXCAN::parse_status_tire_span_display(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	status_tire_span_display = (((payload[0] == 0x00) && (payload[1] == 0xFE)) ? false : true);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("tire_span_display", status_tire_span_display);
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_SECONDS_SINCE_BAT_REMOVAL))
INT8U BMWXCAN::parse_seconds_since_bat_removal(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_seconds_since_bat_removal = ((payload[3] * 256 * 256 * 256) + (payload[2] * 256 * 256) + (payload[1] * 256) + payload[0]);
	value_days_since_bat_removal = ((payload[5] * 256) + payload[4]);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("seconds_since_bat_removal", value_seconds_since_bat_removal);
		this->data_write("days_since_bat_removal", value_days_since_bat_removal);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_INSIDE_TEMP_LIGHT_SOLAR_SENSORS))
INT8U BMWXCAN::parse_inside_temp_light_solar_sensors(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_inside_temperature = ((float)(payload[3] / 10) + 6);
	value_inside_light_sensor = payload[0];
	value_inside_solar_sensor = payload[4];
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("inside_temperature", value_inside_temperature);
		this->data_write("inside_light_sensor", value_inside_light_sensor);
		this->data_write("inside_solar_sensor", value_inside_solar_sensor);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_MILEAGE_RANGE_TANK_LEVEL))
INT8U BMWXCAN::parse_mileage_range_tank_level(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "KOMBI");

	value_odometer = ((payload[2] * 256 * 256) + (payload[1] * 256) + payload[0]);
	value_range = (((payload[7] * 256) + payload[6]) / 16);
	value_average_fuel = payload[3];
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("odometer", value_odometer);
		this->data_write("range", value_range);
		this->data_write("average_fuel", value_average_fuel);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_CHECKCONTROL_MESSAGE_MIL))
INT8U BMWXCAN::parse_checkcontrol_message_mil(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "KOMBI");
	
	status_checkcontrol_message_mil = false;
	status_checkcontrol_message_mil_limited = false;

	switch (payload[1])
	{
		case 0x00:
			status_checkcontrol_message_mil = true;
			break;
		case 0xB8:
			status_checkcontrol_message_mil = true;
			status_checkcontrol_message_mil_limited = true;
			break;
	}
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("checkcontrol_message_mil", status_checkcontrol_message_mil);
		this->data_write("checkcontrol_message_mil_limited", status_checkcontrol_message_mil_limited);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_CHECKCONTROL_MESSAGE_DSC))
INT8U BMWXCAN::parse_checkcontrol_message_dsc(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "KOMBI");

	status_checkcontrol_message_dsc = false;
	status_checkcontrol_message_dsc_limited = false;

	if (payload[0] == 0xB8)
	{
		switch (payload[2])
		{
			case 0x01:
				status_checkcontrol_message_dsc = true;
				break;
			case 0x02:
				status_checkcontrol_message_dsc = true;
				status_checkcontrol_message_dsc_limited = true;
				break;
		}
	}
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("checkcontrol_message_dsc", status_checkcontrol_message_dsc);
		this->data_write("checkcontrol_message_dsc_limited", status_checkcontrol_message_dsc_limited);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_STATUS_HANDBRAKE))
INT8U BMWXCAN::parse_status_handbrake(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	status_handbrake_active = ((payload[0] == 254) ? true : false);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("handbrake_active", status_handbrake_active);
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_AVERAGE_SPEED_AND_MILEAGE))
INT8U BMWXCAN::parse_average_speed_and_mileage(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_average_mileage = ((((payload[1] & B00001111) * 256) + payload[0]) / 10);
	value_average_speed = (((payload[2] * 256) + (payload[0] & B11110000)) / 10);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("average_mileage", value_average_mileage);
		this->data_write("average_speed", value_average_speed);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_FUEL_LEVEL_SENSORS))
INT8U BMWXCAN::parse_fuel_level_sensors(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "JBE");

	value_fuel_level_sensor_left = (((payload[1] * 256) + payload[0]) / 160);
	value_fuel_level_sensor_right = (((payload[3] * 256) + payload[2]) / 160);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("fuel_level_sensor_left", value_fuel_level_sensor_left);
		this->data_write("fuel_level_sensor_right", value_fuel_level_sensor_right);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

INT8U BMWXCAN::parse_vin(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "CAS");

	memset(value_vin, '\0', sizeof(value_vin));
	
	int i;
	
	for (i = 0; i < BMWXCAN_MSG_LENGTH_VIN; i++)
	{
		value_vin[i] = payload[i];
	}
	
	value_vin[8] = '\0';
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("vin", value_vin);
#endif
	
	return parse_registration(payload);
}

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_STATUS_BATTERY_AND_ENGINE))
INT8U BMWXCAN::parse_status_battery_and_engine(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	value_battery_voltage = ((float)(((payload[1] - 240) * 256) + payload[0]) / 68);
	status_battery_charging = ((payload[2] == 0x00) ? true : false);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("battery_voltage", value_battery_voltage);
		this->data_write("battery_charging", status_battery_charging);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_POSITION_WINDOW_FRONT_DRIVER))
INT8U BMWXCAN::parse_position_window_front_driver(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "JBE");

	value_position_window_front_driver = payload[1];

	status_direction_window_front_driver_up = ((payload[2] == 240) ? true : false);
	status_direction_window_front_driver_stop = ((payload[2] == 224) ? true : false);
	status_direction_window_front_driver_down = ((payload[2] == 254) ? true : false);
	status_button_window_front_driver_pressed = ((payload[0] == 0x00) ? false : true);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("direction_window_front_driver_up", status_direction_window_front_driver_up);
		this->data_write("direction_window_front_driver_stop", status_direction_window_front_driver_stop);
		this->data_write("direction_window_front_driver_down", status_direction_window_front_driver_down);
		this->data_write("button_window_front_driver_pressed", status_button_window_front_driver_pressed);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_POSITION_WINDOW_REAR_DRIVER))
INT8U BMWXCAN::parse_position_window_rear_driver(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "JBE");

	value_position_window_rear_driver = payload[1];

	status_direction_window_rear_driver_up = ((payload[2] == 240) ? true : false);
	status_direction_window_rear_driver_stop = ((payload[2] == 224) ? true : false);
	status_direction_window_rear_driver_down = ((payload[2] == 254) ? true : false);
	status_button_window_rear_driver_pressed = ((payload[0] == 0x00) ? false : true);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("direction_window_rear_driver_up", status_direction_window_rear_driver_up);
		this->data_write("direction_window_rear_driver_stop", status_direction_window_rear_driver_stop);
		this->data_write("direction_window_rear_driver_down", status_direction_window_rear_driver_down);
		this->data_write("button_window_rear_driver_pressed", status_button_window_rear_driver_pressed);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_POSITION_WINDOW_FRONT_PASSENGER))
INT8U BMWXCAN::parse_position_window_front_passenger(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "JBE");

	value_position_window_front_passenger = payload[1];

	status_direction_window_front_passenger_up = ((payload[2] == 240) ? true : false);
	status_direction_window_front_passenger_stop = ((payload[2] == 224) ? true : false);
	status_direction_window_front_passenger_down = ((payload[2] == 254) ? true : false);
	status_button_window_front_passenger_pressed = ((payload[0] == 0x00) ? false : true);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("direction_window_front_passenger_up", status_direction_window_front_passenger_up);
		this->data_write("direction_window_front_passenger_stop", status_direction_window_front_passenger_stop);
		this->data_write("direction_window_front_passenger_down", status_direction_window_front_passenger_down);
		this->data_write("button_window_front_passenger_pressed", status_button_window_front_passenger_pressed);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_POSITION_WINDOW_REAR_PASSENGER))
INT8U BMWXCAN::parse_position_window_rear_passenger(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "JBE");

	value_position_window_rear_passenger = payload[1];

	status_direction_window_rear_passenger_up = ((payload[2] == 240) ? true : false);
	status_direction_window_rear_passenger_stop = ((payload[2] == 224) ? true : false);
	status_direction_window_rear_passenger_down = ((payload[2] == 254) ? true : false);
	status_button_window_rear_passenger_pressed = ((payload[0] == 0x00) ? false : true);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("direction_window_rear_passenger_up", status_direction_window_rear_passenger_up);
		this->data_write("direction_window_rear_passenger_stop", status_direction_window_rear_passenger_stop);
		this->data_write("direction_window_rear_passenger_down", status_direction_window_rear_passenger_down);
		this->data_write("button_window_rear_passenger_pressed", status_button_window_rear_passenger_pressed);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_POSITION_ROOF))
INT8U BMWXCAN::parse_position_roof(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	if ((payload[0] == 0xFF) && (payload[1] == 0x04) && (payload[2] == 0xFB))
	{
		status_roof_open = true;
		status_roof_tipped = true;
	}
	else if ((payload[0] == 0x4F) && (payload[1] == 0xFF) && (payload[2] == 0xFE))
	{
		status_roof_open = true;
		status_roof_tipped = false;
	}
	else
	{
		status_roof_open = false;
		status_roof_tipped = false;
	}
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("roof_open", status_roof_open);
		this->data_write("roof_tipped", status_roof_tipped);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_FASTEN_SEATBELT_SIGN))
INT8U BMWXCAN::parse_fasten_seatbelt_sign(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "KOMBI");

	status_fasten_seatbelt_sign_active = ((payload[3] == 0x29) ? true : false);
	
#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
		this->data_write("fasten_seatbelt_sign_active", status_fasten_seatbelt_sign_active);
#endif
	return BMWXCAN_CMP_OK;
}
#endif

#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_CONTROL_SUN_ROLLERS))
INT8U BMWXCAN::parse_control_sun_rollers(INT8U *payload)
{
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");

	status_sun_rollers_move_rear_window = ((payload[0] == 0xFD) ? true : false);
	status_sun_rollers_move_all = ((payload[0] == 0xFE) ? true : false);

#ifndef WITH_TARGET_ARDUINO	
	if (this->message_parse_output == true)
	{
		this->data_write("sun_rollers_move_rear_window", status_sun_rollers_move_rear_window);
		this->data_write("sun_rollers_move_all", status_sun_rollers_move_all);
	}
#endif
	return BMWXCAN_CMP_OK;
}
#endif

INT8U BMWXCAN::parse_registration(INT8U *payload)
{
#ifndef LICENSED_FOR_VIN
	return BMWXCAN_CMP_IMMIDIATE_EXIT;
#else
	char vin[7];
	
	memset(vin, '\0', sizeof vin);
	sprintf(vin, "%s", LICENSED_FOR_VIN);
	
	if (strcmp(value_vin, vin) == 0)
	{
	  return BMWXCAN_CMP_OK;
	}
	
	return BMWXCAN_CMP_IMMIDIATE_EXIT;
#endif
}

INT8U BMWXCAN::message_parse(INT32U can_id, INT8U *payload, INT8U length)
{
#ifndef WITH_TARGET_ARDUINO
	buffering_db_open();
	if (this->message_parse_output == true)
	  data_write("is_parsing", (bool)true);
#endif
	
	INT8U retval = BMWXCAN_CMP_UNREGISTERED_ID;

	last_message_time = millis();
	last_message_length = length;
	last_can_id = can_id;

	memset(last_message_data, '\0', sizeof(last_message_data));
	bytecpy(last_message_data, payload);
	
	memset(last_message_source, '\0', sizeof(last_message_source));
	bytecpy(last_message_source, "unknown");
	
	switch (can_id)
	{
#if (defined(WITH_CAN_ID_THROTTLE_AND_RPM_F) || defined(WITH_CAN_ID_ALL)) && defined(WITH_F_SERIES)
		case BMWXCAN_CAN_ID_THROTTLE_AND_RPM_F:
			if (length != BMWXCAN_MSG_LENGTH_THROTTLE_AND_RPM_F)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_throttle_and_rpm_f(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_TORQUE_CLUTCH_BRAKE))
		case BMWXCAN_CAN_ID_TORQUE_CLUTCH_BRAKE:
			if (length != BMWXCAN_MSG_LENGTH_TORQUE_CLUTCH_BRAKE)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_torque_clutch_brake(payload);
			break;
#endif
#if defined(WITH_E_SERIES) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_RPM_AND_THROTTLE_POSITION))
		case BMWXCAN_CAN_ID_RPM_AND_THROTTLE_POSITION:
			if (length != BMWXCAN_MSG_LENGTH_RPM_AND_THROTTLE_POSITION)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_rpm_and_throttle_position(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_ALIVE_COUNTER))
		case BMWXCAN_CAN_ID_ALIVE_COUNTER:
			if (length != BMWXCAN_MSG_LENGTH_ALIVE_COUNTER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_alive_counter(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_ABS_BRAKE_COUNTER))
		case BMWXCAN_CAN_ID_ABS_BRAKE_COUNTER:
			if (length != BMWXCAN_MSG_LENGTH_ABS_BRAKE_COUNTER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_abs_brake_counter(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_STEERING_WHEEL_POS_A))
		case BMWXCAN_CAN_ID_STEERING_WHEEL_POS_A:
			if (length != BMWXCAN_MSG_LENGTH_STEERING_WHEEL_POS_A)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_steering_wheel_pos_a(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_STEERING_WHEEL_POS_B))
		case BMWXCAN_CAN_ID_STEERING_WHEEL_POS_B:
			if (length != BMWXCAN_MSG_LENGTH_STEERING_WHEEL_POS_B)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_steering_wheel_pos_b(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_INDIVIDUAL_WHEEL_SPEED))
		case BMWXCAN_CAN_ID_INDIVIDUAL_WHEEL_SPEED:
			if (length != BMWXCAN_MSG_LENGTH_INDIVIDUAL_WHEEL_SPEED)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_individual_wheel_speed(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_AIRBAG_SEATBELT_COUNTER))
		case BMWXCAN_CAN_ID_AIRBAG_SEATBELT_COUNTER:
			if (length != BMWXCAN_MSG_LENGTH_AIRBAG_SEATBELT_COUNTER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_airbag_seatbelt_counter(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_DOOR_STATUS_FRONT_PASSENGER))
		case BMWXCAN_CAN_ID_DOOR_STATUS_FRONT_PASSENGER:
			if (length != BMWXCAN_MSG_LENGTH_DOOR_STATUS_FRONT_PASSENGER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_door_status_front_passenger(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_DOOR_STATUS_REAR_PASSENGER))
		case BMWXCAN_CAN_ID_DOOR_STATUS_REAR_PASSENGER:
			if (length != BMWXCAN_MSG_LENGTH_DOOR_STATUS_REAR_PASSENGER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_door_status_rear_passenger(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_DOOR_STATUS_FRONT_DRIVER))
		case BMWXCAN_CAN_ID_DOOR_STATUS_FRONT_DRIVER:
			if (length != BMWXCAN_MSG_LENGTH_DOOR_STATUS_FRONT_DRIVER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_door_status_front_driver(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_DOOR_STATUS_REAR_DRIVER))
		case BMWXCAN_CAN_ID_DOOR_STATUS_REAR_DRIVER:
			if (length != BMWXCAN_MSG_LENGTH_DOOR_STATUS_REAR_DRIVER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_door_status_rear_driver(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_BOOT_STATUS))
		case BMWXCAN_CAN_ID_BOOT_STATUS:
			if (length != BMWXCAN_MSG_LENGTH_BOOT_STATUS)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_boot_status(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_WINDOW_LIFTER_CONTROL_FRONT_DRIVER))
		case BMWXCAN_CAN_ID_WINDOW_LIFTER_CONTROL_FRONT_DRIVER:
			if (length != BMWXCAN_MSG_LENGTH_WINDOW_LIFTER_CONTROL_FRONT_DRIVER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_window_lifter_control_front_driver(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_WINDOW_LIFTER_CONTROL_FRONT_PASSENGER))
		case BMWXCAN_CAN_ID_WINDOW_LIFTER_CONTROL_FRONT_PASSENGER:
			if (length != BMWXCAN_MSG_LENGTH_WINDOW_LIFTER_CONTROL_FRONT_PASSENGER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_window_lifter_control_front_passenger(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_WINDOW_LIFTER_CONTROL_REAR_DRIVER))
		case BMWXCAN_CAN_ID_WINDOW_LIFTER_CONTROL_REAR_DRIVER:
			if (length != BMWXCAN_MSG_LENGTH_WINDOW_LIFTER_CONTROL_REAR_DRIVER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_window_lifter_control_rear_driver(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_WINDOW_LIFTER_CONTROL_REAR_PASSENGER))
		case BMWXCAN_CAN_ID_WINDOW_LIFTER_CONTROL_REAR_PASSENGER:
			if (length != BMWXCAN_MSG_LENGTH_WINDOW_LIFTER_CONTROL_REAR_PASSENGER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_window_lifter_control_rear_passenger(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_IGNITION_STATUS))
		case BMWXCAN_CAN_ID_IGNITION_STATUS:
			if (length != BMWXCAN_MSG_LENGTH_IGNITION_STATUS)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_ignition_status(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_TIMER_AND_CRUISE_CONTROL_STATUS))
		case BMWXCAN_CAN_ID_TIMER_AND_CRUISE_CONTROL_STATUS:
			if (length != BMWXCAN_MSG_LENGTH_TIMER_AND_CRUISE_CONTROL_STATUS)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_timer_and_cruise_control_status(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_ABS_BRAKE))
		case BMWXCAN_CAN_ID_ABS_BRAKE:
			if (length != BMWXCAN_MSG_LENGTH_ABS_BRAKE)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_abs_brake(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_SPEED))
		case BMWXCAN_CAN_ID_SPEED:
			if (length != BMWXCAN_MSG_LENGTH_SPEED)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_speed(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_SPEED_HANDBRAKE))
		case BMWXCAN_CAN_ID_SPEED_HANDBRAKE:
			if (length != BMWXCAN_MSG_LENGTH_SPEED_HANDBRAKE)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_speed_handbrake(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_I_DRIVE_CONTROL))
		case BMWXCAN_CAN_ID_I_DRIVE_CONTROL:
			if (length != BMWXCAN_MSG_LENGTH_I_DRIVE_CONTROL)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_i_drive_control(payload);
			break;			
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_PDC_SENSOR_DATA))
		case BMWXCAN_CAN_ID_PDC_SENSOR_DATA:
			if (length != BMWXCAN_MSG_LENGTH_PDC_SENSOR_DATA)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_pdc_sensor_data(payload);
			break;			
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_PDC_BUTTON))
		case BMWXCAN_CAN_ID_PDC_BUTTON:
			if (length != BMWXCAN_MSG_LENGTH_PDC_BUTTON)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_pdc_button(payload);
			break;			
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_ENGINE_TEMP_AND_PRESSURE))
		case BMWXCAN_CAN_ID_ENGINE_TEMP_AND_PRESSURE:
			if (length != BMWXCAN_MSG_LENGTH_ENGINE_TEMP_AND_PRESSURE)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_engine_temp_and_pressure(payload);
			break;			
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_STEERING_WHEEL_BUTTONS))
		case BMWXCAN_CAN_ID_STEERING_WHEEL_BUTTONS:
			if (length != BMWXCAN_MSG_LENGTH_STEERING_WHEEL_BUTTONS)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_steering_wheel_buttons(payload);
			break;					
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_INTERIOR_LIGHT_SWITCH))
		case BMWXCAN_CAN_ID_INTERIOR_LIGHT_SWITCH:
			if (length != BMWXCAN_MSG_LENGTH_INTERIOR_LIGHT_SWITCH)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_interior_light_switch(payload);
			break;					
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_SEAT_HEATING_DRIVER))
		case BMWXCAN_CAN_ID_SEAT_HEATING_DRIVER:
			if (length != BMWXCAN_MSG_LENGTH_SEAT_HEATING_DRIVER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_seat_heating_driver(payload);
			break;					
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_SEAT_HEATING_PASSENGER))
		case BMWXCAN_CAN_ID_SEAT_HEATING_PASSENGER:
			if (length != BMWXCAN_MSG_LENGTH_SEAT_HEATING_PASSENGER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_seat_heating_passenger(payload);
			break;					
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_ADJUST_STEERING_WHEEL))
		case BMWXCAN_CAN_ID_ADJUST_STEERING_WHEEL:
			if (length != BMWXCAN_MSG_LENGTH_ADJUST_STEERING_WHEEL)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_adjust_steering_wheel(payload);
			break;					
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_INDICATOR_STALK))
		case BMWXCAN_CAN_ID_INDICATOR_STALK:
			if (length != BMWXCAN_MSG_LENGTH_INDICATOR_STALK)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_indicator_stalk(payload);
			break;					
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_SEAT_ADJUSTMENT_DRIVER))
		case BMWXCAN_CAN_ID_SEAT_ADJUSTMENT_DRIVER:
			if (length != BMWXCAN_MSG_LENGTH_SEAT_ADJUSTMENT_DRIVER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_seat_adjustment_driver(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_INDICATORS))
		case BMWXCAN_CAN_ID_INDICATORS:
			if (length != BMWXCAN_MSG_LENGTH_INDICATORS)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_indicators(payload);
			break;
#endif
		case BMWXCAN_CAN_ID_CRASH:
			if (length != BMWXCAN_MSG_LENGTH_CRASH)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_crash(payload);
			break;
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_CRUISE_CONTROL_SPEED_STEP))
		case BMWXCAN_CAN_ID_CRUISE_CONTROL_SPEED_STEP:
			if (length != BMWXCAN_MSG_LENGTH_CRUISE_CONTROL_SPEED_STEP)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_cruise_control_speed_step(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_INSTRUMENTS_DIMMER))
		case BMWXCAN_CAN_ID_INSTRUMENTS_DIMMER:
			if (length != BMWXCAN_MSG_LENGTH_INSTRUMENTS_DIMMER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_instruments_dimmer(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_LIGHTS_SWITCH))
		case BMWXCAN_CAN_ID_LIGHTS_SWITCH:
			if (length != BMWXCAN_MSG_LENGTH_LIGHTS_SWITCH)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_lights_switch(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_CARKEY_BUTTON))
		case BMWXCAN_CAN_ID_CARKEY_BUTTON:
			if (length != BMWXCAN_MSG_LENGTH_CARKEY_BUTTON)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_carkey_button(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_AIRCONDITION))
		case BMWXCAN_CAN_ID_AIRCONDITION:
			if (length != BMWXCAN_MSG_LENGTH_AIRCONDITION)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_aircondition(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_AIRCONDITION_DEMISTER))
		case BMWXCAN_CAN_ID_AIRCONDITION_DEMISTER:
			if (length != BMWXCAN_MSG_LENGTH_AIRCONDITION_DEMISTER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_aircondition_demister(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_PDC))
		case BMWXCAN_CAN_ID_PDC:
			if (length != BMWXCAN_MSG_LENGTH_PDC)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_pdc(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_WINDSCREEN_WIPER_STATUS))
		case BMWXCAN_CAN_ID_WINDSCREEN_WIPER_STATUS:
			if (length != BMWXCAN_MSG_LENGTH_WINDSCREEN_WIPER_STATUS)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_windscreen_wiper_status(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_I_DRIVEH_CONTROLLER_ROTARY_CONTROL))
		case BMWXCAN_CAN_ID_I_DRIVEH_CONTROLLER_ROTARY_CONTROL:
			if (length != BMWXCAN_MSG_LENGTH_I_DRIVEH_CONTROLLER_ROTARY_CONTROL)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_i_driveh_controller_rotary_control(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_I_DRIVEH_CONTROLLER_DIRECTION_BUTTONS))
		case BMWXCAN_CAN_ID_I_DRIVEH_CONTROLLER_DIRECTION_BUTTONS:
			if (length != BMWXCAN_MSG_LENGTH_I_DRIVEH_CONTROLLER_DIRECTION_BUTTONS)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_i_driveh_controller_direction_buttons(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_CONTROL_SUN_ROLLERS))
		case BMWXCAN_CAN_ID_CONTROL_SUN_ROLLERS:
			if (length != BMWXCAN_MSG_LENGTH_CONTROL_SUN_ROLLERS)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_control_sun_rollers(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_RADIO_STATUS))
		case BMWXCAN_CAN_ID_RADIO_STATUS:
			if (length != BMWXCAN_MSG_LENGTH_RADIO_STATUS)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_radio_status(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_REAR_VIEW_MIRROR_LIGHT_SENSOR))
		case BMWXCAN_CAN_ID_REAR_VIEW_MIRROR_LIGHT_SENSOR:
			if (length != BMWXCAN_MSG_LENGTH_REAR_VIEW_MIRROR_LIGHT_SENSOR)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_rear_view_mirror_light_sensor(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_WIPER_STALK))
		case BMWXCAN_CAN_ID_WIPER_STALK:
			if (length != BMWXCAN_MSG_LENGTH_WIPER_STALK)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_wiper_stalk(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_STOP_WATCH))
		case BMWXCAN_CAN_ID_STOP_WATCH:
			if (length != BMWXCAN_MSG_LENGTH_STOP_WATCH)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_stop_watch(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_DOOR_LOCKING_VALIDATION))
		case BMWXCAN_CAN_ID_DOOR_LOCKING_VALIDATION:
			if (length != BMWXCAN_MSG_LENGTH_DOOR_LOCKING_VALIDATION)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_door_locking_validation(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_BC_RESET_AVERAGE_SPEED_MPG))
		case BMWXCAN_CAN_ID_BC_RESET_AVERAGE_SPEED_MPG:
			if (length != BMWXCAN_MSG_LENGTH_BC_RESET_AVERAGE_SPEED_MPG)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_bc_reset_average_speed_mpg(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_OUTSIDE_TEMPERATURE))
		case BMWXCAN_CAN_ID_OUTSIDE_TEMPERATURE:
			if (length != BMWXCAN_MSG_LENGTH_OUTSIDE_TEMPERATURE)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_outside_temperature(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_STATUS_REAR_WINDOW_HEATING))
		case BMWXCAN_CAN_ID_STATUS_REAR_WINDOW_HEATING:
			if (length != BMWXCAN_MSG_LENGTH_STATUS_REAR_WINDOW_HEATING)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_status_rear_window_heating(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_AIRCONDITION_STATUS))
		case BMWXCAN_CAN_ID_AIRCONDITION_STATUS:
			if (length != BMWXCAN_MSG_LENGTH_AIRCONDITION_STATUS)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_aircondition_status(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_AIRCONDITION_DRIVER))
		case BMWXCAN_CAN_ID_AIRCONDITION_DRIVER:
			if (length != BMWXCAN_MSG_LENGTH_AIRCONDITION_DRIVER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_aircondition_driver(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_AIRCONDITION_PASSENGER))
		case BMWXCAN_CAN_ID_AIRCONDITION_PASSENGER:
			if (length != BMWXCAN_MSG_LENGTH_AIRCONDITION_PASSENGER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_aircondition_passenger(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_INSIDE_LIGHTS))
		case BMWXCAN_CAN_ID_INSIDE_LIGHTS:
			if (length != BMWXCAN_MSG_LENGTH_INSIDE_LIGHTS)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_inside_lights(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_DATE_TIME))
		case BMWXCAN_CAN_ID_DATE_TIME:
			if (length != BMWXCAN_MSG_LENGTH_DATE_TIME)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_date_time(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_SEATBELT_SENSORS))
		case BMWXCAN_CAN_ID_SEATBELT_SENSORS:
			if (length != BMWXCAN_MSG_LENGTH_SEATBELT_SENSORS)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_seatbelt_sensors(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_DOOR_BOOT_BONNET_STATUS))
		case BMWXCAN_CAN_ID_DOOR_BOOT_BONNET_STATUS:
			if (length != BMWXCAN_MSG_LENGTH_DOOR_BOOT_BONNET_STATUS)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_door_boot_bonnet_status(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_GEAR_LEVER))
		case BMWXCAN_CAN_ID_GEAR_LEVER:
			if (length != BMWXCAN_MSG_LENGTH_GEAR_LEVER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_gear_lever(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_CAR_TILT))
		case BMWXCAN_CAN_ID_CAR_TILT:
			if (length != BMWXCAN_MSG_LENGTH_CAR_TILT)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_car_tilt(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_STATUS_TIRE_SPAN_DISPLAY))
		case BMWXCAN_CAN_ID_STATUS_TIRE_SPAN_DISPLAY:
			if (length != BMWXCAN_MSG_LENGTH_STATUS_TIRE_SPAN_DISPLAY)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_status_tire_span_display(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_SECONDS_SINCE_BAT_REMOVAL))
		case BMWXCAN_CAN_ID_SECONDS_SINCE_BAT_REMOVAL:
			if (length != BMWXCAN_MSG_LENGTH_SECONDS_SINCE_BAT_REMOVAL)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_seconds_since_bat_removal(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_INSIDE_TEMP_LIGHT_SOLAR_SENSORS))
		case BMWXCAN_CAN_ID_INSIDE_TEMP_LIGHT_SOLAR_SENSORS:
			if (length != BMWXCAN_MSG_LENGTH_INSIDE_TEMP_LIGHT_SOLAR_SENSORS)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_inside_temp_light_solar_sensors(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_MILEAGE_RANGE_TANK_LEVEL))
		case BMWXCAN_CAN_ID_MILEAGE_RANGE_TANK_LEVEL:
			if (length != BMWXCAN_MSG_LENGTH_MILEAGE_RANGE_TANK_LEVEL)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_mileage_range_tank_level(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_CHECKCONTROL_MESSAGE_MIL))
		case BMWXCAN_CAN_ID_CHECKCONTROL_MESSAGE_MIL:
			if (length != BMWXCAN_MSG_LENGTH_CHECKCONTROL_MESSAGE_MIL)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_checkcontrol_message_mil(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_CHECKCONTROL_MESSAGE_DSC))
		case BMWXCAN_CAN_ID_CHECKCONTROL_MESSAGE_DSC:
			if (length != BMWXCAN_MSG_LENGTH_CHECKCONTROL_MESSAGE_DSC)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_checkcontrol_message_dsc(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_STATUS_HANDBRAKE))
		case BMWXCAN_CAN_ID_STATUS_HANDBRAKE:
			if (length != BMWXCAN_MSG_LENGTH_STATUS_HANDBRAKE)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_status_handbrake(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_FUEL_LEVEL_SENSORS))
		case BMWXCAN_CAN_ID_FUEL_LEVEL_SENSORS:
			if (length != BMWXCAN_MSG_LENGTH_FUEL_LEVEL_SENSORS)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_fuel_level_sensors(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_AVERAGE_SPEED_AND_MILEAGE))
		case BMWXCAN_CAN_ID_AVERAGE_SPEED_AND_MILEAGE:
			if (length != BMWXCAN_MSG_LENGTH_AVERAGE_SPEED_AND_MILEAGE)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_average_speed_and_mileage(payload);
			break;
#endif
		case BMWXCAN_CAN_ID_VIN:
			if (length != BMWXCAN_MSG_LENGTH_VIN)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_vin(payload);
			break;
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_STATUS_BATTERY_AND_ENGINE))
		case BMWXCAN_CAN_ID_STATUS_BATTERY_AND_ENGINE:
			if (length != BMWXCAN_MSG_LENGTH_STATUS_BATTERY_AND_ENGINE)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_status_battery_and_engine(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_POSITION_WINDOW_FRONT_DRIVER))
		case BMWXCAN_CAN_ID_POSITION_WINDOW_FRONT_DRIVER:
			if (length != BMWXCAN_MSG_LENGTH_POSITION_WINDOW_FRONT_DRIVER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_position_window_front_driver(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_POSITION_WINDOW_REAR_DRIVER))
		case BMWXCAN_CAN_ID_POSITION_WINDOW_REAR_DRIVER:
			if (length != BMWXCAN_MSG_LENGTH_POSITION_WINDOW_REAR_DRIVER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_position_window_rear_driver(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_POSITION_WINDOW_FRONT_PASSENGER))
		case BMWXCAN_CAN_ID_POSITION_WINDOW_FRONT_PASSENGER:
			if (length != BMWXCAN_MSG_LENGTH_POSITION_WINDOW_FRONT_PASSENGER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_position_window_front_passenger(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_POSITION_WINDOW_REAR_PASSENGER))
		case BMWXCAN_CAN_ID_POSITION_WINDOW_REAR_PASSENGER:
			if (length != BMWXCAN_MSG_LENGTH_POSITION_WINDOW_REAR_PASSENGER)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_position_window_rear_passenger(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_POSITION_ROOF))
		case BMWXCAN_CAN_ID_POSITION_ROOF:
			if (length != BMWXCAN_MSG_LENGTH_POSITION_ROOF)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_position_roof(payload);
			break;
#endif
#if (defined(WITH_E_SERIES) || defined(WITH_F_SERIES)) && (defined(WITH_CAN_ID_ALL) || defined(WITH_CAN_ID_FASTEN_SEATBELT_SIGN))
		case BMWXCAN_CAN_ID_FASTEN_SEATBELT_SIGN:
			if (length != BMWXCAN_MSG_LENGTH_FASTEN_SEATBELT_SIGN)
				retval = BMWXCAN_CMP_INVALID_LENGTH;
			else
				retval = parse_fasten_seatbelt_sign(payload);
			break;
#endif
#ifdef WITH_IGNORED_IDS
		case BMWXCAN_CAN_ID_SPEED_K_CAN:
		case BMWXCAN_CAN_ID_SPEED_K_CAN_2:
		case BMWXCAN_CAN_ID_WARMTH_CLIMATE:
		case BMWXCAN_CAN_ID_WARMTH_MOTOR:
		case BMWXCAN_CAN_ID_DISPLAY_GEARING_DATA:
		case BMWXCAN_CAN_ID_COUNTER_AND_DOOR_STATUS:
		case BMWXCAN_CAN_ID_DOOR_STATUS_IGN:
		case BMWXCAN_CAN_ID_CONTROL_ROOF:
		case BMWXCAN_CAN_ID_I_DRIVEH_CONTROLLER_REPLY:
		case BMWXCAN_CAN_ID_CONTROL_CENTRAL_LOCKING:
		case BMWXCAN_CAN_ID_LCD_BRIGHTNESS:
		case BMWXCAN_CAN_ID_CONTROL_MONITOR:
		case BMWXCAN_CAN_ID_STATUS_CLIMATE_STANDOPS:
		case BMWXCAN_CAN_ID_CONTROL_EXTEN_WATERPUMP:
		case BMWXCAN_CAN_ID_STATUS_SERVICE_CALL:
		case BMWXCAN_CAN_ID_STATUS_DRIVING_LIGHTS:
		case BMWXCAN_CAN_ID_VEHICLE_MODE:
		case BMWXCAN_CAN_ID_CONTROL_ALC:
		case BMWXCAN_CAN_ID_DRIVER_DISPLAY_ROTATIONAREA:
		case BMWXCAN_CAN_ID_STATUS_FUEL_PUMP:
		case BMWXCAN_CAN_ID_STATUS_MONITOR_FRONT:
		case BMWXCAN_CAN_ID_BORDCOMPUTER_DATA:
		case BMWXCAN_CAN_ID_BORDCOMPUTER_TRAVEL_DATA:
		case BMWXCAN_CAN_ID_BORDCOMPUTER_START_DATA:
		case BMWXCAN_CAN_ID_BORDCOMPUTER_INFO_DATA:
		case BMWXCAN_CAN_ID_OUTSIDE_TEMP_AND_RANGE:
		case BMWXCAN_CAN_ID_CONTROL_DISPLAY_SERVICEREQ:
		case BMWXCAN_CAN_ID_RDA_REQUEST_DATASTORE:
		case BMWXCAN_CAN_ID_SET_TIME_AND_DATE:
		case BMWXCAN_CAN_ID_STATUS_REVERSE:
		case BMWXCAN_CAN_ID_POWERMANAGEMENT_CONTROL:
		case BMWXCAN_CAN_ID_STATUS_CONSUMER_SHUTDOWN:
		case BMWXCAN_CAN_ID_POST_TIME_POWER_SUPPLY:
		case BMWXCAN_CAN_ID_STATUS_MEMORYPOS_CKM:
		case BMWXCAN_CAN_ID_STATUS_LIGHT_CKM:
		case BMWXCAN_CAN_ID_STATUS_CLIMATE_CKM:
		case BMWXCAN_CAN_ID_KEYFOB:
			// CURRENTLY IGNORED
			retval = BMWXCAN_CMP_IGNORED_ID;
			break;
#endif
#ifdef WITH_UNKNOWN_IDS
		case BMWXCAN_CAN_ID_UNKNOWN_1:
		case BMWXCAN_CAN_ID_UNKNOWN_2:
		case BMWXCAN_CAN_ID_UNKNOWN_3:
		case BMWXCAN_CAN_ID_UNKNOWN_4:
		case BMWXCAN_CAN_ID_UNKNOWN_5:
		case BMWXCAN_CAN_ID_UNKNOWN_6:
		case BMWXCAN_CAN_ID_UNKNOWN_7:
		case BMWXCAN_CAN_ID_UNKNOWN_8:
		case BMWXCAN_CAN_ID_UNKNOWN_9:
		case BMWXCAN_CAN_ID_UNKNOWN_10:
		case BMWXCAN_CAN_ID_UNKNOWN_11:
		case BMWXCAN_CAN_ID_UNKNOWN_12:
		case BMWXCAN_CAN_ID_UNKNOWN_13:
		case BMWXCAN_CAN_ID_UNKNOWN_14:
		case BMWXCAN_CAN_ID_UNKNOWN_15:
		case BMWXCAN_CAN_ID_UNKNOWN_16:
		case BMWXCAN_CAN_ID_UNKNOWN_17:
		case BMWXCAN_CAN_ID_UNKNOWN_18:
		case BMWXCAN_CAN_ID_UNKNOWN_19:
		case BMWXCAN_CAN_ID_UNKNOWN_20:
		case BMWXCAN_CAN_ID_UNKNOWN_21:
		case BMWXCAN_CAN_ID_UNKNOWN_22:
		case BMWXCAN_CAN_ID_UNKNOWN_23:
		case BMWXCAN_CAN_ID_UNKNOWN_24:
		case BMWXCAN_CAN_ID_UNKNOWN_25:
		case BMWXCAN_CAN_ID_UNKNOWN_26:
		case BMWXCAN_CAN_ID_UNKNOWN_27:
		case BMWXCAN_CAN_ID_UNKNOWN_28:
		case BMWXCAN_CAN_ID_UNKNOWN_29:
		case BMWXCAN_CAN_ID_UNKNOWN_30:
		case BMWXCAN_CAN_ID_UNKNOWN_31:
		case BMWXCAN_CAN_ID_UNKNOWN_33:
		case BMWXCAN_CAN_ID_UNKNOWN_37:
		case BMWXCAN_CAN_ID_UNKNOWN_38:
		case BMWXCAN_CAN_ID_UNKNOWN_45:
		case BMWXCAN_CAN_ID_UNKNOWN_58:
		case BMWXCAN_CAN_ID_UNKNOWN_60:
		case BMWXCAN_CAN_ID_UNKNOWN_61:
		case BMWXCAN_CAN_ID_UNKNOWN_63:
		case BMWXCAN_CAN_ID_UNKNOWN_69:
		case BMWXCAN_CAN_ID_UNKNOWN_70:
		case BMWXCAN_CAN_ID_UNKNOWN_71:
		case BMWXCAN_CAN_ID_UNKNOWN_72:
		case BMWXCAN_CAN_ID_UNKNOWN_73:
		case BMWXCAN_CAN_ID_UNKNOWN_74:
		case BMWXCAN_CAN_ID_UNKNOWN_75:
		case BMWXCAN_CAN_ID_UNKNOWN_76:
		case BMWXCAN_CAN_ID_UNKNOWN_77:
			retval = BMWXCAN_CMP_UNKNOWN_ID;
			break;
#endif
	}
	
	if (retval == BMWXCAN_CMP_OK)
		last_successful_message = millis();
	
	last_message_result = retval;
	
#ifndef WITH_TARGET_ARDUINO
	database_write(can_id, payload, length, retval);
	if (this->message_parse_output == true)
	  data_write("is_parsing", (bool)false);
#endif
	
#ifndef LICENSED_FOR_VIN
	message_count++;
	
	if (message_count > 65534)
	  exit(BMWXCAN_CMP_IMMIDIATE_EXIT);
#endif
	
	if (retval == BMWXCAN_CMP_IMMIDIATE_EXIT)
	  exit(BMWXCAN_CMP_IMMIDIATE_EXIT);
	
	return retval;
}

bool BMWXCAN::check_license()
{
#ifndef WITH_TARGET_ARDUINO
#ifndef LICENSED_FOR_VIN
  return false;
#else
  std::stringstream lc;
  
  lc << LICENSED_FOR_VIN << "libbmwxcan" << LICENSED_FOR_NAME << "Oliver Welter" << std::endl;
  
  std::string str(lc.str());
  
  unsigned char result[MD5_DIGEST_LENGTH];
  MD5((unsigned char*)str.c_str(), str.size(), result);

  std::ostringstream sout;
  sout<<std::hex<<std::setfill('0');
  
  for(long long c: result)
  {
    sout<<std::setw(2)<<(long long)c;
  }
  
  std::string ckey(sout.str());
  std::string lkey(LICENSED_WITH_KEY);

  if (ckey.compare(lkey) == 0)
    return true;
#endif
#else
  char *lc;
  
  sprintf(lc, "%s%s%s%s", LICENSED_FOR_VIN, "libbmwxcan", LICENSED_FOR_NAME, "Oliver Welter");

  if (sizeof lc > 30)
    return true;
#endif
  
  return false;
}
