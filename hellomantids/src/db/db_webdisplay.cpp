#include "db.h"

#include "../globals.h"
#include <mdz_thr_mutex/lock_shared.h>

#include <mdz_mem_vars/a_string.h>
#include <mdz_mem_vars/a_datetime.h>
#include <mdz_mem_vars/a_uint32.h>

#include <mdz_mem_vars/a_var.h>
#include <arpa/inet.h>


using namespace Mantids::Memory;
using namespace Mantids::Database;
using namespace Mantids::Authentication;
using namespace Mantids::Application;


json DB::getMessages(Manager *auth, Session *sess, const json &)
{
    json  ret;
    auto connectionKeys = Globals::getFastRPC()->getConnectionKeys();

    Abstract::UINT32 id;
    Abstract::STRING message,createdBy,creationDate;

    Mantids::Threads::Sync::Lock_RD lock(mutex);

    std::shared_ptr<QueryInstance> i = db.query("SELECT `id`,`message`,`createdBy`,`creationDate` FROM posts;",
                               {
                               },
                               { &id, &message , &createdBy,&creationDate});
    int j=0;
    while (i->ok && i->query->step())
    {

        ret[j]["id"] = id.getValue();
        ret[j]["message"] = message.getValue();
        ret[j]["createdBy"] = createdBy.getValue();
        ret[j]["creationDate"] = creationDate.getValue();

        j++;
    }
    return ret;
}

json DB::getAdd1(Mantids::Authentication::Manager *auth, Mantids::Authentication::Session *sess, const json &payload)
{
    uint32_t x = JSON_ASUINT(payload,"value",0);
    return  x + 1;
}
