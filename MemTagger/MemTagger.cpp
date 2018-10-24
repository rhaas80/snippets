#include <new>
#include <memory>
#include <search.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cassert>

namespace MemTagger {
/// tracks to which memory tag a given allocation belongs
// Nothing in this file may use C++ style new/delete since it almost certainly
// leads to an infinite loop.

#define STACK_INCREMENT 100 /// get memory for tag stack in chunks of this size
#define HAVE_FORWARD_ITERATOR 0
#define TEST 1

/// a doubly linked list, used to keep track of the currently allocated chunks
/// of memory
struct dlist
{
  public:
    /// a single doubly linked node
    struct dnode {
      /// linkage
      dnode *succ, *pred;
      /// amount of memory recorded by this node
      size_t size;
    };

    enum nocopytag_t {nocopy};

    dlist(const char *nm) {
      init(strdup(nm), true);
    };
    dlist(const char *nm, nocopytag_t tag) {
      init(nm,false);
    };
    ~dlist() {
      if(owns_name)
        free(const_cast<void*>(static_cast<const void*>(name)));
      owns_name = false;
      name = NULL;
      // unhook nodes so that remove does not fail
      head->pred = tail;
      tail->succ = head;
    };
    dlist(const dlist& dl) {
      if(&dl == this) return;

      init(strdup(dl.name),true);
      if(dl.head->succ) { // not empty list
        this->head->pred = (dnode*)&this->headtail;
        this->tail->pred = (dnode*)&this->head;
      }
    };

    /// check if two lists track the same memory tag
    static int cmp(const void* a, const void *b) {
      return strcmp(((const dlist*)a)->name, ((const dlist*)b)->name);
    }

    /// the tag this list tracks
    const char* get_name() const {return this->name;};

    // modify list
    /// append node to front of list
    void push_front(dnode *nd) {
      nd->succ = this->head;
      nd->pred = reinterpret_cast<dnode*>(&this->head);
      this->head->pred = nd;
      this->head = nd;
    }

    /// remove node from list
    static void remove(dnode* nd) {
      nd->pred->succ = nd->succ;
      nd->succ->pred = nd->pred;
    }

    /// iterator interface
#if HAVE_FORWARD_ITERATOR
    class iterator;
#endif
    class const_iterator {
      public:
        ~const_iterator() {};
        const_iterator(const const_iterator& it) : current(*it) {};
#if HAVE_FORWARD_ITERATOR
        const_iterator(const iterator& it) : current(it.current) {};
#endif
        const dnode* operator*() const {return current;};
        const_iterator& operator++() {current=current->succ;return *this;};
        bool operator!=(const const_iterator& it) const {
          return this->current!=it.current;
        };
      private:
        const_iterator(const dnode* cr) : current(cr) {};
        const_iterator();
      friend class dlist;
      private:
        const dnode *current;
    };

#if HAVE_FORWARD_ITERATOR
    class iterator {
      public:
        ~iterator() {};
        iterator(const iterator& it) : current(it.current), succ(it.succ) {};
        dnode* operator*() const {return current;};
        iterator& operator++() {current=succ;succ=current->succ;return *this;};
        bool operator!=(const iterator& it) const {
          return this->current!=it.current;
        };
      private:
        iterator(dnode* cr) : current(cr),succ(current->succ) {};
        iterator();
      friend class dlist;
      private:
        dnode *current, *succ;
    };
#endif

    const_iterator begin() const {
      return const_iterator(this->head);
    };
    const_iterator end() const {
      return const_iterator((dnode*)&this->headtail);
    };

#if HAVE_FORWARD_ITERATOR
    iterator begin() {
      return iterator(this->head);
    };
    iterator end() {
      return iterator((dnode*)&this->headtail);
    };
#endif

  private:
    dlist& operator=(const dlist&);
    /// set of an empty list
    /// @param nm tag that the list tracks
    void init(const char *nm, bool own) {
      this->head = reinterpret_cast<dnode*>(&this->headtail);
      this->headtail = NULL;
      this->tail = reinterpret_cast<dnode*>(&this->head);
      this->name = nm;
      this->owns_name = own;
    }

  private:
    const char *name;             /// the tag we track
    bool owns_name;               /// do we need to call free() on name?
    dnode *head,*headtail,*tail;  /// conflated first and last node
};


/// holds the current stack of tracker tags
// there may be only one of these since new/delete are global operators
class TagStack {
  public:
    TagStack() {};
    ~TagStack() {
      tdestroy(troot, free_node);
      troot = NULL;
      free(memlist);
      memlist_Cap = 0;
      memlist_Top = -1;
      memlist = NULL;
    };

