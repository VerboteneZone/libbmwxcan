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

#ifndef libbmwxcan_h
#define libbmwxcan_h

#ifndef WITH_TARGET_ARDUINO
#include "config.h"
#endif

#include "defines.h"

#ifndef WITH_TARGET_ARDUINO
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <utility>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <mysql/mysql.h>
#include <openssl/md5.h>
#else
#include <Arduino.h>
#endif

#ifndef PROGNAME
#define PROGNAME "libbmwxcan"
#endif

#ifndef PROGVER
#define PROGVER "1.0"
#endif

class BMWXCAN
{
public:
	BMWXCAN();
	
	INT8U message_parse(INT32U can_id, INT8U *payload, INT8U length);
	boolean writing_allowed(INT32U can_id);
#ifndef WITH_TARGET_ARDUINO
	~BMWXCAN();
	
	boolean message_parse_output = false;
	boolean fifo_writing_enabled = false;
	boolean dbdata_writing_enabled = false;
	boolean dbdata_crash_snapshots = false;
	boolean buffering_enabled = false;
	boolean buffering_memory_prefilter_enabled = false;
	boolean buffering_raw_payload_table_enabled = false;
	boolean buffering_translation_table_enabled = false;

	char *path_realtime_data = CONFIG_PATH_REALTIME_DATA;
	
	char *mysql_hostname = CONFIG_MYSQL_DEFAULT_HOSTNAME;
	char *mysql_username = CONFIG_MYSQL_DEFAULT_USERNAME;
	char *mysql_password = CONFIG_MYSQL_DEFAULT_PASSWORD;
	char *mysql_database = CONFIG_MYSQL_DEFAULT_DATABASE;
	int mysql_port = CONFIG_MYSQL_DEFAULT_PORT;
#endif		
	unsigned long long last_message_time				= 0;
	unsigned long long last_successful_message			= 0;
	
	char last_message_source[9];
	INT8U last_message_data[9];
	INT8U last_message_length                           = 0;
	INT8U last_message_result							= 0;
	INT32U last_can_id									= 0;
		
	unsigned long value_throttle_position				= 0;
	unsigned long value_rpm								= 0;
	float value_torque									= 0;
	float value_power_kw								= 0;
	float value_power_hp								= 0;
	INT8U value_clutch									= 0;
	INT8U value_brake									= 0;
	INT8U value_alive_counter							= 0;
	INT8U value_abs_brake_counter						= 0;
	unsigned long value_steering_wheel_pos_absolute		= 0;
	unsigned long value_steering_wheel_pos_change		= 0;
	float value_steering_wheel_position_degree			= 0;
	float value_steering_wheel_change_degree			= 0;
	unsigned long value_wheel_speed_1					= 0;
	unsigned long value_wheel_speed_2					= 0;
	unsigned long value_wheel_speed_3					= 0;
	unsigned long value_wheel_speed_4					= 0;
	INT8U value_airbag_seatbelt_counter					= 0;
	INT8U value_cruise_control_timer					= 0;
	INT8U value_cruise_control_speed					= 0;
	INT8U value_abs_timer								= 0;
	INT8U value_abs_counter								= 0;
	INT8U value_brake_pressure							= 0;
	unsigned long value_speed_1							= 0;
	unsigned long value_speed_2							= 0;
	unsigned long value_speed_3							= 0;
	unsigned long value_speed_counter					= 0;
	unsigned long value_speed_mph						= 0;
	unsigned long value_speed_kmh						= 0;
	float value_speed_fps								= 0;
	float value_speed_mps								= 0;
	float value_speed_knots								= 0;
	INT8U value_i_drive_spin_sensitive					= 0;
	INT8U value_i_drive_spin_rough						= 0;
	unsigned long value_i_drive_last_action				= 0;
	INT8U value_pdc_sensor_front_1						= 0;
	INT8U value_pdc_sensor_front_2						= 0;
	INT8U value_pdc_sensor_front_3						= 0;
	INT8U value_pdc_sensor_front_4						= 0;
	INT8U value_pdc_sensor_rear_5						= 0;
	INT8U value_pdc_sensor_rear_6						= 0;
	INT8U value_pdc_sensor_rear_7						= 0;
	INT8U value_pdc_sensor_rear_8						= 0;
	long value_engine_temperature						= 0;
	float value_engine_oil_pressure_hpa					= 0;
	INT8U value_crash_counter							= 0;
	INT8U value_cruise_control_speed_step				= 0;
	INT8U value_instruments_dimmer						= 0;
	INT8U value_i_driveh_rotary_upcounter				= 0;
	INT8U value_i_driveh_rotary_direction_counter		= 0;
	INT8U value_wiper_intermittant_speed				= 0;
	INT8U value_radio_status							= 0;
	INT8U value_rear_view_mirror_light_sensor			= 0;
	unsigned long value_stop_watch						= 0;
	INT8U value_stop_watch_heartbeat					= 0;
	unsigned long value_door_locking_valid_lock_time	= 0;
	unsigned long value_door_locking_valid_unlock_time	= 0;
	float value_outside_temperature						= 0;
	long value_aircondition_temperature_driver			= 0;
	long value_aircondition_temperature_passenger		= 0;
	INT8U value_aircondition_fan_speed					= 0;
	INT8U value_date_time_hour							= 0;
	INT8U value_date_time_minute						= 0;
	INT8U value_date_time_day							= 0;
	INT8U value_date_time_month							= 0;
	unsigned long value_date_time_year					= 0;
	long value_car_tilt_rl								= 0;
	long value_car_tilt_fr								= 0;
	unsigned long value_seconds_since_bat_removal		= 0;
	unsigned long value_days_since_bat_removal			= 0;
	float value_inside_temperature						= 0;
	INT8U value_inside_light_sensor						= 0;
	INT8U value_inside_solar_sensor						= 0;
	unsigned long value_odometer						= 0;
	unsigned long value_range							= 0;
	unsigned long value_bc_reset_average_speed_time		= 0;
	unsigned long value_bc_reset_average_mpg_time		= 0;
	INT8U value_average_fuel							= 0;
	INT8U value_fuel_level_sensor_left					= 0;
	INT8U value_fuel_level_sensor_right					= 0;
	float value_average_speed							= 0;
	float value_average_mileage							= 0;
	float value_battery_voltage							= 0;
	INT8U value_position_window_front_driver			= 0;
	INT8U value_position_window_rear_driver				= 0;
	INT8U value_position_window_front_passenger			= 0;
	INT8U value_position_window_rear_passenger			= 0;
	char value_gear_lever[3];
	
