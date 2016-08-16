#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eina.hh>
#include <Eo.hh>

#include <algorithm>

#include "eina_cxx_suite.h"
#include "simple.eo.hh"

START_TEST(eina_cxx_iterator_equal)
{
  efl::eina::eina_init eina_init;

  efl::eina::ptr_list<int> list;
  ck_assert(list.size() == 0);
  ck_assert(list.empty());

  list.push_back(new int(5));
  list.push_back(new int(10));
  list.push_back(new int(15));
  list.push_back(new int(20));

  efl::eina::iterator<int> iterator = list.ibegin()
    , last_iterator = list.iend();

  int result[] = {5, 10, 15, 20};

  ck_assert(std::equal(iterator, last_iterator, result));
}
END_TEST

START_TEST(eina_cxx_eo_iterator_equal)
{
  efl::eina::eina_init eina_init;
  efl::eo::eo_init eo_init;

  efl::eina::list<nonamespace::Simple> list;

  nonamespace::Simple const w1;
  nonamespace::Simple const w2;
  nonamespace::Simple const w3;
  nonamespace::Simple const w4;

  list.push_back(w1);
  list.push_back(w2);
  list.push_back(w3);
  list.push_back(w4);

  efl::eina::iterator<nonamespace::Simple> iterator = list.ibegin()
    , last_iterator = list.iend();

  nonamespace::Simple const result[] = {w1, w2, w3, w4};

  ck_assert(std::equal(iterator, last_iterator, result));
}
END_TEST

void
eina_test_iterator(TCase *tc)
{
  tcase_add_test(tc, eina_cxx_iterator_equal);
  tcase_add_test(tc, eina_cxx_eo_iterator_equal);
}
