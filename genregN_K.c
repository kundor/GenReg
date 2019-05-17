/* genreg by Markus Meringer */
/* e-mail markus@btm2xg.mat.uni-bayreuth.de */

#define NOTSET 127

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned long ULONG;
typedef unsigned int UINT;

static char lst[24];
static FILE *lstfile;

#ifndef NVERT
#define NVERT 16
#endif

#ifndef DEGREE
#define DEGREE 5
#endif

#ifndef gsize
#define gsize 21     /* (n*n - n + 11) / 12 + 1 */
#endif
#if gsize != (NVERT*NVERT - NVERT + 11) / 12 + 1
#error "Wrong gsize. Should be ceil((NVERT^2 - NVERT)/12) + 1."
#endif

#ifndef SPLITLEVEL
#if DEGREE == 3
#define SPLITLEVEL 11
#elif DEGREE == 7
#define SPLITLEVEL 6
#elif 3 < DEGREE && DEGREE < 7
#define SPLITLEVEL (12 - DEGREE)
#else
#error "You must define SPLITLEVEL."
#endif
#endif

#ifndef N6CHAR
#if NVERT != 16
#error "You must define N6CHAR."
#endif
#define N6CHAR 'O'
#endif

static int splitlevel = NOTSET;
static int storeall = 0;
static int codestdout = 0;
static ULONG anz = 0, tostore = 0, calls;
static UINT jobs = 0, jobnr = 0;

static int springen, i1, j1, girth_exact;
static int adj[NVERT+1][NVERT+1] = {0};
static int nbr[NVERT+1][DEGREE+1] = {0};
static int transp[NVERT+1][NVERT+1], zbk[NVERT+1][NVERT+1+DEGREE*2], einsen[NVERT+1][NVERT+1], part[NVERT+1][NVERT+1];
static int kmn[NVERT+1], grad[NVERT+1], lgrad[NVERT+1];

static unsigned char gbytes[gsize + 2];

static void komprtofile() {
   // Note: unrolling a la 
   // gbytes[1] = 63 + (32*adj[1][2] + 16*adj[1][3] + 8*adj[2][3] + 4*adj[1][4] + 2*adj[2][4] + 1*adj[3][4]);
   // doesn't seem to speed it up
   int i, j, ibit = 6, jbit = 0;
   for (int i = 1; i < gsize; ++i)
      gbytes[i] = 63;
   for (i = 2; i <= NVERT; ++i) {
      for (j = 1; j <= DEGREE; ++j) {
         if (nbr[i][j] < i) {
            jbit = ibit + nbr[i][j] - 1;
            gbytes[jbit/6] += 32 >> (jbit % 6);
         }
      }
      ibit += (i - 1);
   }
   fputs((char*)&gbytes, lstfile);
}

/*
   girthstart ermittelt Taillenweite
   und gibt diese zurÅck bzw 0,
   falls Knoten 3 nicht auf dem
   (ersten Taillenkreis liegt
   wird nur aufgerufen, wenn
   1.Kreis soeben geschlossen wurde
   */

static int girthstart() {
   int tw = 2, last = 1, now = 2, next;

   while(now != 3) {
      next = nbr[now][1];
      if(next == last)
         next = nbr[now][2];
      if(next == 0 || next == 2)
         return 0;
      last = now;
      now = next;
      tw++;
   }
   return tw;
}

/*
   girthcheck2 testet, ob sich die
   Taillenweite nach EinfÅgen
   von Kante(mx, my) verkleinert,
   wenn ja wird weue tw zurÅckgegeben
   (d.h. G nicht max), sonst 127
   */

