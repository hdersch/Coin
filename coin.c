/* coin	- solve coin weighing puzzle
   Provide the shortest set of weighings to identify one fake coin in 
   arbitrary many. Both static and dynamic solutions are given.


   Copyright (C) 2020 - Helmut Dersch  der@hs-furtwangen.de
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/*------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>


// Compile with: gcc -O4 -o coin coin.c

static int verbose = 1;

static inline void
logd(   const char *fmt,
        ...     )
{
        va_list ap;
        if(verbose) {
                va_start(ap, fmt);
                vprintf(fmt, ap);
        }
}
                


enum{ C_EQUAL = 0, C_MORE, C_LESS, C_DOUBLE };

static inline int
balance(        int x,
                int y   )
{
        if(x < y)
                return C_LESS;
        else if(x == y)
                return C_EQUAL;
        else
                return C_MORE;
}


/* Two types of solution are provided:
 * 1. Dynamic or sequential, i.e. a tree like decision chart.
 * 2. Static using fixed weighings and identifying the fake coin using
 *    a base-3 code ("heavy-code", "light-code").
 * 
 * Dynamic weigh strategy.
 * The structure coin_cfg describes a set of coin-solutions:
 * all_equal = 1 means the coins might all be equal (non-false)
 * C_EQUAL - coins which have correct weight
 * C_LESS - coins which might be too light, but not too heavy
 * C_MORE - coins which might be too heavy, but not too light
 * C_DOUBLE - coins which might be too heavy or too light
 * 
 * There are basically two types of sets:
 * Type A:
 * There are some sets of type C_DOUBLE, and 0 or more of type equal, 
 * and all coins might be equal (all_equal = 1).
 * Type B:
 * There are some sets of type C_MORE, some of type C_LESS, and some of type C_EQUAL,
 * but none of type C_DOUBLE. all_equal = 0, i.e. not all coins are equal.
 * 
 * The initial set is of type A (all coins are of type C_DOUBLE, and they may be equal).
 * Applying a weighing operation to a type A set splits it into two B sets 
 * (weighing result C_MORE and C_LESS)
 * and one A-set (weighing result C_EQUAL). 
 * Applying a weighing operation to a type B set splits it into three type B-sets.
 * Any weighing strategy is therefor a chain of splittings of A and B-sets.
 * 
 * The functions get_sel_A and get_sel_B determine the optimum coin selections for
 * the different types, meaning that the size of the three parts after splitting
 * differ by no more than 1 (except the first weighing, where such a solution
 * not always exists, see code).
 */

typedef struct{
        int all_equal;
        int size[4];
        int *sel[4];
} coin_cfg;


/* n-coins are coded as ints denoting the position of the false coin (1,....,n)
 * n > 0 heavy false coin, n < 0 light false coin, n = 0 no false coin
 * A set of coin solutions is terminated by INT_MAX
 * Initial set of possible solutions:
 * nc = {0, 1, 2, ...., n, -1, -2, ....., -n, INT_MAX}
 */
static int*
new_coins(      int n_coins   )                                 // total number of coins
{
        int k;
        int *poss = malloc((2 * n_coins + 2) * sizeof(int));    // all possibilities
        
        poss[0] = 0;                                            // no false coin
        for(k = 1; k < n_coins + 1; k++) {
                poss[k] = k;                                    // coin k is false and heavy
                poss[k + n_coins] = -k;                         // coin k is false and light
        }
        poss[2 * n_coins + 1] = INT_MAX;                        // terminating token
        return poss;
}

/* Size of this possibility set.
 */
static inline int
npos(           const int *poss  )
{
        int np = 0;
        while(*poss++ != INT_MAX)
                np++;
        return np;
}


/* print possibility set if its size is 1 or 0
 */
static void
print_coin(     int *poss   )
{
        switch(npos(poss)){
        case 0:
                logd(" --");                            // impossible
                break;
        case 1:
                if(*poss == 0)
                        logd(" ==");                    // no false coin
                else if(*poss > 0)
                        logd("%2d+", *poss);            // coin with number *poss is false and heavy
                else
                        logd("%2d-", -*poss);           // coin with number -*poss is false and light
                break;
        default:
                logd("   ");
                break;
        }
                
}



