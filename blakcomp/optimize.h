// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * optimize.h:  Header for optimize.c
 */

#ifndef _OPTMIMIZE_H
#define _OPTMIMIZE_H

// Optimization functions.
void SimplifyExpression(expr_type e);
list_type optimize_message_statements(list_type stmts);
int optimize_call(id_type id, list_type *args);

// Comparison functions.
int compare_expression(expr_type e1, expr_type e2);
int compare_id(id_type i1, id_type i2);
int compare_call(call_stmt_type c1, call_stmt_type c2);
int compare_constant(const_type c1, const_type c2);

// Used for getting the name of a message.
const char* get_message_name(call_stmt_type call);
int get_message_idnum(call_stmt_type call);

#endif /* #ifndef _OPTMIMIZE_H */
