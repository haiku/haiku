#ifndef character_sets_H
#define character_sets_H

#include <CharacterSet.h>

namespace BPrivate {

extern const BCharacterSet * character_sets_by_id[];
extern const uint32 character_sets_by_id_count;
extern const BCharacterSet ** character_sets_by_MIBenum;
extern uint32 maximum_valid_MIBenum;

}

#endif // character_sets_H
