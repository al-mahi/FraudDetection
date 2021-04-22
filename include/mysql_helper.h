#include <mysql/mysql.h>
#include <iostream>
#include <memory>
#include <assert.h>
#include <cstring>
#include <fstream>
#include <vector>
#include <iomanip>

#ifndef FRAUDDETECTION_MYSQL_HELPER_H
#define FRAUDDETECTION_MYSQL_HELPER_H

struct ConnParam {
    const char *host, *user, *passwd, *database;
};


class MySQLHelper {
private:
    const char *host, *user, *passwd, *database;
    MYSQL *connection;
public:
    MySQLHelper(const char *host, const char *user, const char *passwd, const char *database)
            : host(host), user(user), passwd(passwd), database(database), connection(mysql_init(NULL)) {
    }

    virtual ~MySQLHelper() {
        if (connection->free_me)
            mysql_close(connection);
    }


    int connect() {
        connection = mysql_real_connect(connection, host, user, passwd, database, 0, NULL,
                                        0);
        if (connection == NULL) {
            std::cout << "MySQL Connection Error: " << mysql_error(connection) << std::endl;
            return -1;
        }
        return 1;
    }

    void queryResult(std::string path) {
        std::fstream outFile{path, std::ios::out | std::ios::app};
        MYSQL_RES *res = mysql_use_result(connection);
        int col = mysql_num_fields(res);
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != NULL) {
            for (auto i = 0; i < col; ++i){
                std::cout << std::left << std::setw(30) << row[i] << " ";
                outFile << std::left << std::setw(30) << row[i] << " ";
            }
            std::cout << std::endl;
            outFile << std::endl;
        }
        mysql_free_result(res);
        outFile.close();
    }


    int queryAndDump(const std::string queryStr, const std::string path) {
        if (!connect()) {
            std::cout << "MySQL Could not connect: " << mysql_error(connection) << std::endl;
            return -1;
        }
        if (mysql_query(connection, queryStr.c_str())) {
            std::cout << "MySQL Query Error: " << mysql_error(connection) << std::endl;
            return -1;
        }
        queryResult(path);

        return 1;
    }


    int insertQuery(const std::string values, const std::string tableName, bool verbose) {
        auto queryStr = "insert into " + tableName + " values(" + values + " );";

//        std::cout << queryStr << std::endl;
        if (!connect()) {
            std::cout << "MySQL Could not connect: " << mysql_error(connection) << std::endl;
            return -1;
        }
        if (mysql_query(connection, queryStr.c_str())) {
            std::cout << "MySQL Query Error: " << mysql_error(connection) << std::endl;
            std::cout << "            Query: " << queryStr << std::endl;
            return -1;
        }
        if (verbose) {
            auto n = mysql_affected_rows(connection);
            std::cout << "MySQL Rows affected: " << n << std::endl;
        }
        return 1;
    }

    int createTableQuery(const std::string tableName, const std::string columns, bool verbose) {
        auto queryStr = "create table " + tableName + " ( " + columns + " );";
        if (!connect()) {
            std::cout << "MySQL Could not connect: " << mysql_error(connection) << std::endl;
            return -1;
        }
        if (mysql_query(connection, queryStr.c_str())) {
            std::cout << "MySQL Query: " << mysql_error(connection) << std::endl;
            std::cout << "            Query: " << queryStr << std::endl;
            return mysql_errno(connection);
        }

        return 1;

    }
};

std::string stripStr(std::string str) {
    int len = str.size();
    int li = 0, ri = str.size() - 1;
    while (li < str.size() and (str[li] == ' ' or str[li] == '\n' or str[li] == '\r')) {
        li++;
        len--;
    }
    if (len <= 0) return "";
    while (ri > 0 and (str[ri] == ' ' or str[ri] == '\n' or str[li] == '\r')) {
        ri--;
        len--;
    }
    if (len <= 0) return "";;
    return str.substr(li, len);
}

std::string wrapQuote(std::string str) {
    return "\"" + str + "\"";
}

std::vector<std::string> split(std::string str, char delimiter = ' ') {
    std::vector<std::string> tokens;
    std::string token;

    for (const auto &c: str) {
        if (c != delimiter)
            token += c;
        else {
            token = stripStr(token);
            tokens.push_back(token);
            token.clear();
        }
    }
    tokens.push_back(token);
    return tokens;
}

