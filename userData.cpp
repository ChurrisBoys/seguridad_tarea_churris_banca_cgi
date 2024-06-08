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
        cerr << "Error: Statement init failed: " << mysql_error(conn) << endl;
        exit(1);
    }

    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        cerr << "Error: Statement prepare failed: " << mysql_stmt_error(stmt) << endl;
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
    string input;
    getline(cin, input); // Read input from stdin

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

    // If the action(a) equals CT(CreateTransaction) then create a transaction
    if(urlAction == "CT") {
        // GET and Validate data
        char* bodyChar = (char*) input.c_str();

        char* delimiter = "&";
        char* bodyTokensChars = strtok(bodyChar, delimiter);

        string intendedSender;
        string intendedReceiver;
        float intendedAmountToTransfer;
        float sendersMoney;

        // Prepare the query
        auto makeTransactionStmt = makeSqlStatement(conn, "INSERT INTO Transactions (sender,receiver,amount,currency_kind) VALUES (?, ?, ?, ?)");
        MYSQL_BIND param[4];
        memset(param, 0, sizeof(param));
        int i = 0;

        // Getting all the data in the body of the HTTP request
        while (bodyTokensChars != nullptr) {
            // Bind parameters
            param[i].buffer_type = MYSQL_TYPE_STRING;
            param[i].buffer = bodyTokensChars;
            param[i].buffer_length = strlen(bodyTokensChars);
            if(i == 0)
                intendedSender = string(bodyTokensChars);
            else if(i == 1)
                intendedReceiver = string(bodyTokensChars);
            else if(i == 2)
                intendedAmountToTransfer = atof(bodyTokensChars);
            ++i;
            bodyTokensChars = strtok(nullptr, delimiter);
        }
        mysql_stmt_bind_param(makeTransactionStmt, param);

        //===========================  Check if the sender has enough money to make the transfer
        auto enoughMoneyStmt = makeSqlStatement(conn,  "SELECT b.balance FROM Balance b WHERE b.username = ?");

        // Bind result
        MYSQL_BIND enoughMoneyParam[1];
        memset(enoughMoneyParam, 0, sizeof(enoughMoneyParam));
        enoughMoneyParam[0].buffer_type = MYSQL_TYPE_STRING;
        enoughMoneyParam[0].buffer = (char*) intendedSender.c_str();
        enoughMoneyParam[0].buffer_length = intendedSender.size();
        mysql_stmt_bind_param(enoughMoneyStmt, enoughMoneyParam);

        // Bind result
        MYSQL_BIND enoughMoneyResult[1];
        memset(enoughMoneyResult, 0, sizeof(enoughMoneyResult));
        enoughMoneyResult[0].buffer_type = MYSQL_TYPE_FLOAT;
        enoughMoneyResult[0].buffer = (char *)&sendersMoney;
        mysql_stmt_bind_result(enoughMoneyStmt, enoughMoneyResult);

        // Execute the query
        if (mysql_stmt_execute(enoughMoneyStmt)) {
            cerr << "Error: Statement execute failed " << endl;
            return 1;
        }

        // Fetch and print results
        if (mysql_stmt_fetch(enoughMoneyStmt) == 0) {
            if(sendersMoney < intendedAmountToTransfer) {
                cout << "Error: not enough money to transfer" << endl;
                return 0;
            }
            // Important: Cleaning mysql to let him be able to perform more sql queries
            mysql_stmt_free_result(enoughMoneyStmt);
        } else {
            cout << "Error: The sender of the transfer is invalid" << endl;
            return 1;
        }
        //=====

        // Bind result
        char sender[100];
        char receiver[100];
        float moneyToTransfer;
        signed char currency;
        MYSQL_BIND result[4];
        memset(result, 0, sizeof(result));
        result[0].buffer_type = MYSQL_TYPE_STRING;
        result[0].buffer = (char *)&sender;
        result[0].buffer_length = 100;

        result[1].buffer_type = MYSQL_TYPE_STRING;
        result[1].buffer = (char *)&receiver;
        result[1].buffer_length = 100;

        result[2].buffer_type = MYSQL_TYPE_FLOAT;
        result[2].buffer = (char *)&moneyToTransfer;

        result[3].buffer_type = MYSQL_TYPE_TINY;
        result[3].buffer = (char *)&currency;
        mysql_stmt_bind_result(makeTransactionStmt, result);

        // Execute the query
        if (mysql_stmt_execute(makeTransactionStmt)) {
            cout << "Error: Statement execute failed: " << "Probably the receiver of the transfer is not valid" << endl;
            return 1;
        }
        //===========================  If the above query execute was succesful, then Reduce the Balance from the sender and increment the one from the receiver
            // TODO: Make conversion of currency
        string sqlReceiverBaseMoney = "SELECT b.balance FROM Balance b WHERE b.username='" + intendedReceiver + "'";
        float receiversMoney;
        if (mysql_query(conn, sqlReceiverBaseMoney.c_str()) == 1) {
            cout << "Error: Balance substraction failed " << endl;
            return 1;
        }
        // Store the result set
        MYSQL_RES *res = mysql_store_result(conn);
        if (res == NULL) {
            cout << "Error: Store result failed " << endl;
            return 1;
        }
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            receiversMoney = atof(row[0]);
        }

        string sqlUpdateSenderQuery = "UPDATE Balance SET balance=" + to_string(sendersMoney-intendedAmountToTransfer) + " WHERE username=" + "'" + intendedSender + "'";
        string sqlUpdateReceiverQuery = "UPDATE Balance SET balance=" + to_string(receiversMoney+intendedAmountToTransfer) + " WHERE username=" + "'" + intendedReceiver + "'";
        if (mysql_query(conn, sqlUpdateSenderQuery.c_str()) == 1) {
            cout << "Error: Balance substraction failed " << endl;
            return 1;
        }
        if (mysql_query(conn, sqlUpdateReceiverQuery.c_str()) == 1) {
            cout << "Error: Balance addition failed "<< endl;
            return 1;
        }
        //======

        cout << "Transaction succesful" << endl;
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
    std::regex userpattern("^[a-zA-Z]{1,80}(\.[a-zA-Z]{1,80})?$"); 
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
