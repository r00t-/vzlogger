/**
 * S0 Meter base class
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011-2014, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Steffen Vogel <info@steffenvogel.de>
 * @author Thorben Thuermer <r00t@constancy.org>
 */
 
#ifndef _BASES0_H_
#define _BASES0_H_

#include <termios.h>

#include <protocols/Protocol.hpp>

namespace vz {
	namespace protocol {
class BaseS0 : public vz::protocol::Protocol {

public:
	BaseS0(std::list<Option> options);
	virtual ~BaseS0();

	int open();
	int close();
	ssize_t read(std::vector<Reading> &rds, size_t n);

  protected:
	virtual int _wait_impulse()=0;
	virtual int _open()=0;
	virtual int _close()=0;

	std::string _device;
	int _fd;	/* file descriptor of port */

  private:
	int _resolution;
	int _counter;

};
}}
#endif /* _BASES0_H_ */
