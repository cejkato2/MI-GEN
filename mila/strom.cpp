/* strom.cpp */

#include "strom.h"
#include "tabsym.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


extern char *jmeno;

static int TraceCode = 1;

FILE *code = stdout;

/* pc = program counter  */
#define  pc 7

/* mp = "memory pointer" points
 * to top of memory (for temp storage)
 */
#define  mp 6

/* gp = "global pointer" points
 * to bottom of memory for (global)
 * variable storage
 */
#define gp 5

/* accumulator */
#define  ac 0

/* 2nd accumulator */
#define  ac1 1

/* TM location number for current instruction emission */
static int emitLoc = 0 ;

/* Highest TM location emitted so far
   For use in conjunction with emitSkip,
   emitBackup, and emitRestore */
static int highEmitLoc = 0;

/* Procedure emitComment prints a comment line 
 * with comment c in the code file
 */
void emitComment( char * c )
{ 
  if (TraceCode) {
    fprintf(code,"* %s\n",c);
  }
}

/* Procedure emitRO emits a register-only
 * TM instruction
 * op = the opcode
 * r = target register
 * s = 1st source register
 * t = 2nd source register
 * c = a comment to be printed if TraceCode is TRUE
 */
void emitRO( char *op, int r, int s, int t, char *c)
{ fprintf(code,"%3d:  %5s  %d,%d,%d ",emitLoc++,op,r,s,t);
  if (TraceCode) fprintf(code,"\t%s",c) ;
  fprintf(code,"\n") ;
  if (highEmitLoc < emitLoc) highEmitLoc = emitLoc ;
} /* emitRO */

/* Procedure emitRM emits a register-to-memory
 * TM instruction
 * op = the opcode
 * r = target register
 * d = the offset
 * s = the base register
 * c = a comment to be printed if TraceCode is TRUE
 */
void emitRM( char * op, int r, int d, int s, char *c)
{ fprintf(code,"%3d:  %5s  %d,%d(%d) ",emitLoc++,op,r,d,s);
  if (TraceCode) fprintf(code,"\t%s",c) ;
  fprintf(code,"\n") ;
  if (highEmitLoc < emitLoc)  highEmitLoc = emitLoc ;
} /* emitRM */

/* Function emitSkip skips "howMany" code
 * locations for later backpatch. It also
 * returns the current code position
 */
int emitSkip( int howMany)
{  int i = emitLoc;
   emitLoc += howMany ;
   if (highEmitLoc < emitLoc)  highEmitLoc = emitLoc ;
   return i;
} /* emitSkip */

/* Procedure emitBackup backs up to 
 * loc = a previously skipped location
 */
void emitBackup( int loc)
{ if (loc > highEmitLoc) emitComment("BUG in emitBackup");
  emitLoc = loc ;
} /* emitBackup */

/* Procedure emitRestore restores the current 
 * code position to the highest previously
 * unemitted position
 */
void emitRestore(void)
{ emitLoc = highEmitLoc;}

/* Procedure emitRM_Abs converts an absolute reference 
 * to a pc-relative reference when emitting a
 * register-to-memory TM instruction
 * op = the opcode
 * r = target register
 * a = the absolute location in memory
 * c = a comment to be printed if TraceCode is TRUE
 */
void emitRM_Abs( char *op, int r, int a, char * c)
{ fprintf(code,"%3d:  %5s  %d,%d(%d) ",
               emitLoc,op,r,a-(emitLoc+1),pc);
  ++emitLoc ;
  if (TraceCode) fprintf(code,"\t%s",c) ;
  fprintf(code,"\n") ;
  if (highEmitLoc < emitLoc) highEmitLoc = emitLoc ;
} /* emitRM_Abs */


/* tmpOffset is the memory offset for temps
   It is decremented each time a temp is
   stored, and incremeted when loaded again
*/
static int tmpOffset = 0;

static int line_num;

// konstruktory a destruktory