static void 
print_coins(    int **c  )
{
        if(npos(c[C_MORE]) > 1 && npos(c[C_EQUAL]) > 1 && npos(c[C_LESS]) > 1)
                return;
        print_coin(c[C_MORE]);
        logd(", ");
        print_coin(c[C_EQUAL]);
        logd(", ");
        print_coin(c[C_LESS]);
}
                        

/* Add one integer x to array s, reallocate to new size n.
 */
static inline int*
add_int(        int x,
                int *s,
                int n   )
{
        s = realloc(s, n * sizeof(int));
        s[n - 1] = x;
        return s;
}

/* Number of possible solutions for this coin configuration.
 */
static int
num_pos(        coin_cfg *cfg   )
{
        return cfg->size[C_LESS] + cfg->size[C_MORE] + cfg->size[C_DOUBLE] * 2 + cfg->all_equal;
} 


static coin_cfg*
get_cfg(        int *c,                 // possibility set
                int n   )               // total number of coins
{
        int ci, *cl, k;
        coin_cfg *cfg = malloc(sizeof(coin_cfg));
        bzero(cfg, sizeof(coin_cfg));
        
        for(k = 0; k < n; k++) {
                int hit = 0, type = C_EQUAL;
                cl = c;
                while((ci = *cl++) != INT_MAX) {
                        if(ci > 0 && ((ci - 1) == k)) {
                                hit++;
                                type = C_MORE;
                        } else if(ci < 0 && ((-ci - 1) == k)) {
                                hit++;
                                type = C_LESS;
                        } else if(ci == 0)
                                cfg->all_equal = 1;
                }
                if(hit == 2)
                        type = C_DOUBLE;
                cfg->sel[type] = add_int(k, cfg->sel[type], ++cfg->size[type]);
        }
        return cfg;
}

static void
free_cfg(       coin_cfg *c     )
{
        int k;
        for(k = 0; k < 4; k++)
                if(c->sel[k])
                        free(c->sel[k]);
        free(c);
}

static void
print_vector(   int *c,
                int n   )
{
        while(n-- > 1)
                logd("%2d ", *c++ + 1);
        if(n >= 0)
                logd("%2d", *c + 1);
}

static void
print_vectors(  int *s1,
                int *s2,
                int k   )
{
        logd("(");
        print_vector(s1, k);
        logd(" | ");
        print_vector(s2, k);
        logd(")");
}



static void
print_cfg(      coin_cfg *c     )
{
        printf("==: %d\n", c->all_equal);
        printf("N= :" );
        print_vector(c->sel[C_EQUAL], c->size[C_EQUAL]);
        printf("\nN+ :" );
        print_vector(c->sel[C_MORE], c->size[C_MORE]);
        printf("\nN- :" );
        print_vector(c->sel[C_LESS], c->size[C_LESS]);
        printf("\nN+-:" );
        print_vector(c->sel[C_DOUBLE], c->size[C_DOUBLE]);
        printf("\n");
}


/*      Utility function to create the numbers n1, n2, k, and l
 *      required for case B
 *      Left arm of scale:      n1 coins from N+, n2 coins from N-
 *      Right arm of scale:     (N+ - n1) coins from N+, k coins from N-, l coins from N=
 *      n1, n2, and k must be >= 0, l maybe < 0 meaning (-l) coins from N= on the left arm.
 *      Result is returned in array r = {n1, n2, k, l}
 */
static void
get_nkl(        coin_cfg *cfg,
                int *r          )
{
        int n_more = cfg->size[C_MORE], n_less = cfg->size[C_LESS], 
                np = n_more + n_less, // number of possible solutions
                n1 = 0, n2 = 0, l = 0, k;
        
        switch(np % 3){
        case 0:
                if(n_more % 2) {
                        l = 2;
                        n1 = (n_more + 1) / 2;
                        n2 = (n_less - n1 + 2) / 3;
                } else {
                        n1 = n_more / 2;
                        n2 = (n_less - n1) / 3;
                }
                break;
        case 1:
                if(n_more % 2) {
                        l = 1;
                        n1 = (n_more + 1) / 2;
                        n2 = (n_less - n1 + 1) / 3;
                } else {
                        n1 = n_more / 2;
                        n2 = (n_less - n1 - 1) / 3;
                }
                break;
        case 2:
                if(n_more % 2) {
                        l = -1;
                        n1 = (n_more - 1) / 2;
                        n2 = (n_less - n1 - 1) / 3;
                } else {
                        n1 = n_more / 2;
                        n2 = (n_less - n1 + 1) / 3;
                }
                break;
        }
        k = 2 * n1 + n2 - n_more - l;
        
        r[0] = n1;
        r[1] = n2;
        r[2] = k;
        r[3] = l;
}


