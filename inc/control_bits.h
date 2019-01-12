/*
 * control_bits.h
 *
 *  Created on: 22 paź 2018
 *      Author: root
 */

#ifndef INC_CONTROL_BITS_H_
#define INC_CONTROL_BITS_H_

#define GLUE(a, b)     a##b

/* single-bit macros, used for control bits */
#define SET_(what, p, m) GLUE(what, p) |= (1 << (m))
#define CLR_(what, p, m) GLUE(what, p) &= ~(1 << (m))
#define TOGGLE_(what, p, m) GLUE(what, p) ^= (1 << (m))
#define GET_(/* PIN, */ p, m) GLUE(PIN, p) & (1 << (m))
#define SET(what, x) SET_(what, x)
#define CLR(what, x) CLR_(what, x)
#define TOGGLE(what, x)	TOGGLE_(what, x)
#define GET(x) (GET_(x))


#endif /* INC_CONTROL_BITS_H_ */