static int girthcheck2(int mx, int my, int tw) {
   int *status, *xnachb, *ynachb;
   int welle = 1, a, i, j, x, y;
   xnachb = nbr[mx];
   ynachb = nbr[my];

   if(tw == 4) {
      for(i = 1; i < grad[mx]; i++) {
         x = xnachb[i];
         for(j = 1; j < grad[my]; j++)
            if(x == ynachb[j])
               return 3;
      }
      return 127;
   }

   if(tw == 5) {
      for(i = 1; i < grad[mx]; i++) {
         x = xnachb[i];
         for(j = 1; j < grad[my]; j++) {
            y = ynachb[j];
            if(x == y)
               return 3;
            if(adj[x][y])
               return 4;
         }
      }
      return 127;
   }

   status = zbk[0];
   for(i = 1; i <= NVERT; i++)
      status[i] = 0;
   status[mx] = 1;
   status[my] = 2;
   zbk[2][1] = my;
   zbk[2][0] = 1;
   for(i = 1; i < grad[mx]; i++) {
      x = xnachb[i];
      zbk[3][i] = x;
      status[x] = 3;
   }
   zbk[3][0] = grad[mx] - 1;
   while(++welle < tw - 1) {
      a = 0;
      for(i = 1; i <= zbk[welle][0]; i++) {
         x = zbk[welle][i];
         for(j = 1; j <= grad[x]; j++) {
            y = nbr[x][j];
            if(status[y] == 0) {
               status[y] = welle + 2;
               zbk[welle+2][++a] = y;
            }
            else
               if(status[y] == welle + 1)
                  return welle + 1;
         }
      }
      zbk[welle+2][0] = a;
   }
   return 127;
}

/*
   ongirth0 prueft fuer gerade Taillenweite tw,
   ob Knoten v auf einem Taillenkreis liegt.
   Wenn ja, wird 1 zurueckgegben, sonst 0.
   */

static int ongirth0(int v, int tw) {
   int *status, last, a, h, i, j = 0, x, y;
   status = zbk[0];
   for(i = 1; i <= NVERT; i++)
      status[i] = 0;
   status[v] = -1;
   for(h = 1; h <= DEGREE; h++) {
      last = tw/2 + h-2;
      x = nbr[v][h];
      status[x] = h;
      zbk[last][1] = x;
      zbk[last][0] = 1;
      while(last > h) {
         a = 0;
         for(i = 1; i <= zbk[last][0]; i++) {
            x = zbk[last][i];
            for(j = 1; j <= DEGREE; j++) {
               y = nbr[x][j];
               if(status[y] == 0) {
                  status[y] = h;
                  zbk[last-1][++a] = y;
               }
            }
         }
         zbk[--last][0] = a;
      }
   }
   for(h = 1; h < DEGREE; h++) {
      for(i = 1; i <= zbk[h][0]; i++) {
         x = zbk[h][i];
         for(j = 1; j <= DEGREE; j++) {
            y = nbr[x][j];
            if(status[y] >= 0)
               status[y] = h;
         }
      }
      for(last = h + 1; last <= DEGREE; last++) {
         for(i = 1; i <= zbk[last][0]; i++) {
            x = zbk[last][i];
            j = 0;
            for(j = 1; j <= DEGREE; j++) {
               y = nbr[x][j];
               if(status[y] == h)
                  return 1;
            }
         }
      }
   }
   return 0;
}

/*
   ongirth0 prueft fuer ungerade Taillenweite tw,
   ob Knoten v auf einem Taillenkreis liegt.
   Wenn ja, wird 1 zurueckgegben, sonst 0.
   */

static int ongirth1(int v, int tw) {
   int *status, last, a, h, i, j = 0, x, y;
   status = zbk[0];
   for(i = 1; i <= NVERT; i++)
      status[i] = 0;
   status[v] = -1;
   for(h = 1; h <= DEGREE; h++) {
      last = tw/2 + h-1;
      x = nbr[v][h];
      status[x] = h;
      zbk[last][1] = x;
      zbk[last][0] = 1;
      while(last > h) {
         a = 0;
         for(i = 1; i <= zbk[last][0]; i++) {
            x = zbk[last][i];
            for(j = 1; j <= DEGREE; j++) {
               y = nbr[x][j];
               if(status[y] == 0) {
                  status[y] = h;
                  zbk[last-1][++a] = y;
               }
            }
         }
         zbk[--last][0] = a;
      }
      if(h > 1) {
         for(i = 1; i <= zbk[h][0]; i++) {
            x = zbk[h][i];
            for(j = 1; j <= DEGREE; j++) {
               y = nbr[x][j];
               if(status[y] > 0)
                  return 1;
            }
         }
      }
   }
   return 0;
}