	char value_vin[8];

	boolean status_date_time_is_pm					= false;
	boolean status_date_time_valid					= false;
	boolean status_brake_pressed						= false;
	boolean status_brake_foot_on						= false;
	boolean status_clutch_depressed						= false;
	boolean status_engine_running						= false;
	boolean status_accelerator_full_pushed				= false;
	boolean status_accelerator_foot_on					= false;
	boolean status_steering_wheel_turn_right			= false;
	boolean status_steering_wheel_turn_left				= false;
	boolean status_car_is_rolling						= false;
	boolean status_door_front_passenger_open			= false;
	boolean status_door_front_passenger_locked			= false;
	boolean status_door_front_passenger_alert_active	= false;
	boolean status_door_rear_passenger_open				= false;
	boolean status_door_rear_passenger_locked			= false;
	boolean status_door_rear_passenger_alert_active		= false;
	boolean status_door_front_driver_open				= false;
	boolean status_door_front_driver_locked				= false;
	boolean status_door_front_driver_alert_active		= false;
	boolean status_door_rear_driver_open				= false;
	boolean status_door_rear_driver_locked				= false;
	boolean status_door_rear_driver_alert_active		= false;
	boolean status_boot_open							= false;
	boolean status_boot_locked							= false;
	boolean status_boot_alert_active					= false;
	boolean status_boot_release_button_pressed			= false;
	boolean status_window_lifter_control_front_driver_button_front_driver_up_position_1 = false;
	boolean status_window_lifter_control_front_driver_button_front_driver_up_position_2 = false;
	boolean status_window_lifter_control_front_driver_button_rear_driver_up_position_1 = false;
	boolean status_window_lifter_control_front_driver_button_rear_driver_up_position_2 = false;
	boolean status_window_lifter_control_front_driver_button_front_passenger_up_position_1 = false;
	boolean status_window_lifter_control_front_driver_button_front_passenger_up_position_2 = false;
	boolean status_window_lifter_control_front_driver_button_rear_passenger_up_position_1 = false;
	boolean status_window_lifter_control_front_driver_button_rear_passenger_up_position_2 = false;
	boolean status_window_lifter_control_front_driver_button_front_driver_down_position_1 = false;
	boolean status_window_lifter_control_front_driver_button_front_driver_down_position_2 = false;
	boolean status_window_lifter_control_front_driver_button_rear_driver_down_position_1 = false;
	boolean status_window_lifter_control_front_driver_button_rear_driver_down_position_2 = false;
	boolean status_window_lifter_control_front_driver_button_front_passenger_down_position_1 = false;
	boolean status_window_lifter_control_front_driver_button_front_passenger_down_position_2 = false;
	boolean status_window_lifter_control_front_driver_button_rear_passenger_down_position_1 = false;
	boolean status_window_lifter_control_front_driver_button_rear_passenger_down_position_2 = false;
	boolean status_window_lifter_control_front_passenger_button_front_passenger_up_position_1 = false;
	boolean status_window_lifter_control_front_passenger_button_front_passenger_up_position_2 = false;
	boolean status_window_lifter_control_front_passenger_button_front_passenger_down_position_1 = false;
	boolean status_window_lifter_control_front_passenger_button_front_passenger_down_position_2 = false;
	boolean status_window_lifter_control_rear_passenger_button_rear_passenger_up_position_1 = false;
	boolean status_window_lifter_control_rear_passenger_button_rear_passenger_up_position_2 = false;
	boolean status_window_lifter_control_rear_passenger_button_rear_passenger_down_position_1 = false;
	boolean status_window_lifter_control_rear_passenger_button_rear_passenger_down_position_2 = false;
	boolean status_window_lifter_control_rear_driver_button_rear_driver_up_position_1 = false;
	boolean status_window_lifter_control_rear_driver_button_rear_driver_up_position_2 = false;
	boolean status_window_lifter_control_rear_driver_button_rear_driver_down_position_1 = false;
	boolean status_window_lifter_control_rear_driver_button_rear_driver_down_position_2 = false;
	boolean status_ignition_key_beeing_inserted			= false;
	boolean status_ignition_key_is_in_slot				= false;
	boolean status_ignition_key_position_1				= false;
	boolean status_ignition_key_position_2				= false;
	boolean status_ignition_key_start_engine			= false;
	boolean status_cruise_control_active				= false;
	boolean status_cruise_control_correcting_speed		= false;
	boolean status_braking								= false;
	boolean status_handbrake_active						= false;
	boolean status_i_drive_button_menu					= false;
	boolean status_i_drive_button_voice					= false;
	boolean status_i_drive_pressed						= false;
	boolean status_i_drive_up							= false;
	boolean status_i_drive_down							= false;
	boolean status_i_drive_left							= false;
	boolean status_i_drive_right						= false;
	boolean status_steering_wheel_button_volume_up 		= false;
	boolean status_steering_wheel_button_volume_down 	= false;
	boolean status_steering_wheel_button_up 			= false;
	boolean status_steering_wheel_button_down 			= false;
	boolean status_steering_wheel_button_telephone 		= false;
	boolean status_steering_wheel_button_voice 			= false;
	boolean status_steering_wheel_button_diamond		= false;
	boolean status_steering_wheel_button_star 			= false;
	boolean status_steering_wheel_button_horn			= false;
	boolean status_seat_heating_button_driver_more			= false;
	boolean status_seat_heating_button_passenger_more		= false;
	boolean status_seat_heating_button_driver_off			= false;
	boolean status_seat_heating_button_passenger_off		= false;
	boolean status_interior_light_switch				= false;
	boolean status_adjust_steering_wheel_up				= false;
	boolean status_adjust_steering_wheel_down			= false;
	boolean status_adjust_steering_wheel_closer			= false;
	boolean status_adjust_steering_wheel_farther		= false;
	boolean status_indicator_stalk_up_position_1		= false;
	boolean status_indicator_stalk_up_position_2		= false;
	boolean status_indicator_stalk_down_position_1		= false;
	boolean status_indicator_stalk_down_position_2		= false;
	boolean status_indicator_stalk_high_beam			= false;
	boolean status_indicator_stalk_flash				= false;
	boolean status_indicator_stalk_button_check			= false;
	boolean status_indicator_stalk_button_bc			= false;
	boolean status_seat_adjustment_driver_release_button= false;
	boolean status_seat_adjustment_driver_m 			= false;
	boolean status_seat_adjustment_driver_1 			= false;
	boolean status_seat_adjustment_driver_2 			= false;
	boolean status_indicators_left						= false;
	boolean status_indicators_right						= false;
	boolean status_indicators_hazzard					= false;
	boolean status_crash								= false;
	boolean status_instrument_lights_active				= false;
	boolean status_lights_switch_zero				= false;
	boolean status_lights_switch_position				= false;
	boolean status_lights_switch_crossing				= false;
	boolean status_lights_switch_auto				= false;
	boolean status_lights_fog_front_pressed				= false;
	boolean status_lights_fog_rear_pressed				= false;
	boolean status_lights_fog_both_pressed				= false;
	boolean status_lights_brakelight 					= false;
	boolean status_lights_rear_foglight 				= false;
	boolean status_lights_front_foglight				= false;
	boolean status_lights_backlight 					= false;
	boolean status_lights_sidelight 					= false;
	boolean status_lights_dipbeam 						= false;
	boolean status_lights_mainbeam 						= false;
	boolean status_lights_interior						= false;
	boolean status_lights_exterior						= false;
	boolean status_carkey_button_lock					= false;
	boolean status_carkey_button_unlock					= false;
	boolean status_carkey_button_boot					= false;	
	boolean status_aircondition_on 						= false;
	boolean status_aircondition_cooling 				= false;
	boolean status_aircondition_circulate_auto 			= false;
	boolean status_aircondition_circulate_manual 		= false;
	boolean status_aircondition_rear_demister_on		= false;
	boolean status_pdc_button_pressed			= false;
	boolean status_pdc_on								= false;
	boolean status_windscreen_wiper_going_up			= false;
	boolean status_i_driveh_button_back_pressed 		= false;
	boolean status_i_driveh_button_back_long 			= false;
	boolean status_i_driveh_button_option_pressed 		= false;
	boolean status_i_driveh_button_option_long 			= false;
	boolean status_i_driveh_button_radio_pressed 		= false;
	boolean status_i_driveh_button_radio_long 			= false;
	boolean status_i_driveh_button_cd_pressed 			= false;
	boolean status_i_driveh_button_cd_long 				= false;
	boolean status_i_driveh_button_nav_pressed 			= false;
	boolean status_i_driveh_button_nav_long 			= false;
	boolean status_i_driveh_button_tel_pressed 			= false;
	boolean status_i_driveh_button_tel_long 			= false;
	boolean status_i_driveh_joystick_or_menu_pressed 	= false;
	boolean status_i_driveh_joystick_or_menu_long 		= false;
	boolean status_i_driveh_joystick_up 				= false;
	boolean status_i_driveh_joystick_up_long 			= false;
	boolean status_i_driveh_joystick_right 				= false;
	boolean status_i_driveh_joystick_right_long 		= false;
	boolean status_i_driveh_joystick_down 				= false;
	boolean status_i_driveh_joystick_down_long 			= false;
	boolean status_i_driveh_joystick_left 				= false;
	boolean status_i_driveh_joystick_left_long 			= false;
	boolean status_wiper_intermittant 					= false;
	boolean status_wiper_continuous_standard 			= false;
	boolean status_wiper_continuous_fast 				= false;
	boolean status_wiper_wash		 					= false;
	boolean status_wiper_single_wipe 					= false;
	boolean status_wiper_rear_active 					= false;
	boolean status_wiper_rear_wash 						= false;
	boolean status_rear_window_heating					= false;
	boolean status_aircondition_active					= false;
	boolean status_inside_lights						= false;
	boolean status_seatbelt_sensor_triggered_driver		= false;
	boolean status_seatbelt_sensor_triggered_passenger	= false;
	boolean status_bonnet_open							= false;
	boolean status_gear_lever_p							= false;
	boolean status_gear_lever_r							= false;
	boolean status_gear_lever_n							= false;
	boolean status_gear_lever_d							= false;
	boolean status_gear_lever_ds						= false;
	boolean status_gear_lever_m1						= false;
	boolean status_gear_lever_m2						= false;
	boolean status_gear_lever_m3						= false;
	boolean status_gear_lever_m4						= false;
	boolean status_gear_lever_m5						= false;
	boolean status_gear_lever_m6						= false;
	boolean status_gear_lever_m7						= false;
	boolean status_tire_span_display					= false;
	boolean status_checkcontrol_message_mil				= false;
	boolean status_checkcontrol_message_mil_limited		= false;
	boolean status_checkcontrol_message_dsc				= false;
	boolean status_checkcontrol_message_dsc_limited		= false;
	boolean status_battery_charging						= false;
	boolean status_direction_window_rear_passenger_up 	= false;
	boolean status_direction_window_rear_passenger_stop = false;
	boolean status_direction_window_rear_passenger_down = false;
	boolean status_button_window_rear_passenger_pressed = false;
	boolean status_direction_window_front_passenger_up 	= false;
	boolean status_direction_window_front_passenger_stop= false;
	boolean status_direction_window_front_passenger_down= false;
	boolean status_button_window_front_passenger_pressed= false;
	boolean status_direction_window_rear_driver_up 		= false;
	boolean status_direction_window_rear_driver_stop	= false;
	boolean status_direction_window_rear_driver_down 	= false;
	boolean status_button_window_rear_driver_pressed 	= false;
	boolean status_direction_window_front_driver_up 	= false;
	boolean status_direction_window_front_driver_stop	= false;
	boolean status_direction_window_front_driver_down	= false;
	boolean status_button_window_front_driver_pressed	= false;
	boolean status_roof_open							= false;
	boolean status_roof_tipped							= false;
	boolean status_fasten_seatbelt_sign_active			= false;
	boolean status_sun_rollers_move_rear_window		= false;
	boolean status_sun_rollers_move_all			= false;
	