Var::Var(int a, bool rv)
{ addr = a; rvalue = rv; }

Numb::Numb(int v)
{ value = v; }

int Numb::Value()
{ return value; }

Bop::Bop(Operator o, Expr *l, Expr *r)
{ op = o; left = l; right = r; }

Bop::~Bop()
{ delete left; delete right; }

UnMinus::UnMinus(Expr *e)
{ expr = e; }

UnMinus::~UnMinus()
{ delete expr; }

Assign::Assign(Var *v, Expr *e)
{ var = v; expr = e; }

Assign::~Assign()
{ delete var; delete expr; }

Write::Write(Expr *e)
{ expr = e; }

Write::~Write()
{ delete expr; }

If::If(Expr *c, Statm *ts, Statm *es)
{ cond = c; thenstm = ts; elsestm = es; }

If::~If()
{
   delete cond;
   if (thenstm)
      delete thenstm;
   if (elsestm)
      delete elsestm;
}

While::While(Expr *c, Statm *b)
{ cond = c; body = b; }

While::~While()
{ delete body; }

StatmList::StatmList(Statm *s, StatmList *n)
{ statm = s; next = n; }

StatmList::~StatmList()
{ delete statm; delete next; }

Prog::Prog(StatmList *s)
{ stm = s; }

Prog::~Prog()
{ delete stm; } 

// definice metody Optimize

Node *Bop::Optimize() 
{
   left = (Expr*)(left->Optimize());
   right = (Expr*)(right->Optimize());
   Numb *l = dynamic_cast<Numb*>(left);
   Numb *r = dynamic_cast<Numb*>(right);
   if (!l || !r) return this;
   int res;
   int leftval = l->Value();
   int rightval = r->Value();
   switch (op) {
   case Plus:
      res = leftval + rightval;
      break;
   case Minus:
      res = leftval - rightval;
      break;
   case Times:
      res = leftval * rightval;
      break;
   case Divide:
      res = leftval / rightval;
      break;
   case Eq:
      res = leftval == rightval;
      break;
   case NotEq:
      res = leftval != rightval;
      break;
   case Less:
      res = leftval < rightval;
      break;
   case Greater:
      res = leftval > rightval;
      break;
   case LessOrEq:
      res = leftval <= rightval;
      break;
   case GreaterOrEq:
      res = leftval >= rightval;
      break;
   }
   delete this;
   return new Numb(res);
}

Node *UnMinus::Optimize()
{
   expr = (Expr*)expr->Optimize();
   Numb *e = dynamic_cast<Numb*>(expr);
   if (!e) return this;
   e = new Numb(-e->Value());
   delete this;
   return e;
}

Node *Assign::Optimize()
{
   expr = (Var*)(expr->Optimize());
   return this;
}

Node *Write::Optimize()
{
   expr = (Expr*)(expr->Optimize());
   return this;
}

Node *If::Optimize()
{
   cond = (Expr*)(cond->Optimize());
   thenstm = (Statm*)(thenstm->Optimize());
   if (elsestm)
      elsestm = (Statm*)(elsestm->Optimize());
   Numb *c = dynamic_cast<Numb*>(cond);
   if (!c) return this;
   Node *res;
   if (c->Value()) {
      res = thenstm; thenstm = 0;
   } else {
      res = elsestm; elsestm = 0;
   }
   delete this;
   return res;
}

Node *While::Optimize()
{
   cond = (Expr*)(cond->Optimize());
   body = (Statm*)(body->Optimize());
   Numb *c = dynamic_cast<Numb*>(cond);
   if (!c) return this;
   if (!c->Value()) {
      delete this;
      return new Empty;
   }


   return this;
}

Node *StatmList::Optimize()
{
   StatmList *s = this;
   do {
      s->statm = (Statm*)(s->statm->Optimize());
      s = s->next;
   }
   while (s);
   return this;
}