static int knoten_als_eins(int v, int tw) {
   int i, j, x;
   if(tw == 3) {
      for(i = 1; i <= DEGREE; i++) {
         x = nbr[v][i];
         for(j = i + 1; j <= DEGREE; j++)
            if(adj[x][nbr[v][j]])
               return 1;
      }
      return 0;
   } else if(tw%2 == 0) {
      return ongirth0(v, tw);
   } else {
      return ongirth1(v, tw);
   }
}

static void sprungindex(int i, int j) {
   int r, s, x, y, z, e;
   i1 = 1;
   j1 = j;
   for(r = 1; r <= i; r++) {
      e = lgrad[r];
      for(s = r + 1; e < DEGREE && (r < i || s < j); s++) {
         z = adj[r][s]; e += z;
         if(z) {
            x = kmn[r];
            y = kmn[s];
            if(x > y) {
               z = x;
               x = y;
               y = z;
            }
            if(x > i1 || (x == i1 && y > j1)) {
               i1 = x;
               j1 = y;
            }
         }
      }
   }
   x = kmn[i];
   y = kmn[j];
   if(x > y) {
      z = x;
      x = y;
      y = z;
   }
   if(x > i1 || (x == i1 && y > j1)) {
      i1 = x;
      j1 = y;
   }
}

static void transpinv(int *mperm) {
   int i, x, re, li;
   i = 2*mperm[0];
   while(i > 0) {
      re = mperm[i--];
      li = mperm[i--];
      x = kmn[re];
      kmn[re] = kmn[li];
      kmn[li] = x;
   }
}

static int maxinblock(int *zeile, int *mperm, int e, int li, int re) {
   int i, x;
   i = 2*mperm[0];
   while(e >= 0 && li <= re) {
      while(zeile[kmn[li]]) {
         e--;
         if(++li > re)
            return e;
      }
      if(li == re)
         return e;
      while(zeile[kmn[re]] == 0)
         if(--re == li)
            return e;
      mperm[++i] = li;
      mperm[++i] = re;
      mperm[0]++;
      x = kmn[li];
      kmn[li] = kmn[re];
      kmn[re] = x;
      li++;
      re--;
      e--;
   }
   return e;
}

static int maxinzeile(int tz) {
   int b, *zeile, *block, *eintr, *mperm;
   int e, li, re, erg = 0;
   zeile = adj[kmn[tz]];
   block = part[tz];
   eintr = einsen[tz];
   mperm = transp[tz];
   mperm[0] = 0;

   for(b = 1; b <= block[0] && erg == 0; b++) {
      e = eintr[b];
      li = block[b];
      re = block[b+1] - 1;
      erg = maxinblock(zeile, mperm, e, li, re);
   }
   if(erg == 0)
      return 0;
   else if(erg > 0)
     return 1;   /*urspr. Nummerierung groesser*/
   sprungindex(tz, li + e);
   return -1;
}

static int maxrekneu(int tz) {
   int i, x, e;
   int erg;
   if(tz >= NVERT - 1)
      return 0;
   /*x = kmn[tz];*/
   erg = maxinzeile(tz);
   if(erg == 0)
      erg = maxrekneu(tz + 1);
   transpinv(transp[tz]);
   e = part[0][tz];
   for(i = tz + 1; i <= e && erg == 1; i++) {
      x = kmn[tz];
      kmn[tz] = kmn[i];
      kmn[i] = x;
      erg = maxinzeile(tz);
      if(erg == 0)
         erg = maxrekneu(tz + 1);
      transpinv(transp[tz]);
      x = kmn[tz];
      kmn[tz] = kmn[i];
      kmn[i] = x;
   }
   return erg;
}

