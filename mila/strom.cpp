/* strom.cpp */

#include "strom.h"
#include "tabsym.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <map>


extern char *jmeno;

bool expr_cmp_var(Expr *l, Expr *r)
{
  Var *lt = dynamic_cast<Var *>(l);
  Var *rt = dynamic_cast<Var *>(r);

  if ((lt == NULL) || (rt == NULL)) {
    //they are not the same type        
    return false;
  }

  if (lt->addr != rt->addr) {
    return false;
  }
  return true;  
}

bool expr_cmp_numb(Expr *l, Expr *r)
{
  Numb *lt = dynamic_cast<Numb *>(l);
  Numb *rt = dynamic_cast<Numb *>(r);

  if ((lt == NULL) || (rt == NULL)) {
    //they are not the same type        
    return false;
  }

  if (lt->Value() != rt->Value()) {
    return false;
  }
  return true;  
}


bool subtree_cmp(Bop *l, Bop *r)
{
  if (l->op != r->op) {
    return false;
  }

  if (expr_cmp_var(l->left, r->left)) {
    if (expr_cmp_var(l->right, r->right)) {
      return true;
    } else if (expr_cmp_numb(l->right, r->right)) {
      return true;
    } else {
      return false;
    }
  }

  if (expr_cmp_var(l->left, r->right)) {
    if (expr_cmp_var(l->right, r->left)) {
      return true;
    } else if (expr_cmp_numb(l->right, r->left)) {
      return true;
    } else {
      return false;
    }
  }
}

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
{
  if (statm != NULL) {
    delete statm; 
  }
  if (next != NULL) {
    delete next; 
  }
}

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
  if (!l || !r) {
    return this;
  } else {
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

  std::vector<Bop *> listBops;
  std::vector<Bop *>::iterator it;

  expr->findBops(listBops);

  for (it=listBops.begin(); it!=listBops.end(); ++it) {
    (*it)->printNode();
    fprintf(stderr, "\n");
  }

  std::map<Bop *, std::vector<Bop *> > dupl;
  std::map<Bop *, std::vector<Bop *> >::iterator duplit;

  for (int l=0; l<listBops.size(); ++l) {
    for (int r=l+1; r<listBops.size(); ++r) {
      if (subtree_cmp(listBops[l], listBops[r]) == true) {
        dupl[listBops[l]].push_back(listBops[r]);
      }
    }
    std::vector<Bop *> vec = dupl[listBops[l]];
    if (vec.size() > 0) {
      dupl[listBops[l]].push_back(listBops[l]);
    }
  }

  std::map<Bop *, std::vector<Bop *> >::iterator max;
  Bop *maxBop = NULL;
  int maxcount = 0;
  for (duplit=dupl.begin(); duplit!=dupl.end(); ++duplit) {
    if (duplit->second.size() > maxcount) {
      maxcount = (*duplit).second.size();
      max = duplit;
      maxBop = duplit->first;
    }
  }
  fprintf(stderr, "Max amount of duplicates: %i\n", maxcount);

  if (maxBop != NULL) {
    maxBop->printNode();
    fprintf(stderr, "\n");

    // subtrees for replacing
    std::vector<Bop *> vec = dupl.find(maxBop)->second;
    for (it=vec.begin(); it!=vec.end(); ++it) {
      (*it)->printNode();
      fprintf(stderr, "\n");
    }

    if (vec.size() > 0) {
      //make new assigment to temp
      Assign *temp = new Assign(new Var(0, false), vec.back());
      vec.pop_back();
      StatmList *oldlist = new StatmList(this, NULL);
      StatmList *templist = new StatmList(temp, oldlist);

      expr->replaceSubtree(vec);
      
      return templist;
    }
  }


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
  if (c != NULL) {
    if (!c->Value()) {
      delete this;
      return new Empty;
    }
  }


  bool cleanup = true;
  StatmList *newWhile = new StatmList(NULL, NULL);

  fprintf(stderr, "WHILE cycle for optimizing\n");
  body->printNode();
  fprintf(stderr, "\n\n");
  StatmList *stl = dynamic_cast<StatmList *>(body);
  Assign *assig = NULL;
  StatmList *buffer = NULL;
  if (stl != NULL) {
    fprintf(stderr, "body is StatmList\n");
    buffer = stl;

    std::map<int, int> lvarlist;
    std::map<int, int> rvarlist;
    std::map<int, StatmList *> replacelist;
    std::map<int, int>::iterator it;
    std::map<int, StatmList *>::iterator itstl;
    
    while (buffer != NULL) {
      assig = dynamic_cast<Assign *>(buffer->statm);            

      if (assig != NULL) {
        fprintf(stderr, "Assign in while\n");
        /* add variable on the left side to
         * expected
         */
        lvarlist[assig->var->addr]++;
        
        if (dynamic_cast<Numb *>(assig->expr) != NULL) {
          fprintf(stderr, "const assign to %i\n", assig->var->addr);
          replacelist[assig->var->addr] = buffer;
        }
      }
      buffer->statm->findVars(rvarlist);  
      
      buffer = buffer->next;
    }

    for (it=lvarlist.begin(); it!=lvarlist.end(); ++it) {
      if (it->second == 1) {
        fprintf(stderr, "assign of var %i used once\n", it->first);
        if (replacelist[it->first] != NULL) {
          
          replacelist[it->first]->printNode();
          fprintf(stderr, "\n");
        }
      }
    }

    for (it=rvarlist.begin(); it!=rvarlist.end(); ++it) {
      fprintf(stderr, "var %i used %i\n", it->first, it->second);
    }

    fprintf(stderr, "Modifiing\n");
    for (itstl=replacelist.begin(); itstl!=replacelist.end(); ++itstl) {
      if (rvarlist.find(itstl->first) == rvarlist.end()) {
        itstl->second->printNode();
      } else {
        replacelist.erase(itstl);
      }
    }
    if (replacelist.empty() != true) {
      for (itstl=replacelist.begin(); itstl!=replacelist.end(); ++itstl) {
        StatmList *remove = itstl->second;
        StatmList *begin = stl;
        if (newWhile->statm == NULL) {
          newWhile->statm = this;
        }

        if (begin == remove) {
          StatmList *temp = new StatmList(begin->statm, newWhile);
          begin->statm = new Empty();
          newWhile = temp;
          cleanup = false;
          fprintf(stderr, "\nmodified:\n");
          newWhile->printNode();
          fprintf(stderr, "\n");
        } else {
          while (begin->next != remove) {
            begin = begin->next;
          }
          begin->next = begin->next->next;
          begin = new StatmList(remove, newWhile);
          newWhile = begin;
          cleanup = false;
        }
      }
    }
  } 
  
  fprintf(stderr, "\n\n--------------------------\n");

  //body->printNode();

  if (cleanup == true) {
    delete newWhile;
    return this;
  }
  return newWhile;
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
  fprintf(stderr, "Prog optimizing\n");
  stm = (StatmList*)(stm->Optimize());
  return this;
}

