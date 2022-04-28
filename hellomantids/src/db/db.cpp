#include "db.h"

#include <mdz_thr_mutex/lock_shared.h>

#include <mdz_mem_vars/a_string.h>
#include <mdz_mem_vars/a_datetime.h>
#include <mdz_mem_vars/a_bool.h>
#include <mdz_mem_vars/a_int32.h>
#include <mdz_mem_vars/a_uint32.h>
#include <mdz_mem_vars/a_int64.h>
#include <mdz_mem_vars/a_uint16.h>

#include <mdz_mem_vars/a_var.h>
#include <arpa/inet.h>

#include "../globals.h"

using namespace Mantids::Memory;
using namespace Mantids::Database;
using namespace Mantids::Authentication;
using namespace Mantids::Application;

std::atomic<bool> DB::statusOK;
std::string DB::dbName;
std::string DB::dbHost;
uint16_t DB::dbPort;
AuthData DB::dbAuth;

Mantids::Threads::Sync::Mutex_Shared DB::mutex;
Mantids::Database::SQLConnector_MariaDB DB::db;

bool DB::start()
{
    dbHost = Globals::getLC_Database_Host();
    dbName = Globals::getLC_Database_DbName();
    dbPort = Globals::getLC_Database_Port();
    dbAuth.setUser(Globals::getLC_Database_DbUser());
    dbAuth.setPass(Globals::getLC_Database_DbPass());

    statusOK = db.connect(dbHost, dbPort, dbAuth, dbName);
    
    if (statusOK)
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO, "Succesfully connected to MySQL Database server '%s' ", dbHost.c_str());
    else
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_CRITICAL, "Failed to connect to the database: %s @%s:%d", db.getDBName().c_str(), db.getDBHostname().c_str(), db.getDBPort());
        return false;
    }

    if (!initSchema())
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Failed to initialize schema @ MySQL Database");
        return false;
    }

    return statusOK;
}

bool DB::getStatusOK()
{
    return statusOK;
}


bool DB::initSchema()
{
    bool r = true;

    if (r && !db.dbTableExist("posts"))
    {
        if (! db.query("CREATE TABLE `posts` (\n"
                       "       `id`           INTEGER PRIMARY KEY AUTO_INCREMENT,\n"
                       "       `message`           VARCHAR(1024)    NOT NULL,\n"
                       "       `createdBy`         VARCHAR(256)    DEFAULT NULL,\n"
                       "       `creationDate`      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP\n"
                       ");\n"))
            r=false;
    }

    return r;
}

json DB::runLocalRPCMethod(const std::string &methodName,  Manager *auth, Session *sess, const json &payload, json *error)
{
    json r;

    if (0==1) {}
    // MESSAGES:
    else if ( methodName == "stats.getAdd1" ) r = getAdd1(auth,sess,payload);
    else if ( methodName == "stats.getMessages" ) r = getMessages(auth,sess,payload);
    else if ( methodName == "control.createMessage" ) r = createMessage(auth,sess,payload);
    else if ( methodName == "control.removeMessage" ) r = removeMessage(auth,sess,payload);

    else
    {
        if (error)
        {
            (*error)["succeed"] = false;
            (*error)["errorId"] = 1;
            (*error)["errorMessage"] = "Method Not Found.";
        }
    }

    if (error)
    {
        (*error)["succeed"] = true;
        (*error)["errorId"] = 0;
        (*error)["errorMessage"] = "Execution OK.";
    }

    return r;
}