Node *Prog::Optimize()
{
   stm = (StatmList*)(stm->Optimize());
   return this;
}

// definice metody Translate

void Var::Translate()
{
//TODO
   Gener(TA, addr);
   if (rvalue)
      Gener(DR);
   /* Louden */
//    case IdK :
//      if (TraceCode) emitComment("-> Id") ;
//      loc = st_lookup(tree->attr.name);
//      emitRM("LD",ac,loc,gp,"load id value");
//      if (TraceCode)  emitComment("<- Id") ;
//      break; /* IdK */
}

void Numb::Translate()
{
//TODO
   Gener(TC, value);
   /* Louden */
//    case ConstK :
//      if (TraceCode) emitComment("-> Const") ;
//      /* gen code to load integer constant using LDC */
//      emitRM("LDC",ac,tree->attr.val,0,"load const");
//      if (TraceCode)  emitComment("<- Const") ;
}

void Bop::Translate()
{
//TODO
   left->Translate();
   right->Translate();
   Gener(BOP, op);
   /* Louden */
//   if (TraceCode) emitComment("-> Op") ;
//   p1 = tree->child[0];
//   p2 = tree->child[1];
//   /* gen code for ac = left arg */
//   cGen(p1);
//   /* gen code to push left operand */
//   emitRM("ST",ac,tmpOffset--,mp,"op: push left");
//   /* gen code for ac = right operand */
//   cGen(p2);
//   /* now load left operand */
//   emitRM("LD",ac1,++tmpOffset,mp,"op: load left");
//   switch (tree->attr.op) {
//     case PLUS :
//       emitRO("ADD",ac,ac1,ac,"op +");
//       break;
//     case MINUS :
//       emitRO("SUB",ac,ac1,ac,"op -");
//       break;
//     case TIMES :
//       emitRO("MUL",ac,ac1,ac,"op *");
//       break;
//     case OVER :
//       emitRO("DIV",ac,ac1,ac,"op /");
//       break;
//     case LT :
//       emitRO("SUB",ac,ac1,ac,"op <") ;
//       emitRM("JLT",ac,2,pc,"br if true") ;
//       emitRM("LDC",ac,0,ac,"false case") ;
//       emitRM("LDA",pc,1,pc,"unconditional jmp") ;
//       emitRM("LDC",ac,1,ac,"true case") ;
//       break;
//     case EQ :
//       emitRO("SUB",ac,ac1,ac,"op ==") ;
//       emitRM("JEQ",ac,2,pc,"br if true");
//       emitRM("LDC",ac,0,ac,"false case") ;
//       emitRM("LDA",pc,1,pc,"unconditional jmp") ;
//       emitRM("LDC",ac,1,ac,"true case") ;
//       break;
//     default:
//       emitComment("BUG: Unknown operator");
//       break;
//   } /* case op */
//   if (TraceCode)  emitComment("<- Op") ;
//   break; /* OpK */
}

void UnMinus::Translate()
{
//TODO
   expr->Translate();
   Gener(UNM);
}

void Assign::Translate()
{
//TODO
   var->Translate();
   expr->Translate();
   Gener(ST);

  /* Louden */
// if (TraceCode) emitComment("-> assign") ;
// /* generate code for rhs */
// cGen(tree->child[0]);
// /* now store value */
// loc = st_lookup(tree->attr.name);
// emitRM("ST",ac,loc,gp,"assign: store value");
// if (TraceCode)  emitComment("<- assign") ;
}

void Write::Translate()
{
  /* generate code for expression to write */
  expr->Translate();
  /* now output it */
  emitRO("OUT",ac,0,0,"write ac");
}