// definice metody Translate

void Var::Translate()
{
  // rvalue (load)
  if (TraceCode) emitComment("-> Id") ;
  emitRM("LD",ac,addr,gp,"load id value");
  if (TraceCode)  emitComment("<- Id") ;
}

void Numb::Translate()
{
  if (TraceCode) emitComment("-> Const") ;
  /* gen code to load integer constant using LDC */
  printf("* CONST %i\n", value);
  emitRM("LDC",ac,value,0,"load const");
  if (TraceCode) emitComment("<- Const") ;
}

void Bop::Translate()
{
  if (TraceCode)  emitComment("-> BinOp") ;
  /* gen code for ac = left arg */
  left->Translate();
  /* gen code to push left operand */
  emitRM("ST",ac,tmpOffset--,mp,"op: push left");

  /* gen code for ac = right operand */
  right->Translate();

  /* now load left operand */
  emitRM("LD",ac1,++tmpOffset,mp,"op: load left");
  /*
   enum Operator {Plus, Minus, Times, Divide,
   Eq, NotEq, Less, Greater, LessOrEq, GreaterOrEq};
   */
  switch (op) {
    case Plus :
      emitRO("ADD",ac,ac1,ac,"op +");
      break;
    case Minus :
      emitRO("SUB",ac,ac1,ac,"op -");
      break;
    case Times :
      emitRO("MUL",ac,ac1,ac,"op *");
      break;
    case Divide :
      emitRO("DIV",ac,ac1,ac,"op /");
      break;
    case Less :
      emitRO("SUB",ac,ac1,ac,"op <") ;
      emitRM("JLT",ac,2,pc,"br if true") ;
      emitRM("LDC",ac,0,ac,"false case") ;
      emitRM("LDA",pc,1,pc,"unconditional jmp") ;
      emitRM("LDC",ac,1,ac,"true case") ;
      break;
    case Eq :
      emitRO("SUB",ac,ac1,ac,"op ==") ;
      emitRM("JEQ",ac,2,pc,"br if true");
      emitRM("LDC",ac,0,ac,"false case") ;
      emitRM("LDA",pc,1,pc,"unconditional jmp") ;
      emitRM("LDC",ac,1,ac,"true case") ;
      break;
    case NotEq:
      emitRO("SUB",ac,ac1,ac,"op !=") ;
      emitRM("JNE",ac,2,pc,"br if true") ;
      emitRM("LDC",ac,0,ac,"false case") ;
      emitRM("LDA",pc,1,pc,"unconditional jmp") ;
      emitRM("LDC",ac,1,ac,"true case") ;
      break;
    case Greater:
      emitRO("SUB",ac,ac1,ac,"op !=") ;
      emitRM("JGT",ac,2,pc,"br if true") ;
      emitRM("LDC",ac,0,ac,"false case") ;
      emitRM("LDA",pc,1,pc,"unconditional jmp") ;
      emitRM("LDC",ac,1,ac,"true case") ;
      break;
    case LessOrEq:
      emitRO("SUB",ac,ac1,ac,"op !=") ;
      emitRM("JLE",ac,2,pc,"br if true") ;
      emitRM("LDC",ac,0,ac,"false case") ;
      emitRM("LDA",pc,1,pc,"unconditional jmp") ;
      emitRM("LDC",ac,1,ac,"true case") ;
      break;
    case GreaterOrEq:
      emitRO("SUB",ac,ac1,ac,"op !=") ;
      emitRM("JGE",ac,2,pc,"br if true") ;
      emitRM("LDC",ac,0,ac,"false case") ;
      emitRM("LDA",pc,1,pc,"unconditional jmp") ;
      emitRM("LDC",ac,1,ac,"true case") ;
      break;
    default:
      emitComment("!!!!!BUG: Unknown operator!!!!!!");
      break;
  } /* case op */
  if (TraceCode)  emitComment("<- BinOp") ;
}

