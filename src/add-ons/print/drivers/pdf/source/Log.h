
#ifndef LOG_H
#define LOG_H

#if LOGGING
  #define LOG(l) fprintf l
#else
  #define LOG(l) ___do_nothing()
  inline void ___do_nothing() { }
#endif

#endif
