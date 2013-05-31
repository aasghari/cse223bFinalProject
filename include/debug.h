/*
 * debug.h
 *
 *  Created on: May 27, 2013
 *      Author: aasghari
 */

#ifndef DEBUG_H_
#define DEBUG_H_
#ifdef NO_DEBUG

#define debug if(false) std::cout<<"("<<__FILE__<<":"<<__LINE__<<"): "

#else

#define debug std::cout<<"("<<__FILE__<<":"<<__LINE__<<"): "

#endif


#endif /* DEBUG_H_ */