	unsigned long offset_throttle						= 31232;	
	INT8U speed_correction_value							= 208;	

	bool check_license();
private:
	const INT32U write_protected_can_ids[6] = {
		BMWXCAN_CAN_ID_SPEED,
		BMWXCAN_CAN_ID_AIRCONDITION,
		BMWXCAN_CAN_ID_CHECKCONTROL_MESSAGE_MIL,
		BMWXCAN_CAN_ID_CHECKCONTROL_MESSAGE_DSC,
		BMWXCAN_CAN_ID_GEAR_LEVER,
#ifdef WITH_IGNORED_IDS
		BMWXCAN_CAN_ID_OUTSIDE_TEMP_AND_RANGE
#endif
	};

#ifndef WITH_TARGET_ARDUINO
	MYSQL *buffering_db;
	char *buffering_errmsg;

	void buffering_db_open();
	void snapshot_buffering_db();
	
	boolean fifo_write(char *fifo, char *msg);
	boolean database_write(INT32U can_id, INT8U *payload, INT8U length, INT8U retval);
	boolean database_write(char *key, char *value);
	boolean data_write(char *key, char *value);
	boolean data_write(char *key, int d);
	boolean data_write(char *key, INT8U d);
	boolean data_write(char *key, INT32U d);
	boolean data_write(char *key, float f);
	boolean data_write(char *key, boolean b);
	boolean data_write(char *key, long l);
	boolean data_write(char *key, unsigned long long l);
	
