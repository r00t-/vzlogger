#ifndef _METER_S0_H_
#define _METER_S0_H_
#include "protocols/BaseS0.hpp"
#include "protocols/MeterS0.hpp"

class MeterS0 : public vz::protocol::BaseS0 {
  public:
	MeterS0(std::list<Option> options);
  protected:
	int _wait_impulse();
	int _open();
	int _close();
};
#endif
