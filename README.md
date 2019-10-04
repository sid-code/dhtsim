# DHT simulator

This is an abstract network simulator created for the purpuse of
simulating distributed hash tables. It is written in C++17 and has no
dependencies other than the standard library. Either `clang` or `g++` can
be used to compile it.

## The network

There are some components to the simulation. Most importantly, there
is the "network." In a very abstact sense, it holds all of the
participants and transfers messages between them. The network
implementation in `network.hpp` describes one of the simplest possible
networks: a fully connected network. Every node can send a message to
every other node in the same amount of time.

The network assigns an address to everybody that joins. In the
`CentralizedNetwork` class, that is the template parameter. I usually
use `uint32_t` as the address, but really anything could be used, as
most of the classes have the address as a template parameter.

So why is the address type a template parameter? (insert mumblings of
eventually using ns-3) I don't know and it was probably a mistake.

## The application

Each node is represented by the application that runs on it. See
`application.hpp` for the most abstract definition of an application.

An application is something that can "send" and "receive" messages on
the network. At the core, the network can command the application to
produce a message to be sent (`Application::unqueueOut`) or feed a
message into it (`Application::recv`). Every epoch, or "tick",
`Application::tick` is called on every application in the
network. Each application then has a chance to do whatever it
wants. One can hope that it stored the messages it received in a queue
or something like that so that it can access them now.

## The message

Just look at `message.hpp`. That's a message. (The template parameter
is the address type again. These things sneak into every piece of
code.)

The 'tag' field is to check for responses. If you're responding to a
message you got, you use the same tag. That way, the application that
sent it knows that it was indeed a response.

The 'hops' field is to count the number of hops the message took. This
may be useful later, but I don't use it now.

## `base.hpp`

This file contains a subclass of `Application` that provides many
functionalities such as queues for messages and callbacks for
responses.

## `pingonly.hpp`

Finally. This file contains a subclass of `BaseApplication` that
demonstrates how to actually write a real application. This
application just responds to pings. See the enum at the top of the
file that defines the messgae types? This is just giving names to some
integers.

All this class needs to do is implement `handleMessage`. The `ping`
method is for convenience. See how it's used by `main.cpp`.

### Sidenote: why is everything in these header files?

Because that's just how C++ template classes work. Just having the
*templated method signatures* in the header without the method
definitions isn't enough, you need all the method definitions so that
the .cpp file that includes the template knows how to instantiate the
methods with a template parameter. In the absence of templates, the
linker knows how to fetch methods from other .cpp files that were
compiled, but since the compiler is done running, all it has to work
here are templates that were never instantiated in the first place,
ergo nothing.

A template is not a class; and template class instance methods are not
methods. They are all *templates*, and have no meaning until their
template parameters are passed some value. These instantiations occur
in various places. In `main.cpp` there are a few that look like
`SomethingSomethingApplication<uint32_t>`.

See the bottom of network.cpp, where the `CentralizedNetwork` template
is instantiated. That puts the `CentralizedNetwork<uint32_t>` methods
into the `network.o` object file where they can be seen by other
compilation units. This is another way to get around the header-only
problem. As you can see, both approaches have its drawbacks. I will
never be able to instantiate `CentralizedNetwork` outside of the
`network.cpp` file, but on the other hand I don't have to include
a fat header. I have yet to decide which option is better.

# What does main.cpp do?

It runs the main loop. Build everything with `make` and then run the
binary it made.
