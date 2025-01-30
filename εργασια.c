#include <stdio.h>      // Εισαγωγή βιβλιοθήκης για είσοδο/έξοδο δεδομένων (π.χ., printf, scanf)
#include <stdlib.h>     // Εισαγωγή βιβλιοθήκης για λειτουργίες όπως malloc, free, exit
#include <string.h>     // Εισαγωγή βιβλιοθήκης για χειρισμό αλφαριθμητικών (π.χ., strcpy, snprintf)
#include <unistd.h>     // Εισαγωγή βιβλιοθήκης για λειτουργίες συστήματος (π.χ., fork, sleep)
#include <arpa/inet.h>  // Εισαγωγή βιβλιοθήκης για χειρισμό δικτυακών διευθύνσεων (π.χ., inet_pton)
#include <sys/wait.h>   // Εισαγωγή βιβλιοθήκης για λειτουργίες διαχείρισης διεργασιών (π.χ., wait)
#include <sys/types.h>  // Εισαγωγή βιβλιοθήκης για τύπους δεδομένων που αφορούν διεργασίες
#include <pthread.h>    // Εισαγωγή βιβλιοθήκης για χρήση νήματος (threads)

// Ορισμοί παραμέτρων
#define PORT 8080               // Θύρα για τη σύνδεση του server
#define PRODUCT_COUNT 20        // Αριθμός προϊόντων στον κατάλογο
#define CLIENT_COUNT 5          // Αριθμός πελατών
#define MAX_BUFFER 1024         // Μέγεθος μέγιστου buffer για ανταλλαγή δεδομένων

// Δομή για αποθήκευση στοιχείων προϊόντος
typedef struct {
    char description[50];        // Περιγραφή του προϊόντος
    float price;                 // Τιμή του προϊόντος
    int item_count;              // Αριθμός διαθέσιμων τεμαχίων
    int request_count;           // Αριθμός αιτημάτων για το προϊόν
    int sold_count;              // Αριθμός πωλήσεων του προϊόντος
    char failed_users[CLIENT_COUNT][50];  // Πίνακας με τα ονόματα των πελατών που αποτύχανε να παραγγείλουν
    int failed_count;            // Αριθμός αποτυχημένων προσπαθειών για το προϊόν
} Product;

// Κατάλογος προϊόντων
Product catalog[PRODUCT_COUNT];
// Μεταβλητές για στατιστικά του συστήματος
int total_orders = 0, successful_orders = 0, failed_orders = 0;
float total_revenue = 0.0;  // Συνολικά έσοδα από τις παραγγελίες

// Αρχικοποίηση του καταλόγου προϊόντων
void initialize_catalog() {
    // Γέμισμα του καταλόγου με 20 προϊόντα
    for (int i = 0; i < PRODUCT_COUNT; i++) {
        snprintf(catalog[i].description, 50, "Product_%d", i);  // Δίνει όνομα στο προϊόν
        catalog[i].price = (i + 1) * 10.0;  // Ορίζει την τιμή του προϊόντος
        catalog[i].item_count = 2;  // Αρχικά διαθέσιμα 2 τεμάχια για κάθε προϊόν
        catalog[i].request_count = 0;  // Αρχικά κανένα αίτημα για το προϊόν
        catalog[i].sold_count = 0;  // Αρχικά δεν έχουν πουληθεί προϊόντα
        catalog[i].failed_count = 0;  // Αρχικά δεν υπάρχουν αποτυχημένες παραγγελίες
    }
}

// Επεξεργασία παραγγελίας πελάτη
void process_order(int client_socket, char *client_name) {
    int product_id;
    // Διαβάζουμε το id του προϊόντος από τον πελάτη
    read(client_socket, &product_id, sizeof(int));
    product_id %= PRODUCT_COUNT;  // Διασφαλίζουμε ότι το id του προϊόντος είναι έγκυρο

    catalog[product_id].request_count++;  // Αυξάνουμε το πλήθος των αιτημάτων για το προϊόν
    if (catalog[product_id].item_count > 0) {  // Αν υπάρχει απόθεμα για το προϊόν
        catalog[product_id].item_count--;  // Αποθηκεύουμε την πώληση (μειώνουμε το απόθεμα)
        catalog[product_id].sold_count++;  // Αυξάνουμε τον αριθμό των πωλήσεων
        total_revenue += catalog[product_id].price;  // Προσθέτουμε το έσοδο από την πώληση
        successful_orders++;  // Αυξάνουμε τον αριθμό των επιτυχημένων παραγγελιών

        char response[MAX_BUFFER];
        snprintf(response, MAX_BUFFER, "Order successful: %s, Total: $%.2f\n",
                 catalog[product_id].description, catalog[product_id].price);  // Μήνυμα επιτυχίας
        write(client_socket, response, strlen(response) + 1);  // Στέλνουμε το μήνυμα πίσω στον πελάτη
    } else {  // Αν το προϊόν είναι εκτός αποθέματος
        failed_orders++;  // Αυξάνουμε τον αριθμό των αποτυχημένων παραγγελιών
        snprintf(catalog[product_id].failed_users[catalog[product_id].failed_count], 50, "%s", client_name);  // Καταγράφουμε τον πελάτη που απέτυχε
        catalog[product_id].failed_count++;  // Αυξάνουμε τον αριθμό των αποτυχημένων προσπαθειών

        char response[MAX_BUFFER];
        snprintf(response, MAX_BUFFER, "Order failed: %s is out of stock\n", catalog[product_id].description);  // Μήνυμα αποτυχίας
        write(client_socket, response, strlen(response) + 1);  // Στέλνουμε το μήνυμα αποτυχίας στον πελάτη
    }

    total_orders++;  // Αυξάνουμε τον συνολικό αριθμό παραγγελιών
    sleep(1);  // Καθυστέρηση 1 δευτερολέπτου για την επεξεργασία της παραγγελίας
}

