#include "library.h"

// Function to cloe connection with client
void close_connection(int client_sock) {
    printf("Closing connection with client socket %d\n", client_sock);
    close(client_sock);
    pthread_exit(NULL);
}

// Function to check if the client has disconnected or has sent invalid data
void check_client_ping(int r, int client_sock) {
    if (r < 0) {
        perror("Failed to receive data from client");
        close_connection(client_sock);
    }
    if (r == 0) {
        printf("Client disconnected\n");
        close_connection(client_sock);
    }
}

// Function to authorize the library member
int authorize_admin(int admin_id, char *token) {
    int admin_fd;
    Admin admin;

    // Open the members file
    admin_fd = open("admins.dat", O_RDONLY);

    struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    // read locking the member file
    fcntl(admin_fd, F_SETLKW, &lock);

    // Read the members file line by line
    while (read(admin_fd, &admin, sizeof(Admin)) > 0) {
        // Check if the username matches with any record
        if (admin.admin_id == admin_id) {
            if (!strcmp(admin.token, token)) {
                // unlock the file and close it
                lock.l_type = F_UNLCK;
                fcntl(admin_fd, F_SETLK, &lock);
                close(admin_fd);

                // the admin is successfully authorized - return the ID
                return admin.admin_id;
            } else {
                // unlock the file and close it
                lock.l_type = F_UNLCK;
                fcntl(admin_fd, F_SETLK, &lock);
                close(admin_fd);

                // the token is incorrect
                return INCORRECT_TOKEN;
            }
        }
    }

    return INCORRECT_USERNAME_OR_ID;
}

// Function to authorize the library member
int authorize_member(char *username, char *token) {
    int member_fd;
    Member member;

    // Open the members file
    member_fd = open("members.dat", O_RDONLY);

    struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    // Read locking the member file
    fcntl(member_fd, F_SETLKW, &lock);

    // Read the members file line by line
    while (read(member_fd, &member, sizeof(Member)) > 0) {
        // Check if the username matches with any record
        if (!strcmp(member.username, username)) {
            if (!strcmp(member.token, token)) {
                // unlock the file and close it
                lock.l_type = F_UNLCK;
                fcntl(member_fd, F_SETLK, &lock);
                close(member_fd);

                // the member is successfully authorized - return the ID
                return member.member_id;
            } else {
                // unlock the file and close it
                lock.l_type = F_UNLCK;
                fcntl(member_fd, F_SETLK, &lock);
                close(member_fd);

                // the token is incorrect
                return INCORRECT_TOKEN;
            }
        }
    }

    return INCORRECT_USERNAME_OR_ID;
}

// Function to read lock files
void read_lock_file(struct flock *lock, int fd) {
    lock->l_type = F_RDLCK;
    fcntl(fd, F_SETLKW, lock);
}

// Function to write lock files
void write_lock_file(struct flock *lock, int fd) {
    lock->l_type = F_WRLCK;
    fcntl(fd, F_SETLKW, lock);
}

// Function to unlock files
void unlock_file(struct flock *lock, int fd) {
    lock->l_type = F_UNLCK;
    fcntl(fd, F_SETLK, lock);
}

