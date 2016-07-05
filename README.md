(non) Randomness

I began by looking at the given random number generator and trying to find a
weakness. Right away I noticed that I could exploit their using the current unix
time in their seed generation because I can get the timestamp from a file in C.
Also, I noticed that because they XOR the lower 16 bits with the unix
nanoseconds, the upper 16 bits are going to remain the same because they would
be XORed with 16 upper bits with a value of zero. I also looked up the max
length of a PID and found that it was 16, so the upper 16 bits will remain the
same even after XORing with the previous two numbers. I figured that I would
start by brute-forcing the bottom 16 bits because that would give me all
possible combinations of what the seconds, nanoseconds, and PID would have been.

First off, I wrote a program in which I encrypted my own text with a special
seed and then tried to decrypt it by mirroring the encrypt function and
changing a few function calls/parameters. Once I got that working, I got to
work on making my program work for the actual test cases.

I wrote my program and compiled it with the following command:

    $ gcc -o findkey findkey.c -lssl -lcrypto

(This command is now in my Makefile). And ran my program with this command:

    $ nice ./findkey /playpen/crypto/des1.enc results.out

I got some seg faults and tried manipulating the bits by masking and ANDing or
ORing. I would mask out the bottom 16 bits like the example below:

Timestamp: 10100001010100101010010100110011
Min seed:  10100001010100100000000000000000

Then iterate through candidate seeds from the min to a max seed (min
seed + 2^16):

Max seed:  10100001010100001111111111111111

I kept trying over and over again unsuccessfully until I realized that
maybe I should try the other test files.

When I ran the command:

    $ nice ./findkey /playpen/crypto/des3.enc results.out

I found the seed and successfully decrypted the file. Seed: 493200.
Interestingly, the timestamp for the file was from the 1970's, so it became
clear to me that the timestamp and the actual seed may not be as close as I
had previously thought. A few seconds (or more) could have passed between when
the unix time was called in the generator program and created. Moreover, the
timestamp can also be changed, as one can see from the 70's date. Also, after
unsuccessful attempts at all the other files, I realized that my algorithm
didn't cover enough of a time span and that I needed to brute force more bits.  

So what to do? I obviously didn't want to brute force all 4,294,967,295 options.

I went and looked at file’s timestamps started trying a few new options and
playing around with the bits and checking timestamps. No luck.

I finally decided to mask and brute force more bits, so I tried 20 instead of
16. I was able to crack des4.enc. I increased it by another byte and was able
to crack des1.enc. However, every time I increased the brute force range, each
file that I had cracked in a shorter amount of time before took longer, because
I was brute forcing some bits that in the past I didn't need to. I also
finally cracked des2.enc, but only after increasing the brute force bit space
to cover over 26 bits. This took awhile.

So I realized that I needed to find a way to optimize my program. I spent some
time making my program multithreaded hoping that it would improve the runtime,
but it actually made it worse and less predictable. I finally realized that
instead of hardcoding the brute force range, I could start at a reasonable
minimum (16 bits) and slowly expand the search space by one bit until I found
the seed. This was perfect because it allowed the seeds that were easier to
find to still be generated earlier on and my algorithm never checked any seed
value more than once. After perfecting this algorithm, my test cases passed
as follows:

    $ nice ./findkey /playpen/crypto/des3.enc results.out

This took less than a second to find the seed.

    $ nice ./findkey /playpen/crypto/des4.enc results.out

This took less about 2 seconds to find the seed.     

    $ nice ./findkey /playpen/crypto/des1.enc results.out

This took about 45 seconds to find the seed.

    $ nice ./findkey /playpen/crypto/des2.enc results.out

This took about 4 minutes to find the seed.

As you can see, the overall runtime greatly improved and 3/4 of the solutions
were found in under a minute. Yay!