void If::Translate()
{
  if (TraceCode) emitComment("-> if") ;
  /* generate code for test expression */
  cond->Translate();
  int savedLoc1 = emitSkip(1) ;
  emitComment("if: jump to else belongs here");

  /* recurse on then part */
  thenstm->Translate();
  int savedLoc2 = emitSkip(1) ;
  emitComment("if: jump to end belongs here");
  int currentLoc = emitSkip(0) ;
  emitBackup(savedLoc1) ;
  emitRM_Abs("JEQ",ac,currentLoc,"if: jmp to else");
  emitRestore() ;

  if (elsestm) {
    elsestm->Translate();
    currentLoc = emitSkip(0) ;
    emitBackup(savedLoc2) ;
    emitRM_Abs("LDA",pc,currentLoc,"jmp to end") ;
    emitRestore() ;
  } else   {
    emitBackup(savedLoc1) ;
    emitRestore() ;
  }

  if (TraceCode)  emitComment("<- if") ;
}

//    case RepeatK:
//       if (TraceCode) emitComment("-> repeat") ;
//       p1 = tree->child[0] ;
//       p2 = tree->child[1] ;
//       savedLoc1 = emitSkip(0);
//       emitComment("repeat: jump after body comes back here");
//       /* generate code for body */
//       cGen(p1);
//       /* generate code for test */
//       cGen(p2);
//       emitRM_Abs("JEQ",ac,savedLoc1,"repeat: jmp back to body");
//       if (TraceCode)  emitComment("<- repeat") ;

void While::Translate()
{
//TODO
   int a1 = GetIC();
   cond->Translate();
   int a2 = Gener(IFJ);
   body->Translate();
   Gener(JU, a1);
   PutIC(a2);
}

void StatmList::Translate()
{
   StatmList *s = this;
   do {
      s->statm->Translate();
      s = s->next;
   } while (s);
}

void Prog::Translate()
{
   char * s = (char *) malloc((strlen(jmeno)+7)*sizeof(char));
   strcpy(s,"File: ");
   strcat(s,jmeno);
   emitComment("TINY Compilation to TM Code");
   emitComment(s);
   /* generate standard prelude */
   emitComment("Standard prelude:");
   emitRM("LD",mp,0,ac,"load maxaddress from location 0");
   emitRM("ST",ac,0,ac,"clear location 0");
   emitComment("End of standard prelude.");
   /* generate code for TINY program */

   stm->Translate();

   /* finish */
   emitComment("End of execution.");
   emitRO("HALT",0,0,0,"");
}

      
//Louden read
//case ReadK:
//     emitRO("IN",ac,0,0,"read integer value");
//     loc = st_lookup(tree->attr.name);
//     emitRM("ST",ac,loc,gp,"read: store value");
//     break;


Expr *VarOrConst(char *id)
{
   int v;
   DruhId druh = idPromKonst(id,&v);
   switch (druh) {
   case IdProm:
      return new Var(v, true);
   case IdKonst:
      return new Numb(v);
   }
}

// print
void Var::printNode()
{
  printf("Var");
}

void Numb::printNode()
{
  printf("Numb");
}

void Bop::printNode()
{
  printf("BinOp");
  left->printNode();
  printf("%i", op);
  right->printNode();
}

void UnMinus::printNode()
{
  printf("UnMinus");
  expr->printNode();
}

void Assign::printNode()
{
  printf("Assign");
  var->printNode();
  expr->printNode();
}

void Write::printNode()
{
  printf("Write");
  expr->printNode();
}

void If::printNode()
{
  printf("If (");
  cond->printNode();
  printf(") {");
  thenstm->printNode();
  printf("}");
  if (elsestm) {
    printf("else{");
    elsestm->printNode();
    printf("}");
  }
}

void While::printNode()
{
  printf("While (");
  cond->printNode();
  printf(") {");
  body->printNode();
  printf("}");
}

void StatmList::printNode()
{
  printf("StatmList:");
  StatmList *s = this;
  do {
    s->statm->printNode();
    s = s->next;
  }
  while (s);
}

void Empty::printNode()
{
  printf("Empty");
}

void Prog::printNode()
{
  printf("Prog");
  //StatmList
  stm->printNode();
}