/* Exchange the values of N+ and N- in this coin configuration
 */
static void
switch_more_less(       coin_cfg *c     )
{
        int k = c->size[C_LESS];
        c->size[C_LESS] = c->size[C_MORE];
        c->size[C_MORE] = k;
        
        int *s = c->sel[C_LESS];
        c->sel[C_LESS] = c->sel[C_MORE];
        c->sel[C_MORE] = s;
}



/*      Selections for case A
 *      Left arm of scale:      n coins from N+-
 *      Right arm of scale:     (n - l) coins from N+-, l coins from N=, if available,
 *                              else n coins from N+-
 *      returns number of coins for each side of scale
 */
static int
get_sel_A(      coin_cfg *cfg,
                int **s1,                               // coins for left arm of scale
                int **s2        )                       // coins for right arm of scale
{
        int m = cfg->size[C_DOUBLE], l = 0, n = 0, j;
        
        switch(m % 3){
        case 0:
                n = m / 3;
                break;
        case 1: 
                if(cfg->size[C_EQUAL]) {
                        n = (m + 2) / 3;
                        l = 1;
                } else {
                        n = (m - 1) / 3; // oder (m + 2) / 3;
                }
                break;
        case 2:
                n = (m + 1) / 3;
                break;
        }

        *s1 = malloc(n * sizeof(int));
        *s2 = malloc(n * sizeof(int));
        
        for(j = 0; j < n; j++)
                (*s1)[j] = cfg->sel[C_DOUBLE][j];
        if(l == 0) {
                for(j = 0; j < n; j++)
                        (*s2)[j] = cfg->sel[C_DOUBLE][j + n];
        } else {
                for(j = 0; j < n - 1; j++)
                        (*s2)[j] = cfg->sel[C_DOUBLE][j + n];
                (*s2)[n - 1] = cfg->sel[C_EQUAL][0];
        }
        return n;
}
        
/*      Selections for case B
 *      see utility function get_nkl for details.
 *      returns number of coins for each side of scale
 */
static int
get_sel_B(      coin_cfg *cfg,          
                int **s1,                       // coins for left arm of scale
                int **s2        )               // coins for right arm of scale
{
        int r[4];
        get_nkl(cfg, r);
        if(r[0] < 0 || r[1] < 0 || r[2] < 0) {  // impossible, try complement
                switch_more_less(cfg);
                get_nkl(cfg, r);
                if(r[0] < 0 || r[1] < 0 || r[2] < 0)
                        return 0;
        }
        
        int n_more = cfg->size[C_MORE], 
                n1 = r[0], n2 = r[1], k = r[2], l = r[3], ns, j, i;
        
        ns = n1 + n2 + (l < 0 ? -l : 0);
        *s1 = malloc(ns * sizeof(int));
        *s2 = malloc(ns * sizeof(int));
        
        for(j = 0; j < n1; j++)
                (*s1)[j] = cfg->sel[C_MORE][j];
        for(; j < n1 + n2; j++)
                (*s1)[j] = cfg->sel[C_LESS][j - n1];
        if(l < 0)
                (*s1)[j] = cfg->sel[C_EQUAL][0];
        
        for(j = 0; j < n_more - n1; j++)
                (*s2)[j] = cfg->sel[C_MORE][j + n1];
        for(; j < n_more - n1 + k; j++)
                (*s2)[j] = cfg->sel[C_LESS][j - (n_more - n1) + n2];
        for(i = 0; i < l; i++)
                (*s2)[j++] = cfg->sel[C_EQUAL][i];

        return ns;
}


/*      Determine configuration type (A or B), select and apply suitable strategy.
 */
