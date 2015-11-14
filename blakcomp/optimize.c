// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * optimize.c:  Perform optimizations (currently just constant folding).
 */

#include "blakcomp.h"
#include "bkod.h"

extern int optimize_bof;  /* Should we perform optimizations during compilation? */
extern function_type Functions[]; /* Built-in C calls */
extern int numfuncs; /* Number of C calls */
extern id_struct BuiltinIds[]; /* Built-in class/message/parameters (identifiers) */
extern int numbuiltins; /* Number of built-in identifiers */
extern int lineno; /* Current line number being parsed */

/************************************************************************/
/*
 * SimplifyExpression:  Attempt to simplify given expression.  Modifies e.
 */
void SimplifyExpression(expr_type e)
{
   expr_type e1, e2;
   const_type c1, c2;

   switch (e->type)
   {
   case E_UNARY_OP:
      e1 = e->value.unary_opval.exp;
      if (e1->type != E_CONSTANT)
    break;

      c1 = e1->value.constval;
      if (c1->type != C_NUMBER)
         break;

      switch (e->value.unary_opval.op)
      {
      case NEG_OP:
         c1->value.numval = -c1->value.numval;
         e->type = E_CONSTANT;
         e->value.constval = c1;
         break;

      case NOT_OP:
         c1->value.numval = !c1->value.numval;
         e->type = E_CONSTANT;
         e->value.constval = c1;
         break;

      case BITNOT_OP:
         c1->value.numval = ~c1->value.numval;
         e->type = E_CONSTANT;
         e->value.constval = c1;
         break;

      case PRE_INC_OP:
         c1->value.numval = ++c1->value.numval;
         e->type = E_CONSTANT;
         e->value.constval = c1;
         break;

      case PRE_DEC_OP:
         c1->value.numval = --c1->value.numval;
         e->type = E_CONSTANT;
         e->value.constval = c1;
         break;
      }
      break;

   case E_BINARY_OP:
      e1 = e->value.binary_opval.left_exp;
      e2 = e->value.binary_opval.right_exp;
      if (e1->type != E_CONSTANT || e2->type != E_CONSTANT)
         break;

      c1 = e1->value.constval;
      c2 = e2->value.constval;

      if (c1->type != C_NUMBER || c2->type != C_NUMBER)
         break;

      switch (e->value.binary_opval.op)
      {
      case PLUS_OP:
         c1->value.numval = c1->value.numval + c2->value.numval;
         e->type = E_CONSTANT;
         e->value.constval = c1;
         break;

      case MINUS_OP:
         c1->value.numval = c1->value.numval - c2->value.numval;
         e->type = E_CONSTANT;
         e->value.constval = c1;
         break;

      case AND_OP:
         c1->value.numval = c1->value.numval && c2->value.numval;
         e->type = E_CONSTANT;
         e->value.constval = c1;
         break;

      case OR_OP:
         c1->value.numval = c1->value.numval || c2->value.numval;
         e->type = E_CONSTANT;
         e->value.constval = c1;
         break;

      case MULT_OP:
         c1->value.numval = c1->value.numval * c2->value.numval;
         e->type = E_CONSTANT;
         e->value.constval = c1;
         break;

      case DIV_OP:
         if (c2->value.numval == 0)
         {
            action_error("Division by zero");
            break;
         }
         c1->value.numval = c1->value.numval / c2->value.numval;
         e->type = E_CONSTANT;
         e->value.constval = c1;
         break;

      case MOD_OP:
         if (c2->value.numval == 0)
         {
            action_error("Division by zero");
            break;
         }
         c1->value.numval = c1->value.numval % c2->value.numval;
         e->type = E_CONSTANT;
         e->value.constval = c1;
         break;

      case BITAND_OP:
         c1->value.numval = c1->value.numval & c2->value.numval;
         e->type = E_CONSTANT;
         e->value.constval = c1;
         break;

      case BITOR_OP:
         c1->value.numval = c1->value.numval | c2->value.numval;
         e->type = E_CONSTANT;
         e->value.constval = c1;
         break;
      }
      break;
   }
}
/************************************************************************/
/*
 * optimize_message_statements:  Takes a list_type of message statements
 *   and returns a modified list. Currently the only optimization performed
 *   is to remove any $ assignments to local vars that are already $. This
 *   is done to any local vars not yet modified (as all locals are initiated
 *   to $ by blakserv) and any instances where the local is set to $ again
 *   more than once after being used. This function also suggests potential
 *   optimizations for Send() calls, where a message contains an identical
 *   call (same object, message ID and arguments), but does not modify them.
 */
