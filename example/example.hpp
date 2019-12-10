// This is an example file with instructions on how to write a node.
// Read all of the comments, including this one. Also, you should read
// the kademlia source code to see how I implemented the RPC functions
// like findNodes and friends. Specifically the use of callbacks.
//
// 1) Make a directory for your node type. Mine is "kademlia"
//
// 2) Go into the makefile and add your directory to the sources and
// headers lists
//
// 3) Copy this file into the directory, fill it all in and
// stuff. This may involve also writing a cpp file to implement all
// the functions.
//
// 4) Then, try "make". Fix errors, and repeat. If you can't parse the
// error, post it on slack.
//
// You can also play around with main.cpp to test your node.


#ifndef DHTSIM_EXAMPLE_H
#define DHTSIM_EXAMPLE_H

#include "application.hpp"
#include "dhtnode.hpp"
#include "base.hpp"
#include "message.hpp"
#include "callback.hpp"

// Kademlia #includes more things, can check there to see if there's
// something you need

namespace dhtsim {
// You should change this to whatever you need to use.
using ExampleKeyType = uint32_t;

// This class will use multiple inheritance:
class ExampleNode :
	// It's an application that runs on a network with 32 bit addresses.
	public BaseApplication<uint32_t>,
	// It's a distributed hash table node.
	public DHTNode<ExampleKeyType, std::vector<unsigned char>>
	//        ----------------------------^      ---------^
	// The first parameter is the key type, the second is the
	// value type.  You can make the key parameter anything you
	// want, but you should use std::vector<unsigned char> for the
	// value type.
{
public:
	// Some type aliases for convenience. You may need/want to add more here.
	using GetCallbackSet =
		DHTNode<ExampleKeyType, std::vector<unsigned  char>>::GetCallbackSet;

	// About CallbackSets: A callback set is essentially a
	// collection of functions for the success case and a
	// collection of functions for the failure case. See
	// callback.hpp for constructor signatures. Fundamentally, the
	// send function in base.hpp takes a CallbackSet so you have
	// to chain everything off that. (I really hope that makes
	// sense)

	// Now, we need to implement the virtual methods from both
	// BaseApplication and DHTNode.

	// BaseApplication:
	virtual void handleMessage(const Message<uint32_t>& m);

	// DHTNode
	virtual ExampleKeyType put(const std::vector<unsigned char>& v);
	virtual void get(const ExampleKeyType& k, GetCallbackSet callback);
	virtual ExampleKeyType getKey(const std::vector<unsigned char>& value);

	// That's all you need to implement. However, you may want to
	// split up the functionality into different
	// functions. Declare them here.
private:

};


// Make a cpp file to go alongside this one and put all your definitions in it.

}

#endif
