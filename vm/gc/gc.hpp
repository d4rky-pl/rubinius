#ifndef RBX_VM_GC_HPP
#define RBX_VM_GC_HPP

#include "oop.hpp"
#include "builtin/object.hpp"

namespace rubinius {

  class ObjectMemory;
  class CallFrame;
  class VariableScope;
  class GlobalCache;
  class StackVariables;

  namespace capi {
    class Handles;
  }

  typedef std::vector<Object*> ObjectArray;

  class ObjectVisitor {
  public:
    virtual ~ObjectVisitor() { }
    virtual Object* call(Object*) = 0;
  };

  class GCData {
    Roots& roots_;
    CallFrameLocationList& call_frames_;
    VariableRootBuffers& variable_buffers_;
    capi::Handles* handles_;
    capi::Handles* cached_handles_;
    GlobalCache* global_cache_;

  public:
    GCData(STATE);
    GCData(Roots& r, CallFrameLocationList& l, VariableRootBuffers& b,
           capi::Handles* handles = NULL, capi::Handles* cached_handles = NULL,
           GlobalCache *cache = NULL)
      : roots_(r)
      , call_frames_(l)
      , variable_buffers_(b)
      , handles_(handles)
      , cached_handles_(cached_handles)
      , global_cache_(cache)
    {}

    Roots& roots() {
      return roots_;
    }

    CallFrameLocationList& call_frames() {
      return call_frames_;
    }

    VariableRootBuffers& variable_buffers() {
      return variable_buffers_;
    }

    capi::Handles* handles() {
      return handles_;
    }

    capi::Handles* cached_handles() {
      return cached_handles_;
    }

    GlobalCache* global_cache() {
      return global_cache_;
    }
  };

  class GarbageCollector {
  protected:
    ObjectMemory* object_memory_;

  private:
    ObjectArray* weak_refs_;

  public:
    GarbageCollector(ObjectMemory *om);
    virtual ~GarbageCollector()  { }

    virtual Object* saw_object(Object*) = 0;
    void scan_object(Object* obj);
    void delete_object(Object* obj);
    void walk_call_frame(CallFrame* top_call_frame);
    void saw_variable_scope(CallFrame* call_frame, StackVariables* scope);

    void visit_variable_scope(CallFrame* call_frame, StackVariables* scope,
        ObjectVisitor& visit);

    void visit_call_frame(CallFrame* top, ObjectVisitor& visit);

    Object* mark_object(Object* obj) {
      if(!obj || !obj->reference_p()) return obj;
      Object* tmp = saw_object(obj);
      if(tmp) return tmp;
      return obj;
    }

    void visit_roots(Roots& roots, ObjectVisitor& visit);
    void visit_call_frames_list(CallFrameLocationList& call_frames, ObjectVisitor& visit);
    void unmark_all(GCData& data);
    void clean_weakrefs(bool check_forwards=false);

    VM* state();

    friend class ObjectMark;
  };


}

#endif
