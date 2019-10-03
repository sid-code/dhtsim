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
network: a fully connected network. Every node can send a message to
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
or something like that.

## The message

Just look at `message.hpp`. That's a message. (The template parameter
is the address type again. These things sneak into every piece of
code.)

The 'tag' field is to check for responses. If you're responding to a
message you got, you use the same tag. That way, the application that
sent it knows that it was indeed a response.

## `base.hpp`

This file contains a subclass of `Application` that provides many
functionalities such as queues for messages and callbacks for
responses.


# What does main.cpp do?

It runs the main loop. Build everything with `make` and then run the
binary it made.