static int
getselection(   coin_cfg *cfg,
                int **s1,
                int **s2        )
{
        if(cfg->size[C_MORE] == 0 && cfg->size[C_LESS] == 0 && cfg->all_equal)
                return get_sel_A(cfg, s1, s2);
        
        if(cfg->size[C_DOUBLE] == 0 && !cfg->all_equal)
                return get_sel_B(cfg, s1, s2);
        
        printf("Cannot handle this configuration: \n");
        print_cfg(cfg);
        printf("\n");
        exit(1);
}
        


                        
/*  Add weighs of len coins from set c
 *  return 0 if all coins have correct weight,
 *  1 if heavier, -1 if lighter
 */
static inline int
sum(    const int c,                // possible coin set
        const int *s,               // selection of coins (0,..., total_num_coins - 1)
        const int len         )     // size of selection
{
        int k;
        for(k = 0; k < len; k++) {
                if(c > 0 && (c - 1) == s[k])
                        return 1;
                else if(c < 0 && (-c - 1) == s[k])
                        return -1;
        }
        return 0;
}

/*      Perform a weighing of coins.
*/
void
weigh(  int *c,                 // base set of possibilities
        int *s1,                // coins on left side of scale (0,....., total_num_coins - 1)
        int *s2,                // coins on right side of scale (0,....., total_num_coins - 1)
        int len,                // number of coins on each side
        int **r         )       // results
{
        int ci, n[3], k;
        r[C_EQUAL] = r[C_MORE] = r[C_LESS] = NULL;
        n[0] = n[1] = n[2] = 0;
        while((ci = *c++) != INT_MAX) {
                int bal = balance(sum(ci, s1, len), sum(ci, s2, len));
                r[bal] = add_int(ci, r[bal], ++n[bal]);
        }
        for(k = 0; k < 3; k++)
                r[k] = add_int(INT_MAX, r[k], ++n[k]);
}


static int ident = 0;
static char *prefix = "";

static void
print_ident()
{
        int k;
        for(k = 0; k < ident; k++)
                logd("    ");
        logd(prefix);
}

static inline int
max3(   int a,
        int b,
        int c   )
{
        int mab = a > b ? a : b;
        return mab > c ? mab : c;
}


static int
weigh_sequential(       int *c,                 // possibilities
                        int nc  );              // number of coins


/* Apply weighing to possibility set, and split it into three sets
 * depending on weighing results.
 * These are then weighed and splitted recursively
 */
static int
split_selection(        int *c,                 // possibilities
                        int nc,                 // number of coins
                        int *s1,                // coins on left arm of balance
                        int *s2,                // coins on right side of balance
                        int k           )       // size of selection
{
        int r1, r2, r3, *cr[3];
        
        /* create possibility sets for the three weighing results */
        weigh(c, s1, s2, k, cr);
        free(c);
                        
        /* print results */
        ident++;
        print_ident();
        print_vectors(s1, s2, k);
        logd(" [%d, %d, %d] ", npos(cr[C_MORE]), npos(cr[C_EQUAL]), npos(cr[C_LESS]));
        print_coins(cr);
        logd("\n");
        
        /* recursively call weigh_sequential on the three results */
        prefix = "+";
        r1 = weigh_sequential(cr[C_MORE], nc);
        prefix = "=";
        r2 = weigh_sequential(cr[C_EQUAL], nc);
        prefix = "-";
        r3 = weigh_sequential(cr[C_LESS], nc);
        
        ident--;
        /* return number of weighing steps required */
        return 1 + max3(r1, r2, r3);
}                        



static int
weigh_sequential(       int *c,                 // coins
                        int nc  )               // number of coins
{
        
        coin_cfg *cfg = get_cfg(c, nc);
        if(num_pos(cfg) <= 1) {                 // we're finished
                free(c);
                free_cfg(cfg);
                return 0;
        }
        
        /* determine optimum selection of coins for left (sl1) and right (sl2) side of balance */
        int *sl1, *sl2, n_sel = getselection(cfg, &sl1, &sl2);
        if(n_sel > 0) {
                int r = split_selection(c, nc, sl1, sl2, n_sel);
                free(sl1);
                free(sl2);
                free_cfg(cfg);
                return r;
        }
        printf("Error\n");        
        exit(0);
}

/* Static weigh strategy 
* See http://paulbourke.net/fun/counterfeit.html for the terms used
* The saturated case (number of coins) = (3^(number of weighings) - 1) / 2 - 1
* is determined algorithmically, the other cases are solved by extending
* the closest saturated case.
*/

