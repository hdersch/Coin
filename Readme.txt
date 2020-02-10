Coin

Solving the coin weighing puzzle

This program provides two general solutions for the
coin weighing or balance puzzle: One of n > 2 coins might be
heavier or lighter than the others. Identify this coin, if it
exists, and find out whether it is heavier or lighter.
This task has to be accomplished using the fewest possible number (k) 
of weighings with a balance scale.

It is well known, that for a given k at most n = (3^k-1)/2-1 coins
can be analyzed. Finding a specific weigh strategy, however, may
be difficult and time consuming. Brute-force computations fail for 
even modest n ( >~ 20) due to  the large number of possibilities.
There have been quite a number of suggestions for algorithmic solutions
in the literature (see references below) and this program implements and extends
some of those ideas. For details of the algorithms please see the source 
code in the file "coin.c". 

Installation:
Compile with "gcc -O4 -o coin coin.c" or any other C-compiler.

Usage:
For a sequential (tree-like) solution with (e.g.) n = 15 use the command
./coin -n 15
Response:

Command line:  ./coin -n 15
Weigh strategy for 15 coins:

    ( 1  2  3  4  5 |  6  7  8  9 10) [10, 11, 10] 
        +( 1  2  3  6 |  4  5  7 11) [4, 3, 3] 
            +( 1  2 |  3  4) [2, 1, 1]    ,  7-,  3+
                +( 1 |  2) [1, 0, 1]  1+,  --,  2+
            =( 8 |  9) [1, 1, 1]  9-, 10-,  8-
            -( 4 |  5) [1, 1, 1]  4+,  6-,  5+
        =(11 12 | 13 14) [4, 3, 4] 
            +(11 | 12) [1, 2, 1] 11+,    , 12+
                =(13 | 14) [1, 0, 1] 14-,  --, 13-
            =(15 |  1) [1, 1, 1] 15+,  ==, 15-
            -(13 | 14) [1, 2, 1] 13+,    , 14+
                =(11 | 12) [1, 0, 1] 12-,  --, 11-
        -( 6  7  8  1 |  9 10  2 11) [4, 3, 3] 
            +( 6  7 |  8  1) [2, 1, 1]    ,  2-,  8+
                +( 6 |  7) [1, 0, 1]  6+,  --,  7+
            =( 3 |  4) [1, 1, 1]  4-,  5-,  3-
            -( 9 | 10) [1, 1, 1]  9+,  1-, 10+

Required 4 weighings. Time: 0 seconds.

Explanation: 
We start by balancing coins 1-5 against coins 6-10.
This splits the total number of possibilities (31) into three sets 
with sizes [10, 11, 10]. Suppose the result is "equal weight".
Then we proceed to prefix "=" and weigh coins 11-12 against 13-14.
If the left side is heavier, we then go to "+" and weigh 11 against 12.
If we achieve "+" again, we are finished, and have identified coin 11
to be too heavy. If we get "=", we need one more step: 13 against 14.
In general, we need at most 4 steps, sometimes 3 are enough.
The result "==" means that no false counterfeit coin is present,
and the results "--" are logically impossible.

This sometimes called dynamic solution has weighing steps depending
on previous results. There is also a general static solution with constant
weighing combinations. It is calculated using the command:
./coin -n 15 -s


Command line:  ./coin -n 15 -s
Static weigh strategy for 15 coins:

 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 

+
 0  0  0  0  0  0  0  0  0  0  0  0  0  1  2 
 0  0  0  0  1  1  1  1  1  2  2  2  2  2  0 
 0  1  1  2  0  1  1  2  2  0  0  1  2  2  0 
 1  0  2  2  1  0  2  1  2  0  1  0  2  0  1 
-
 0  0  0  0  0  0  0  0  0  0  0  0  0  2  1 
 0  0  0  0  2  2  2  2  2  1  1  1  1  1  0 
 0  2  2  1  0  2  2  1  1  0  0  2  1  1  0 
 2  0  1  1  2  0  1  2  1  0  2  0  1  0  2 

(14 | 15)
( 5  6  7  8  9 | 10 11 12 13 14)
( 2  3  6  7 12 |  4  8  9 13 14)
( 1  5  8 11 15 |  3  4  7  9 13)

Required 4 weighings. Time: 0 seconds.

Explanation:
We have to perform the 4 weighings and note their
results (0 - balanced, 1 -left side heavy, 2 -left side light).
Then we get a 4-number code (eg 0120) which we have to find 
in the table prefixed "+" or "-", in this case in table "-"
at position 12, meaning coin 12 is too light.
Code (0000) means that no counterfeit coin exists.

There are many similar puzzles (e.g. counterfeit coin may only be
light, but not heavy, or there certainly is one counterfeit coin etc)
which can be easily created by reducing the algorithms in this program.

hdersch




References:
1. https://en.wikipedia.org/wiki/Balance_puzzle

2. https://hectorpefo.github.io/2017-02-24-weighing-coins/

3. https://www.geeksforgeeks.org/decision-trees-fake-coin-puzzle/

4. http://paulbourke.net/fun/counterfeit.html

5. L. Halbeisen and Norbert Hungerbühler. The general counterfeit coin problem. Discrete Math-
ematics, 147(1-3) (1995), 139-150.

6. Weighing Designs to Detect a Single Counterfeit Coin
Jyotirmoy Sarkar and Bikas K Sinha,  Resonance, February 2016

7. J. Dominguez-Montes. Solution to the Counterfeit Coin Problem and its Generalization.
arXiv:1005.1391, (2010).