list_type optimize_message_statements(list_type stmts)
{
   if (!optimize_bof)
      return stmts;

   Table id_table = table_create(128);

   for (list_type it = stmts; it != NULL; it = it->next)
   {
      stmt_type st = (stmt_type)it->data;
      if (st->type != S_ASSIGN)
         continue;

      assign_stmt_type assign = st->value.assign_stmt_val;

      if (assign->rhs->type == E_CALL)
      {
         stmt_type call_st1 = (stmt_type)assign->rhs->value.callval;
         call_stmt_type call1 = call_st1->value.call_stmt_val;

         if (call1->function != SENDMESSAGE)
            continue;

         for (list_type it2 = it->next; it2 != NULL; it2 = it2->next)
         {
            stmt_type st2 = (stmt_type)it2->data;
            if (st2->type != S_ASSIGN)
               continue;
            assign_stmt_type assign2 = st2->value.assign_stmt_val;
            if (assign2->rhs->type != E_CALL)
               continue;
            stmt_type call_st2 = (stmt_type)assign2->rhs->value.callval;
            call_stmt_type call2 = call_st2->value.call_stmt_val;

            if (call2->function != SENDMESSAGE)
               continue;
            if (compare_call(call1, call2))
            {
               // Same at this point
               const char *call_name = get_message_name(call2);
               write_optimize_log(st2->lineno, "Potential optimization: Duplicate Send call to message %s",
                  call_name);
               // Can't remove the duplicate calls yet until there's a way to
               // determine any differences the subsequent calls will have.
               // Until then, this still serves as a way to identify accidentlly
               // duplicated message calls.
               //assign2->rhs->type = E_IDENTIFIER;
               //assign2->rhs = make_expr_from_id(duplicate_id(assign->lhs));
            }
         }
      }
      else if (assign->rhs->type == E_CONSTANT && assign->lhs->type == I_LOCAL)
      {
         // If this is the first time we've seen this local and it's getting
         // assigned NIL, discard it. If it isn't getting assigned NIL, then
         // mark it as used.
         id_type id;
         id = (id_type)table_lookup(id_table, assign->lhs, id_hash, id_compare);
         if (assign->rhs->value.constval->type == C_NIL)
         {
            if (!id || !id->used)
            {
               write_optimize_log(st->lineno, "Optimization: Throwing out $ assignment to $ var %s",
                  assign->lhs->name);
               st->type = S_NOOP;
            }
            else if (id->used)
            {
               table_delete_item(id_table, id, id_hash, id_compare);
               assign->lhs->used = false;
               table_insert(id_table, assign->lhs, id_hash, id_compare);
            }
         }
         else
         {
            if (id)
               table_delete_item(id_table, id, id_hash, id_compare);
            assign->lhs->used = true;
            table_insert(id_table, assign->lhs, id_hash, id_compare);
         }
      }
   }

   table_delete(id_table);

   return stmts;
}
/************************************************************************/
/*
 * optimize_call:  Takes an id_type (function ID) and arguments (list_type).
 *   Currently just attempts to optimize Nth calls, by converting instances
 *   of Nth(i,1) to First(i) and instances of Nth(i,Length(i)) to Last(i).
 *   Returns the index (to the built-in Functions array) of the required
 *   function (Nth, First or Nth) and modifies the args list appropriately.
 */
int optimize_call(id_type id, list_type *args)
{
   int index = id->idnum;

   if (!optimize_bof)
      return index;

   const char *fname = Functions[index].name;
   if (stricmp(fname, "nth") != 0)
      return index;

   list_type arg = *args;
   arg_type arg2 = (arg_type)arg->next->data;
   if (arg2->type != ARG_EXPR)
      return index;

   if (arg2->value.expr_val->type == E_CONSTANT
      && arg2->value.expr_val->value.constval->type == C_NUMBER
      && arg2->value.expr_val->value.constval->value.numval == 1)
   {
      // First(i) is faster than Nth(i,1), so change the call.
      fname = "First";
      for (int i = 0; i < numfuncs; ++i)
         if (stricmp(Functions[i].name, fname) == 0)
         {
            index = i;
            break;
         }
      *args = list_create(arg->data);
      write_optimize_log(lineno, "Optimization: Nth(i,1) call changed to First(i)");
   }
   else if (arg2->value.expr_val->type == E_CALL)
   {
      stmt_type call = (stmt_type)arg2->value.expr_val->value.callval;
      if (call->value.call_stmt_val->function == LENGTH)
      {
         arg_type arg1 = (arg_type)arg->data;
         arg_type len_arg = (arg_type)call->value.call_stmt_val->args->data;
         if (arg1->type == ARG_EXPR && len_arg->type == ARG_EXPR
            && compare_expression(arg1->value.expr_val, len_arg->value.expr_val))
         {
            // Last(i) is faster than Nth(i,Length(i)), so change the call.
            fname = "Last";
            for (int i = 0; i < numfuncs; ++i)
               if (stricmp(Functions[i].name, fname) == 0)
               {
                  index = i;
                  break;
               }
            *args = list_create(arg->data);
            write_optimize_log(lineno, "Optimization: Nth(i,Length(i)) call changed to Last(i)");
         }
      }
   }

   return index;
}
/************************************************************************/
/*
 * compare_expression:  Compares two expr_type structs. Returns true if
 *    structures are identical, false otherwise.
 */
