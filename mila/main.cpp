/* main.c */
/* syntakticky analyzator */

#include "lexan.h"
#include "parser.h"
#include "strom.h"
#include "zaspoc.h"
#include <stdio.h>

char *jmeno;

int main(int argc, char *argv[])
{
   fprintf(stderr, "Syntakticky analyzator\n");
   if (argc == 1) {
      fprintf(stderr, "Vstup z klavesnice, zadejte zdrojovy text\n");
      jmeno = NULL;
   } else {
      jmeno = argv[1];
      fprintf(stderr, "Vstupni soubor %s\n", jmeno);
   }
   InitLexan(jmeno);
   CtiSymb();
   Prog *prog = Program();
   prog->printNode();
   fprintf(stderr, "\n\n");
   prog = (Prog*)(prog->Optimize());
   prog->printNode();
   fprintf(stderr, "\n");
   prog->Translate();
//   Print();
   //Run();
}
