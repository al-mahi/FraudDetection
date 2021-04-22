##### Introduction

This is a commandline program written in C++. This program imports data from CSV file to 
mysql database. Then uses two simple rules to detect fradulent transactions.

###### Rule 1
If a transaction by certain user is more than 2 standard deviation of it's transactions 
then it is flagged as fradulent. Such information is printed in screen and saved in file1.txt.
Following is an example of such transactions.

Name                           Account Number                 Transaction Number             Merchant                       Transaction Amount             
Michael Smith                  11111                          15                             KROGER                         2300.00                        
Kolmetz Willard                14281                          14                             VZWRLSS*APOCC                  351.05                         
Albares Cammy                  14829                          3                              STADIUM                        348.40                         
Albares Cammy                  14829                          4                              POSTO                          264.19                         


###### Rule 2
Rule 2 marks the transactions happened in a state other than where the person registered its account.
Results are written in file2.txt and also printed on screen.

The rules have been implemented using SQL queries which can be found in rule*.txt file.


A additional dump file of the database is added com_db_dump.sql file. 

###### Build

This program is written and built in Ubuntu linux.
FindMySQL script has been used to easily find out sql path by CMake.
Reference: https://gitlab.kitware.com/cmake/community/-/wikis/contrib/modules/FindMySQL
Install mysqlclient library using the command in debian linux
sudo apt install libmysqlclient-dev

I have put the library headers in the misc folder in case someone is interested to look into it.
This headers usually found in /usr/include/mysql after installing the libarary.
The library has been linked using CMake command target_link_libraries(fraud_detector ${MYSQL_LIBRARY})
where ${MYSQL_LIBRARY} variable has been defined in FindMySQL.cmake

For windows similar methods can be used. But I have not tested on widows
g++ myprog.cpp \
-I"C:\Program Files (x86)\MySQL\MySQL Connector C 6.1\include" \
-L"C:\Program Files\MySQL\MySQL Connector C 6.1\lib" -lmysql

It is CMake project. To make create directory to store the build files.
FradudDetection$ mkdir build
FradudDetection$cd build
build$ cmake ..
build$ make
FradudDetection$ ./build/fraud_detector