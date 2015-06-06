#include <iostream>
#include <pqxx/pqxx>
#include <numeric>
#include <chrono>
#include <regex>

#include <thread>
#include <mutex>
#include <memory>

#include "random.hh"
#include "grammar.hh"
#include "relmodel.hh"
#include "schema.hh"

#include "config.h"

#include "log.hh"

using namespace std;
using namespace pqxx;

using namespace std::chrono;

extern "C" {
#include <unistd.h>
}

mutex mtx;

void worker(vector<shared_ptr<query_spec> > *queue, scope *s, milliseconds *ms)
{
  shared_ptr<query_spec> result;
  while (true) {
    auto g0 = high_resolution_clock::now();
    //	  prepare_stmt gen(scope);
    result = make_shared<query_spec>((struct prod *)0, s);
    auto g1 = high_resolution_clock::now();
    mtx.lock();
    queue->push_back(result);
    *ms += duration_cast<milliseconds>(g1-g0);
    while (queue->size() > 500) {
      mtx.unlock();
      this_thread::sleep_for (chrono::milliseconds(100));
      mtx.lock();
    }
    mtx.unlock();
  }
}

int main()
{
  cerr << "sqlsmith " << GITREV << endl;
  smith::rng.seed(getpid());
  try
    {
      connection c;
      schema.summary();
      scope scope;
      schema.fill_scope(scope);
      work w(c);
      w.commit();

      vector<shared_ptr<logger> > loggers;
      loggers.push_back(make_shared<cerr_logger>());
      loggers.push_back(make_shared<pqxx_logger>("host=/var/run/postgresql port=5432 dbname=smith"));
      
      {
	work w(c);
	w.exec("set statement_timeout to '1s';"
	       "set client_min_messages to 'ERROR';"
	       "set lc_messages to 'C';");
	w.commit();
      }
      milliseconds query_time(0);
      milliseconds gen_time(0);

      vector<shared_ptr<query_spec> > queue;
      std::thread t(&worker, &queue, &scope, &gen_time);
//       std::thread t2(&worker, &queue, &scope, &gen_time);
//       std::thread t3(&worker, &queue, &scope, &gen_time);



      while (1) {
	  work w(c);

	  mtx.lock();
	  while (queue.size() == 0) {
	    mtx.unlock();
	    this_thread::sleep_for (std::chrono::milliseconds(10));
	    mtx.lock();
	  }
	  
	  shared_ptr<query_spec> stmt = queue.back();
	  queue.pop_back();
	  query_spec &gen = *stmt;
	  mtx.unlock();

	  for (auto l : loggers)
	    l->generated(gen);
	  
	  std::ostringstream s;
	  gen.out(s);

	  try {
	    auto q0 = high_resolution_clock::now();
	    result r = w.exec(s.str() + ";");
	    auto q1 = high_resolution_clock::now();
	    query_time =  duration_cast<milliseconds>(q1-q0);
	    for (auto l : loggers)
	      l->executed(gen);
	    w.abort();
	  } catch (const pqxx::sql_error &e) {
	    for (auto l : loggers)
	      try {
		l->error(gen, e);
	      } catch (std::runtime_error &e) {
		cerr << endl << "log failed: " << typeid(*l).name() << ": "
		     << e.what() << endl;
	      }
	  }
      }
    }
  catch (const std::exception &e)
    {
      cerr << e.what() << endl;
      return 1;
    }
}
