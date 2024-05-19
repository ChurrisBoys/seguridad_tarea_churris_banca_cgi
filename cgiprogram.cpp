#include <iostream>
#include <cstring>
#include <mariadb/mysql.h>

using namespace std;

int main() {
    cout << "Content-type: text/plain\n\n";  // Changed to plain text for simplicity
     // Read POST data: assuming input is directly from stdin
    string input;
    getline(cin, input); // Reads one line from stdin, modify if expecting more data

    
    //===========================  Connection with the database
    // Initialize Connection
    MYSQL *conn;
    if (!(conn = mysql_init(0)))
    {
        fprintf(stderr, "unable to initialize connection struct\n");
    }

    if (!mysql_real_connect(
        conn,                 // Connection
        "127.0.0.1",// Host
        "churris",            // User account
        "password",   // User password
        "churrisbanca_bancaria",               // Default database
        3306,                 // Port number
        NULL,                 // Path to socket file
        0                     // Additional options
    ))
    {
        // Report the failed-connection error & close the handle
        fprintf(stderr, "Error connecting to Server: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }
    //=====

    // Accesing the database data
    MYSQL_STMT *stmt;
    char *query;    

    stmt = mysql_stmt_init(conn);
    if (!stmt) {
      printf("Statement init failed: %s\n", mysql_error(conn));
    }

    // Executing query
    query = "select * from Balance";
                        
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
	    printf("Statement prepare failed: %s\n", mysql_stmt_error(stmt));
    } else {
    	puts("Statement prepare OK!");
    }

    // Store the result
    char username[100];
    float money;
    signed char currency;
    MYSQL_BIND result[3];
    memset(result, 0, sizeof(result));
    result[0].buffer_type = MYSQL_TYPE_STRING;
    result[0].buffer = (char *)&username;
    result[0].buffer_length = sizeof(username);
    result[1].buffer_type = MYSQL_TYPE_FLOAT;
    result[1].buffer = (char *)&money;
    result[2].buffer_type = MYSQL_TYPE_TINY;
    result[2].buffer = (char *)&currency;
    mysql_stmt_store_result(stmt);
    mysql_stmt_bind_result(stmt, result);
                        
    if (mysql_stmt_execute(stmt)) {
    	printf("Statement execute failed: %s\n", mysql_stmt_error(stmt));
    }
    printf("The result has %d fields\n", mysql_stmt_field_count(stmt));

    // Fetch rows
    while (mysql_stmt_fetch(stmt) == 0) {
        // Process the results
        printf("Username: %s, Money: %f, Currency: %d\n", username, money, currency);
    }

    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);

    mysql_close(conn);
   
    // Send a specific response back
    cout << "helloooooooo its meeee, I was wonderingg if after all these years you lied to meeeee. To go oveeeer, everythiiiing " << endl;
    cout << input << endl;

    return 0;
}