static int maxstartneu(int tw) {
   int i, j, e;
   int erg;
   for(i = NVERT - 1; i > 1; i--) {   /*durchlauft die punktw. Stabilisatoren*/
      e = part[0][i];             /*Stab(n-2),...,Stab(0) = S(n)*/
      for(j = i + 1; j <= e; j++) {
         kmn[i] = j;
         kmn[j] = i;              /*Transposition (i, j)*/
         erg = maxinzeile(i);
         if(erg == 0)
            erg = maxrekneu(i + 1); /*durchlaeuft Nebenklasse (i, j)Stab{0,...,i}*/
         transpinv(transp[i]);
         kmn[i] = i;
         kmn[j] = j;
         if(erg == -1)
            return 0;
      }
   }

   for(j = 2; j <= NVERT; j++)              /*durchlauft die Nebenklassen von Stab(1)*/
      if(knoten_als_eins(j, tw)) {
         kmn[1] = j;
         kmn[j] = 1;              /*Transposition (i,j)*/
         erg = maxinzeile(1);
         if(erg == 0)
            erg = maxrekneu(2);   /*durchlaeuft Nebenklasse (i, j)Stab{0,...,i}*/
         transpinv(transp[1]);
         kmn[1] = 1;
         kmn[j] = j;
         if(erg == -1)
            return 0;
      }

   return 1;
}

static int maxinzeile1(int tz) {
   int e, li, re, erg = 0;
   int b, *zeile, *block, *eintr, *mperm;
   zeile = adj[kmn[tz]];
   block = part[tz];
   eintr = einsen[tz];
   mperm = transp[tz];
   mperm[0] = 0;
   for(b = 1; b <= block[0] && erg == 0; b++) {
      e = eintr[b];
      li = block[b];
      re = block[b+1] - 1;
      erg = maxinblock(zeile, mperm, e, li, re);
   }
   if(erg == 0)
      return 0;
   else
      if(erg > 0)
         return 1;
   return -1;
}

static int maxrekneu1(int tz) {
   int i, x, e;
   int erg;
   if(tz == NVERT - 1)
      return 0;
   /*x = kmn[tz];*/
   erg = maxinzeile1(tz);
   if(erg == 0)
      erg = maxrekneu1(tz+1);
   transpinv(transp[tz]);
   e = part[0][tz];
   for(i = tz + 1; i <= e && erg == 1; i++) {
      x = kmn[tz];
      kmn[tz] = kmn[i];
      kmn[i] = x;
      erg = maxinzeile1(tz);
      if(erg == 0)
         erg = maxrekneu1(tz+1);
      transpinv(transp[tz]);
      x = kmn[tz];
      kmn[tz] = kmn[i];
      kmn[i] = x;
   }
   return erg;
}

static int maxstartneu1(int vm) {
   int i, j, e;
   int erg;
   for(i = vm; i > 1; i--) {
      e = part[0][i];
      for(j = i + 1; j <= e; j++) {
         kmn[i] = j;
         kmn[j] = i;
         erg = maxinzeile1(i);
         if(erg == 0 && i < NVERT - 1)
            erg = maxrekneu1(i+1);
         transpinv(transp[i]);
         kmn[i] = i;
         kmn[j] = j;
         if(erg == -1)
            return 0;
      }
   }

   for(j = 2; j <= vm; j++)
      if(grad[j] == DEGREE) {
         kmn[1] = j;
         kmn[j] = 1;
         erg = maxinzeile1(1);
         if(erg == 0)
            erg = maxrekneu1(2);
         transpinv(transp[1]);
         kmn[1] = 1;
         kmn[j] = j;
         if(erg == -1)
            return 0;
      }

   return 1;
}