static inline int
ipow(   int x,
        int n   )
{
        int r = 1;
        while(n--)
                r *= x;
        return r;
}

/* Base 3 complement of x :  turn 1s into 2s and vice versa
 */
static int
op(     int x   )
{
        int c = 1, s = 0;
        for(; x; x /= 3, c *= 3) {
                switch(x % 3){
                case 1:
                        s += 2 * c;
                        break;
                case 2:
                        s += c;
                        break;
                }
        }
        return s;
}

/* Base 3 digit of x at position n (n = 0: rightmost)
 */
static inline int
digit(  int x,
        int n   )        
{
        while(n-- > 0)
                x /= 3;
        return x % 3;
}

/* Given heavy-codes hcode (size nc), check if
 * t or op(t) is contained, fail if yes.
 */
static int
isfree(         int t, 
                int *hcode, 
                int nc  )
{
        int j;
        for(j = 0; j < nc; j++)
                if(t == hcode[j] || t == op(hcode[j]))
                        return 0;
        return 1;
}


/* Utility function for getbase
 * Given heavy codes b (size n) find a number that isfree
 */
static int
missing(        int *b,
                int n,
                int N   )
{
        int *t = calloc(N + 1, sizeof(int)), k;
        for(k = 0; k < n; k++) {
                t[b[k]] = 1;
                t[op(b[k])] = 1;
        }
        for(k = 1; k <= N; k++)
                if(!t[k]){
                        free(t);
                        return k;
                }
        printf("Error.\n");
        exit(0);
}
                        

/* Compare function for sorting heavy-codes
 */
static int 
compare(        const void *a, 
                const void *b   )
{
        int a1 = *(int*)a, b1 = *(int*)b;
        return (a1 < b1 ? -1 : (a1 == b1 ? 0 : 1));
}


        
/* Algorithmic solution for the saturated case n = (3^k-1)/2-1
 * Works by extending the previous (k - 1) solution b as follows:
 * 1. Extend each code in b with base-3 digit 0 
 * 2. Extend each code in b with base-3 digit 1
 * 3. Extend each code in b with base-3 digit 2
 * 4. Add code (2, 0, 0,..., 0)
 * 5. Find missing number m in b, add code (0, m) and (1, op(m))
 * Total number = 3*size(b)+3
 */
static void
getbase(        int k,          // number of weighings
                int *b  )       // array for heavy-codes (must be allocated)
{
        if(k == 2) {
                b[0] = 1;
                b[1] = 8;
                b[2] = 3;
                return;
        }
        getbase(k - 1, b);
        int c = ipow(3, k - 1),
                n =  c / 2 - 1, j, 
                m = missing(b, n, c - 1);
        for(j = 0; j < n; j++) {
                b[n + j] = b[j] + c;
                b[2 * n + j] = b[j] + 2 * c;
        }
        b[2 * n + j] = 2 * c;
        b[2 * n + j + 1] = m;
        b[2 * n + j + 2] = c + op(m);
        qsort(b, ipow(3, k) / 2 - 1, sizeof(int), compare);
}

static void
print_static(   int k,
                int nc,
                int *hcode      )
{
        int j, i, *s1 = NULL, *s2 = NULL, n1, n2;
        for(j = 0; j < nc; j++)         // print coins
                logd("%2d ", j + 1);
        logd("\n\n+\n");
        for(i = k - 1; i >= 0; i--){    // print hcodes
                for(j = 0; j < nc; j++)
                        logd("%2d ", digit(hcode[j], i));
                logd("\n");
        }
        logd("-\n");
        for(i = k - 1; i >= 0; i--){    // print lcodes
                for(j = 0; j < nc; j++)
                        logd("%2d ", digit(op(hcode[j]), i));
                logd("\n");
        }
        logd("\n");
        for(i = k - 1; i >= 0; i--){    // print equations/ weighings
                n1 = n2 = 0;
                for(j = 0; j < nc; j++) {
                        switch(digit(hcode[j], i)){
                        case 1:
                                s1 = add_int(j, s1, ++n1);
                                break;
                        case 2:
                                s2 = add_int(j, s2, ++n2);
                                break;
                        }
                }
                if(n1 == 0 || n1 != n2) {
                        printf("Error print_static.\n");
                        exit(1);
                }
                print_vectors(s1, s2, n1);
                logd("\n");
        }
        free(s1);
        free(s2);
}

