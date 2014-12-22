/**
 * S0 Hutschienenzähler directly connected to an rs232 port
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Steffen Vogel <info@steffenvogel.de>
 */
/*
 * This file is part of volkzaehler.org
 *
 * volkzaehler.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * volkzaehler.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with volkszaehler.org. If not, see <http://www.gnu.org/licenses/>.
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#include "protocols/MeterRaspiS0.hpp"
#include "Options.hpp"
#include <VZException.hpp>

MeterRaspiS0::MeterRaspiS0(std::list<Option> options)
		: Protocol("raspis0")
		, _counter(0)
{
	OptionList optlist;

	try {
		_device = optlist.lookup_string(options, "gpiofile");
	} catch (vz::VZException &e) {
		print(log_error, "Missing gpiofile or invalid type", "");
		throw;
	}

	try {
		_resolution = optlist.lookup_int(options, "resolution");
	} catch (vz::OptionNotFoundException &e) {
		_resolution = 1000;
	} catch (vz::VZException &e) {
		print(log_error, "Failed to parse resolution", "");
		throw;
	}
	if (_resolution < 1) throw vz::VZException("Resolution must be greater than 0.");
}

MeterRaspiS0::~MeterRaspiS0() {
	//free((void*)_device);
}

int MeterRaspiS0::open() {

	/* open port */
	int fd = ::open(_device.c_str(), O_RDONLY);

	if (fd < 0) {
		print(log_error, "open(%s): %s", "", _device.c_str(), strerror(errno));
		return ERR;
	}

	_fd = fd;

	return SUCCESS;
}

int MeterRaspiS0::close() {
	::close(_fd);
	return SUCCESS; /* close serial port */
}

ssize_t MeterRaspiS0::read(std::vector<Reading> &rds, size_t n) {

	struct timeval time1;
	struct timeval time2;
	char buf[8];
	int res;

	/* blocking until GPIO is high (=idle, previous pulse is over)*/
	buf[0]='U';
	while (1){
		if (lseek(_fd,0,SEEK_SET)) throw vz::VZException("fseek() failed");
		res=::read(_fd, buf, 8);
		if (res != 2) return 0;
		//if (res==2) print(log_error, "gpio state is: '%c'", "", buf[0]);
		if (buf[0]=='1') break; // pin is high
		usleep(10000);
	}
	gettimeofday(&time1, NULL);

	/* blocking until GPIO is low (=start of new pulse) */
	buf[0]='U';
	while (buf[0]!='0'){
		if (lseek(_fd,0,SEEK_SET)) throw vz::VZException("fseek() failed");
		res=::read(_fd, buf, 8);
		if (res != 2) return 0;
		//if (res==2) print(log_error, "gpio state is: '%c'", "", buf[0]);
		if (buf[0]=='0') break; // pin is low
		usleep(10000);
	}
	gettimeofday(&time2, NULL);

	double t1 = time1.tv_sec + time1.tv_usec / 1e6;
	double t2 = time2.tv_sec + time2.tv_usec / 1e6;
	double value = ( 3600000 ) / ( (t2-t1) * _resolution ) ;

	/* store current timestamp */
	rds[0].identifier(new StringIdentifier("Power"));
	rds[0].time(time2);
	rds[0].value(value);

	rds[1].identifier(new StringIdentifier("Impulse"));
	rds[1].time(time2);
	rds[1].value(2);

	print(log_debug, "Reading S0 - n=%d power=%f", name().c_str(), n, rds[0].value());
	/* wait some ms for debouncing */
	usleep(30000);

	return 2;
}