/*
   semiverf erstellt part[x+1],
   die Verfeinerung von part[x]
   aufgrund der Eintraege in Zeile x.
   semiverf wird erst aufgerufen,
   wenn Zeile x gefuellt ist.
   */

static void semiverf(int x) {
   int *nextpart, *block, blanz, blockgr, einsanz, i;
   block = part[x];
   blanz = block[0];
   nextpart = part[x+1];
   einsanz = einsen[x][1];
   blockgr = block[2] - block[1];

   if(blockgr == 1) {
      nextpart[0] = 0;
      part[0][x+1] = x + 1;
   }
   else if(einsanz == 1) {
      nextpart[1] = x + 2;                                   /*blockgr-1;*/
      nextpart[0] = 1;
      part[0][x+1] = x + 1;
   }
   else if(einsanz == blockgr || einsanz == 0) {
      nextpart[1] = x + 2;                                   /*blockgr-1;*/
      nextpart[0] = 1;
      part[0][x+1] = x + blockgr;
   }
   else {
      nextpart[1] = x + 2;                                   /*einsanz-1;*/
      nextpart[2] = x + einsanz + 1;                           /*blockgr-1;*/
      nextpart[0] = 2;
      part[0][x+1] = x + einsanz;
   }

   for(i = 2; i <= blanz; i++) {
      einsanz = einsen[x][i];
      blockgr = block[i+1] - block[i];
      if(einsanz == blockgr || einsanz == 0) {
         nextpart[++nextpart[0]] = block[i];              /*blockgr;*/
      }
      else {
         nextpart[++nextpart[0]] = block[i];              /*einsanz;*/
         nextpart[++nextpart[0]] = block[i] + einsanz;      /*blockgr-einsanz;*/
      }
   }
   nextpart[nextpart[0]+1] = NVERT + 1;
}

/*
   ordrek erledigt das Einsetzen
   der Kanten, ruft ggf Girth-
   und Maxtest auf, steuert die
   Ausgabe. Uebergabeparameter:
   (mx,my) zuletzt einges. Kante
   vm min. Knoten mit grad = 0
   vor Einsetzen von (mx,my)
   tw Taillenweite vor Einsetzen
   von (mx,my). tw = 0, falls noch
   kein Kreis existiert
   lblock Block von part[mx], wo
   eingesetzt wurde
   */

