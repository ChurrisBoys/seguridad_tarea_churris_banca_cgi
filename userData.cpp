#include <iostream>
#include <cstring>
#include <mariadb/mysql.h>
#include <nlohmann/json.hpp> // Retrieved from https://github.com/nlohmann/json
#include <regex>

using namespace std;

MYSQL_STMT* makeSqlStatement(MYSQL* conn, char* query) {
    MYSQL_STMT* stmt;
    stmt = mysql_stmt_init(conn);
    if (!stmt) {
        cerr << "Error: Statement init failed " << endl;
        exit(1);
    }

    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        cerr << "Error: Statement prepare failed" << endl;
        exit(1);
    }

    return stmt;
}

int main() {
    cout << "Content-type: application/json\n\n";  

    // Getting query variable to determine the action
    char* urlVariablesChar = getenv("QUERY_STRING");
    string urlVariablesString(urlVariablesChar);
    string urlAction;
    if(!urlVariablesString.empty())
        urlAction = strtok(urlVariablesChar, "a=");

    // Read POST data
    char*  temp = new char[0xFFFF];
    // getline(cin, input);
    temp[0xFFFE] = 0;
    std::cin.getline(temp, 0xFFFE);
    string input(temp);
    delete[] temp;

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
        cerr << "Error connecting to Server " << endl;
        mysql_close(conn);
        return 1;
    }

    std::regex userpattern("^[a-zA-Z]{1,80}(\.[a-zA-Z]{1,80})?$");
    std::regex amountpattern("^[0123456789]{1,6}(\.[0123456789]{1,3})?$");

    // If the action(a) equals CT(CreateTransaction) then create a transaction
    if(urlAction == "CT") {
        // GET and Validate data
        char* bodyChar = (char*) input.c_str();

        char* delimiter = "&";
        char* bodyTokensChars = strtok(bodyChar, delimiter);

        string intendedSender;
        string intendedReceiver;
        float intendedAmountToTransfer;

        // Prepare the query
        auto makeTransactionStmt = makeSqlStatement(conn, "CALL MakeTransaction(?, ?, ?)");
        int i = 0;

        // Getting all the data in the body of the HTTP request
        while (bodyTokensChars != nullptr) {
            if(i == 0)
                intendedSender = string(bodyTokensChars);
            else if(i == 1)
                intendedReceiver = string(bodyTokensChars);
            else if(i == 2) {
                if (!regex_match(string(bodyTokensChars), amountpattern)){
			cerr << "Invalid data" << endl;
			return 1;
		}
                intendedAmountToTransfer = atof(bodyTokensChars);
                if (intendedAmountToTransfer <= 0) {
			cerr << "Invalid data" << endl;
			return 1;
		}
	    }
            ++i;
            bodyTokensChars = strtok(nullptr, delimiter);
        }
        if (!regex_match(intendedSender, userpattern) || !regex_match(intendedReceiver, userpattern)) {
		cerr << "Invalid data" << endl;
		return 1;
	}

        // Bind parameters
        MYSQL_BIND param[3];
        memset(param, 0, sizeof(param));

        param[0].buffer_type = MYSQL_TYPE_STRING;
        param[0].buffer = (char*)intendedSender.c_str();
        param[0].buffer_length = intendedSender.length();

        param[1].buffer_type = MYSQL_TYPE_STRING;
        param[1].buffer = (char*)intendedReceiver.c_str();
        param[1].buffer_length = intendedReceiver.length();

        param[2].buffer_type = MYSQL_TYPE_FLOAT;
        param[2].buffer = (char*)&intendedAmountToTransfer;
        param[2].buffer_length = sizeof(intendedAmountToTransfer);

        mysql_stmt_bind_param(makeTransactionStmt, param);

       // Execute the stored procedure
        if (mysql_stmt_execute(makeTransactionStmt)) {
            cerr << "Error: Statement execute failed" << endl;
            return 1;
        }

        cout << "Transaction successful" << endl;

        mysql_stmt_close(makeTransactionStmt);

        return 1;
    }
    else if(urlAction == "S") {
        auto getAllUserTransactionsStmt = makeSqlStatement(conn, "SELECT t.sender, t.receiver, t.amount, t.currency_kind, t.publish_date FROM Transactions t WHERE t.sender=? OR t.receiver=? ORDER BY t.publish_date ASC");
        char* bodyChar = (char*) input.c_str();
        char* username = strtok(bodyChar, "=");
        username = strtok(nullptr, "=");
        if(username == nullptr) {
            cout << "Error: username not provided" << endl;
            return 1;
        }
        string usernameString(username);
        nlohmann::json jsonDatabaseResult;

        // Bind result
        MYSQL_BIND userParam[2];
        memset(userParam, 0, sizeof(userParam));
        userParam[0].buffer_type = MYSQL_TYPE_STRING;
        userParam[0].buffer = (char *)usernameString.c_str();
        userParam[0].buffer_length = usernameString.size();
        userParam[1].buffer_type = MYSQL_TYPE_STRING;
        userParam[1].buffer = (char *)usernameString.c_str();
        userParam[1].buffer_length = usernameString.size();
        mysql_stmt_bind_param(getAllUserTransactionsStmt, userParam);

        // Bind result
        char sender[100];
        char receiver[100];
        char publish_date[100];
        float amount;
        signed char currency;
        MYSQL_BIND result[5];
        memset(result, 0, sizeof(result));
        result[0].buffer_type = MYSQL_TYPE_STRING;
        result[0].buffer = (char *)&sender;
        result[0].buffer_length = 100;
        result[1].buffer_type = MYSQL_TYPE_STRING;
        result[1].buffer = (char *)&receiver;
        result[1].buffer_length = 100;
        result[2].buffer_type = MYSQL_TYPE_FLOAT;
        result[2].buffer = (char *)&amount;
        result[3].buffer_type = MYSQL_TYPE_TINY;
        result[3].buffer = (char *)&currency;
        result[4].buffer_type = MYSQL_TYPE_STRING;
        result[4].buffer = publish_date;
        result[4].buffer_length = sizeof(publish_date);
        mysql_stmt_bind_result(getAllUserTransactionsStmt, result);

        // Execute the query
        if (mysql_stmt_execute(getAllUserTransactionsStmt)) {
            cerr << "Error: Statement execute failed " << endl;
            return 1;
        }

        // Fetch and print results
        while (mysql_stmt_fetch(getAllUserTransactionsStmt) == 0) {
            jsonDatabaseResult.push_back({sender, receiver, amount, currency});
        }

        cout << jsonDatabaseResult.dump() << endl;
        return 0;
    }

    // Extract username from input
    size_t pos = input.find("username=");
    string username;
    if (pos != string::npos) {
        username = input.substr(pos + 9);
	if(!regex_match(username, userpattern)){
		cerr << " Invalid data" << endl;
		return 1;
	}
    } else {
        cout << "Error: username not provided" << endl;
        return 1;
    }

    // Prepare the query
    MYSQL_STMT *stmt;
    char query[] = "SELECT balance, currency FROM Balance WHERE username=?";
    stmt = mysql_stmt_init(conn);
    if (!stmt) {
        cerr << "Error: Statement init failed " << endl;
        return 1;
    }

    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        cerr << "Error: Statement prepare failed " << endl;
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
        cerr << "Error: Statement execute failed " << endl;
        return 1;
    }

    // Fetch and print results
    if (mysql_stmt_fetch(stmt) == 0) {
        // cout << "Username: " << username << "\n";
        // cout << "Balance: " << balance << "\n";
        // cout << "Currency: " << (currency == 1 ? "Churricoin" : "Euro") << "\n";

        // Converting response to json
        nlohmann::json jsonDatabaseResult = {
            {"Username", username},
            {"Balance", balance},
            {"Currency", (currency == 1 ? "Churricoin" : "Euro")}
        };
        cout << jsonDatabaseResult.dump() << endl;
    } else {
        cout << "Error: No data found" << endl;
    }

    mysql_stmt_close(stmt);
    mysql_close(conn);

    return 0;
}
