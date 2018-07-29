# Makefile for libbmwxcan
#
# Copyright (C) 2017-2018 Oliver Welter <contact@verbotene.zone>
#
# This is free software, for non-commercial use, licensed under
# The Non-Profit Open Software License version 3.0 (NPOSL-3.0).
#
# See /LICENSE for more information.
#

gnu_c_compiler := /usr/bin/gcc
gnu_c_plusplus := /usr/bin/g++

ldconfig := ldconfig
strip := strip

strip_options := -s -R .comment -R .gnu.version --strip-unneeded
gpp_options := -std=gnu++11 -w -Os -ffunction-sections -fdata-sections -Wl,--gc-sections
gpp_libs := -lcrypto -lmysqlclient

path_lib:=./src/lib
path_bin:=./src/bin
path_cgibin:=./src/cgi-bin
path_web:=./src/web
path_include:=./src/include

install_path_lib:=/usr/lib/bmwxcan
install_path_bin:=/usr/bin
install_path_cgibin:=/usr/lib/cgi-bin/bmwxcan
install_path_web:=/var/www/html/bmwxcan

file_lib:=libbmwxcan
file_bin:=bmwxcan
file_cgibin:=bmwxcan-cgi

libbmwxcan_so_major:=1
libbmwxcan_so_version:=1.01

licensed-for-name:=Oliver Welter
licensed-for-vin:=B515982
license-key=jpojfowiejofjowijef

config_options := -DWITHOUT_TARGET_ARDUINO -DWITH_CAN_ID_ALL -DWITH_E_SERIES

all:
	@echo "make clean|compile|install"
	@echo "Current config options $(config_options)"
	
clean:
	rm $(path_lib)/$(file_lib).a >/dev/null 2>&1 || /bin/true
	rm $(path_lib)/$(file_lib).so.$(libbmwxcan_so_version) >/dev/null 2>&1 || /bin/true
	rm $(path_bin)/$(file_bin) >/dev/null 2>&1 || /bin/true
	rm $(path_cgibin)/$(file_cgibin) >/dev/null 2>&1 || /bin/true

ifeq ($(with-unknown-ids), yes)
 	config_options += -DWITH_UNKNOWN_IDS
endif
ifeq ($(with-ignored-ids), yes)
 	config_options += -DWITH_IGNORED_IDS
endif

$(path_lib)/$(file_lib).a:
	$(gnu_c_plusplus) $(config_options) $(gpp_options) -I$(path_include) -fPIC $(gpp_libs) -c $(path_lib)/$(file_lib).cpp -o $(path_lib)/$(file_lib).a
	$(strip) $(strip_options) $(path_lib)/$(file_lib).a
	
$(path_lib)/$(file_lib).so.$(libbmwxcan_so_version): $(path_lib)/$(file_lib).a
	$(gnu_c_plusplus) -shared -Wl,-soname,$(file_lib).so.$(libbmwxcan_so_major) -o $(path_lib)/$(file_lib).so.$(libbmwxcan_so_version) $(path_lib)/$(file_lib).a
	
$(path_bin)/$(file_bin): $(path_lib)/$(file_lib).so.$(libbmwxcan_so_version)
	$(gnu_c_plusplus) $(config_options) $(gpp_options) -Wall -I$(path_include) $(gpp_libs) -L$(path_lib) -lbmwxcan $(path_bin)/$(file_bin).cpp -o $(path_bin)/$(file_bin)
	$(strip) $(strip_options) $(path_bin)/$(file_bin)
	
$(path_cgibin)/$(file_cgibin):
	$(gnu_c_plusplus) $(config_options) $(gpp_options) -Wall -I$(path_include) $(gpp_libs) $(path_cgibin)/$(file_cgibin).cpp -o $(path_cgibin)/$(file_cgibin)
	$(strip) $(strip_options) $(path_cgibin)/$(file_cgibin)
	
libbmwxcan: $(path_lib)/$(file_lib).so.$(libbmwxcan_so_version)

bmwxcan: $(path_bin)/$(file_bin)

bmwxcan-cgi: $(path_cgibin)/$(file_cgibin)

compile: bmwxcan bmwxcan-cgi

install: $(path_bin)/$(file_bin) $(path_cgibin)/$(file_cgibin)
	mkdir -p $(install_path_lib) >/dev/null 2>&1 || /bin/true
	mkdir -p $(install_path_bin) >/dev/null 2>&1 || /bin/true
	mkdir -p $(install_path_cgibin) >/dev/null 2>&1 || /bin/true
	mkdir -p $(install_path_web) >/dev/null 2>&1 || /bin/true
	cp -f $(path_lib)/$(file_lib).so.$(libbmwxcan_so_version) $(install_path_lib)
	$(ldconfig) -n $(install_path_lib)
	cp -f $(path_bin)/$(file_bin) $(install_path_bin)
	cp -f $(path_cgibin)/$(file_cgibin) $(install_path_cgibin)
	cp -af $(path_web)/* $(install_path_web)
	chmod 750 $(install_path_bin)/$(file_bin)
	chmod 755 $(install_path_cgibin)/$(file_cgibin)
