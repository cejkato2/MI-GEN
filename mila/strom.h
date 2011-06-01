/* strom.h */

#ifndef _STROM_
#define _STROM_

#include "zaspoc.h"

class Node {
public:
   virtual Node *Optimize() {return this;}
   virtual void Translate() = 0;
   virtual ~Node() {}
   virtual void printNode() = 0;
};

class Expr : public Node {
};

class Statm : public Node {
};

class Var : public Expr {
public:
   int addr;
   bool rvalue;
   Var(int, bool);
   virtual void Translate();
   virtual void printNode();
};

class Numb : public Expr {
   int value;
public:
   Numb(int);
   virtual void Translate();
   int Value();
   virtual void printNode();
};

class Bop : public Expr {
   Operator op;
   Expr *left, *right;
public:
   Bop(Operator, Expr*, Expr*);
   virtual ~Bop();
   virtual Node *Optimize();
   virtual void Translate();
   virtual void printNode();
};

class UnMinus : public Expr {
   Expr *expr;
public:
   UnMinus(Expr *e);
   virtual ~UnMinus();
   virtual Node *Optimize();
   virtual void Translate();
   virtual void printNode();
};

class Assign : public Statm {
public:
   Var *var;
   Expr *expr;
   Assign(Var*, Expr*);
   virtual ~Assign();
   virtual Node *Optimize();
   virtual void Translate();
   virtual void printNode();
};

class Write : public Statm {
   Expr *expr;
public:
   Write(Expr*);
   virtual ~Write();
   virtual Node *Optimize();
   virtual void Translate();
   virtual void printNode();
};

class  If : public Statm {
   Expr *cond;
   Statm *thenstm;
   Statm *elsestm;
public:
   If(Expr*, Statm*, Statm*);
   virtual ~If();
   virtual Node *Optimize();
   virtual void Translate();
   virtual void printNode();
};

class While : public Statm {
   Expr *cond;
   Statm *body;
public:
   While(Expr*, Statm*);
   virtual ~While();
   virtual Node *Optimize();
   virtual void Translate();
   virtual void printNode();
};

class  StatmList : public Statm {
public:
   Statm *statm;
   StatmList *next;
   StatmList(Statm*, StatmList*);
   virtual ~StatmList();
   virtual Node *Optimize();
   virtual  void Translate();
   virtual void printNode();
};

class Empty : public Statm {
   virtual void Translate() {}
   virtual void gen_loud() {}
   virtual void printNode();
};

class Prog : public Node {
   StatmList *stm;
public:
   Prog(StatmList*);
   virtual ~Prog();
   virtual Node *Optimize();
   virtual void Translate();
   virtual void printNode();
};

Expr *VarOrConst(char*);

#endif
