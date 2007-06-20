#ifndef CHARACTER_SET_ROSTER_H
#define CHARACTER_SET_ROSTER_H

#include <SupportDefs.h>
#include <Messenger.h>

namespace BPrivate {

/**
 * @file   BCharacterSetRoster.h
 * @author Andrew Bachmann
 * @brief  Defines BCharacterSetRoster
 *
 * @see BCharacterSet.h
 **/

class BCharacterSet;

class BCharacterSetRoster {
    /**
     * @class BCharacterSetRoster
     * @brief An object for finding or enumerating character sets
     **/
public:
    /**
     * @brief initialize the roster to the first character set
     **/
    BCharacterSetRoster();
    virtual ~BCharacterSetRoster();
    
    /**
     * @brief get the next available character set
     * @return B_NO_ERROR if it's valid, B_BAD_VALUE if it is not
     **/
    status_t GetNextCharacterSet(BCharacterSet * charset);
    /**
     * @brief resets the iterator to the first character set
     * @return B_NO_ERROR if it's valid, B_BAD_VALUE if it is not
     **/
    status_t RewindCharacterSets();
    
    /**
     * @brief register BMessenger to receive notifications of character set events
     * @return B_NO_ERROR if watching was okay, B_BAD_VALUE if poorly formed BMessenger
     **/
    static status_t StartWatching(BMessenger target);
    /**
     * @brief stop sending notifications to BMessenger
     * @return B_NO_ERROR if stopping went okay, B_BAD_VALUE if poorly formed BMessenger
     **/
    static status_t StopWatching(BMessenger target);
    
    /**
     * @brief return the character set with the given font id, or NULL if none exists
     * This function executes in constant time.
     * @return the character set with the given font id, or NULL if none exists
     **/
    static const BCharacterSet * GetCharacterSetByFontID(uint32 id);
    /**
     * @brief return the character set with the given conversion id, or NULL if none exists
     * This function executes in constant time.
     * @return the character set with the given conversion id, or NULL if none exists
     **/
    static const BCharacterSet * GetCharacterSetByConversionID(uint32 id);
    /**
     * @brief return the character set with the given MIB enum, or NULL if none exists
     * This function executes in constant time.
     * @return the character set with the given MIB enum, or NULL if none exists
     **/
    static const BCharacterSet * GetCharacterSetByMIBenum(uint32 MIBenum);

    /**
     * @brief return the character set with the given print name, or NULL if none exists
     * This function executes in linear time.
     * @return the character set with the given print name, or NULL if none exists
     **/
    static const BCharacterSet * FindCharacterSetByPrintName(const char * name);
    /**
     * @brief return the character set with the given name, or NULL if none exists
     * This function will match aliases as well.
     * This function executes in linear time.
     * @return the character set with the given name, or NULL if none exists
     **/
    static const BCharacterSet * FindCharacterSetByName(const char * name);
private:
    uint32 index; //! the state variable for iteration
};

}

#endif // CHARACTER_SET_ROSTER_H
