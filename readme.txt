V adresari mila/ se nachazi kompletni prekladac z jazyka Mila do vlastni
zasobnikove architektury, vcetne interpreteru.
Vse se vola z main.c a muze byt snadno orezano jen na pozadovane casti.
Lze prelozit pomoci
g++ *.cpp



V adresari interpreter/ je interpreter RISC procesoru
podle knihy Louden, Kenneth.C.: Compiler Construction
Kompletni kody vcetne prekladace a odkazem na knihu lze najit na
http://www.cs.sjsu.edu/~louden/cmptext/
Do interpreteru byla pridana instrukce INC

Interpreter lze zkompilovat pomoci
gcc tm.c

V adresari se nachazi ukazkovy program ve vstupnim jazyce Loudenova prekladace
(velmi podobny Mile) a vysledny prelozeny program. Interpreterem lze prelozeny program
i krokovat.