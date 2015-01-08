
#ifndef __CCEVENT_H__
#define __CCEVENT_H__

#include "CCRef.h"
#include "../platforms/CrossAny.h"
//#include "platform/CCPlatformMacros.h"

NS_CROSSANY_BEGIN

class Node;

/**
 *   Base class of all kinds of events.
 */
class Event : public Ref{
public:
    enum class Type{TOUCH, KEYBOARD, ACCELERATION, MOUSE, FOCUS, GAME_CONTROLLER, CUSTOM };
    
protected:
    /** Constructor */
    Event(Type type);
public:
    /** Destructor */
    virtual ~Event();

    /** Gets the event type */
	inline Type getType() const { return _type; };
    
    /** Stops propagation for current event */
    inline void stopPropagation() { _isStopped = true; };
    
    /** Checks whether the event has been stopped */
    inline bool isStopped() const { return _isStopped; };
    
    /** @brief Gets current target of the event
     *  @return The target with which the event associates.
     *  @note It onlys be available when the event listener is associated with node. 
     *        It returns 0 when the listener is associated with fixed priority.
     */
    inline Node* getCurrentTarget() { return _currentTarget; };
    
protected:
    /** Sets current target */
    inline void setCurrentTarget(Node* target) { _currentTarget = target; };
    
	Type _type;     ///< Event type
    
    bool _isStopped;       ///< whether the event has been stopped.
    Node* _currentTarget;  ///< Current target
    
    friend class EventDispatcher;
};

NS_CROSSANY_END


#endif // __CCEVENT_H__
