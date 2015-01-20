/**
 * S0 meter connected to raspberry pi gpio pin
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Steffen Vogel <info@steffenvogel.de>
 * @author Thorben Thuermer <r00t@constancy.org>
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
#include <poll.h>

#include "protocols/MeterRaspiS0.hpp"
#include "Options.hpp"
#include <VZException.hpp>

MeterRaspiS0::MeterRaspiS0(std::list<Option> options)
		: Protocol("raspis0")
		, _counter(0)
{
	OptionList optlist;

	try {
		_gpiopin = optlist.lookup_int(options, "gpiopin");
	} catch (vz::VZException &e) {
		print(log_error, "Missing gpiopin or invalid gpio pin number", "");
		throw;
	}
	_device.append("/sys/class/gpio/gpio");
	_device.append(std::to_string(_gpiopin));
	_device.append("/value");

	try {
		_resolution = optlist.lookup_int(options, "resolution");
	} catch (vz::OptionNotFoundException &e) {
		_resolution = 1000;
	} catch (vz::VZException &e) {
		print(log_error, "Failed to parse resolution", "");
		throw;
	}
	if (_resolution < 1) throw vz::VZException("Resolution must be greater than 0.");

	_prev_pulse_time.tv_sec=0;
	_prev_pulse_time.tv_usec=0;
}

MeterRaspiS0::~MeterRaspiS0() {
	//free((void*)_device);
}

int MeterRaspiS0::open() {
	std::string name;
	int fd,res;

	if (!::access(_device.c_str(),F_OK)){
		// exists
	} else {
		fd=::open("/sys/class/gpio/export",O_WRONLY);
		if (fd<0) throw vz::VZException("open export failed");
		name.clear();
		name.append(std::to_string(_gpiopin));
		name.append("\n");
		
		res=write(fd,name.c_str(), name.length()+1);
		if ((name.length()+1)!=res) throw vz::VZException("export failed");
		::close(fd);
	}

	name.clear();
	name.append("/sys/class/gpio/gpio");
	name.append(std::to_string(_gpiopin));
	name.append("/direction");
	fd = ::open(name.c_str(), O_WRONLY);
	if (fd<0) throw vz::VZException("open direction failed");
	res=::write(fd,"in\n",3);
	if (3!=res) throw vz::VZException("set direction failed");
	if (::close(fd)<0) throw vz::VZException("set direction failed");

	name.clear();
	name.append("/sys/class/gpio/gpio");
	name.append(std::to_string(_gpiopin));
	name.append("/edge");
	fd = ::open(name.c_str(), O_WRONLY);
	if (fd<0) throw vz::VZException("open edge failed");
	res=::write(fd,"rising\n",8);
	if (8!=res) throw vz::VZException("set edge failed");
	if (::close(fd)<0) throw vz::VZException("set edge failed");

	name.clear();
	name.append("/sys/class/gpio/gpio");
	name.append(std::to_string(_gpiopin));
	name.append("/active_low");
	fd = ::open(name.c_str(), O_WRONLY);
	if (fd<0) throw vz::VZException("open active_low failed");
	res=::write(fd,"0\n",2);
	if (2!=res) throw vz::VZException("set active_low failed");
	if (::close(fd)<0) throw vz::VZException("set active_low failed");
	
	/* open port */
	fd = ::open(_device.c_str(), O_RDONLY|O_EXCL);

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

	struct timeval time2;
	int ret;
	struct pollfd mypollfd;

	mypollfd.fd=_fd;
	mypollfd.events=POLLPRI;;
	mypollfd.revents=0;

	print(log_debug, "waiting for an impulse", name().c_str());
	poll(&mypollfd,1,-1); // poll 1 fd, wait forever
	gettimeofday(&time2, NULL);
	print(log_debug, "got an impulse", name().c_str());
	usleep(3000); // wait some ms for debouncing
	::read(_fd,NULL,0); // clear edge detection event

	// we read a pulse
	rds[0].identifier(new StringIdentifier("Impulse"));
	rds[0].time(time2);
	rds[0].value(1);

	ret=1;

	// if we have a previous pulse, calculate power
	if (_prev_pulse_time.tv_sec!=0){
		double t1 = _prev_pulse_time.tv_sec + _prev_pulse_time.tv_usec / 1e6;
		double t2 = time2.tv_sec + time2.tv_usec / 1e6;
		double value = ( 3600000 ) / ( (t2-t1) * _resolution ) ;

		rds[1].identifier(new StringIdentifier("Power"));
		rds[1].time(time2);
		rds[1].value(value);

		ret=2;
		print(log_debug, "Reading S0 - n=%d power=%f", name().c_str(), n, rds[0].value());
	} else {
		print(log_debug, "Reading S0 - n=%d", name().c_str(), n);
	}

	_prev_pulse_time.tv_sec=time2.tv_sec;
	_prev_pulse_time.tv_usec=time2.tv_usec;

	return ret;
}

