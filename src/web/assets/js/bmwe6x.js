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

function r() { $("#main").slideUp("fast", function() { $(this).html("").show(); init(); }); } $(document).ready(function() { setTimeout('r()', 3000); });

var dataFrameBusy = false;
var dataFrameTimer = null;

var tFuel = null;
var tSpeed = null;
var tRPM = null;
var tTemp = null;
var tPS = null;
var tNM = null;

var fColorPlate = "rgba(30, 30, 30, .75)";
var fColorMajorTicks = "#ff00d4";
var fColorMinorTicks = "#cc00d4";
var fColorUnits = "#ff00d4";
var fColorNumbers = "#cc00d4";
var fColorNeedle = "#ff22f4";
var fColorNeedleEnd = "#cc11f4";
var fColorNeedleLinear = "#fafafa";
var fColorNeedleEndLinear = "#dadada";
var fColorHighlight1 = "rgba(90, 90, 90, .75)";
var fColorHighlight2 = "rgba(200, 30, 190, .75)";
var fColorValueBoxBackground = "#909090";

var fBlinkerFreq = 350;

var cBlinkerRight = 0;
var cBlinkerRightMax = 0;
var cBlinkerLeft = 0;
var cBlinkerLeftMax = 0;

var bBlinkerLeft = false;
var bBlinkerRight = false;
var bBlinkerWarn = false;
var bBlinkerWarnSet = false;

var tBlinkerLeft = null;
var tBlinkerRight = null;
var tBlinkerWarn = null;

function turn_left()
{
    turn_straight();
    
    bBlinkerLeft = true;
    
    blinker_left();
}

function turn_right()
{
    turn_straight();
    
    bBlinkerRight = true;
    
    blinker_right();
}

function turn_warn()
{
    blinker_clear();
    
    bBlinkerWarnSet = true;
    bBlinkerWarn = true;
    
    blinker_warn();
}

function turn_unwarn()
{
    blinker_clear();
    
    bBlinkerWarnSet = false;
}

function turn_straight()
{
    blinker_clear();
    
    if (bBlinkerWarnSet == true)
    {
        turn_warn();
    }
}

function blinker_clear()
{
    bBlinkerRight = false;
    bBlinkerLeft = false;
    bBlinkerWarn = false;
    
    try
    {
        clearTimeout(tBlinkerRight);
    }
    catch(e)
    {
        ;
    }
    
    try
    {
        clearTimeout(tBlinkerLeft);
    }
    catch(e)
    {
        ;
    }

    try
    {
        clearTimeout(tBlinkerWarn);
    }
    catch(e)
    {
        ;
    }
    
    $("#blinker-left").removeClass("blinker-on");
    $("#blinker-right").removeClass("blinker-on");
    $("#blinkwarn i").removeClass("blinker-warn");
}

function blinker_warn()
{
    try
    {
        clearTimeout(tBlinkerWarn);
    }
    catch(e)
    {
        ;
    }
    
    if (bBlinkerWarn == false)
    {
        $("#blinker-left").removeClass("blinker-on");
        $("#blinker-right").removeClass("blinker-on");
        $("#blinkwarn i").removeClass("blinker-warn");
        return;
    }
    
    $("#blinker-left").toggleClass("blinker-on");
    $("#blinker-right").toggleClass("blinker-on");
    $("#blinkwarn i").toggleClass("blinker-warn");
        
    tBlinkerWarn = setTimeout('blinker_warn()', fBlinkerFreq);
}

function blinker_left()
{
    try
    {
        clearTimeout(tBlinkerLeft);
    }
    catch(e)
    {
        ;
    }
    
    if (bBlinkerLeft == false)
    {
        $("#blinker-left").removeClass("blinker-on");
        return;
    }
    
    if (cBlinkerLeftMax > 0)
    {
        if (cBlinkerLeft >= (cBlinkerLeftMax * 2))
        {
            turn_straight();
            return;
        }
    }
    
    cBlinkerLeft++;
    
    if (cBlinkerLeft > 65535)
    {
        turn_straight();
        return;
    }
    
    $("#blinker-left").toggleClass("blinker-on");
        
    tBlinkerLeft = setTimeout('blinker_left()', fBlinkerFreq);
}

