# Coin
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

