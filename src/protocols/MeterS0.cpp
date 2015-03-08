
#include "protocols/BaseS0.hpp"
#include "protocols/MeterS0.hpp"

#include "Options.hpp"
#include <VZException.hpp>

MeterS0::MeterS0(std::list<Option> options)
		: BaseS0(options)
//		, _counter(0)
{
}

int MeterS0::_wait_impulse(){return 0;}
int MeterS0::_open(){return 0;}
int MeterS0::_close(){return 0;}
