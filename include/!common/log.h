#ifndef __LOG_H
#define __LOG_H

// toggle this for debugging
#define DEBUGGING_ENABLED 1

#ifdef DEBUGGING_ENABLED

#define BEGIN_DEBUG   Serial.begin(115200);

#define DEBUG(msg)    Serial.print(msg);
#define DEBUGLN(msg)  Serial.println(msg);

#else

#define BEGIN_DEBUG   ;

#define DEBUG(msg)    ;
#define DEBUGLN(msg)  ;

#endif

#endif
