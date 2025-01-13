#include "library.h"

void check_server_response(int r) {
    if (r < 0) {
        perror("Failed to receive data from server");
        exit(EXIT_FAILURE);
    }
    if (r == 0) {
        printf("Connection closed by server\n");
        exit(EXIT_FAILURE);
    }
}

int main() {
    int sock, choice, admin_id;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    ssize_t r;

    // Create client socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &(server_addr.sin_addr)) <= 0) {
        perror("Invalid server IP address");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to connect to server");
        exit(EXIT_FAILURE);
    }

    printf("Enter 1 to log in as a member, 2 to log in as an admin: ");
    scanf("%d", &choice);

    switch (choice) {
        case 1:
            // member login
            send(sock, "MEMBER", 6, 0);

            // receive the prompt to enter username
            r = recv(sock, buffer, BUFFER_SIZE, 0);
            check_server_response(r);
            buffer[r] = '\0';
            if (strcmp(buffer, "USERNAME")) {
                printf("Invalid response from server\n");
                exit(EXIT_FAILURE);
            }

            // send the username
            printf("Enter your username: ");
            scanf("%s", buffer);
            send(sock, buffer, strlen(buffer), 0);

            // receive the prompt to enter password
            r = recv(sock, buffer, BUFFER_SIZE, 0);
            check_server_response(r);
            buffer[r] = '\0';
            if (strcmp(buffer, "TOKEN")) {
                printf("Invalid response from server\n");
                exit(EXIT_FAILURE);
            }

            // send the password
            printf("Enter your password: ");
            scanf("%s", buffer);
            send(sock, buffer, strlen(buffer), 0);

            // receive the login status (success or failure)
            r = recv(sock, buffer, BUFFER_SIZE, 0);
            check_server_response(r);
            if (!strcmp(buffer, "MEMBER_LOGIN_SUCCESS")) {
                printf("Logged in as a member.\n");
            } else if (!strcmp(buffer, "MEMBER_INCORRECT_TOKEN")) {
                printf("Password incorrect.\n");
                exit(EXIT_FAILURE);
            } else {
                printf("%s\n", buffer);
                printf("Username incorrect.\n");
                exit(EXIT_FAILURE);
            }

            // member functions
            while (1) {
                printf(MEMBER_PROMPT);
                scanf("%d", &choice);

                Book books[MAX_BOOKS];
                int i, book_id, b = 0;
                switch (choice) {
                    case 1:
                        // view books
                        send(sock, "VIEW_BOOKS", 10, 0);

                        printf("+-------+----------------------------------------------------+----------------------+----------------------+\n");
                        printf("| ID    | Title                                              | Author               | Genre                |\n");
                        printf("+-------+----------------------------------------------------+----------------------+----------------------+\n");
                        while ((r = recv(sock, &books[i], sizeof(Book), 0)) > 0) {
                            if (r < sizeof(Book)) {
                                if (r == 11) {
                                    printf("+-------+----------------------------------------------------+----------------------+----------------------+\n");
                                } else {
                                    perror("Failed to receive data from server");
                                }
                                break;
                            }
                            printf("| %5d | %-50s | %-20s | %-20s |\n", books[i].book_id, books[i].title, books[i].author, books[i].genre);
                            i++;
                        }

                        break;

                    case 2:
                        // search for a book
                        send(sock, "SEARCH_BOOK", 11, 0);

                        r = recv(sock, buffer, BUFFER_SIZE, 0);
                        check_server_response(r);

                        getchar();
                        printf("Enter the keyword to search for the book (by title or author): ");
                        scanf("%[^\n]", buffer);
                        getchar();
                        send(sock, buffer, strlen(buffer), 0);

                        printf("+-------+----------------------------------------------------+----------------------+----------------------+\n");
                        printf("| ID    | Title                                              | Author               | Genre                |\n");
                        printf("+-------+----------------------------------------------------+----------------------+----------------------+\n");
                        while ((r = recv(sock, &books[i], sizeof(Book), 0)) > 0) {
                            if (r < sizeof(Book)) {
                                if (r == 11) {
                                    printf("+-------+----------------------------------------------------+----------------------+----------------------+\n");
                                } else {
                                    perror("Failed to receive data from server");
                                }
                                break;
                            }
                            printf("| %5d | %-50s | %-20s | %-20s |\n", books[i].book_id, books[i].title, books[i].author, books[i].genre);
                            i++;
                        }
                        break;

                    case 3:
                        // borrow a book
                        send(sock, "BORROW_BOOK", 11, 0);

                        r = recv(sock, buffer, BUFFER_SIZE, 0);
                        check_server_response(r);
                        if (r == 26) {
                            printf("You have already borrowed the maximum number of books.\n");
                            break;
                        }

                        printf("Enter the ID of the book you want to borrow: ");
                        scanf("%d", &book_id);
                        send(sock, &book_id, sizeof(book_id), 0);

                        r = recv(sock, buffer, BUFFER_SIZE, 0);
                        check_server_response(r);
                        if (r == 14) {
                            printf("Book with ID %d borrowed successfully.\n", book_id);
                        } else if (r == 23) {
                            printf("Book with ID %d is not available.\n", book_id);
                        } else if (r == 22) {
                            printf("Book with ID %d does not exist.\n", book_id);
                        } else {
                            perror("Failed to retrieve data from server");
                        }
                        break;

                    case 4:
                        // return a book
                        send(sock, "RETURN_BOOK", 11, 0);

                        r = recv(sock, buffer, BUFFER_SIZE, 0);
                        check_server_response(r);

                        printf("Enter the ID of the book you want to return: ");
                        scanf("%d", &book_id);
                        send(sock, &book_id, sizeof(book_id), 0);

                        r = recv(sock, buffer, BUFFER_SIZE, 0);
                        check_server_response(r);
                        if (r == 14) {
                            printf("Book with ID %d returned successfully.\n", book_id);
                        } else if (r == 20) {
                            printf("Book with ID %d is not borrowed by you.\n", book_id);
                        } else {
                            perror("Failed to retrieve data from server");
                        }
                        break;

                    case 5:
                        // see the list of books borrowed by you
                        send(sock, "BORROWED_BOOKS", 14, 0);

                        printf("+-------+----------------------------------------------------+----------------------+----------------------+\n");
                        printf("| ID    | Title                                              | Author               | Genre                |\n");
                        printf("+-------+----------------------------------------------------+----------------------+----------------------+\n");

                        while ((r = recv(sock, &books[i], sizeof(Book), 0)) > 0) {
                            if (r < sizeof(Book)) {
                                if (r == 11) {
                                    printf("+-------+----------------------------------------------------+----------------------+----------------------+\n");
                                } else {
                                    perror("Failed to receive data from server");
                                }
                                break;
                            }
                            printf("| %5d | %-50s | %-20s | %-20s |\n", books[i].book_id, books[i].title, books[i].author, books[i].genre);
                            i++;
                        }

                        break;

                    case 6:
                        // change password
                        send(sock, "CHANGE_PASSWORD", 15, 0);

                        r = recv(sock, buffer, BUFFER_SIZE, 0);
                        check_server_response(r);
                        if (r != 12) {
                            perror("Failed to receive data from server");
                            break;
                        }

                        printf("Enter your new password: ");
                        scanf("%s", buffer);
                        send(sock, buffer, strlen(buffer), 0);

                        r = recv(sock, buffer, BUFFER_SIZE, 0);
                        check_server_response(r);
                        if (r == 16) {
                            printf("Password changed successfully.\n");
                        } else {
                            perror("Failed to retrieve data from server");
                        }

                        break;

                    case 7:
                        // log out
                        send(sock, "LOGOUT", 6, 0);

                        printf("Logging out...\n");
                        b = 1;
                        break;

                    default:
                        printf("Invalid choice\n");
                        break;
                }
                if (b) {
                    break;
                }
            }
            break;

        case 2:
            // Admin login
            send(sock, "ADMIN", 5, 0);

            // receive the prompt to enter admin ID
            r = recv(sock, buffer, BUFFER_SIZE, 0);
            check_server_response(r);
            buffer[r] = '\0';
            if (strcmp(buffer, "ADMIN_ID")) {
                printf("Invalid response from server\n");
                exit(EXIT_FAILURE);
            }

            // send the admin ID
            printf("Enter your admin ID: ");
            scanf("%d", &admin_id);
            send(sock, &admin_id, sizeof(admin_id), 0);

            // receive the prompt to enter the password
            r = recv(sock, buffer, BUFFER_SIZE, 0);
            check_server_response(r);
            buffer[r] = '\0';
            if (strcmp(buffer, "TOKEN")) {
                printf("Invalid response from server\n");
                exit(EXIT_FAILURE);
            }

            // send the password
            printf("Enter your password: ");
            scanf("%s", buffer);
            send(sock, buffer, strlen(buffer), 0);

            // receive the login status (success or failure)
            r = recv(sock, buffer, BUFFER_SIZE, 0);
            check_server_response(r);
            if (!strcmp(buffer, "ADMIN_LOGIN_SUCCESS")) {
                printf("Logged in as an admin.\n");
            } else if (!strcmp(buffer, "ADMIN_INCORRECT_TOKEN")) {
                printf("Password incorrect.\n");
                exit(EXIT_FAILURE);
            } else {
                printf("Admin ID incorrect.\n");
                exit(EXIT_FAILURE);
            }

            // admin functions
            while (1) {
                printf(ADMIN_PROMPT);
                scanf("%d", &choice);

                Book book;
                Member member;
                Copy copy;
                int book_id, member_id, copy_id, b = 0;
                switch (choice) {
                    case 1:
                        // view the list of books
                        send(sock, "VIEW_BOOKS", 10, 0);

                        printf("+-------+----------------------------------------------------+----------------------+----------------------+\n");
                        printf("| ID    | Title                                              | Author               | Genre                |\n");
                        printf("+-------+----------------------------------------------------+----------------------+----------------------+\n");
                        while ((r = recv(sock, &book, sizeof(Book), 0)) > 0) {
                            if (r < sizeof(Book)) {
                                if (r == 11) {
                                    printf("+-------+----------------------------------------------------+----------------------+----------------------+\n");
                                } else {
                                    perror("Failed to receive data from server");
                                }
                                break;
                            }
                            printf("| %5d | %-50s | %-20s | %-20s |\n", book.book_id, book.title, book.author, book.genre);
                        }

                        break;

                    case 2:
                        // add a new book
                        send(sock, "ADD_BOOK", 8, 0);

                        r = recv(sock, buffer, BUFFER_SIZE, 0);
                        check_server_response(r);
                        if (r == 9) {
                            printf("You have reached the maximum number of books.\n");
                            break;
                        } else if (r == 18) {
                            perror("Failed to receive data from server");
                            break;
                        }

                        getchar();
                        printf("Enter the title of the book: ");
                        scanf("%[^\n]", book.title);
                        getchar();

                        printf("Enter the author of the book: ");
                        scanf("%[^\n]", book.author);
                        getchar();

                        printf("Enter the genre of the book: ");
                        scanf("%[^\n]", book.genre);
                        getchar();

                        send(sock, &book, sizeof(Book), 0);

                        r = recv(sock, &book_id, sizeof(int), 0);
                        check_server_response(r);
                        if (book_id != -1) {
                            printf("Book with ID %d added successfully.\n", book_id);
                        } else {
                            printf("Failed to add the book.\n");
                        }

                        break;

                    case 3:
                        // remove a book
                        send(sock, "REMOVE_BOOK", 11, 0);

                        r = recv(sock, buffer, BUFFER_SIZE, 0);
                        check_server_response(r);
                        if (r != 7) {
                            perror("Failed to receive data from server");
                            break;
                        }

                        printf("Enter the ID of the book you want to remove: ");
                        scanf("%d", &book_id);
                        send(sock, &book_id, sizeof(book_id), 0);

                        r = recv(sock, buffer, BUFFER_SIZE, 0);
                        check_server_response(r);
                        if (r == 12) {
                            printf("Book with ID %d removed successfully.\n", book_id);
                        } else if (r == 22) {
                            printf("Book with ID %d does not exist.\n", book_id);
                        } else {
                            perror("Failed to retrieve data from server");
                        }

                        break;

                    case 4:
                        // view the list of members
                        send(sock, "VIEW_MEMBERS", 12, 0);

                        printf("+-------+----------------------+\n");
                        printf("| ID    | Username             |\n");
                        printf("+-------+----------------------+\n");
                        while ((r = recv(sock, &member, sizeof(Member), 0)) > 0) {
                            if (r < sizeof(Member)) {
                                if (r == 11) {
                                    printf("+-------+----------------------+\n");
                                } else {
                                    perror("Failed to receive data from server");
                                }
                                break;
                            }
                            printf("| %5d | %-20s |\n", member.member_id, member.username);
                        }

                        break;

                    case 5:
                        // add a new member
                        send(sock, "ADD_MEMBER", 10, 0);

                        r = recv(sock, buffer, BUFFER_SIZE, 0);
                        check_server_response(r);
                        if (r == 11) {
                            printf("You have reached the maximum number of members.\n");
                            break;
                        } else if (r == 18) {
                            perror("Failed to receive data from server");
                            break;
                        }

                        Member member;
                        member.member_id = 1;

                        getchar();
                        printf("Enter the username of the member: ");
                        scanf("%[^\n]", member.username);
                        getchar();

                        printf("Set a password for the member (should not have spaces): ");
                        scanf("%s", buffer);
                        send(sock, &member, sizeof(Member), 0);

                        r = recv(sock, &member_id, sizeof(int), 0);
                        check_server_response(r);
                        if (member_id != -1) {
                            printf("Member with ID %d added successfully.\n", member_id);
                        } else {
                            printf("Failed to add the member.\n");
                        }

                        break;

                    case 6:
                        // remove a member
                        send(sock, "REMOVE_MEMBER", 13, 0);

                        r = recv(sock, buffer, BUFFER_SIZE, 0);
                        check_server_response(r);
                        if (r == 11) {
                            printf("You have reached the maximum number of members.\n");
                            break;
                        } else if (r == 18) {
                            perror("Failed to receive data from server");
                            break;
                        }

                        printf("Enter the ID of the member you want to remove: ");
                        scanf("%d", &member_id);
                        send(sock, &member_id, sizeof(member_id), 0);

                        r = recv(sock, buffer, BUFFER_SIZE, 0);
                        check_server_response(r);
                        if (r == 14) {
                            printf("Member with ID %d removed successfully.\n", member_id);
                        } else if (r == 24) {
                            printf("Member with ID %d does not exist.\n", member_id);
                        } else {
                            perror("Failed to retrieve data from server");
                        }

                        break;

                    case 7:
                        // view the list of copies
                        send(sock, "VIEW_COPIES", 11, 0);

                        printf("+-------+-------+----------------------+\n");
                        printf("| Book  | Copy  | Borrowed by (ID)     |\n");
                        printf("+-------+-------+----------------------+\n");
                        while ((r = recv(sock, &copy, sizeof(Copy), 0)) > 0) {
                            if (r < sizeof(Copy)) {
                                if (r == 11) {
                                    printf("+-------+-------+----------------------+\n");
                                } else {
                                    perror("Failed to receive data from server");
                                }
                                break;
                            }
                            printf("| %5d | %5d | %20d |\n", copy.book_id, copy.copy_id, copy.borrowed_member_id);
                        }

                        break;

                    case 8:
                        // add a copy of a book
                        send(sock, "ADD_COPY", 8, 0);

                        r = recv(sock, buffer, BUFFER_SIZE, 0);
                        check_server_response(r);
                        if (r != 12) {
                            perror("Failed to receive data from server");
                            break;
                        }

                        printf("Enter the ID of the book you want to add a copy of: ");
                        scanf("%d", &book_id);
                        send(sock, &book_id, sizeof(book_id), 0);

                        r = recv(sock, &copy_id, sizeof(int), 0);
                        check_server_response(r);
                        if (copy_id != -1) {
                            printf("Copy of book with ID %d added successfully.\n", copy_id);
                        } else {
                            printf("Failed to add a copy of the book.\n");
                        }

                        break;

                    case 9:
                        // remove a copy
                        send(sock, "REMOVE_COPY", 11, 0);

                        r = recv(sock, buffer, BUFFER_SIZE, 0);
                        check_server_response(r);
                        if (r != 7) {
                            perror("Failed to receive data from server");
                            break;
                        }

                        printf("Enter the ID of the book you want to remove a copy of: ");
                        scanf("%d", &book_id);
                        send(sock, &book_id, sizeof(book_id), 0);

                        printf("Enter the ID of the copy you want to remove: ");
                        scanf("%d", &copy_id);
                        send(sock, &copy_id, sizeof(copy_id), 0);

                        r = recv(sock, buffer, BUFFER_SIZE, 0);
                        check_server_response(r);
                        if (r == 12) {
                            printf("Copy of book with ID %d and copy ID %d removed successfully.\n", book_id, copy_id);
                        } else if (r == 22) {
                            printf("Copy with book ID %d and copy ID %d does not exist.\n", book_id, copy_id);
                        } else {
                            perror("Failed to retrieve data from server");
                        }

                        break;

                    case 10:
                        // change password
                        send(sock, "CHANGE_PASSWORD", 15, 0);

                        r = recv(sock, buffer, BUFFER_SIZE, 0);
                        check_server_response(r);
                        if (r != 12) {
                            perror("Failed to receive data from server");
                            break;
                        }

                        printf("Enter your new password: ");
                        scanf("%s", buffer);
                        send(sock, buffer, strlen(buffer), 0);

                        r = recv(sock, buffer, BUFFER_SIZE, 0);
                        check_server_response(r);
                        if (r == 16) {
                            printf("Password changed successfully.\n");
                        } else {
                            perror("Failed to retrieve data from server");
                        }

                        break;

                    case 11:
                        // log out
                        send(sock, "LOGOUT", 6, 0);

                        printf("Logging out...\n");
                        b = 1;
                        break;
                }
                if (b) {
                    break;
                }
            }

            break;

        default:
            printf("Invalid choice\n");
            break;
    }

    printf("Exiting...\n");
    close(sock);
    return 0;
}