// Εκτύπωση αναφοράς
void generate_report() {
    printf("\n--- Eshop Report ---\n");
    for (int i = 0; i < PRODUCT_COUNT; i++) {  // Για κάθε προϊόν στον κατάλογο
        printf("Product: %s\n", catalog[i].description);  // Εκτυπώνουμε το όνομα του προϊόντος
        printf("  Requests: %d\n", catalog[i].request_count);  // Εκτυπώνουμε πόσα αιτήματα έχουν γίνει για το προϊόν
        printf("  Sold: %d\n", catalog[i].sold_count);  // Εκτυπώνουμε πόσα τεμάχια πουλήθηκαν
        printf("  Failed Users: ");  // Εκτυπώνουμε τους χρήστες που απέτυχαν να παραγγείλουν
        for (int j = 0; j < catalog[i].failed_count; j++) {
            printf("%s ", catalog[i].failed_users[j]);
        }
        printf("\n");
    }
    printf("\nSummary:\n");
    printf("  Total Orders: %d\n", total_orders);  // Συνολικός αριθμός παραγγελιών
    printf("  Successful Orders: %d\n", successful_orders);  // Αριθμός επιτυχημένων παραγγελιών
    printf("  Failed Orders: %d\n", failed_orders);  // Αριθμός αποτυχημένων παραγγελιών
    printf("  Total Revenue: $%.2f\n", total_revenue);  // Συνολικά έσοδα
}

// Λειτουργία του server
void run_server() {
    initialize_catalog();  // Αρχικοποιούμε τον κατάλογο προϊόντων

    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Δημιουργούμε το socket για τον server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Ρυθμίζουμε την διεύθυνση του server
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Συνδέουμε το socket στην διεύθυνση
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Ξεκινάμε να ακούμε για συνδέσεις
    if (listen(server_fd, CLIENT_COUNT) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Eshop is running...\n");

    // Αποδεχόμαστε συνδέσεις πελατών
    for (int i = 0; i < CLIENT_COUNT; i++) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        char client_name[50];
        snprintf(client_name, 50, "Client_%d", i + 1);  // Δημιουργούμε όνομα για τον πελάτη

        if (fork() == 0) {  // Δημιουργούμε μια νέα διεργασία για κάθε πελάτη
            for (int j = 0; j < 10; j++) {  // Ο πελάτης κάνει 10 παραγγελίες
                process_order(client_socket, client_name);
            }
            close(client_socket);
            exit(0);  // Ολοκληρώνουμε τη διεργασία του πελάτη
        }
        close(client_socket);  // Κλείνουμε το socket του server
    }

    // Περιμένουμε να τελειώσουν όλες οι διεργασίες των πελατών
    while (wait(NULL) > 0);
    generate_report();  // Εκτυπώνουμε την αναφορά

    close(server_fd);  // Κλείνουμε το socket του server
}

// Λειτουργία του client
void run_client(int client_id) {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Ρυθμίζουμε τη διεύθυνση του server
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address\n");
        return;
    }

    // Συνδέουμε το client με τον server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection failed\n");
        return;
    }

    char buffer[MAX_BUFFER] = {0};
    for (int i = 0; i < 10; i++) {  // Ο πελάτης κάνει 10 παραγγελίες
        int product_id = rand() % 20;  // Τυχαίο προϊόν από τον κατάλογο
        write(sock, &product_id, sizeof(int));  // Στέλνουμε το id του προϊόντος στον server

        read(sock, buffer, MAX_BUFFER);  // Διαβάζουμε την απάντηση του server
        printf("Client %d: %s", client_id, buffer);  // Εκτυπώνουμε την απάντηση
        sleep(1);  // Καθυστέρηση 1 δευτερολέπτου
    }

    close(sock);  // Κλείνουμε τη σύνδεση με τον server
}

int main(int argc, char *argv[]) {
    if (argc < 2) {  // Αν δεν δώσουμε το σωστό όρισμα, εμφανίζεται μήνυμα χρήσης
        printf("Usage: %s <server|client>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "server") == 0) {  // Αν το όρισμα είναι "server", τρέχουμε τον server
        run_server();
    } else if (strcmp(argv[1], "client") == 0) {  // Αν το όρισμα είναι "client", τρέχουμε τον client
        for (int i = 0; i < CLIENT_COUNT; i++) {
            if (fork() == 0) {
                run_client(i + 1);  // Κάθε πελάτης εκκινεί το δικό του client
                exit(0);
            }
        }

        while (wait(NULL) > 0);  // Περιμένουμε να τελειώσουν όλοι οι πελάτες
    } else {
        printf("Invalid argument. Use 'server' or 'client'.\n");  // Λανθασμένο όρισμα
    }

    return 0;
}