int creatTransactionDb(std::string pathCSV, std::string dbTable, ConnParam connParam) {
    /***
     * select tv1.account_number, tv1.transaction_amount, tv1.transaction_type, stdTable.s double(10.2), stdTable.a double(10.2) from tv1 inner join (select account_number, std(transaction_amount) as s, avg(transaction_amount) as a from tv1 where transaction_type = 'debit'  group by account_number) stdTable on tv1.account_number = stdTable.account_number where tv1.transaction_amount > stdTable.a+ 2*stdTable.s or tv1.transaction_amount < stdTable.a - 2 * stdTable.s;
     */

    auto tabCreationHelper = std::unique_ptr<MySQLHelper>(
            new MySQLHelper(connParam.host, connParam.user, connParam.passwd, connParam.database));

    std::string cols = "account_number int not null,"
                       "transaction_datetime datetime,"
                       "transaction_amount double(10,2) not null,"
                       "transaction_type varchar(255),"
                       "post_date date,"
                       "merchant_number varchar(255),"
                       "merchant_name varchar(255),"
                       "merchant_state varchar(255) not null,"
                       "merchant_category int,"
                       "tran_number int not null,"
                       "PRIMARY KEY(account_number, tran_number)";

    auto ok = tabCreationHelper->createTableQuery(dbTable.c_str(), cols, false);

    if (ok != 1 and ok != 1050) {
        std::cout << "MySQL Table creation error: " << std::endl;
        return -1;
    }

    std::string line;
    std::ifstream is;
    is.open(pathCSV);

    if (!is.is_open()) {
        perror("Error open");
        return -1;
    }
    if (ok == 1) {
        getline(is, line);
        while (getline(is, line)) {
//            std::cout << line << std::endl;
            auto ws = split(line, ',');
            std::string values = "";
            for (int i = 0; i < 8; i++) {
                if (i == 1) {
                    if (ws[i].length() > 8) {
                        std::string date = ws[i].substr(4, 4) + "-" + ws[i].substr(0, 2) + "-" + ws[i].substr(2, 2);
                        std::string time = ws[i].substr(9, 9);
                        values += (wrapQuote(date + " " + time) + ",");
                    }
                } else if (i == 2 and ws[i].size() > 0) {
                    if (ws[i][ws[i].size() - 1] == '-')
                        values += (wrapQuote(ws[i].substr(0, ws[i].size() - 1)) + "," + wrapQuote("debit") + ",");
                    else if (ws[i][ws[i].size() - 1] == '+')
                        values += (ws[i].substr(0, ws[i].size() - 1) + "," + wrapQuote("deposit") + ",");
                    else
                        values += (wrapQuote(ws[i]) + "," + wrapQuote("debit") + ",");
                } else if (i == 3) {
                    if (ws[i].length() == 7) ws[i] = "0" + ws[i];
                    if (ws[i].length()) {
                        std::string date = ws[i].substr(4, 4) + "-" + ws[i].substr(0, 2) + "-" + ws[i].substr(2, 2);
                        values += (wrapQuote(date) + ",");
                    }
                } else if (i == 5) {
//                values += (wrapQuote(ws[i]) + ",");
                    std::string mrdesc = ws[i];
                    std::string state = "";
                    int len = mrdesc.length();
                    if (mrdesc.substr(len - 2, 2) != "US")
                        state = mrdesc.substr(len - 2, 2);
                    else
                        state = mrdesc.substr(len - 4, 2);
                    int ri = len;
                    while (len) {
                        len--;
                        if (mrdesc[len] == ' ') ri = len;
                    }
                    std::string merchant = mrdesc.substr(0, ri);
                    values += wrapQuote(merchant) + "," + wrapQuote(state) + ",";

                } else if (i == 7) {
                    values += wrapQuote(ws[i].substr(0, ws[i].length() - 1));
                } else {
                    values += (wrapQuote(ws[i]) + ",");
                }
            }

            auto insertHelper = std::unique_ptr<MySQLHelper>(
                    new MySQLHelper(connParam.host, connParam.user, connParam.passwd, connParam.database));
            insertHelper->insertQuery(values.c_str(), dbTable.c_str(), false);
        }
//            std::cout << std::endl;
// select account_number, std(transaction_amount) from tv1 where transaction_type = 'debit'  group by account_number;
    }

    return 0;
}