function blinker_right()
{
    try
    {
        clearTimeout(tBlinkerRight);
    }
    catch(e)
    {
        ;
    }
    
    if (bBlinkerRight == false)
    {
        $("#blinker-right").removeClass("blinker-on");
        return;
    }
    
    if (cBlinkerRightMax > 0)
    {
        if (cBlinkerRight >= (cBlinkerRightMax * 2))
        {
            turn_straight();
            return;
        }
    }
    
    cBlinkerRight++;
    
    if (cBlinkerRight > 65535)
    {
        turn_straight();
        return;
    }
    
    $("#blinker-right").toggleClass("blinker-on");
        
    tBlinkerRight = setTimeout('blinker_right()', fBlinkerFreq);
}

function get_dataframe()
{
  if (dataFrameBusy == true)
    return;
    
  dataFrameBusy = true;
  
  try
  {
    clearTimeout(dataFrameTimer);
  }
  catch(e)
  {
    ;
  }
  
  $.ajax({
    cache: false,
    type: "GET",
    dataType: "json",
    url: "/cgi-bin/bmwe6xcgi/dataframe.json",//?average_fuel,battery_charging,battery_voltage,bonnet_open,boot_locked,boot_open,checkcontrol_message_dsc,checkcontrol_message_dsc_limited,checkcontrol_message_mil,checkcontrol_message_mil_limited,crash,crash_counter,cruise_control_active,cruise_control_correcting_speed,date_time_day,date_time_hour,date_time_minute,date_time_month,date_time_second,date_time_year,door_front_driver_locked,door_front_driver_open,door_front_passenger_locked,door_front_passenger_open,door_rear_driver_locked,door_rear_driver_open,door_rear_passenger_locked,door_rear_passenger_open,engine_running,engine_temperature,fasten_seatbelt_sign_active,gear_lever,handbrake_active,ignition_key_beeing_inserted,ignition_key_is_in_slot,ignition_key_position_1,ignition_key_position_2,ignition_key_start_engine,indicators_hazzard,indicators_left,indicators_right,inside_temperature,lights_backlight,lights_dipbeam,lights_front_foglight,lights_mainbeam,lights_rear_foglight,odometer,outside_temperature,pdc_active,power_hp,range,rpm_value,speed_kmh,tire_span_display,torque_nm,instruments_dimmer,instrument_lights_active",
    success: function(json)
    {
      var jSpeed = 0;
      var jRPM = 0;
      var jTemp = 0;
      var jFuel = 0;
      var jPS = 0;
      var jNM = 0;
      
      var jSecond = 0;
      var jMinute = 0;
      var jHour = 0;
      var jDay = 0;
      var jMonth = 0;
      var jYear = 0;
            
      if (json.date_time_second == 253)
      {
          jSecond = "--";
          jMinute = "--";
          jHour = "--";
      }
      else
      {
          jSecond = ((json.date_time_second < 10) ? "0" : "") + json.date_time_second;
          jMinute = ((json.date_time_minute < 10) ? "0" : "") + json.date_time_minute;
          jHour = ((json.date_time_hour < 10) ? "0" : "") + json.date_time_hour;
      }
      
      jDay = ((json.date_time_day < 10) ? "0" : "") + json.date_time_day;
      jMonth = ((json.date_time_month < 10) ? "0" : "") + json.date_time_month;
      jYear = json.date_time_year;
      
      var iBattery = "fa-battery-0";
      var iOutertemp = "fa-thermometer-1";
      var iInnertemp = "fa-thermometer-1";
      var iGear = "fa-gears";
      var iStatus = "fa-thumbs-o-up";

      var sStatus = "Alles OK";
      var vStatus = "";
      var cStatus = "";
      
      var keyBeeingInserted = json.ignition_key_beeing_inserted;
      var keyInSlot = json.ignition_key_is_in_slot;
      var keyInPos1 = json.ignition_key_position_1;
      var keyInPos2 = json.ignition_key_position_2;
      var keyStartEngine = json.ignition_key_start_engine;
      var engineRunning = json.engine_running;
      var dimmerValue = Math.round(parseInt((100 / 253 * json.instruments_dimmer) / 100), 1);
      
      if (dimmerValue > 1)
          dimmerValue = 1;
      else if (dimmerValue < 0.1)
          dimmerValue = 0.1;
      
      var instrumentLightsActive = ((json.instrument_lights_active == 1) ? 1 : 0);
      var instrumentsDimmer = ((json.instrument_lights_active == 1) ? dimmerValue : 0);
      
      if (instrumentLightsActive == 1)
      {
          $("#background").css("opacity", instrumentsDimmer);
      }
      else
      {
          $("#background").css("opacity", 1);
      }
      
      if (jSecond == "undefined")
          jSecond = "--";
      if (jHour == "undefined")
          jHour = "--";
      if (jMinute == "undefined")
          jMinute = "--";
      if (jMonth == "undefined")
          jMonth = "--";
      if (jDay == "undefined")
          jDay = "--";
      if (jYear == "undefined")
          jYear = "----";
           
      $("#battery .value").html($.number(json.battery_voltage, 1, ",", "."));
      $("#gear .value").html(json.gear_lever);
      $("#clock .time").html(((jSecond == "--") ? "Datum" : jHour + ":" + jMinute + " Uhr"));
      $("#clock .date").html(jDay + "." + jMonth + "." + jYear);
      $("#outertemp .value").html($.number(json.outside_temperature, 1, ",", "."));
      
      if ((json.door_front_driver_locked == 0) && (json.door_front_passenger_locked == 0) && (json.door_rear_driver_locked == 0) && (json.door_rear_passenger_locked == 0) && (json.boot_locked == 0))
      {
          $("#lockunlock i").attr("class", "fa fa-unlock");
      }
      else if ((json.door_front_driver_locked == 1) && (json.door_front_passenger_locked == 1) && (json.door_rear_driver_locked == 1) && (json.door_rear_passenger_locked == 1) && (json.boot_locked == 1))
      {
          $("#lockunlock i").attr("class", "fa fa-lock");
      }
      else
      {
          $("#lockunlock i").attr("class", "fa fa-unlock-alt");
      }
      
      if (json.fasten_seatbelt_sign_active == 1)
      {
          $("#seatbelt").addClass("light-red");
      }
      else
      {
          $("#seatbelt").removeClass("light-red");
      }
      
      if (json.handbrake_active == 1)
      {
          $("#handbrake").addClass("light-red");
      }
      else
      {
          $("#handbrake").removeClass("light-red");
      }
      
      if ((json.cruise_control_active == 1) && (json.cruise_control_correcting_speed == 0))
      {
          $("#cruisecontrol").addClass("light-green");
      }
      else if (json.cruise_control_correcting_speed == 1)
      {
          $("#cruisecontrol").removeClass("light-green");
          $("#cruisecontrol").addClass("light-orange");
      }
      else
      {
          $("#cruisecontrol").removeClass("light-green");
          $("#cruisecontrol").removeClass("light-orange");
      }
      
      if (json.pdc_active == 1)
      {
          $("#pdc").addClass("light-green");
      }
      else
      {
          $("#pdc").removeClass("light-green");
      }
      
      if ((json.lights_rear_foglight == 0) && (json.lights_front_foglight == 0))
      {
          $("#foglight").removeClass("light-orange").removeClass("light-cyan").removeClass("light-pink");
      }
      else if ((json.lights_rear_foglight == 1) && (json.lights_front_foglight == 0))
      {
          $("#foglight").removeClass("light-orange").removeClass("light-cyan").removeClass("light-pink");
          $("#foglight").addClass("light-orange");          
      }
      else if ((json.lights_rear_foglight == 0) && (json.lights_front_foglight == 1))
      {
          $("#foglight").removeClass("light-orange").removeClass("light-cyan").removeClass("light-pink");
          $("#foglight").addClass("light-cyan");          
      }
      else if ((json.lights_rear_foglight == 1) && (json.lights_front_foglight == 1))
      {
          $("#foglight").removeClass("light-orange").removeClass("light-cyan").removeClass("light-pink");
          $("#foglight").addClass("light-pink");          
      }
      
      if ((json.lights_backlight == 0) && (json.lights_mainbeam == 0) && (json.lights_dipbeam == 0))
      {
          $("#light").removeClass("light-green").removeClass("light-white").removeClass("light-blue");
      }
      else if ((json.lights_backlight == 1) && (json.lights_mainbeam == 0) && (json.lights_dipbeam == 0))
      {
          $("#light").removeClass("light-green").removeClass("light-white").removeClass("light-blue");
          $("#light").addClass("light-white");
      }
      else if ((json.lights_mainbeam == 1) && (json.lights_dipbeam == 0))
      {
          $("#light").removeClass("light-green").removeClass("light-white").removeClass("light-blue");
          $("#light").addClass("light-green");
      }
      else if (json.lights_dipbeam == 1)
      {
          $("#light").removeClass("light-green").removeClass("light-white").removeClass("light-blue");
          $("#light").addClass("light-blue");
      }
      
      if ((json.indicators_hazzard == 1) && (bBlinkerWarnSet == false))
      {
          turn_warn();
      }
      else if ((json.indicators_left == 1) && (bBlinkerLeft == false))
      {
          turn_left();
      }
      else if ((json.indicators_right == 1) && (bBlinkerRight == false))
      {
          turn_right();
      }
      else if (((json.indicators_left == 0) && (bBlinkerLeft == true)) || ((json.indicators_right == 0) && (bBlinkerRight == true)))
      {
          turn_straight();
      }
      else if ((json.indicators_hazzard == 0) && (bBlinkerWarnSet == true))
      {
          turn_unwarn();
      }

      if ((keyInSlot == 0) && (keyInPos1 == 0) && (keyInPos2 == 0) && (keyStartEngine == 0))
      {
          $("#odometer .value").html("---");
          $("#range .value").html("---");
          
          $("#cockpit-main").hide("scale", "slow");
          $("#innertemp .value").html("-.--");
          $("#innertemp .fa").class("fa fa-temperature-1");
      }
      else
      {
          $("#odometer .value").html($.number(json.odometer, 0, ",", "."));
          $("#range .value").html($.number(json.range, 0, ",", "."));      
          
          $("#cockpit-main").show("scale", "slow");
          $("#innertemp .value").html($.number(json.inside_temperature, 1, ",", "."));
      }
      
      if ((keyBeeingInserted == 0) && (keyInSlot == 0) && (keyInPos1 == 0) && (keyInPos2 == 0) && (keyStartEngine == 0))
      {
          iStatus = "fa-power-off";
          sStatus = "Kein Schl&uuml;ssel";
          
          blinker_clear();
      }
      else if ((keyInSlot == 1) || (keyBeeingInserted == 1))
      {
          iStatus = "fa-key";
          sStatus = "Schl&uuml;ssel";
          vStatus = "steckt";
      }
      else if (keyInPos1 == 1)
      {
          iStatus = "fa-toggle-on";
          sStatus = "Motor";
          vStatus = "aus";
      }
      else if ((keyInPos2 == 1) && (engineRunning == 0))
      {
          iStatus = "fa-toggle-off";
          sStatus = "Z&uuml;ndung";
          cStatus = "codewhite";
          vStatus = "an";
      }
      else if ((keyInPos2 == 1) && (engineRunning == 1))
      {
          if (json.bonnet_open == 1)
          {
              iStatus = "fa-car";
              sStatus = "Motorhaube";
              vStatus = "offen";
              cStatus = "codeorange";
          }
          else if (json.boot_open == 1)
          {
              iStatus = "fa-taxi";
              sStatus = "Kofferraum";
              vStatus = "offen";
              cStatus = "codeorange";
          }
          else if (json.door_rear_driver_open == 1)
          {
              iStatus = "fa-folder-open-o";
              sStatus = "Fahrerfondt&uuml;r";
              vStatus = "offen";
              cStatus = "codeorange";
          }
          else if (json.door_rear_passenger_open == 1)
          {
              iStatus = "fa-folder-open-o";
              sStatus = "Beifahrerfondt&uuml;r";
              vStatus = "offen";
              cStatus = "codeorange";
          }
          else if (json.door_front_passenger_open == 1)
          {
              iStatus = "fa-folder-open-o";
              sStatus = "Beifahrert&uuml;r";
              vStatus = "offen";
              cStatus = "codeorange";
          }
          else if (json.door_front_driver_open == 1)
          {
              iStatus = "fa-folder-open-o";
              sStatus = "Fahrert&uuml;r";
              vStatus = "offen";
              cStatus = "codeorange";
          }
          else if (json.range <= 25)
          {
              iStatus = "fa-tint";
              sStatus = "Kraftstoff";
              vStatus = "kritisch";
              cStatus = "codered";
          }
          else if (json.range <= 50)
          {
              iStatus = "fa-tint";
              sStatus = "Kraftstoff";
              vStatus = "Warnung";
              cStatus = "codeorange";
          }
          else if (json.range <= 100)
          {
              iStatus = "fa-tint";
              sStatus = "Kraftstoff";
              vStatus = "Reserve";
              cStatus = "codeyellow";
          }
          else if (json.engine_temperature >= 100)
          {
              iStatus = "fa-thermometer-full";
              sStatus = "K&uuml;hlwasser";
              vStatus = "zu hei&szlig;";
              cStatus = "codered";
          }
          else if ((json.engine_temperature <= 80) && (json.rpm_value > 4500))
          {
              iStatus = "fa-tachometer";
              sStatus = "Motordrehzahl";
              vStatus = "kritisch";
              cStatus = "codered";
          }
          else if ((json.engine_temperature <= 80) && (json.rpm_value > 3500))
          {
              iStatus = "fa-tachometer";
              sStatus = "Motordrehzahl";
              vStatus = "Warnung";
              cStatus = "codeyellow";
          }
          else if ((json.engine_temperature > 80) && (json.rpm_value > 5500))
          {
              iStatus = "fa-tachometer";
              sStatus = "Motordrehzahl";
              vStatus = "kritisch";
              cStatus = "codered";
          }
          else if ((json.engine_temperature > 80) && (json.rpm_value > 4500))
          {
              iStatus = "fa-tachometer";
              sStatus = "Motordrehzahl";
              vStatus = "Warnung";
              cStatus = "codeyellow";
          }
          else if ((json.crash == 1) && (json.crash_counter > 0))
          {
              iStatus = "fa-wheelchair";
              sStatus = "Unfallalarm";
              cStatus = "codeblue";
          }
          else if (json.tire_span_display == 1)
          {
              iStatus = "fa-circle-o-notch";
              sStatus = "Reifenpanne";
              cStatus = "codeyellow";
          }
          else if (json.checkcontrol_message_mil == 1)
          {
              iStatus = "fa-wrench";

              if (json.checkcontrol_message_mil_limited == 1)
              {
                  sStatus = "Motorwarnung";
                  cStatus = "codeyellow";
              }
              else
              {
                  sStatus = "Motorfehler";
                  cStatus = "codered";
              }
          }
          else if (json.checkcontrol_message_dsc == 1)
          {
              iStatus = "fa-rotate-left";

              if (json.checkcontrol_message_dsc_limited == 1)
              {
                  sStatus = "DSC-Warnung";
                  cStatus = "codeyellow";
              }
              else
              {
                  sStatus = "DSC-Fehler";
                  cStatus = "codered";
              }
          }
      }
      
      if ($("#info .value").html() != vStatus)
      {
          $("#info").effect("highlight").effect("highlight");
      }
      
      $("#info").attr("class", "segment " + cStatus);
      $("#info .value").html(vStatus);
      $("#info .title").html(sStatus);
      $("#info .fa").attr("class", "fa " + iStatus);
      
      if (json.gear_lever == "r")
      {
          iGear = "fa-arrow-circle-down";
      }
      else if (json.gear_lever == "n")
      {
          iGear = "fa-pause-circle";
      }
      else if (json.gear_lever == "d")
      {
          iGear = "fa-arrow-circle-up";
      }
      else if (json.gear_lever == "p")
      {
          iGear = "fa-stop-circle";
      }
      else if (json.gear_lever == "ds")
      {
          iGear = "fa-rocket";
      }
      else
      {
          iGear = "fa-gears";
      }
      
      $("#gear .fa").attr("class", "fa " + iGear);
      
      if (json.outside_temperature > 30)
      {
          iOutertemp = "fa-thermometer-4";
      }
      else if (json.outside_temperature > 20)
      {
          iOutertemp = "fa-thermometer-3";
      }
      else if (json.outside_temperature > 10)
      {
          iOutertemp = "fa-thermometer-2";
      }
      else if (json.outside_temperature > 3)
      {
          iOutertemp = "fa-thermometer-1";
      }
      else
      {
          iOutertemp = "fa-snowflake-o";
      }
      
      if (json.inside_temperature > 30)
      {
          iInnertemp = "fa-thermometer-4";
      }
      else if (json.inside_temperature > 20)
      {
          iInnertemp = "fa-thermometer-3";
      }
      else if (json.inside_temperature > 10)
      {
          iInnertemp = "fa-thermometer-2";
      }
      else if (json.inside_temperature > 3)
      {
          iInnertemp = "fa-thermometer-1";
      }
      else
      {
          iInnertemp = "fa-snowflake-o";
      }
      
      $("#innertemp .fa").attr("class", "fa " + iInnertemp);
      $("#outertemp .fa").attr("class", "fa " + iOutertemp);
      
      if (json.battery_charging == 1)
      {
          iBattery = "fa-flash";
      }
      else if (json.battery_voltage >= 12)
      {
          iBattery = "fa-battery-4";    
      }
      else if (json.battery_voltage >= 11.5)
      {
          iBattery = "fa-battery-3";
      }
      else if (json.battery_voltage >= 11.0)
      {
          iBattery = "fa-battery-2";
      }
      else if (json.battery_voltage >= 10.5)
      {
          iBattery = "fa-battery-1";
      }
      else
      {
          iBattery = "fa-battery-0";
      }

      $("#battery .fa").attr("class", "fa " + iBattery);

      if (keyInPos2 == 1)
      {
          jSpeed = json.speed_kmh;
          jRPM = json.rpm_value;
          jFuel = json.average_fuel;
          jTemp = json.engine_temperature;
          jPS = json.power_hp;
          jNM = json.torque_nm;
          
          if (jNM > 1900)
              jNM = 0;
      }
      
      jOuterTemp = json.outside_temperature;
      
      if (tSpeed.value != jSpeed)
          tSpeed.value = jSpeed;
      
      if (tRPM.value != jRPM)
          tRPM.value = jRPM;
      
      if (tTemp.value != jTemp)
          tTemp.value = jTemp;
      
      if (tFuel.value != jFuel)
          tFuel.value = jFuel;
          
      if (tNM.value != jNM)
          tNM.value = jNM;
          
      if (tPS.value != jPS)
          tPS.value = jPS;
          
      dataFrameBusy = false;
      dataFrameTimer = setTimeout('get_dataframe()', 500);
    },
    error: function(a,b)
    {
      dataFrameBusy = false;
      dataFrameTimer = setTimeout('get_dataframe()', 1500);
    }
  });
}