static void ordrek(int mx, int my, int vm, int tw, int lblock) {
   int i;

   if(my > NVERT - DEGREE && grad[mx] < DEGREE && NVERT - my < DEGREE - grad[mx])   /*noch genug Platz in Zeile mx,damit grad(mx) = k?*/
      return;                                /*nicht besonders effektiv*/

   if(mx >= NVERT - DEGREE && grad[mx] == DEGREE)
      for(i = my + 1; i <= NVERT; i++)                   /*testet,ob in Spalte i noch genug Platz*/
         if(NVERT - mx - 1 < DEGREE - grad[i])               /*damit grad(i) = k werden kann (**)*/
            return;

   if(my < vm) {   /*my<vm notw. fuer neue Kreise*/
      if(tw > 3) { /*falls Taillenweite >3, testen, ob sie nach einfÅgen von (mx,my)*/
         if(girthcheck2(mx, my, tw) < tw)     /*gleich geblieben ist,*/
            return;                          /*wenn nein abbrechen*/
      }
      else
         if(tw == 0) { /*dies ist der 1.Kreis mit girthstart seine Laenge*/
            tw = girthstart();                /*(=Taillenweite) ermittelt*/
            if(tw == 0)
               return;
         }
   }

   while(mx < NVERT && grad[mx] == DEGREE) {               /*Zeile mx voll?*/
      semiverf(mx++);                     /*verfeinerte Partition berechnen*/
      lblock = 1;                       /*Einfuegen wieder moeglichst links*/
      if(mx == splitlevel) {
         if(maxstartneu1(vm - 1) == 0)
            return;
         if(mx == splitlevel)               /*hier wird das aufteilen auf*/
            if(++calls%jobs != jobnr)       /*mehrere jobs organisiert*/
               return;
      }
   }                                    /*mx ist nun min. Knoten mit grad<k*/

   if(vm <= my)
      vm = my + 1;                        /*vm ist nun min. Knoten mit grad=0*/

   if(mx == NVERT && grad[NVERT] == DEGREE) {   /*Bedingung fuer Regularitaet*/
      if(maxstartneu(tw)) {
         anz++;
         if(storeall)
            komprtofile();
         else
            if(tostore > 0) {
               komprtofile();
               if(--tostore == 0) {
                  fclose(lstfile);
               }
            }
         return;
      }
      springen = 1;                            /*Lerneffekt aktivieren*/
      return;
   }

   for(i = lblock; i <= part[mx][0]; i++)          /*Einsetzen der neuen Kante*/
      if((my = part[mx][i] + einsen[mx][i]) < part[mx][i+1]) { /*moegliche Kante (mx, my)*/
         if(grad[my] < DEGREE && my <= vm) {         /*Bedingung my <= vm notw. fuer zusammenhaengende Graphen*/
            adj[mx][my] = adj[my][mx] = 1;
            nbr[mx][++grad[mx]] = my;
            nbr[my][++grad[my]] = mx;
            einsen[mx][i]++;
            lgrad[my]++;

            ordrek(mx, my, vm, tw, i);

            adj[mx][my] = adj[my][mx] = 0;
            nbr[mx][grad[mx]] = 0;              /*diese beiden Zeilen koennten*/
            nbr[my][grad[my]] = 0;              /*noch weggelassen werden*/
            grad[mx]--;
            grad[my]--;
            lgrad[my]--;
            einsen[mx][i]--;

            if(mx >= NVERT - DEGREE && NVERT - mx - 1 < DEGREE - grad[my])
               return;                      /*siehe (**)*/
            if(springen) {
               if(adj[i1][j1] == 0)
                  springen = 0;
               else
                  return;
            }
         }
      }
   return;
}


/*
   ordstart initialisiert die benoetigten Datenstrukturen:

   adj enthaelt die Adjazenzmatrix, also
   adj[i][j] = 1,falls i,j verbunden, =0 sonst

   nbr enthaelt die Nachbarschaftsliste, d.h.
   nbr[i][1],...,nbr[i][k] sind die Nachbarn von i

   part[i] enthaelt die Blockzerlegung aufgrund
   der Eintraege in Zeile i-1 und part[i-1].
   Zeile i muss so gefuellt werden, dass innerhalb
   der Bloecke von part[i] die Einsen links und
   die Nullen rechts stehen.
   part[i][0] : Anzahl der Bloecke;
   part[i][j] : Beginn des j-ten Blocks
Ausnahme: j = part[i][0], dann n+1;
part[0][i] : Ende des 1.Blocks von Zeile i vor
evtl. Abspalten eines 1er Blocks

einsen[i] enthaelt Anzahl der Einsen in Zeile i
bzgl. der Blockzerlegung von part[i].
einsen[i][j] : Anzahl der Einsen in Block j

lgrad[i] enthaelt Anzahl der Einsen
in Zeile i links der Diagonalen
*/

