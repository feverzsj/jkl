#include <iostream>
#include <jkl/mysql/env.hpp>
#include <jkl/mysql/conn.hpp>
#include <jkl/mysql/stmt.hpp>


using namespace std;
using namespace jkl;


int main()
try{
    mysql_env env;

    mysql_conn sql;

    if(! sql.connect())
    {
        cout << "Failed to connect to server: " << sql.err_str() << endl;
        return -1;
    }

    sql.query("CREATE DATABASE IF NOT EXISTS testdb");
    sql.query("DROP TABLE IF EXISTS Cars");
    sql.query("CREATE TABLE Cars(Id INT, Name TEXT, Price INT)");


    mysql_stmt stmt{sql};
    stmt.prepare("INSERT INTO Cars VALUES(?,?,?)");

    stmt.exec(1, "Audi"      , 52642 );
    stmt.exec(2, "Mercedes"  , 57127 );
    stmt.exec(3, "Skoda"     , 9000  );
    stmt.exec(4, "Volvo"     , 29000 );
    stmt.exec(5, "Bentley"   , 350000);
    stmt.exec(6, "Citroen"   , 21000 );
    stmt.exec(7, "Hummer"    , 41400 );
    stmt.exec(8, "Volkswagen", 21600 );

    int id;
    std::string name;
    int price;

    stmt.prepare("SELECT * FROM Cars");
    stmt.exec();
    // stmt.store_result(); // optional
    
    while(stmt.fetch(id, name, price))
    {
        cout << id << ", " << name << ", " << price << endl;
    }

    sql.query("SELECT * FROM Cars");
    
    mysql_step_res sr = sql.step_result();

    while(char** row = sr.fetch_row())
    {
        cout << row[0] << ", " << row[1] << ", " << row[2] << endl;
    }

    mysql_full_res fr = sql.full_result();
    while(char** row = fr.fetch_row())
    {
        cout << row[0] << ", " << row[1] << ", " << row[2] << endl;
    }

}
catch(std::exception const& e)
{
    cout << e.what() << endl;
}
catch(...)
{
    cout << "unknown exception" << endl;
}