void UnMinus::Translate()
{
  expr->Translate();

  /* load const 0 */
  emitRM("LDC",ac1,0,0,"load const0");
  /* ac = 0 - ac */
  emitRO("SUB",ac,ac1,ac,"unMinus: val=0-val") ;
}

void Assign::Translate()
{
  if (TraceCode) emitComment("-> assign") ;
  /* generate code for rhs */
  expr->Translate();

  /* now store value */
  emitRM("ST",ac,var->addr,gp,"assign: store value");
  printf("* stored into var %i\n", var->addr);
  if (TraceCode) emitComment("<- assign") ;
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

void While::Translate()
{
  if (TraceCode)  emitComment("<- while") ;
  int zacatek = emitSkip(0);
  cond->Translate();
  int ifjump = emitSkip(1);
  body->Translate();
  emitRM_Abs("LDA",pc,zacatek,"jmp back to cond") ;

  int konec = emitSkip(0);
  emitBackup(ifjump);
  emitRM_Abs("JEQ",ac,konec,"jmp to end");

  /* restore address - after while (){ } */
  emitRestore();
  if (TraceCode)  emitComment("<- while") ;
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
   free(s);
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
  fprintf(stderr, "Var(%i)", addr);
}

void Numb::printNode()
{
  fprintf(stderr, "Numb");
}

void Bop::printNode()
{
  fprintf(stderr, "(");
  left->printNode();
  fprintf(stderr, "BinOp%i", op);
  right->printNode();
  fprintf(stderr, ")", op);
}

void UnMinus::printNode()
{
  fprintf(stderr, "UnMinus");
  expr->printNode();
}

void Assign::printNode()
{
  fprintf(stderr, "Assign");
  var->printNode();
  expr->printNode();
}

void Write::printNode()
{
  fprintf(stderr, "Write");
  expr->printNode();
}

void If::printNode()
{
  fprintf(stderr, "If (");
  cond->printNode();
  fprintf(stderr, ") {");
  thenstm->printNode();
  fprintf(stderr, "}");
  if (elsestm) {
    fprintf(stderr, "else{");
    elsestm->printNode();
    fprintf(stderr, "}");
  }
}

void While::printNode()
{
  fprintf(stderr, "While (");
  cond->printNode();
  fprintf(stderr, ") {");
  body->printNode();
  fprintf(stderr, "}");
}

void StatmList::printNode()
{
  fprintf(stderr, "StatmList:");
  StatmList *s = this;
  do {
    s->statm->printNode();
    s = s->next;
  }
  while (s);
}

void Empty::printNode()
{
  fprintf(stderr, "Empty");
}

void Prog::printNode()
{
  fprintf(stderr, "Prog");
  //StatmList
  stm->printNode();
}

// findVars
void Var::findVars(std::map<int, int> &list)
{
  list[addr]++;  
}

void Bop::findVars(std::map<int, int> &list)
{
  left->findVars(list);
  right->findVars(list);
}

void UnMinus::findVars(std::map<int, int> &list)
{
  expr->findVars(list);
}

void Assign::findVars(std::map<int, int> &list)
{
  //var->findVars(list);
  expr->findVars(list);
}

void Write::findVars(std::map<int, int> &list)
{
  expr->findVars(list);
}

void If::findVars(std::map<int, int> &list)
{
  cond->findVars(list);
  thenstm->findVars(list);
  if (elsestm) {
    elsestm->findVars(list);
  }
}

void While::findVars(std::map<int, int> &list)
{
  cond->findVars(list);
  body->findVars(list);
}

void StatmList::findVars(std::map<int, int> &list)
{
  StatmList *s = this;
  do {
    s->statm->findVars(list);
    s = s->next;
  }
  while (s);
}

void Prog::findVars(std::map<int, int> &list)
{
  stm->findVars(list);
}

void Bop::findBops(std::vector<Bop *> &list) 
{
  this->left->findBops(list);
  list.push_back(this);
  this->right->findBops(list);
}

void Bop::replaceSubtree(std::vector<Bop *> &list)
{
  this->left->replaceSubtree(list);
  this->right->replaceSubtree(list);
  std::vector<Bop *>::iterator it;
  for (it=list.begin(); it!=list.end(); ++it) {
    if (this->left == *it) {
      delete this->left;
      this->left = new Var(0, true);
    }
    if (this->right == *it) {
      delete this->right;
      this->right = new Var(0, true);
    }
  }

}
