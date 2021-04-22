#include "../include/mysql_helper.h"


int main() {
    ConnParam param = {"localhost", "db_user", "password", "com_db"};
    creatTransactionDb("data/transactions.csv", "transactions", param);
    creatAccountInfoDb("data/account_info.csv", "account_info", param);
    fraudOnTransactionAmount(param);
    std::cout << std::endl;
    fraudOnTransactionLocation(param);
    return 0;
}
