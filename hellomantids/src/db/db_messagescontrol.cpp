#include "db.h"

#include "../globals.h"
#include <mdz_thr_mutex/lock_shared.h>

#include <mdz_mem_vars/a_string.h>
#include <mdz_mem_vars/a_uint32.h>

#include <mdz_mem_vars/a_var.h>
#include <arpa/inet.h>

#include <inttypes.h>

using namespace Mantids::Memory;
using namespace Mantids::Database;
using namespace Mantids::Authentication;
using namespace Mantids::Application;

json DB::createMessage(Mantids::Authentication::Manager *auth, Mantids::Authentication::Session *sess, const json &payload)
{
    std::string lastSQLError;

    std::string message = JSON_ASSTRING(payload,"message","");

    {
        Mantids::Threads::Sync::Lock_RW lock(mutex);

        Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO, "User '%s' is adding message '%s'", sess->getAuthUser().c_str(),message.c_str());

        if (!
                db.query(&lastSQLError,"INSERT INTO posts (`message`,`createdBy`) VALUES(:message,:createdBy);",
                                        {
                                            {":message",new Abstract::STRING(message)},
                                            {":createdBy",new Abstract::STRING(sess->getAuthUser())}
                                        }))
        {
            Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Can't add post message '%s': %s",message.c_str(),  lastSQLError.c_str() );
            return false;
        }
    }
    return true;
}

json DB::removeMessage(Manager *auth, Session *sess, const json &payload)
{
    uint32_t postId = JSON_ASUINT(payload,"id",0);
    Mantids::Threads::Sync::Lock_RW lock(mutex);
    std::string lastSQLError;

    Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO, "User '%s' is removing node '%" PRIu32 "'", sess->getAuthUser().c_str(),postId);

    if (!db.query(&lastSQLError,"DELETE FROM posts WHERE `id`=:id;",
                                {
                                   {":id",new Abstract::UINT32(postId)}
                                }))
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Can't remove post '%" PRIu32 "': %s",postId,  lastSQLError.c_str() );

        return false;
    }

    return true;
}
