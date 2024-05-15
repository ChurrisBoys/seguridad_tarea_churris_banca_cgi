#include <iostream>
#include <cstring>
#include <mariadb/conncpp.hpp>

using namespace std;

// Print all records in Balance table 
void showBalance(std::unique_ptr<sql::Connection> &conn) {
    try {
        // Create a new Statement
        std::unique_ptr<sql::Statement> stmnt(conn->createStatement());
        // Execute query
        sql::ResultSet *res = stmnt->executeQuery("select * from Balance where username = \'Emilia\'");
        // Loop through and print results
        while (res->next()) {
            std::cout << "username = " << res->getString(1);
            std::cout << ", balance = " << res->getFloat(2);
            std::cout << ", currency = " << res->getBoolean(3) << "\n";
        }
    }
    catch(sql::SQLException& e){
        std::cerr << "Error selecting tasks: " << e.what() << std::endl;
    }
}


int main() {
    cout << "Content-type: text/plain\n\n";  // Changed to plain text for simplicity
     // Read POST data: assuming input is directly from stdin
    string input;
    getline(cin, input); // Reads one line from stdin, modify if expecting more data


    // Connection with the database
    try {
        sql::Driver* driver = sql::mariadb::get_driver_instance();
        sql::SQLString url("jdbc:mariadb://localhost:3306/churrisbanca_social");
        sql::Properties properties({{"user", "churris"}, {"password", "password"}});

        // Establish Connection
        std::unique_ptr<sql::Connection> conn(driver->connect(url, properties));

        showBalance(conn);

    }
    catch(sql::SQLException& e){
        std::cerr << "Error Connecting to MariaDB Platform: " << e.what() << std::endl;
        // Exit (Failed)
        return 1;
    }
   
   
    // Send a specific response back
    cout << "helloooooooo its meeee, I was wonderingg if after all these years you lied to meeeee. To go oveeeer, everythiiiing " << endl;
    cout << input << endl;

    return 0;
}