// Function to handle client requests
void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    int admin_id, member_id;
    char buffer[BUFFER_SIZE];
    char username[USERNAME_SIZE], token[TOKEN_SIZE];

    struct flock lock;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    ssize_t r = recv(client_sock, buffer, BUFFER_SIZE, 0);
    check_client_ping(r, client_sock);
    buffer[r] = '\0';
    if (!strcmp(buffer, "MEMBER")) {
        // request the username from the client
        send(client_sock, "USERNAME", 8, 0);

        // receive the username from the client
        r = recv(client_sock, buffer, BUFFER_SIZE, 0);
        check_client_ping(r, client_sock);
        buffer[r] = '\0';
        strcpy(username, buffer);

        // request the token from the client
        send(client_sock, "TOKEN", 5, 0);

        // receive the token from the client
        r = recv(client_sock, buffer, BUFFER_SIZE, 0);
        check_client_ping(r, client_sock);
        buffer[r] = '\0';
        strcpy(token, buffer);

        // attempt to log the user in as a member of the library
        member_id = authorize_member(username, token);
        if (member_id == INCORRECT_TOKEN) {
            send(client_sock, "MEMBER_INCORRECT_TOKEN", 22, 0);
            printf("Member incorrect token\n");
            close_connection(client_sock);
        } else if (member_id == INCORRECT_USERNAME_OR_ID) {
            send(client_sock, "MEMBER_INCORRECT_USERNAME_OR_ID", 31, 0);
            printf("Member incorrect username or ID\n");
            close_connection(client_sock);
        } else {
            send(client_sock, "MEMBER_LOGIN_SUCCESS", 20, 0);
        }

        while (1) {
            // Receive the request from the client
            r = recv(client_sock, buffer, BUFFER_SIZE, 0);
            check_client_ping(r, client_sock);
            buffer[r] = '\0';

            if (!strcmp(buffer, "VIEW_BOOKS")) {
                int books_fd;
                Book book;

                books_fd = open("books.dat", O_RDONLY);
                read_lock_file(&lock, books_fd);

                // Read the books file line by line to retrieve each record
                while ((r = read(books_fd, &book, sizeof(Book))) > 0) {
                    // Send the book details to the client
                    send(client_sock, &book, sizeof(Book), 0);
                }
                if (r == 0) {
                    // End of file
                    send(client_sock, "END_OF_FILE", 11, 0);
                } else if (r < 0) {
                    // Error reading file
                    send(client_sock, "ERROR_READING_FILE", 18, 0);
                }

                // Unlock the file and close it
                unlock_file(&lock, books_fd);
                close(books_fd);
            }

            else if (!strcmp(buffer, "SEARCH_BOOK")) {
                int books_fd;
                Book book;
                char keyword[TITLE_SIZE];

                // Request the search keyword(s) from the client
                send(client_sock, "KEYWORD", 7, 0);

                // Receive the search keyword(s) from the client
                r = recv(client_sock, buffer, BUFFER_SIZE, 0);
                check_client_ping(r, client_sock);
                buffer[r] = '\0';
                strcpy(keyword, buffer);

                books_fd = open("books.dat", O_RDONLY);
                read_lock_file(&lock, books_fd);

                // Read the books file line by line to search for books with the keyword
                while ((r = read(books_fd, &book, sizeof(Book))) > 0) {
                    if (strstr(book.title, keyword) || strstr(book.author, keyword)) {
                        // Send matching book details to the client
                        send(client_sock, &book, sizeof(Book), 0);
                    }
                }
                if (r == 0) {
                    // End of file
                    send(client_sock, "END_OF_FILE", 11, 0);
                } else if (r < 0) {
                    // Error reading file
                    send(client_sock, "ERROR_READING_FILE", 18, 0);
                }

                unlock_file(&lock, books_fd);
                close(books_fd);
            }

            else if (!strcmp(buffer, "BORROW_BOOK")) {
                int book_id, copies_fd, count_issued = 0, exists = 0, max_reached = 0;
                Copy copy;

                copies_fd = open("copies.dat", O_RDONLY);
                read_lock_file(&lock, copies_fd);

                // Read the copies file line by line to see if the member has reached the maximum limit of borrowed books
                while ((r = read(copies_fd, &copy, sizeof(Copy))) > 0) {
                    if (copy.borrowed_member_id == member_id) {
                        count_issued++;
                        if (count_issued == MAX_BORROWED_BOOKS) {
                            // Maximum limit of borrowed books reached
                            send(client_sock, "MAX_BORROWED_BOOKS_REACHED", 26, 0);
                            max_reached = 1;
                            break;
                        }
                    }
                }
                if (r < 0) {
                    // Error reading file
                    send(client_sock, "ERROR_READING_FILE", 18, 0);
                }
                if (max_reached) {
                    continue;
                }

                unlock_file(&lock, copies_fd);
                close(copies_fd);

                // Request the book ID from the client if the maximum limit is not reached
                send(client_sock, "BOOK_ID", 7, 0);

                // Receive the book ID from the client
                r = recv(client_sock, &book_id, sizeof(int), 0);
                check_client_ping(r, client_sock);
                if (r != 4) {
                    printf("Some error occured in transaction\n");
                    continue;
                }

                copies_fd = open("copies.dat", O_RDWR);
                write_lock_file(&lock, copies_fd);

                // Read the copies file line by line to find an available copy of the book
                while ((r = read(copies_fd, &copy, sizeof(Copy))) > 0) {
                    if (copy.book_id == book_id) {
                        // If a book with the requested book ID exists, then this flag is set to 1
                        exists = 1;
                    }
                    if (copy.book_id == book_id && copy.borrowed_member_id == 0) {
                        lseek(copies_fd, -sizeof(Copy), SEEK_CUR);
                        copy.borrowed_member_id = member_id;
                        write(copies_fd, &copy, sizeof(Copy));

                        // Borrowing book successful
                        send(client_sock, "BORROW_SUCCESS", 14, 0);

                        break;
                    }
                }
                if (r == 0) {
                    if (exists) {
                        // No available copies of the book
                        send(client_sock, "BOOK_COPY_NOT_AVAILABLE", 23, 0);
                    } else {
                        // Book with the requested book ID does not exist
                        send(client_sock, "BOOK_WITH_ID_NOT_FOUND", 22, 0);
                    }
                } else if (r < 0) {
                    // Error reading file
                    send(client_sock, "ERROR_READING_FILE", 18, 0);
                }

                unlock_file(&lock, copies_fd);
                close(copies_fd);
            }

            else if (!strcmp(buffer, "RETURN_BOOK")) {
                int book_id, copies_fd;
                Copy copy;

                // Request the book ID from the client
                send(client_sock, "BOOK_ID", 7, 0);

                // Receive the book ID from the client
                r = recv(client_sock, &book_id, sizeof(int), 0);
                check_client_ping(r, client_sock);
                if (r != 4) {
                    printf("Some error occured in transaction\n");
                    continue;
                }

                copies_fd = open("copies.dat", O_RDWR);
                write_lock_file(&lock, copies_fd);

                // Read the copies file line by line to find the book issued by the member
                while ((r = read(copies_fd, &copy, sizeof(Copy))) > 0) {
                    if (copy.book_id == book_id && copy.borrowed_member_id == member_id) {
                        lseek(copies_fd, -sizeof(Copy), SEEK_CUR);
                        copy.borrowed_member_id = 0;
                        write(copies_fd, &copy, sizeof(Copy));

                        // Returning book successful
                        send(client_sock, "RETURN_SUCCESS", 14, 0);

                        break;
                    }
                }
                if (r == 0) {
                    // The book with given book ID is not issued by the user
                    send(client_sock, "BOOK_COPY_NOT_ISSUED", 20, 0);
                } else if (r < 0) {
                    // Error reading file
                    send(client_sock, "ERROR_READING_FILE", 18, 0);
                }

                unlock_file(&lock, copies_fd);
                close(copies_fd);
            }

            else if (!strcmp(buffer, "BORROWED_BOOKS")) {
                int copies_fd, books_fd, s;
                Copy copy;
                Book book;

                copies_fd = open("copies.dat", O_RDONLY);
                books_fd = open("books.dat", O_RDONLY);
                read_lock_file(&lock, copies_fd);
                read_lock_file(&lock, books_fd);

                // Read the copies file line by line to find books borrowed by the user
                while ((r = read(copies_fd, &copy, sizeof(Copy)) > 0)) {
                    if (copy.borrowed_member_id == member_id) {
                        lseek(books_fd, 0, SEEK_SET);
                        // Read the books file line by line to find the book details of a specific copy
                        while ((s = read(books_fd, &book, sizeof(Book))) > 0) {
                            if (book.book_id == copy.book_id) {
                                // Send the book details to the client
                                send(client_sock, &book, sizeof(Book), 0);
                                break;
                            }
                        }
                        if (s == 0) {
                            // No matching record found for issued copy
                            printf("Some error reading the books file, no matching record found for issued book\n");
                        } else if (s < 0) {
                            // Error reading file
                            send(client_sock, "ERROR_READING_FILE", 18, 0);
                        }
                    }
                }
                if (r == 0) {
                    // End of file
                    send(client_sock, "END_OF_FILE", 11, 0);
                } else if (r < 0) {
                    // Error reading file
                    send(client_sock, "ERROR_READING_FILE", 18, 0);
                }

                unlock_file(&lock, copies_fd);
                unlock_file(&lock, books_fd);
            }

            else if (!strcmp(buffer, "CHANGE_PASSWORD")) {
                int members_fd;
                Member member;

                // Request the new password from the client
                send(client_sock, "NEW_PASSWORD", 12, 0);

                // Receive the new password from the client
                r = recv(client_sock, buffer, BUFFER_SIZE, 0);
                check_client_ping(r, client_sock);
                buffer[r] = '\0';

                members_fd = open("members.dat", O_RDWR);
                write_lock_file(&lock, members_fd);

                // Read the members file line by line to find the given user and replace their password
                while ((r = read(members_fd, &member, sizeof(Member))) > 0) {
                    if (member.member_id == member_id) {
                        lseek(members_fd, -sizeof(Member), SEEK_CUR);
                        strcpy(member.token, buffer);
                        write(members_fd, &member, sizeof(Member));

                        // Password changed successfully
                        send(client_sock, "PASSWORD_CHANGED", 16, 0);

                        break;
                    }
                }
                if (r <= 0) {
                    // Password change unsuccessful
                    send(client_sock, "PASSWORD_CHANGE_FAILED", 22, 0);
                }

                unlock_file(&lock, members_fd);
                close(members_fd);
            }

            else if (!strcmp(buffer, "LOGOUT")) {
                printf("Member %d logging out\n", member_id);
                break;
            }

            else {
                printf("Invalid request from client ignored\n");
            }
        }
    }

    else if (!strcmp(buffer, "ADMIN")) {
        // request the admin ID from the client
        send(client_sock, "ADMIN_ID", 8, 0);

        // receive the admin ID from the client
        r = recv(client_sock, &admin_id, sizeof(int), 0);
        check_client_ping(r, client_sock);

        // request the token from the client
        send(client_sock, "TOKEN", 5, 0);

        // receive the token from the client
        r = recv(client_sock, buffer, BUFFER_SIZE, 0);
        check_client_ping(r, client_sock);
        buffer[r] = '\0';
        strcpy(token, buffer);

        // attempt to log the user in as an admin of the library
        admin_id = authorize_admin(admin_id, token);
        if (admin_id == INCORRECT_TOKEN) {
            send(client_sock, "ADMIN_INCORRECT_TOKEN", 21, 0);
            printf("Admin incorrect token\n");
            close_connection(client_sock);
        } else if (admin_id == INCORRECT_USERNAME_OR_ID) {
            send(client_sock, "ADMIN_INCORRECT_USERNAME_OR_ID", 30, 0);
            printf("Admin incorrect username or ID\n");
            close_connection(client_sock);
        } else {
            send(client_sock, "ADMIN_LOGIN_SUCCESS", 19, 0);
        }

        while (1) {
            // Receive the request from the client
            r = recv(client_sock, buffer, BUFFER_SIZE, 0);
            check_client_ping(r, client_sock);
            buffer[r] = '\0';

            if (!strcmp(buffer, "VIEW_BOOKS")) {
                int books_fd;
                Book book;

                books_fd = open("books.dat", O_RDONLY);
                read_lock_file(&lock, books_fd);

                // Read the books file line by line to retrieve each record
                while ((r = read(books_fd, &book, sizeof(Book))) > 0) {
                    // Send the book details to the client
                    send(client_sock, &book, sizeof(Book), 0);
                }
                if (r == 0) {
                    // End of file
                    send(client_sock, "END_OF_FILE", 11, 0);
                } else if (r < 0) {
                    // Error reading file
                    send(client_sock, "ERROR_READING_FILE", 18, 0);
                }

                // Unlock the file and close it
                unlock_file(&lock, books_fd);
                close(books_fd);
            }

            else if (!strcmp(buffer, "ADD_BOOK")) {
                int books_fd, next_book_id = 1, count = 0, success = 0;
                Book book, newBook;

                books_fd = open("books.dat", O_RDWR);
                read_lock_file(&lock, books_fd);

                // Read the book file line by line to find the next available book ID, while checking if the maximum limit of books has been reached
                while ((r = read(books_fd, &book, sizeof(Book)) > 0)) {
                    if (book.book_id == newBook.book_id) {
                        next_book_id = book.book_id+1;
                    }
                    count++;
                    if (count == MAX_BOOKS) {
                        // Maximum limit of books reached
                        send(client_sock, "MAX_BOOKS", 9, 0);
                        break;
                    }
                }
                if (r < 0) {
                    // Error reading file
                    send(client_sock, "ERROR_READING_FILE", 18, 0);
                    continue;
                }
                if (count == MAX_BOOKS) {
                    continue;
                }

                unlock_file(&lock, books_fd);
                
                // Request the book details from the client
                send(client_sock, "BOOK_DETAILS", 12, 0);

                // Receive the book details from the client
                r = recv(client_sock, &newBook, sizeof(Book), 0);
                check_client_ping(r, client_sock);
                if (r != sizeof(Book)) {
                    printf("Some error occured in transaction\n");
                    continue;
                }
                newBook.book_id = next_book_id;

                write_lock_file(&lock, books_fd);

                lseek(books_fd, 0, SEEK_END);
                if (write(books_fd, &newBook, sizeof(Book)) == sizeof(Book)) {
                    // Book added successfully
                    send(client_sock, &newBook.book_id, sizeof(int), 0);
                } else {
                    // Book not added
                    newBook.book_id = -1;
                    send(client_sock, &newBook.book_id, sizeof(int), 0);
                }

                unlock_file(&lock, books_fd);
                close(books_fd);
            }

            else if (!strcmp(buffer, "REMOVE_BOOK")) {
                int books_fd, removed = 0, book_id;
                Book book;

                // Request the book ID from the client
                send(client_sock, "BOOK_ID", 7, 0);

                // Receive the book ID from the client
                r = recv(client_sock, &book_id, sizeof(int), 0);
                check_client_ping(r, client_sock);
                if (r != 4) {
                    printf("Some error occured in transaction\n");
                    continue;
                }

                books_fd = open("books.dat", O_RDWR);
                write_lock_file(&lock, books_fd);

                // Read the books file line by line to find the book to be removed
                while ((r = read(books_fd, &book, sizeof(Book))) > 0) {
                    if (book.book_id == book_id) {
                        removed = 1;

                        // Iteratively move all the records one row up
                        while ((r = read(books_fd, &book, sizeof(Book))) > 0) {
                            lseek(books_fd, -2*sizeof(Book), SEEK_CUR);
                            write(books_fd, &book, sizeof(Book));
                            lseek(books_fd, sizeof(Book), SEEK_CUR);
                        }
                        if (r == 0) {
                            // End of file - truncate the last (duplicate) record
                            ftruncate(books_fd, lseek(books_fd, 0, SEEK_END)-sizeof(Book));
                        } else if (r < 0) {
                            // Error reading file
                            send(client_sock, "ERROR_READING_FILE", 18, 0);
                        }
                    }
                }
                if (r == 0) {
                    // End of file
                    if (removed) {
                        // Book removed successfully
                        send(client_sock, "BOOK_REMOVED", 12, 0);
                    } else {
                        // Book with the requested book ID does not exist
                        send(client_sock, "BOOK_WITH_ID_NOT_FOUND", 22, 0);
                    }
                } else if (r < 0) {
                    // Error reading file
                    send(client_sock, "ERROR_READING_FILE", 18, 0);
                }

                unlock_file(&lock, books_fd);
                close(books_fd);
            }

            else if (!strcmp(buffer, "VIEW_MEMBERS")) {
                int members_fd;
                Member member;

                members_fd = open("members.dat", O_RDONLY);
                read_lock_file(&lock, members_fd);

                // Read the members file line by line to retrieve each record
                while ((r = read(members_fd, &member, sizeof(Member))) > 0) {
                    // Send the member details to the client
                    send(client_sock, &member, sizeof(Member), 0);
                }
                if (r == 0) {
                    // End of file
                    send(client_sock, "END_OF_FILE", 11, 0);
                } else if (r < 0) {
                    // Error reading file
                    send(client_sock, "ERROR_READING_FILE", 18, 0);
                }

                unlock_file(&lock, members_fd);
                close(members_fd);
            }

            else if (!strcmp(buffer, "ADD_MEMBER")) {
                int member_fd, count = 0, next_member_id = 1;
                Member member, newMember;

                member_fd = open("members.dat", O_RDWR);
                read_lock_file(&lock, member_fd);

                // Read the members file line by line to find the next available member ID while checking if the maximum limit of members has been reached
                while ((r = read(member_fd, &member, sizeof(Member)) > 0)) {
                    if (member.member_id == newMember.member_id) {
                        next_member_id = member.member_id+1;
                    }
                    count++;
                    if (count == MAX_MEMBERS) {
                        // Maximum limit of members reached
                        send(client_sock, "MAX_MEMBERS", 11, 0);
                        break;
                    }
                }
                if (r < 0) {
                    // Error reading file
                    send(client_sock, "ERROR_READING_FILE", 18, 0);
                    continue;
                }
                if (count == MAX_MEMBERS) {
                    continue;
                }

                unlock_file(&lock, member_fd);

                // Request the member details from the client
                send(client_sock, "MEMBER_DETAILS", 14, 0);

                // Receive the member details from the client
                r = recv(client_sock, &newMember, sizeof(Member), 0);
                check_client_ping(r, client_sock);
                if (r != sizeof(Member)) {
                    printf("Some error occured in transaction\n");
                    continue;
                }
                newMember.member_id = next_member_id;

                write_lock_file(&lock, member_fd);

                lseek(member_fd, 0, SEEK_END);
                if (write(member_fd, &newMember, sizeof(Member)) == sizeof(Member)) {
                    // Member added successfully
                    send(client_sock, &newMember.member_id, sizeof(int), 0);
                } else {
                    newMember.member_id = -1;
                    // Member not added
                    send(client_sock, &newMember.member_id, sizeof(int), 0);
                }

                unlock_file(&lock, member_fd);
                close(member_fd);
            }

            else if (!strcmp(buffer, "REMOVE_MEMBER")) {
                int members_fd, removed = 0, member_id;
                Member member;

                // Request the member ID from the client
                send(client_sock, "MEMBER_ID", 9, 0);

                // Receive the member ID from the client
                r = recv(client_sock, &member_id, sizeof(int), 0);
                check_client_ping(r, client_sock);
                if (r != 4) {
                    printf("Some error occured in transaction\n");
                    continue;
                }

                members_fd = open("members.dat", O_RDWR);
                write_lock_file(&lock, members_fd);

                // Read the members file line by line to find the member to be removed
                while ((r = read(members_fd, &member, sizeof(Member))) > 0) {
                    if (member.member_id == member_id) {
                        removed = 1;

                        // Iteratively move all the records one row up
                        while ((r = read(members_fd, &member, sizeof(Member))) > 0) {
                            lseek(members_fd, -2*sizeof(Member), SEEK_CUR);
                            write(members_fd, &member, sizeof(Member));
                            lseek(members_fd, sizeof(Member), SEEK_CUR);
                        }
                        if (r == 0) {
                            // End of file - truncate the last (duplicate) record
                            ftruncate(members_fd, lseek(members_fd, 0, SEEK_CUR)-sizeof(Member));
                        } else if (r < 0) {
                            // Error reading file
                            send(client_sock, "ERROR_READING_FILE", 18, 0);
                        }
                    }
                }
                if (r == 0) {
                    // End of file
                    if (removed) {
                        send(client_sock, "MEMBER_REMOVED", 14, 0);
                    } else {
                        send(client_sock, "MEMBER_WITH_ID_NOT_FOUND", 24, 0);
                    }
                } else if (r < 0) {
                    // Error reading file
                    send(client_sock, "ERROR_READING_FILE", 18, 0);
                }

                unlock_file(&lock, members_fd);
                close(members_fd);
            }

            else if (!strcmp(buffer, "VIEW_COPIES")) {
                int copies_fd;
                Copy copy;

                copies_fd = open("copies.dat", O_RDONLY);
                read_lock_file(&lock, copies_fd);

                // Read the copies file line by line to retrieve each record
                while ((r = read(copies_fd, &copy, sizeof(Copy))) > 0) {
                    // Send the copy details to the client
                    send(client_sock, &copy, sizeof(Copy), 0);
                }
                if (r == 0) {
                    // End of file
                    send(client_sock, "END_OF_FILE", 11, 0);
                } else if (r < 0) {
                    // Error reading file
                    send(client_sock, "ERROR_READING_FILE", 18, 0);
                }

                unlock_file(&lock, copies_fd);
                close(copies_fd);
            }

            else if (!strcmp(buffer, "ADD_COPY")) {
                int copies_fd, exists = 0, count = 0;
                Copy copy, newCopy;

                // Request the copy details from the client
                send(client_sock, "COPY_DETAILS", 12, 0);

                // Receive the copy details from the client
                r = recv(client_sock, &newCopy.book_id, sizeof(int), 0);
                check_client_ping(r, client_sock);
                if (r != 4) {
                    printf("Some error occured in transaction\n");
                    continue;
                }

                copies_fd = open("copies.dat", O_RDWR);
                read_lock_file(&lock, copies_fd);

                // Read the copies file line by line
                while ((r = read(copies_fd, &copy, sizeof(Copy)) > 0)) {
                    if (copy.book_id == newCopy.book_id) {
                        exists = 1;
                        if (copy.copy_id == newCopy.copy_id) {
                            newCopy.copy_id = copy.copy_id+1;
                        }
                    }
                    count++;
                    if (count == MAX_COPIES) {
                        newCopy.copy_id = -1;
                        // Maximum limit of copies reached
                        send(client_sock, &newCopy.copy_id, sizeof(int), 0);
                        break;
                    }
                }
                if (count == MAX_COPIES) {
                    continue;
                }
                if (!exists) {
                    newCopy.copy_id = -1;
                    // Book with the requested book ID does not exist
                    send(client_sock, &newCopy.copy_id, sizeof(int), 0);
                    continue;
                }

                unlock_file(&lock, copies_fd);
                write_lock_file(&lock, copies_fd);

                lseek(copies_fd, 0, SEEK_END);
                if (write(copies_fd, &newCopy, sizeof(Copy)) == sizeof(Copy)) {
                    // Copy added successfully
                    send(client_sock, &newCopy.copy_id, sizeof(int), 0);
                } else {
                    newCopy.copy_id = -1;
                    // Copy not added
                    send(client_sock, &newCopy.copy_id, sizeof(int), 0);
                }

                unlock_file(&lock, copies_fd);
                close(copies_fd);
            }

            else if (!strcmp(buffer, "REMOVE_COPY")) {
                int copies_fd, removed = 0, book_id, copy_id;
                Copy copy;

                // Request the copy ID from the client
                send(client_sock, "COPY_ID", 7, 0);

                // Receive the book and copy ID from the client
                r = recv(client_sock, &book_id, sizeof(int), 0);
                check_client_ping(r, client_sock);
                if (r != 4) {
                    printf("Some error occured in transaction\n");
                    continue;
                }
                r = recv(client_sock, &copy_id, sizeof(int), 0);
                check_client_ping(r, client_sock);
                if (r != 4) {
                    printf("Some error occured in transaction\n");
                    continue;
                }

                copies_fd = open("copies.dat", O_RDWR);
                write_lock_file(&lock, copies_fd);

                // Read the copies file line by line to find the copy to be removed
                while ((r = read(copies_fd, &copy, sizeof(Copy))) > 0) {
                    if (copy.copy_id == copy_id && copy.book_id == book_id) {
                        removed = 1;

                        // Iteratively move all the records one row up
                        while ((r = read(copies_fd, &copy, sizeof(Copy))) > 0) {
                            lseek(copies_fd, -2*sizeof(Copy), SEEK_CUR);
                            write(copies_fd, &copy, sizeof(Copy));
                            lseek(copies_fd, sizeof(Copy), SEEK_CUR);
                        }
                        if (r == 0) {
                            // End of file - truncate the last (duplicate) record
                            ftruncate(copies_fd, lseek(copies_fd, 0, SEEK_CUR)-sizeof(Copy));
                        } else if (r < 0) {
                            // Error reading file
                            send(client_sock, "ERROR_READING_FILE", 18, 0);
                        }
                    }
                }
                if (r == 0) {
                    // End of file
                    if (removed) {
                        // Copy removed successfully
                        send(client_sock, "COPY_REMOVED", 12, 0);
                    } else {
                        // Copy with the requested copy ID does not exist
                        send(client_sock, "COPY_WITH_ID_NOT_FOUND", 22, 0);
                    }
                } else if (r < 0) {
                    // Error reading file
                    send(client_sock, "ERROR_READING_FILE", 18, 0);
                }
                
                // Unlock the file and close it
                lock.l_type = F_UNLCK;
                fcntl(copies_fd, F_SETLK, &lock);
                close(copies_fd);
            }

            else if (!strcmp(buffer, "CHANGE_PASSWORD")) {
                int admins_fd;
                Admin admin;

                // Request the new password from the client
                send(client_sock, "NEW_PASSWORD", 12, 0);

                // Receive the new password from the client
                r = recv(client_sock, buffer, BUFFER_SIZE, 0);
                check_client_ping(r, client_sock);
                buffer[r] = '\0';

                admins_fd = open("admins.dat", O_RDWR);
                write_lock_file(&lock, admins_fd);

                // Read the admins file line by line to find the given admin and replace their password
                while ((r = read(admins_fd, &admin, sizeof(Admin))) > 0) {
                    if (admin.admin_id == admin_id) {
                        lseek(admins_fd, -sizeof(Admin), SEEK_CUR);
                        strcpy(admin.token, buffer);
                        write(admins_fd, &admin, sizeof(Admin));

                        // Password changed successfully
                        send(client_sock, "PASSWORD_CHANGED", 16, 0);

                        break;
                    }
                }
                if (r <= 0) {
                    // Password change unsuccessful
                    send(client_sock, "PASSWORD_CHANGE_FAILED", 22, 0);
                }

                unlock_file(&lock, admins_fd);
                close(admins_fd);
            }

            else if (!strcmp(buffer, "LOGOUT")) {
                printf("Admin %d logging out\n", admin_id);
                break;
            }

            else {
                printf("Invalid request from client ignored\n");
            }
        }
    }
    close_connection(client_sock);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_address, client_address;
    pthread_t thread_id;
    int address_length = sizeof(server_address);

    // Create server socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Set server address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    // Listening on port 8080: this can be changed to any available port number
    server_address.sin_port = htons(SERVER_PORT);

    // Bind the socket to the specified address and port
    if (bind(server_sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Failed to bind socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_sock, MAX_CLIENTS) < 0) {
        perror("Failed to listen for connections");
        exit(EXIT_FAILURE);
    }

    printf("Server started. Listening for connections...\n");

    while (1) {
        // Accept incoming connection
        client_sock = accept(server_sock, (struct sockaddr *) &client_address, (socklen_t *) &address_length);
        if (client_sock < 0) {
            perror("Failed to accept connection");
            exit(EXIT_FAILURE);
        }

        printf("New client connected. Client IP: %s, Port: %d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        // Create a new thread to handle the client
        if (pthread_create(&thread_id, NULL, handle_client, (void *)&client_sock) < 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }

        // Detach the thread to avoid memory leaks
        pthread_detach(thread_id);
    }

    // Close the server socket
    close(server_sock);

    return 0;
}