int compare_expression(expr_type e1, expr_type e2)
{
   if (e1->type != e2->type)
      return false;

   switch (e1->type)
   {
   case E_BINARY_OP:
      if (e1->value.binary_opval.op != e2->value.binary_opval.op)
         return false;
      if (!compare_expression(e1->value.binary_opval.left_exp, e2->value.binary_opval.left_exp))
         return false;
      if (!compare_expression(e1->value.binary_opval.right_exp, e2->value.binary_opval.right_exp))
         return false;
      break;
   case E_UNARY_OP:
      if (e1->value.unary_opval.op != e2->value.unary_opval.op)
         return false;
      if (!compare_expression(e1->value.unary_opval.exp, e2->value.unary_opval.exp))
         return false;
      break;
   case E_IDENTIFIER:
      if (!compare_id(e1->value.idval, e2->value.idval))
         return false;
      break;
   case E_CONSTANT:
      if (!compare_constant(e1->value.constval, e2->value.constval))
         return false;
      break;
   case E_CALL:
      stmt_type call1 = (stmt_type)e1->value.callval;
      stmt_type call2 = (stmt_type)e2->value.callval;
      if (!compare_call(call1->value.call_stmt_val, call2->value.call_stmt_val))
         return false;
      break;
   }

   return true;
}
/************************************************************************/
/*
 * compare_id:  Compares two id_type structs. Returns true if
 *    structures are identical, false otherwise.
 */
int compare_id(id_type i1, id_type i2)
{
   if (i1->type != i2->type)
      return false;
   if (i1->idnum != i2->idnum)
      return false;
   if (stricmp(i1->name, i2->name) != 0)
      return false;
   if (i1->ownernum != i2->ownernum)
      return false;
   return true;
}
/************************************************************************/
/*
 * compare_call:  Compares two call_stmt_type structs. Returns true if
 *    structures are identical, false otherwise.
 */
int compare_call(call_stmt_type c1, call_stmt_type c2)
{
   if (c1->function != c2->function)
      return false;

   list_type ag1 = c1->args, ag2 = c2->args;

   for (; ag1 && ag2; ag1 = ag1->next, ag2 = ag2->next)
   {
      arg_type arg1 = (arg_type)ag1->data;
      arg_type arg2 = (arg_type)ag2->data;
      if (arg1->type != arg2->type)
         return false;
      if (arg1->type == ARG_EXPR)
         if (!compare_expression(arg1->value.expr_val, arg2->value.expr_val))
            return false;
      if (arg1->type == ARG_SETTING)
      {
         if (!compare_id(arg1->value.setting_val->id, arg2->value.setting_val->id))
            return false;
         if (!compare_expression(arg1->value.setting_val->expr, arg2->value.setting_val->expr))
            return false;
      }
   }

   if (ag1 || ag2)
      return false;

   return true;
}
/************************************************************************/
/*
 * compare_constant:  Compares two const_type structs. Returns true if
 *    structures are identical, false otherwise.
 */
int compare_constant(const_type c1, const_type c2)
{
   if (c1->type != c2->type)
      return false;

   switch (c1->type)
   {
   case C_NIL:
      break;
   case C_NUMBER:
   case C_CLASS:
   case C_RESOURCE:
   case C_MESSAGE:
   case C_OVERRIDE:
      if (c1->value.numval != c2->value.numval)
         return false;
      break;
   case C_STRING:
   case C_FNAME:
      if (stricmp(c1->value.stringval, c2->value.stringval) != 0)
         return false;
      break;
      //   case C_FNAME:
      //      if (stricmp(c1->value.fnameval, c2->value.fnameval) != 0)
      //         return false;
      //      break;
   }

   return true;
}
/************************************************************************/
/*
 * get_message_name:  Takes call_stmt_type, returns the const char* message
 *    name, or NULL.
 */
const char* get_message_name(call_stmt_type call)
{
   int call_num = get_message_idnum(call);
   for (list_type list = table_get_all(st.globalvars); list; list = list->next)
   {
      id_type id = (id_type)list->data;
      if (id->type == I_MESSAGE && id->idnum == call_num)
         return id->name;
   }
   for (list_type list = table_get_all(st.missingvars); list; list = list->next)
   {
      id_type id = (id_type)list->data;
      if (id->type == I_MISSING && id->idnum == call_num)
         return id->name;
   }
   return NULL;
}
/************************************************************************/
/*
 * get_message_idnum:  Takes call_stmt_type, returns the integer message
 *   idnum or -1.
 */
int get_message_idnum(call_stmt_type call)
{
   if (call->function == SENDMESSAGE)
   {
      // Skip the first argument.
      arg_type arg1 = (arg_type)call->args->next->data;

      if (arg1->type == ARG_EXPR && arg1->value.expr_val->type == E_CONSTANT)
      {
         const_type cst = arg1->value.expr_val->value.constval;
         if (cst->type == C_MESSAGE)
            return cst->value.numval;
      }
   }

   return -1;
}
