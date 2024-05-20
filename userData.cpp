#include <iostream>
#include <cstring>
#include <mariadb/mysql.h>

using namespace std;

int main() {
    cout << "Content-type: text/plain\n\n";  // Set content type to plain text

    // Read POST data
    string input;
    getline(cin, input); // Read input from stdin

    // input = "username=Emilia";

    // Extract username from input
    size_t pos = input.find("username=");
    string username;
    if (pos != string::npos) {
        username = input.substr(pos + 9);
    } else {
        cout << "Error: username not provided" << endl;
        return 1;
    }

    // Initialize Connection
    MYSQL *conn;
    if (!(conn = mysql_init(0))) {
        cerr << "Error: unable to initialize connection struct" << endl;
        return 1;
    }

    if (!mysql_real_connect(
        conn,                 // Connection
        "127.0.0.1",          // Host
        "churris",            // User account
        "password",           // User password
        "churrisbanca_bancaria", // Default database
        3306,                 // Port number
        NULL,                 // Path to socket file
        0                     // Additional options
    )) {
        cerr << "Error connecting to Server: " << mysql_error(conn) << endl;
        mysql_close(conn);
        return 1;
    }

    // Prepare the query
    MYSQL_STMT *stmt;
    char query[] = "SELECT balance, currency FROM Balance WHERE username=?";
    stmt = mysql_stmt_init(conn);
    if (!stmt) {
        cerr << "Error: Statement init failed: " << mysql_error(conn) << endl;
        return 1;
    }

    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        cerr << "Error: Statement prepare failed: " << mysql_stmt_error(stmt) << endl;
        return 1;
    }

    // Bind parameters
    MYSQL_BIND param[1];
    memset(param, 0, sizeof(param));
    param[0].buffer_type = MYSQL_TYPE_STRING;
    param[0].buffer = (char*)username.c_str();
    param[0].buffer_length = username.size();
    mysql_stmt_bind_param(stmt, param);

    // Bind result
    float balance;
    signed char currency;
    MYSQL_BIND result[2];
    memset(result, 0, sizeof(result));
    result[0].buffer_type = MYSQL_TYPE_FLOAT;
    result[0].buffer = (char *)&balance;
    result[1].buffer_type = MYSQL_TYPE_TINY;
    result[1].buffer = (char *)&currency;
    mysql_stmt_bind_result(stmt, result);

    // Execute the query
    if (mysql_stmt_execute(stmt)) {
        cerr << "Error: Statement execute failed: " << mysql_stmt_error(stmt) << endl;
        return 1;
    }

    // Fetch and print results
    if (mysql_stmt_fetch(stmt) == 0) {
        cout << "Username: " << username << "\n";
        cout << "Balance: " << balance << "\n";
        cout << "Currency: " << (currency == 1 ? "Churricoin" : "Euro") << "\n";
    } else {
        cout << "Error: No data found" << endl;
    }

    mysql_stmt_close(stmt);
    mysql_close(conn);

    return 0;
}