/* Given a number m, check, if hc has zero-(base-3)-digits at positions
 * where m has non-zero digits. If yes, change the zero digits to
 * the complement of the non-zero-digits of m and return this new
 * number. If no, fail, i.e. return 0.
 * Example: m = 5 = base3(0 1 2)
 *         hc = 9 = base3(1 0 0)
 *      return 16 = base3(1 2 1)
 */
static int
mcomplement(    int m,                  // copy hc-digits != 0, set others 
                int hc,                 // to complement of m, fail if both digits != 0
                int k           )       // number of digits to check
{
        int j, c = 1, s = 0;
        for(j = 0; j < k; j++, m /= 3, hc /= 3, c *= 3) {
                int r = m % 3, rh = hc % 3;
                if(r == 0){
                        s += rh * c;
                        continue;
                } else if(rh != 0) // failure
                        return 0;
                s += (r == 1 ? 2 : 1) * c;
        }
        return s;
}

/* Find a suitable candidate and add it to the list of heavy codes hc
 */
static void
add(            int *hc,                // heavy codes, must be allocated
                int k,                  // number of weighings
                int nc          )       // old (current) size of hc
{
        int n = ipow(3, k) - 1;         // possible values 1, ... n
        int j, m;
        
        for(m = 1; m <= n; m++) {       // all possible values
                if( !isfree(m, hc, nc)) // value is in use
                        continue;
                for(j = 0; j < nc; j++) {
                        int t = mcomplement(m, hc[j], k); // try to modify another member 
                        if(t && isfree(t, hc, nc)) {      // success
                                hc[j] = t;                // modify the found other member
                                hc[nc] = m;               // add m
                                qsort(hc, nc + 1, sizeof(int), compare);
                                return;
                        }
                }
        }
        logd("add failure\n");
        exit(0);
}


/* The static weighstrategy for nc coins first finds the closest
 * base solution with (number of coins) <= n_coins, then adds one coin
 * after another to this solution until n_coins is reached.
 */
static int
weigh_static(   int n_coins  )
{
        int k, n = 0, n_base;
        for(k = 2; ; k++){                              // find closest base solution
                n_base = (ipow(3, k) - 1) / 2 - 1;
                if(n_base <= n_coins)
                        n = n_base;
                if(n_base >= n_coins)
                        break;
        }
        
        int *hcode = calloc(n_coins, sizeof(int));      // heavy-codes
        
        getbase(n == n_coins ? k : k - 1, hcode);

        while(n < n_coins) {
                add(hcode, k, n++);
        }
        
        print_static(k, n_coins, hcode);
        return k;
}


static void
usage()
{
        fprintf(stderr, "Usage:\n coin [-s (static)] [-n number_of_coins]\
                [-q (quiet)]\n");
        exit(0);
}


int
main(   int argc,
        char **argv     )
{
        int k, opt, stat = 0, n_steps;
        int n_coins = 12;
        time_t tc;
        
        logd("\n\nCommand line: ");
        for(k = 0; k < argc; k++)
                logd(" %s", argv[k]);
        logd("\n");
        
        while ((opt = getopt(argc, argv, "sqn:h?")) != -1) {
                switch(opt) {
                case 'n': 
                        n_coins = atoi(optarg);
                        break;
                case 'q':
                        verbose = 0;
                        break;
                case 's':
                        stat = 1;
                        break;
                case 'h':
                case '?':
                default:
                        usage();
                        break;
                }
        }
                        
        if(n_coins < 3) {
                printf("There must be more than 2 coins.\n");
                exit(0);
        }

        tc = time(NULL);

        if(stat) {
                logd("Static weigh strategy for %d coins:\n\n", n_coins);  
                n_steps = weigh_static(n_coins);
        } else {        
                logd("Weigh strategy for %d coins:\n\n", n_coins);
                n_steps = weigh_sequential(new_coins(n_coins), n_coins);
        }
        printf("\nRequired %d weighings. Time: %d seconds.\n", n_steps, (int)(time(NULL) - tc));
        
}
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
