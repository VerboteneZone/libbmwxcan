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

#ifndef config_h
#define config_h

#define LICENSED_FOR_NAME "Oliver Welter"
#define LICENSED_FOR_VIN "B123456"
#define LICENSED_WITH_KEY "c1234567890AB"

#ifndef CONFIG_PATH_REALTIME_DATA
#define CONFIG_PATH_REALTIME_DATA "/var/run/bmwxcan"
#endif

#ifndef CONFIG_MYSQL_DEFAULT_HOSTNAME
#define CONFIG_MYSQL_DEFAULT_HOSTNAME "localhost"
#endif

#ifndef CONFIG_MYSQL_DEFAULT_USERNAME
#define CONFIG_MYSQL_DEFAULT_USERNAME "bmwxcan"
#endif

#ifndef CONFIG_MYSQL_DEFAULT_PASSWORD
#define CONFIG_MYSQL_DEFAULT_PASSWORD "bmwxcan"
#endif

#ifndef CONFIG_MYSQL_DEFAULT_DATABASE
#define CONFIG_MYSQL_DEFAULT_DATABASE "bmwxcan"
#endif

#ifndef CONFIG_MYSQL_DEFAULT_PORT
#define CONFIG_MYSQL_DEFAULT_PORT 3306
#endif

#endif