    /// start new memory tracker
    /// @param name tag to track
    static void PushTag(const char *name) {
      dlist key(name, dlist::nocopy);
      dlist ** ptn = (dlist**)tsearch(&key, &troot, dlist::cmp);
      if(*ptn == &key) { // new item, clone it.
        void *buf= malloc(sizeof(**ptn));
        if(!buf) throw std::bad_alloc();
        *ptn = new (buf) dlist(name);
      }
      assert(memlist_Top < 0 ||
             strcmp(name, memlist[memlist_Top]->get_name()) != 0);

      memlist_Top += 1;
      if(memlist_Top >= memlist_Cap) {
        memlist_Cap = memlist_Top+STACK_INCREMENT;
        memlist = static_cast<dlist**>(realloc(memlist,
                                               sizeof(*memlist)*memlist_Cap));
      }
      memlist[memlist_Top] = *ptn;
    };

    /// finish memory tracker. Aborst if name does not agree with top of stack
    /// @param name tag to track
    static void PopTag(const char *name) {
      assert(strcmp(name, memlist[memlist_Top]->get_name()) == 0);
      assert(memlist_Top >= 0);

      memlist_Top -= 1;
    };

    /// generate output for all memory currently allocated
    static void PrintLists() {
      twalk(troot, print_lists_action);
    };

    /// get the top of the stack
    /// @return dlist tracking current tag
    static dlist* GetHead() { return memlist[memlist_Top]; };

  private:
    TagStack(const TagStack&);
    TagStack& operator=(const TagStack&);

    // unfortunately search.h does not provide a calldata pointer to pass in
    // user information, so we have to resort to a static variable
    static void print_lists_action(const void *nodep, const VISIT which,
                                   const int depth) {
      if(which == postorder || which == leaf) {
        dlist *memlist = *(dlist**)nodep;
        printf("In list %s:\n", memlist->get_name());
        for(dlist::const_iterator it = memlist->begin();
            it != memlist->end();
            ++it) {
          printf("  %zu bytes\n",(*it)->size);
        }
      }
    }

    static void free_node(void *nodep) {
      dlist *ml = (dlist*)nodep;
#if HAVE_FORWARD_ITERATOR
      for(dlist::iterator it = ml->begin();
          it != ml->end();
          ++it) {
        dlist::remove(*it);
        free(static_cast<void*>(*it));
      }
#endif
      ml->~dlist();
      free(ml);
    }

  private:
    static void* troot;     /// list of all known tags

    // the actual memory tracking new/delete operators
    static int memlist_Top; /// depth of stack
    static int memlist_Cap; /// max size of stack
    static dlist **memlist; /// the stack of memory tag
}; // class TagStack

void* TagStack::troot = NULL;
int TagStack::memlist_Top = -1;
int TagStack::memlist_Cap = 0;
dlist** TagStack::memlist = NULL;
static TagStack Stack; // only here so that destructor is called
                       // can be safely removed since all that is done is free()

} // namespace MemTagger

// the memory tracking new/delete operators themselves
void * operator new(size_t size) throw(std::bad_alloc)
{
  using namespace MemTagger;
  dlist::dnode * retval = (dlist::dnode*)malloc(sizeof(dlist::dnode)+size);
  if(!retval) throw std::bad_alloc();
  retval->size = size;
  TagStack::GetHead()->push_front(retval);
  printf ("alloc: %zu into %s\n",size, TagStack::GetHead()->get_name());
  return (void*)(retval+1);
}

void * operator new[](size_t size) throw(std::bad_alloc)
{
  return operator new(size);
}

void operator delete(void* p) throw()
{
  using namespace MemTagger;
  dlist::dnode *nd = ((dlist::dnode*)p)-1;
  dlist::remove(nd);
  free((void*)nd);
}

void operator delete[](void* p) throw()
{
  operator delete(p);
}

#if TEST
int main(void)
{
  MemTagger::TagStack::PushTag("first");
  double *a = new double(1);
  double *c = new double[10];
  MemTagger::TagStack::PushTag("second");
  double *b = new double(1);
  MemTagger::TagStack::PopTag("second");
  MemTagger::TagStack::PopTag("first");
  MemTagger::TagStack::PrintLists();
  delete[] a;
  delete[] b;
  delete[] c;
  return 0;
}
#endif
