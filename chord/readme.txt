Download the files, run "make" then you should be able to run "./chord" to test various functions.

Various functions:

"createnode x" creates a new node with ID "x" (should be a number) also has key "x" when first created.
"find x y" finds the key "y" starting from node "x" (x and y should be numbers).
"show x" shows the keys node "x" has.
"show all" shows the finger tables and keys of all nodes.
"crash x" simulates a node "x" crashing or leaving.
"exit" exactly what you expect.

"detect" this is a bit of a special one. I initially started out thinking "I really don't want to deal with error cases right now....so I won't!" And I made the error handling (when referring specifically to nodes leaving I mean) into a toggle function. Basically I would recommend running "detect" as soon as you start the ./chord file, and then forget about it.

a few other commands are available I used when initially building the setup I don't really use them much anymore bu if you want to look into the code for them/what they do feel free.
The ones given above are the primary ones.

There are still some things I ould like to change such as making the finger tables dynamically grow/reduce in size depending on the total number of nodes among others but for now this
is what I have gotten done.