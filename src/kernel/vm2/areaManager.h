#include "area.h"

class areaManager // One of these per process
{
	private:
		list areas;
		team_id team;
	public:
		areaManager ();
		void addArea(area *newArea) {areas.add(newArea);}
		void removeArea(area *oldArea) {areas.remove(oldArea); }
		team_id getTeam(void) {return team;}
		unsigned long getNextAddress(int pages,unsigned long minimum=USER_BASE);
		area *findArea(void *address);
		area *findArea(char *address);
		area *findArea(area_id id);
		void pager(int desperation);
		void saver(void);
		
		bool fault(void *fault_address, bool writeError); // true = OK, false = panic.

		char getByte(unsigned long offset); // This is for testing only
		void setByte(unsigned long offset,char value); // This is for testing only
		int getInt(unsigned long offset); // This is for testing only
		void setInt(unsigned long offset,int value); // This is for testing only
};