int creatAccountInfoDb(std::string pathCSV, std::string dbTable, ConnParam connParam) {

    auto tabCreationHelper = std::unique_ptr<MySQLHelper>(
            new MySQLHelper(connParam.host, connParam.user, connParam.passwd, connParam.database));

    std::string cols = "Name varchar(255),"
                       "street_address varchar(255),"
                       "unit varchar(255),"
                       "city varchar(255),"
                       "state varchar(255),"
                       "zip varchar(255),"
                       "dob varchar(255),"
                       "ssn varchar(255),"
                       "email_address varchar(255),"
                       "mobile_number varchar(255),"
                       "account_number int not null,"
                       "primary key(account_number)";

    auto ok = tabCreationHelper->createTableQuery(dbTable.c_str(), cols, false);

    if (ok != 1 and ok != 1050) {
        std::cout << "MySQL Table creation error: " << std::endl;
        return -1;
    }

    std::string line;
    std::ifstream is;
    is.open(pathCSV);

    if (!is.is_open()) {
        perror("Error open");
        return -1;
    }
    if (ok == 1) {
        getline(is, line);
        while (getline(is, line)) {
            int len = line.length();
            while (line[len-1]=='\n' or line[len-1]=='\r') len--;
            line = line.substr(0, len);

            auto ws = split(line, ',');
            std::string values = wrapQuote(ws[1] + " " + ws[0]) + ",";
            for (int i = 2; i < 12; i++) {
                    values += wrapQuote(ws[i]);
                    if(i<11)
                        values += ",";
            }

            auto insertHelper = std::unique_ptr<MySQLHelper>(
                    new MySQLHelper(connParam.host, connParam.user, connParam.passwd, connParam.database));
            insertHelper->insertQuery(values.c_str(), dbTable.c_str(), false);
        }
    }

    return 0;
}


void fraudOnTransactionAmount(ConnParam connParam){
    std::string path = "file1.txt";
    std::fstream outFile{path, std::ios::out};
    for(auto& col : {"Name", "Account Number", "Transaction Number", "Merchant","Transaction Amount" }){
        std::cout << std::left << std::setw(30) << col << " ";
        outFile << std::left << std::setw(30) << col << " ";
    }
    std::cout << std::endl;
    outFile << std::endl;
    outFile.close();

    std::string fraudTransAmountQuery = "select account_info.Name as Name,\n"
                                        "    account_info.account_number as AccountNumber,\n"
                                        "    transactions.tran_number as TransactionNumber,\n"
                                        "    transactions.merchant_name as Merchant,\n"
                                        "    transactions.transaction_amount as TransactionAmount\n"
                                        "from transactions\n"
                                        "inner join account_info\n"
                                        "on transactions.account_number = account_info.account_number\n"
                                        "    inner join (\n"
                                        "        select account_number,\n"
                                        "            std(transaction_amount) as s,\n"
                                        "            avg(transaction_amount) as a\n"
                                        "        from transactions\n"
                                        "        where transaction_type = 'debit'\n"
                                        "        group by account_number\n"
                                        "    ) stdTableQuery on transactions.account_number = stdTableQuery.account_number\n"
                                        "where transactions.transaction_amount > stdTableQuery.a + 2 * stdTableQuery.s\n"
                                        "    or transactions.transaction_amount < stdTableQuery.a - 2 * stdTableQuery.s;";

    auto qHelper = std::unique_ptr<MySQLHelper>(
            new MySQLHelper(connParam.host, connParam.user, connParam.passwd, connParam.database));
    qHelper->queryAndDump(fraudTransAmountQuery, path);

}


void fraudOnTransactionLocation(ConnParam connParam){
    std::string path = "file2.txt";
    std::fstream outFile{path, std::ios::out};
    for(auto& col : {"Name", "Account Number", "Transaction Number", "Expected Transaction Location","Actual Transaction Location" }){
        std::cout << std::left << std::setw(30) << col << " ";
        outFile << std::left << std::setw(30) << col << " ";
    }
    std::cout << std::endl;
    outFile << std::endl;
    outFile.close();

    std::string fraudTransLocQuery = "select account_info.Name as Name,\n"
                                     "    account_info.account_number as AccountNumber,\n"
                                     "    transactions.tran_number as TransactionNumber,\n"
                                     "    account_info.state as ExpectedState,\n"
                                     "    transactions.merchant_state as ActualState\n"
                                     "    from transactions\n"
                                     "    inner join account_info on transactions.account_number = account_info.account_number\n"
                                     "where transactions.merchant_state != account_info.state;";
    auto qHelper = std::unique_ptr<MySQLHelper>(
            new MySQLHelper(connParam.host, connParam.user, connParam.passwd, connParam.database));
    qHelper->queryAndDump(fraudTransLocQuery,path);


}

#endif //FRAUDDETECTION_MYSQL_HELPER_H
