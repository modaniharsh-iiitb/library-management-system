# Library Management System in C

This library management system (LMS) allows users to log in both as a member and an admin (librarian), and perform the following functions:

### Member:

- View books
- Search for a book
- Borrow a copy
- Return a copy
- See list of borrowed copies
- Change password
- Log out

### Admin:

- See list of books
- Add a book
- Delete a book
- See list of members
- Add a member
- Delete a member
- See list of copies
- Add a copy
- Remove a copy of a book
- Change password
- Log out

## Salient features:

This LMS provides the following features to ensure smooth, bug-free operation of the system:

1. **Concurrent access**: Multiple client systems can work on this LMS together. This is enabled via multithreading in the server, which ensures that the server can deal with various clients at once.
2. **Concurrency control and data consistency**: The usage of the server by multiple client systems does not cause transactional faults. This is ensured by using read and write locks on files that are being read from/written into.
3. **Security control via password authentication**: The server has a password system that blocks users from abusing admin and member privileges. It also ensures that administrative permissions are not accidentally granted to regular users of the system.

## Running the program:

### Server end:

The required files on the server end are: 

- `init.c`
- `server.c`
- `Makefile`
- `library.h`

1. If the `.dat` files are not present in the same directory as the server, make sure to run

```sh
$ make run-init
```

2. To run the server application, run

```sh
$ make server
```

### Client end:

The required files on the client end are:

- `init.c`
- `client.c`
- `Makefile`
- `library.h`

To run the client application, run

```sh
$ make client
```