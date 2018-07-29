<?php
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

// Standard ID: 0x480       DLC: 8  Data: 0x38 0x72 0xFF 0x01 0xFF 0xFF 0xFF 0xFF
ini_set("display_errors", 0);
ini_set("error_reporting", 0);

$fd = fopen("testdata", "r");

while (!feof($fd))
{
	$line = fgets($fd, 4096);
	$checksum = ($retval & 0xFF);

	$field = explode(" ", $line);
	$canid = substr($field[2], 2);
	$length = $field[10];
	$payload = array();
	$payload[] = substr($field[13], 2);
	$payload[] = substr($field[14], 2);
	$payload[] = substr($field[15], 2);
	$payload[] = substr($field[16], 2);
	$payload[] = substr($field[17], 2);
	$payload[] = substr($field[18], 2);
	$payload[] = substr($field[19], 2);
	$payload[] = substr($field[20], 2);
	
	$checksum = ($checksum + $canid) & 0xFF;
	$checksum = ($checksum + $length) & 0xFF;

	echo "00:".$length.":STD:".$canid;

	foreach ($payload as $p)
	{
		if ($p == "")
			continue;
			
		$p = trim($p);

		echo ":".$p;
		
		$checksum = ($checksum + $p) & 0xFF;	
	}
	
	echo ":".strtoupper(str_pad(dechex($checksum), 2, 0, STR_PAD_LEFT))."\n";
}

fclose($fd);
