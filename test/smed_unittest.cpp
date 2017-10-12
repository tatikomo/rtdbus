// file      : smed_unittest.cxx

#include <memory>   // std::auto_ptr
#include <iostream>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

#include "odb/database.hxx"
#include "odb/session.hxx"
#include "odb/transaction.hxx"

//
#include "exchange_smed_database.hxx" // create_database

//#include "smed.hxx"
#include "smed-odb.hxx"

using namespace std;
using namespace odb::core;

#define OK 0
#define NOK 1

int num_arg = 3;
char* val_arg[] = {
    "smed_test",
    "--database",
    "SMED.sqlite"
};

void print_site (const SITES& e)
{
  cout << "site: " << e.name () << endl;
}

void print_data (const DATA& e)
{
  cout << "data: " << e.tag() << " led:" << e.led() << endl;
}

void print_fl (const FIELDS_LINK& e)
{
  cout << "fl: " << e.get_link_id() << " lu:" << e.last_update() << " ns:" << e.need_sync() << endl;
}

int main(int argc, char* argv[])
{
  int status = OK;
  LOG(INFO) << "SMED TEST START";

  try {

    typedef odb::query<SITES> query_site;
    typedef odb::result<SITES> result_site;
    typedef odb::query<DATA> query_data;
    typedef odb::result<DATA> result_data;
    typedef odb::query<FIELDS_LINK> query_fl;
    typedef odb::result<FIELDS_LINK> result_fl;

#if 1
    // NB: внутри вызывается schema_catalog::create_schema(), которая очищает используемые таблицы
    unique_ptr<database> db ( create_database (num_arg, val_arg) );
#else
    // Создаём БД напрямую, поскольку очищать содержимое не нужно, т.к. оно заполнено предыдущими тестами
    unique_ptr<database> db ( new odb::sqlite::database (
      num_arg /*argc*/, val_arg /*argv*/, false, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE ));
#endif
    {
        {
            transaction t (db->begin ());
//#if (VERBOSE>6)
              t.tracer (odb::stderr_full_tracer);
//#endif

            result_site rs (db->query<SITES> (query_site::name == "K45001"));
            //if (!db->query_one<SITES> (query_site::name == "K45001")) {
            if (!rs.empty()) {

              shared_ptr<SITES> YTG (new SITES ("K45001", 0, 0, 0, 0, 0));

              unsigned long ytg_ref = db->persist (*YTG);
              LOG(INFO) << "ytg_ref=" << ytg_ref;

            }
            else {
                LOG(WARNING) << "YTG site already exist";
            }
            t.commit ();
        }

        {

            // We use a session in this and subsequent transactions to make sure
            // that a single instance of any particular object (e.g., employer) is
            // shared among all objects (e.g., SITES) that relate to it.
            //
            {
              session s;
              const std::string looked = "BI4001";
              const std::string tags_template = "/KD2052/%";
              transaction t (db->begin ());
//#if (VERBOSE>6)
              t.tracer (odb::stderr_full_tracer);
//#endif
              // <=============== Получить экземпляр DATA по идентификатору
              unsigned long data_id = 1;
              LOG(INFO) << "Get the DATA element by id=" << data_id;
              shared_ptr<DATA> data (db->load<DATA> (data_id));
              if (data) {
                LOG(INFO) << "DATA(" << data_id << "): " << data->tag() << " LED=" << data->led() << " ID=" << data->id();
                //result_fl r (db->query<FIELDS_LINK> (query_fl::data() == data->id()));
                result_fl r (db->query<FIELDS_LINK> (query_fl::need_sync == 1));
                if (!r.empty()) {
                  for (result_fl::iterator i (r.begin ()); i != r.end (); ++i) {
//                    i.load();
                    print_fl (*i);
                  }
                }
                else {
                  LOG(ERROR) << "There is no FIELDS_LINK";
                }
              }
              else {
                LOG(ERROR) << "There is no DATA(1)";
              }

              // <=============== Получить экземпляр SITES по названию
              LOG(INFO) << "Get the SITE element by name=" << looked;
              shared_ptr<SITES> found_site (db->query_one<SITES> (query_site::name == looked));
              if (found_site) {
                print_site(*found_site);
              }
              else {
                LOG(ERROR) << "There is no " << looked << " site";
              }

              // <=============== Найти DATA по шаблону тега
              LOG(INFO) << "Get the DATA element(s) by tag=" << tags_template;
              result_data r (db->query<DATA> ( query_data::tag.like(tags_template) ));
              if (!r.empty()) {
                for (result_data::iterator i (r.begin ()); i != r.end (); ++i)
                  print_data (*i);
              }
              else {
                  LOG(ERROR) << "There is no DATA with tags like " << tags_template;
              }

              t.commit ();
            }
        }
    }
  }
  catch (const odb::exception& e)
  {
    LOG(ERROR) << e.what ();
    status = NOK;
  }
  catch(...) {
    LOG(ERROR) << "Fail to process database test";
  }

  LOG(INFO) << "SMED TEST DONE";

  return status;
}