	INT8U buffering_memory_payload[32768][8];
	
	boolean snapshot_created = false;
#endif		

	char * int2bin(unsigned int x);
	void bytecpy(byte *dst, byte *src);
	void bytecpy(char *dst, char *src);	
	INT8U parse_throttle_and_rpm_f(INT8U *payload);
	INT8U parse_torque_clutch_brake(INT8U *payload);
	INT8U parse_rpm_and_throttle_position(INT8U *payload);
	INT8U parse_alive_counter(INT8U *payload);
	INT8U parse_abs_brake_counter(INT8U *payload);
	INT8U parse_steering_wheel_position(INT8U *payload);
	INT8U parse_steering_wheel_pos_a(INT8U *payload);
	INT8U parse_steering_wheel_pos_b(INT8U *payload);
	INT8U parse_individual_wheel_speed(INT8U *payload);
	INT8U parse_airbag_seatbelt_counter(INT8U *payload);
	INT8U parse_door_status_front_passenger(INT8U *payload);
	INT8U parse_door_status_rear_passenger(INT8U *payload);
	INT8U parse_door_status_front_driver(INT8U *payload);
	INT8U parse_door_status_rear_driver(INT8U *payload);
	INT8U parse_boot_status(INT8U *payload);
	INT8U parse_window_lifter_control_front_driver(INT8U *payload);
	INT8U parse_window_lifter_control_front_passenger(INT8U *payload);
	INT8U parse_window_lifter_control_rear_driver(INT8U *payload);
	INT8U parse_window_lifter_control_rear_passenger(INT8U *payload);
	INT8U parse_ignition_status(INT8U *payload);
	INT8U parse_timer_and_cruise_control_status(INT8U *payload);
	INT8U parse_abs_brake(INT8U *payload);
	INT8U parse_speed(INT8U *payload);
	INT8U parse_speed_handbrake(INT8U *payload);
	INT8U parse_i_drive_control(INT8U *payload);
	INT8U parse_pdc_sensor_data(INT8U *payload);
	INT8U parse_pdc_button(INT8U *payload);
	INT8U parse_engine_temp_and_pressure(INT8U *payload);
	INT8U parse_steering_wheel_buttons(INT8U *payload);
	INT8U parse_interior_light_switch(INT8U *payload);
	INT8U parse_seat_heating_driver(INT8U *payload);
	INT8U parse_seat_heating_passenger(INT8U *payload);
	INT8U parse_adjust_steering_wheel(INT8U *payload);
	INT8U parse_indicator_stalk(INT8U *payload);
	INT8U parse_seat_adjustment_driver(INT8U *payload);
	INT8U parse_indicators(INT8U *payload);
	INT8U parse_crash(INT8U *payload);
	INT8U parse_cruise_control_speed_step(INT8U *payload);
	INT8U parse_instruments_dimmer(INT8U *payload);
	INT8U parse_lights_switch(INT8U *payload);
	INT8U parse_carkey_button(INT8U *payload);
	INT8U parse_aircondition(INT8U *payload);
	INT8U parse_aircondition_demister(INT8U *payload);
	INT8U parse_pdc(INT8U *pdc);
	INT8U parse_windscreen_wiper_status(INT8U *payload);
	INT8U parse_i_driveh_controller_rotary_control(INT8U *payload);
	INT8U parse_i_driveh_controller_direction_buttons(INT8U *payload);
	INT8U parse_rear_view_mirror_light_sensor(INT8U *payload);
	INT8U parse_wiper_stalk(INT8U *payload);
	INT8U parse_radio_status(INT8U *payload);
	INT8U parse_stop_watch(INT8U *payload);
	INT8U parse_door_locking_validation(INT8U *payload);
	INT8U parse_bc_reset_average_speed_mpg(INT8U *payload);
	INT8U parse_outside_temperature(INT8U *payload);
	INT8U parse_status_rear_window_heating(INT8U *payload);
	INT8U parse_aircondition_status(INT8U *payload);
	INT8U parse_aircondition_driver(INT8U *payload);
	INT8U parse_aircondition_passenger(INT8U *payload);
	INT8U parse_inside_lights(INT8U *payload);
	INT8U parse_date_time(INT8U *payload);
	INT8U parse_seatbelt_sensors(INT8U *payload);
	INT8U parse_door_boot_bonnet_status(INT8U *payload);
	INT8U parse_gear_lever(INT8U *payload);
	INT8U parse_car_tilt(INT8U *payload);
	INT8U parse_status_tire_span_display(INT8U *payload);
	INT8U parse_seconds_since_bat_removal(INT8U *payload);
	INT8U parse_inside_temp_light_solar_sensors(INT8U *payload);
	INT8U parse_mileage_range_tank_level(INT8U *payload);
	INT8U parse_checkcontrol_message_mil(INT8U *payload);
	INT8U parse_checkcontrol_message_dsc(INT8U *payload);
	INT8U parse_status_handbrake(INT8U *payload);
	INT8U parse_fuel_level_sensors(INT8U *payload);
	INT8U parse_average_speed_and_mileage(INT8U *payload);
	INT8U parse_vin(INT8U *payload);
	INT8U parse_status_battery_and_engine(INT8U *payload);
	INT8U parse_position_window_front_driver(INT8U *payload);
	INT8U parse_position_window_rear_driver(INT8U *payload);
	INT8U parse_position_window_front_passenger(INT8U *payload);
	INT8U parse_position_window_rear_passenger(INT8U *payload);
	INT8U parse_position_roof(INT8U *payload);
	INT8U parse_fasten_seatbelt_sign(INT8U *payload);
	INT8U parse_control_sun_rollers(INT8U *payload);
	INT8U parse_registration(INT8U *payload);

#ifndef LICENSED_FOR_VIN
	unsigned long message_count = 0;
#endif
};	
#endif