function init()
{
  $("#main").append('<div id="topbar"> \
  <div class="segment" id="clock"><span class="time">--:--:--</span><br/><span class="date">--.--.----</span></div> \
  <div class="segment" id="odometer"><span class="title">Kilometerstand</span><br/><span class="odo"><span class="value">---</span> km</span></div> \
  <div class="segment" id="innertemp"><span class="title">Innentemp.</span><br/><span class="temp"><i class="fa fa-thermometer-1"></i> <span class="value">--,-</span> &deg;C</span></div> \
  <div class="segment" id="outertemp"><span class="title">Au&szlig;entemp.</span><br/><span class="temp"><i class="fa fa-thermometer-1"></i> <span class="value">--,-</span> &deg;C</span></div> \
  <div class="segment" id="battery"><span class="title">Batteriestatus</span><br/><span class="voltage"><i class="fa fa-battery-0"></i> <span class="value">--,-</span> V</span></div> \
  <div class="segment" id="range"><span class="title">Reichweite</span><br/><span class="range"><i class="fa fa-map-marker"></i> <span class="value">---</span> km</span></div> \
  <div class="segment" id="gear"><span class="title">Getriebe</span><br/><span class="gearshift"><i class="fa fa-gears"></i> <span class="value">-</span></span></div> \
  <div class="segment" id="info"><span class="title">Alles OK</span><br/><span class="information"><i class="fa fa-thumbs-o-up"></i> <span class="value"></span></span></div> \
  <div class="lastseg" id="symbols"> \
  <span id="blinker-left"><i class="fa fa-arrow-left"></i></span> \
  <span id="foglight"><i class="fa fa-low-vision"></i></span> \
  <span id="light"><i class="fa fa-lightbulb-o"></i></span> \
  <span id="handbrake"><i class="fa fa-exclamation-circle"></i></span> \
  <span id="seatbelt"><i class="fa fa-user-circle-o"></i></span> \
  <span id="cruisecontrol"><i class="fa fa-road"></i></span> \
  <span id="pdc"><i class="fa fa-wifi"></i></span> \
  <span id="blinker-right"><i class="fa fa-arrow-right"></i></span> \
  </div> \
  <div class="clear"></div> \
  </div> \
  <div class="container" id="cockpit-main"> \
  <div class="gauge-main"> \
  <div class="dblgauge gauge"> \
  <canvas id="nm"></canvas> \
  <canvas id="ps"></canvas> \
  </div> \
  <div class="gauge"> \
  <canvas id="speed"></canvas> \
  <canvas id="rpm"></canvas> \
  </div> \
  <div class="dblgauge gauge"> \
  <canvas id="temp"></canvas> \
  <canvas id="fuel"></canvas> \
  </div> \
  <div class="clear"></div> \
  </div> \
  </div> \
  <div id="bottombar"> \
  <button class="button" id="lockunlock"><i class="fa fa-lock"></i></button> \
  <button class="button" id="blinkwarn"><i class="fa fa-exclamation-triangle"></i></button> \
  </div>');
  $("#background").css("background-image", "url(/assets/images/background.jpg)");

  tPS = new RadialGauge({
    renderTo: 'ps',
    animationRule: "linear",
    animationDuration: 1500,
    needleCircleInner: false,
    needleCircleOuter: true,
    needleCircleSize: 7,
    needleWidth: 2,
    needleType: "arrow",
    borders: false,
    borderShadowWidth: 0,
    colorPlate: fColorPlate,
    colorMajorTicks: fColorMajorTicks,
    colorMinorTicks: fColorMinorTicks,
    colorUnits: fColorUnits,
    colorNumbers: fColorNumbers,
    colorNeedle: fColorNeedle,
    colorNeedleEnd: fColorNeedleEnd,
    colorValueBoxBackground: fColorValueBoxBackground,
    valueBoxStroke: false,
    valueBoxShadow: false,
    valueBox: true,
    startAngle: 60,
    ticksAngle: 240,
    highlights: [
        {
            "from": 200,
            "to": 225,
            "color": fColorHighlight1
        },
        {
            "from": 225,
            "to": 300,
            "color": fColorHighlight2
        }
    ],
    strokeTicks: true,
    minorTicks: 2,
    majorTicks: [
        "0",
        "25",
        "50",
        "75",
        "100",
        "125",
        "150",
        "175",
        "200",
        "225",
        "250",
        "275",
        "300"
    ],
    maxValue: 300,
    minValue: 0,
    units: "PS",
    height: 150,
    width: 150
  }).draw();  
  
  tNM = new RadialGauge({
    renderTo: 'nm',
    animationRule: "linear",
    animationDuration: 1500,
    needleCircleInner: false,
    needleCircleOuter: true,
    needleCircleSize: 7,
    needleWidth: 2,
    needleType: "arrow",
    borders: false,
    borderShadowWidth: 0,
    colorPlate: fColorPlate,
    colorMajorTicks: fColorMajorTicks,
    colorMinorTicks: fColorMinorTicks,
    colorUnits: fColorUnits,
    colorNumbers: fColorNumbers,
    colorNeedle: fColorNeedle,
    colorNeedleEnd: fColorNeedleEnd,
    colorValueBoxBackground: fColorValueBoxBackground,
    valueBoxStroke: false,
    valueBoxShadow: false,
    valueBox: true,
    startAngle: 60,
    ticksAngle: 240,
    highlights: [
        {
            "from": 490,
            "to": 630,
            "color": fColorHighlight1
        },
        {
            "from": 630,
            "to": 700,
            "color": fColorHighlight2
        }
    ],
    strokeTicks: true,
    minorTicks: 2,
    majorTicks: [
        "0",
        "70",
        "140",
        "210",
        "280",
        "350",
        "420",
        "490",
        "560",
        "630",
        "700"
    ],
    maxValue: 700,
    minValue: 0,
    units: "nm",
    height: 150,
    width: 150
  }).draw();  
  
  tFuel = new RadialGauge({
    renderTo: 'fuel',
    animationRule: "linear",
    animationDuration: 1500,
    needleCircleInner: false,
    needleCircleOuter: true,
    needleCircleSize: 7,
    needleWidth: 2,
    needleType: "arrow",
    borders: false,
    borderShadowWidth: 0,
    colorPlate: fColorPlate,
    colorMajorTicks: fColorMajorTicks,
    colorMinorTicks: fColorMinorTicks,
    colorUnits: fColorUnits,
    colorNumbers: fColorNumbers,
    colorNeedle: fColorNeedle,
    colorNeedleEnd: fColorNeedleEnd,
    colorValueBoxBackground: fColorValueBoxBackground,
    valueBoxStroke: false,
    valueBoxShadow: false,
    valueBox: true,
    startAngle: 60,
    ticksAngle: 240,
    highlights: [
        {
            "from": 7.5,
            "to": 15,
            "color": fColorHighlight1
        },
        {
            "from": 0,
            "to": 7.5,
            "color": fColorHighlight2
        }
    ],
    strokeTicks: true,
    minorTicks: 2,
    majorTicks: [
        "0",
        "15",
        "30",
        "45",
        "60",
        "75"
    ],
    maxValue: 75,
    minValue: 0,
    units: "L",
    height: 150,
    width: 150
  }).draw();  
  
  tSpeed = new RadialGauge({
    renderTo: 'speed',
    animationRule: "linear",
    animationDuration: 1500,
    needleCircleInner: false,
    needleCircleOuter: true,
    needleCircleSize: 7,
    needleWidth: 2,
    needleType: "arrow",
    borders: false,
    borderShadowWidth: 0,
    colorPlate: fColorPlate,
    colorMajorTicks: fColorMajorTicks,
    colorMinorTicks: fColorMinorTicks,
    colorUnits: fColorUnits,
    colorNumbers: fColorNumbers,
    colorNeedle: fColorNeedle,
    colorNeedleEnd: fColorNeedleEnd,
    colorValueBoxBackground: fColorValueBoxBackground,
    valueBoxStroke: false,
    valueBoxShadow: false,
    valueBox: true,
    strokeTicks: true,
    highlights: [
        {
            "from": "49",
            "to": "51",
            "color": fColorHighlight1
        },
        {
            "from": "129",
            "to": "131",
            "color": fColorHighlight2
        }
    ],
    minorTicks: 2,
    majorTicks: [
        "0",
        "20",
        "40",
        "60",
        "80",
        "100",
        "120",
        "140",
        "160",
        "180",
        "200",
        "220",
        "240",
        "260",
        "280",
        "300",
        "320"
    ],
    maxValue: 320,
    minValue: 0,
    units: "Km/h",
    height: 300,
    width: 300,
    fontValue: "Led"
  }).draw();  
  
  tRPM = new RadialGauge({
    renderTo: 'rpm',
    animationRule: "linear",
    animationDuration: 1500,
    needleCircleInner: false,
    needleCircleOuter: true,
    needleCircleSize: 7,
    needleWidth: 2,
    needleType: "arrow",
    borders: false,
    borderShadowWidth: 0,
    colorPlate: fColorPlate,
    colorMajorTicks: fColorMajorTicks,
    colorMinorTicks: fColorMinorTicks,
    colorUnits: fColorUnits,
    colorNumbers: fColorNumbers,
    colorNeedle: fColorNeedle,
    colorNeedleEnd: fColorNeedleEnd,
    colorValueBoxBackground: fColorValueBoxBackground,
    valueBoxStroke: false,
    valueBoxShadow: false,
    valueBox: true,
    highlights: [
        {
            "from": 4.5,
            "to": 5.5,
            "color": fColorHighlight1
        },
        {
            "from": 5.5,
            "to": 6,
            "color": fColorHighlight2
        }
    ],
    strokeTicks: true,
    minorTicks: 2,
    majorTicks: [
        "0",
        "1",
        "2",
        "3",
        "4",
        "5",
        "6"
    ],
    maxValue: 6000,
    minValue: 0,
    units: "U/min",
    height: 300,
    width: 300
  }).draw();  
  
  tTemp = new RadialGauge({
    renderTo: 'temp',
    animationRule: "linear",
    animationDuration: 1500,
    needleCircleInner: false,
    needleCircleOuter: true,
    needleCircleSize: 7,
    needleWidth: 2,
    needleType: "arrow",
    borders: false,
    borderShadowWidth: 0,
    colorPlate: fColorPlate,
    colorMajorTicks: fColorMajorTicks,
    colorMinorTicks: fColorMinorTicks,
    colorUnits: fColorUnits,
    colorNumbers: fColorNumbers,
    colorNeedle: fColorNeedle,
    colorNeedleEnd: fColorNeedleEnd,
    colorValueBoxBackground: fColorValueBoxBackground,
    valueBoxStroke: false,
    valueBoxShadow: false,
    valueBox: true,
    startAngle: 60,
    ticksAngle: 240,
    highlights: [
        {
            "from": 90,
            "to": 100,
            "color": fColorHighlight1
        },
        {
            "from": 100,
            "to": 120,
            "color": fColorHighlight2
        }
    ],
    strokeTicks: true,
    minorTicks: 2,
    majorTicks: [
        "0",
        "30",
        "60",
        "90",
        "120"
    ],
    maxValue: 120,
    minValue: 0,
    units: "°C",
    height: 150,
    width: 150
  }).draw();  
  
  dataFrameTimer = setTimeout('get_dataframe()', 1500);
}