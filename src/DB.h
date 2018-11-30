#ifndef _DB_H_
#define _DB_H_

#include <libconfig.h++>
#include "mysql_connection.h"
#include "mysql_driver.h"
#include "mysql_error.h"


#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

#include "Logging.h"
#include "TargetInfo.h"
#include "FrameInfo.h"

class Target;

class DB: public Logging {
public:
    DB();
    ~DB();
    bool start();
    int init(libconfig::Config& config);
    int writeTarget(TargetInfo& t);
    int writeRegionInfo(FrameInfo& frameInfo, std::string channel);
private:
    sql::ConnectOptionsMap connection_properties;
    DB(const DB &);  
    DB & operator = (const DB &);
    sql::mysql::MySQL_Driver *driver;
    sql::Connection *con = NULL;
    sql::PreparedStatement  *insertIntoEvent;
    sql::PreparedStatement  *insertIntoVehicle;
    sql::PreparedStatement  *updateVehicle;
    sql::PreparedStatement  *regionInsertUpdate;
};
#endif