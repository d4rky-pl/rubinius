#ifndef RBX_BUILTIN_SELECTOR_HPP
#define RBX_BUILTIN_SELECTOR_HPP

#include "builtin/object.hpp"
#include "type_info.hpp"

namespace rubinius {
  class SendSite;
  class Array;

  class Selector : public Object {
  public:
    static const size_t fields = 2;
    static const object_type type = SelectorType;

  private:
    SYMBOL name_;       // slot
    Array* send_sites_; // slot

  public:
    /* accessors */

    attr_accessor(name, Symbol);
    attr_accessor(send_sites, Array);

    /* interface */

    static void init(STATE);
    static Selector* create(STATE, OBJECT name);
    static Selector* lookup(STATE, OBJECT name);
    static void      clear_by_name(STATE,  OBJECT name);

    OBJECT associate(STATE, SendSite* ss);
    void   clear(STATE);
    bool   includes_p(STATE, SendSite* ss);

    class Info : public TypeInfo {
    public:
      BASIC_TYPEINFO(TypeInfo)
    };

  };
};

#endif