void ordstart() {
   int m, h, i, j;

   springen = girth_exact = calls = 0;

   if(DEGREE >= NVERT)
      return;
   if(DEGREE == 0 || DEGREE == 1)
      return;
   if(NVERT%2 == 1 && DEGREE%2 == 1)
      return;


   for(i = 1; i <= NVERT; i++)
      kmn[i] = i;

   part[0][1] = NVERT;
   part[1][0] = 1;
   part[1][1] = 2;
   part[1][2] = NVERT + 1;
   einsen[1][1] = DEGREE;

   for(j = 2; j <= DEGREE + 1; j++) {
      nbr[1][++grad[1]] = j;
      nbr[j][++grad[j]] = 1;
      adj[1][j] = adj[j][1] = 1;
      lgrad[j]++;
   }

   m = --j;
   i = 1;

   if(girth_exact) {
      semiverf(i);
      i = 2;
      j = 3;
      h = 1;
      nbr[i][++grad[i]] = j;
      nbr[j][++grad[j]] = i;
      adj[i][j] = adj[j][i] = 1;
      lgrad[j]++;
      while(part[i][h] != j)
         h++;
      einsen[i][h]++;
   }

   ordrek(i, j, m, 0, 1);

   return;
}

/* ------ main ------ */

void generate() {
   if(splitlevel == NOTSET && jobs > 0)
      splitlevel = SPLITLEVEL; //default_splitlevel(NVERT,DEGREE,GIRTH);

   gbytes[0] = N6CHAR;
   gbytes[gsize] = '\n';
   gbytes[gsize+1] = 0;

   ordstart();

}

void openfiles() {
   if(codestdout)
      lstfile = stdout;
   else if(storeall == 1 || tostore > 0) {
      sprintf(lst, "%02d_%d-%d.g6", NVERT, DEGREE, jobnr);
      lstfile = fopen(lst, "wb");
   }

}

void closefiles() {
   if((storeall == 1 || tostore > anz) && !codestdout) {
      fclose(lstfile);
      if(anz == 0)
         remove(lst);
   }
}

void movefiles() {
   char neu[24];

   if((storeall == 1 || tostore >= anz) && anz > 0 && !codestdout) {
      sprintf(neu, "%02d_%d#%d.g6", NVERT, DEGREE, jobnr);
      remove(neu);
      rename(lst, neu);
   }
}


int main(int argc, char **argv) {
   char neu[24];

   /********** Optionen erkennen **********/

   while(--argc > 0) {
      ++argv;
      if(!strcmp(*argv, "-split")) {
         if(argc > 1) {
            splitlevel = atoi(*++argv);
            argc--;
         }
         if(splitlevel == 0) {
            fprintf(stderr, "%s : kein erlaubter Wert fuer Option -split\n", *argv);
            return 1;
         }
      } else if(!strcmp(*argv, "-s")) {
         if(argc > 1) {
            if(!strcmp(*(argv + 1), "stdout"))
               codestdout = storeall = 1;
            else
               tostore = atoi(*(argv + 1));
         }
         if(tostore > 0 || codestdout) {
            argv++;
            argc--;
         }
         else storeall = 1;
      } else if(!strcmp(*argv, "-m")) {
         if(argc > 1) {
            jobnr = atoi(*++argv);
            argc--;
         }
         if(argc > 1) {
            jobs = atoi(*++argv);
            argc--;
         }
         if(jobs == 0 || jobnr >= jobs) {
            fprintf(stderr, "%d %d : keine erlaubten Werte fuer Option -m\n", jobnr, jobs);
            return 1;
         }
      } else {
         fprintf(stderr, "%s : keine erlaubte Option\n", *argv);
         return 1;
      }
   }

   if(jobs > 0) { /********** -m Modus **********/
      openfiles();
      generate();
      closefiles();
      movefiles();
   }
   else { /********** Normalmodus **********/
      if(codestdout)
         lstfile = stdout;
      else
         if(storeall == 1 || tostore > 0) {
            sprintf(lst, "%02d_%d-U.g6", NVERT, DEGREE);
            lstfile = fopen(lst, "wb");
         }

      generate();

      if((storeall == 1 || tostore > anz) && !codestdout) {
         fclose(lstfile);
         if(anz == 0)
            remove(lst);
      }

      if((storeall == 1 || tostore >= anz) && anz > 0 && !codestdout) {
         sprintf(neu, "%02d_%d.g6", NVERT, DEGREE);
         remove(neu);
         rename(lst, neu);
      }
   }

   return 0;
}
